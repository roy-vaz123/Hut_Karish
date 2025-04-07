#include "PortToPidMap.h"// The map that will keep the pid for packets ports
#include "UserSpaceConfig.h"// for useful headers and shared ptrs
#include "UnixSocketServer.h" 
#include <unistd.h> // files functions (close, unlink, read, write)
#include <sys/socket.h> // for socket functions like accept
#include <syslog.h>// for log (no cout for daemons)
#include <iostream>

// Shared pointers to share data through threads, safe to use end easier to manage the global vars
using PortToPidMapReadPtr = std::shared_ptr<const PortToPidMap>;// for the port to pid map
using PortToPidMapPtr = std::shared_ptr<PortToPidMap>;// for the port to pid map
using UnixSocketServerPtr = std::shared_ptr<UnixSocketServer>;


// The thread for the client (packet hunter) daemon communication (using unix dumain socket), wait for client to connect, then listen for messages from client
void clientConnectionThread(PortToPidMapReadPtr portPidMap, UnixSocketServerPtr unixServer, std::atomic<bool>& running);

// Listen to client and answer
void handleClientConnection(PortToPidMapReadPtr portPidMap, UnixSocketServerPtr unixServer,std::atomic<bool>& running);

int main() {
    
    // Capture signals to end the program
    std::signal(SIGINT, handleSignal); // For Ctrl+C

    // Using syslog for logging, using log_daemon format, writes to /var/log/syslog app name portmon_daemon, print error to conlose
    openlog("portmon_daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
    
    // Initilize the global variables
    NetLinkClientPtr client = std::make_shared<NetLinkClient>();// Create Netlink client
    PortToPidMapPtr portPidMap = std::make_shared<PortToPidMap>(); // Initialize the database of ports and pids
    MessageQueuePtr messageQueue = std::make_shared<MessageQueue>();// Create the message queue
    UnixSocketServerPtr unixServer = std::make_shared<UnixSocketServer>();// initilize the server
    
    // start the thread that listen to new apps and connect them to daemon
    std::thread clientConnection(clientConnectionThread, portPidMap, unixServer, std::ref(running));

    // subscribe to kernel module messages
    if (!client->sendMessage("daemon_subscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    
    // Start the receiver thread, send const pointer to the client (recv is const)
    std::thread packetInfoListener(SharedUserFucntions::recvPacketInfoThread, NetLinkClientRecievePtr(client), messageQueue, std::ref(running));

    // Main loop, poll the queue for messages
    while (running) {
        const pckt_info* pckt = messageQueue->pop();
        if (!pckt) {
            // If queue is empty, wait a bit before checking again (avoid busy looping)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        // A packet was received, update port-PID map
        if(portPidMap->addPidMapping(pckt->dst_port, pckt->proto)){// find the pid of the process using the port      
                std::cout << "Port: " << pckt->dst_port << ", PID: " << portPidMap->getPid(pckt->dst_port) << " added to map"<< std::endl;// logging
        }
        // Free the packet info structure
        client->freePacketInfo(pckt);// free the allocated memory for the message
    }
    
    // Unsubscribe from kernel module and stop receiver thread
    if (!client->sendMessage("daemon_unsubscribe")) {
    std::cerr << "Failed to send message to kernel\n";// switch to log
    return -1;
    }
    packetInfoListener.join();
   
    // Free remainig messages in message queue
    SharedUserFucntions::cleanMessageQueue(messageQueue, client);
    
    // Close active connection if conncected 
    if(unixServer->getClientFd() > 0){
        shutdown(unixServer->getClientFd(), SHUT_RD);
        unixServer->closeClientFd();
    }
    
    // Close server socket and join the app client connection thread
    unixServer->stopListeningForNewClients();
    clientConnection.join();    

    std::cout << "Port Monitor Daemon terminated" << std::endl; // logging
    return 0;
}


// The thread for connected app, wait for messages containing ports and returns pid from the port pid map 
void clientConnectionThread(PortToPidMapReadPtr portPidMap, UnixSocketServerPtr unixServer, std::atomic<bool>& running) {
   
    while(running){   
        // waiting for client to connect
        std::cout << "Waiting for client to connect"<< std::endl;
        
        // Waiting for clients connection
        if(!unixServer->connectToClient()){// blocking call
            // Failed to connect
            if(running){// If The program wasnt ended
                std::cout << "Client faild to connect"<<std::endl;// logging
            }
            unixServer->closeClientFd();
            continue;
        }
        
        // Client connected succecssfully, listen to client and answer
        handleClientConnection(portPidMap, unixServer, running);// blocking call  
        
        // Remove the app from the connectedApps map
        std::cout << "Client disconnected (fd=" << unixServer->getClientFd() << ")\n";// logging

        // disconnect client to allow new one
        unixServer->closeClientFd();
    }
    
}

// Listen to client and answer
void handleClientConnection(PortToPidMapReadPtr portPidMap, UnixSocketServerPtr unixServer,std::atomic<bool>& running){
    if (unixServer->getClientFd() < 0) return; // if connection somehow isnt valid
    
    // Listen to client and and answer its port requests
    uint16_t port;
    pid_t pid;
    while (running) {// On stopping main will close client fd, recieve port will break loop
        if(!unixServer->receivePort(port)) break;// blocking, waiting to recieve message from client
        
        // Get the ports pid or nullptr if unknown and send to client
        pid = portPidMap->getPid(port);
        if(pid) {// send and logging
            std::cout << "Client (fd=" << unixServer->getClientFd() << ") requested port " << port << ", PID: " << pid << "\n";// logging
            unixServer->sendPid(pid);
        } else {
            std::cout << "Client (fd=" << unixServer->getClientFd() << ") requested port " << port << ", PID: unknown\n";// logging
            unixServer->sendPid(-1);// send -1 to client
        }
        
    }
}

