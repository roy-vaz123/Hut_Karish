
#include "AppThreadsMap.h"// for the connected apps threads
#include "ScanFiles.h"// for scanning the files for the pids
#include "UserSpaceConfig.h"// for useful headers and shared ptrs
#include "UnixSocketServer.h" 
#include <unistd.h> // files functions (close, unlink, read, write)
#include <sys/socket.h> // for socket functions like accept
#include <syslog.h>// for log (no cout for daemons)
#include <iostream>

// Shared pointers to share data through threads, safe to use end easier to manage the global vars
using PortToPidMapReadPtr = std::shared_ptr<const ThreadSafeUnorderedMap<uint16_t, pid_t>>;// for the port to pid map
using PortToPidMapPtr = std::shared_ptr<ThreadSafeUnorderedMap<uint16_t, pid_t>>;// for the port to pid map
using UnixSocketServerPtr = std::shared_ptr<const UnixSocketServer>;
using ConnectedAppsMapPtr = std::shared_ptr<AppThreadsMap>;// for the connected apps threads

// Runs on a seperate thread, insert new clients connecting to the connectedApps map
void acceptAppConnectionThread(UnixSocketServerPtr appsCommServer, PortToPidMapReadPtr portPidMap, ConnectedAppsMapPtr connectedApps);

// The thread for connected app, wait for messages containing ports and returns pid from the port pid map 
void appConnectionThread(int clientFd, UnixSocketServerPtr appsCommServer, PortToPidMapReadPtr portPidMap, ConnectedAppsMapPtr connectedApps);

// The thread thall listen and receive messages from the kernel module
void recvPacketInfoThread(NetLinkClientRecievePtr client, MessageQueuePtr messageQueue);

// Return pid of port from map or -1 if not found
pid_t getPidByPort(uint16_t port, PortToPidMapReadPtr portPidMap);

// Update the port pid map with the new pid, return false if falis to find pid in files
bool updatePortPidMap(uint16_t port, PortToPidMapPtr portPidMap, char packetProtocol);

int main() {
    
    // Capture signals to end the program
    std::signal(SIGINT, handleSignal); // For Ctrl+C
    std::signal(SIGTERM, handleSignal);// For terminate (will be sent from main menu script) 
 
    // Using syslog for logging, using log_daemon format, writes to /var/log/syslog app name portmon_daemon, print error to conlose
    openlog("portmon_daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
    
    // Initilize the global variables
    NetLinkClientPtr client = std::make_shared<NetLinkClient>();// Create Netlink client
    PortToPidMapPtr portPidMap = std::make_shared<ThreadSafeUnorderedMap<uint16_t, pid_t>>(); // Initialize the database of ports and PIDs
    ScanFiles::initializePortPidMap(*portPidMap);// Scan the system files to find the PIDs of the processes using the ports
    UnixSocketServerPtr appsCommServer = std::make_shared<UnixSocketServer>(SOCKET_FILE_ADRESS);// Create the Unix socket server to bind with sock file adress
    MessageQueuePtr messageQueue = std::make_shared<MessageQueue>();// Create the message queue
    ConnectedAppsMapPtr connectedApps = std::make_shared<AppThreadsMap>();// Map of connected apps and their threads
    
    
    // start the thread that listen to new apps and connect them to daemon
    std::thread appClientConnectionListener(acceptAppConnectionThread, appsCommServer, portPidMap, connectedApps);

    for(const auto& pair : portPidMap->getMap()){// logging
        std::cout << "port " << pair.first << " pid "<< pair.second << std::endl;
    }
    
    // subscribe to kernel module messages
    if (!client->sendMessage("daemon_subscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    
    // Start the receiver thread, send const pointer to the client (recv is const)
    std::thread packetInfoListener(recvPacketInfoThread, NetLinkClientRecievePtr(client), messageQueue);

    // Main loop: poll the queue for messages
    while (running) {
        const pckt_info* pckt = messageQueue->pop();
        if (!pckt) {
            // If queue is empty, wait a bit before checking again (avoid busy looping)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        // A packet was received, update port-PID map if necceccary
        if(getPidByPort(pckt->dst_port, portPidMap) == -1) {
            if(updatePortPidMap(pckt->dst_port, portPidMap, pckt->proto)){// find the pid of the process using the port
               
                std::cout << "Port: " << pckt->dst_port << ", PID: " << *(portPidMap->get(pckt->dst_port)) << " added to map"<< std::endl;// logging
            }
        }
        // Free the packet info structure
        client->freePacketInfo(pckt);// free the allocated memory for the message
    }

    // Stop receiver thread
    packetInfoListener.join();

    // Unsubscribe from kernel module messages
    if (!client->sendMessage("daemon_unsubscribe")) {
        std::cerr << "Failed to send message to kernel\n";// switch to log
        return -1;
    }
    
    //Free remainig messages in message queue
    cleanMessageQueue(messageQueue, client);

    // Join all connected app threads for clean clousure
    connectedApps->joinAll();
    appClientConnectionListener.join();// join the app connection listener thread
    
    std::cout << "Port Monitor Daemon terminated" << std::endl; // logging
    return 0;
}

// The thread thall listen and receive messages from the kernel module
void recvPacketInfoThread(NetLinkClientRecievePtr client, MessageQueuePtr messageQueue) {
    while (running) {
        const pckt_info* pckt = client->receivePacketInfo();// allocate memory for the packet!!!!!
        if (pckt) {
            messageQueue->push(pckt);
        }
    }
}

// The thread for connected app, wait for messages containing ports and returns pid from the port pid map 
void appConnectionThread(int clientFd, UnixSocketServerPtr appsCommServer, PortToPidMapReadPtr portPidMap, ConnectedAppsMapPtr connectedApps) {
    uint16_t port;
    const pid_t* pid;
    while (running) {
        if(!appsCommServer->receivePort(clientFd, port)) break;// blocking, waiting to recieve message from client
        
        // Get the ports pid or nullptr if unknown and send to client
        pid = portPidMap->get(port);
        if(pid) {// send and logging
            std::cout << "Client (fd=" << clientFd << ") requested port " << port << ", PID: " << *pid << "\n";// logging
            appsCommServer->sendPid(clientFd, *pid);
        } else {
            std::cout << "Client (fd=" << clientFd << ") requested port " << port << ", PID: unknown\n";// logging
            appsCommServer->sendPid(clientFd, -1);// send -1 to client
        }
        
    }

    close(clientFd);

    // Remove the app from the connectedApps map
    std::cout << "Client disconnected (fd=" << clientFd << ")\n";// logging
}

// Runs on a seperate thread, insert new clients connecting to the connectedApps map
void acceptAppConnectionThread(UnixSocketServerPtr appsCommServer, PortToPidMapReadPtr portPidMap, ConnectedAppsMapPtr connectedApps) {
    int serverFd = appsCommServer->getServerFd();
    while (running) {
        int clientFd = accept(serverFd, nullptr, nullptr);// blocking waiting for new apps to connect to daemon
        if (clientFd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "Client connected (fd=" << clientFd << ")\n";// logging
        // create a new thread for the connected app
        connectedApps->insertAppThread(clientFd, std::thread(appConnectionThread, clientFd, appsCommServer, portPidMap, connectedApps));
    }
}

// Return pid of port from map or -1 if not found
pid_t getPidByPort(uint16_t port, PortToPidMapReadPtr portPidMap){
    const pid_t* pid = portPidMap->get(port);
    if (pid) {
        return *pid;
    }
    return -1; // not found
}

// Update the port pid map with the new pid, return false if falis to find pid in files
bool updatePortPidMap(uint16_t port, PortToPidMapPtr portPidMap, char packetProtocol) {
    pid_t pid = ScanFiles::scanForPidByPort(port, packetProtocol);// find the pid of the process using the port
    if(pid == -1) {
        std::cerr << "Failed to find PID for port " << port <<" packet type: "<< packetProtocol << std::endl;
        return false; // failed to find pid
    }
    
    // update the port pid map with the new pid from files 
    portPidMap->insertOrAssign(port, pid);
    return true;
}