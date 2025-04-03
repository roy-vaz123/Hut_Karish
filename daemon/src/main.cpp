#include "ScanFiles.h"
#include "NetLinkClient.h"
#include "MessageQueue.h"
#include "UnixSocketServer.h" 
#include <unordered_map> // Store packets by PID
#include <thread> 
#include <vector>
#include <atomic> // To share bool between threads
#include <iostream>
#include <chrono>
// To print ip address
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>// for log (no cout for daemons)
#include <shared_mutex>// mutex that allows multiple reads simutanesly
#include <memory>// for shared vars (multiple threads)


using PortToPidMapPtr =  std::shared_ptr<std::unordered_map<uint16_t, pid_t>>;


// For apps connected to this daemon, well use hash of appconnection, a new thread will be open for each app
// and close when it disconnects. i chose this approach because its simple and i dont anticipate
// alot of apps opened at once 
std::unordered_map<int, std::thread> connectedApps;
std::mutex appsMapMutex;


std::atomic<bool> running = true;
MessageQueue messageQueue;// a thread safe buffer for kernel messages

// Runs on a seperate thread, insert new clients connecting to the connectedApps map
void acceptLoop(const UnixSocketServer& appsCommServer);

// The thread for connected app, wait for messages containing ports and returns pid from the port pid map 
void handleApp(int clientFd, const UnixSocketServer& appsCommServer);

// The thread thall listen and receive messages from the kernel module
void recvPacketInfoThread(const NetLinkClient& client);

// Function to update the port-PID map when a new port arrives from kernel module
bool updatePortPidMap(std::unordered_map<uint16_t, pid_t>& portPidMap, const pckt_info* pckt);

// Find the pid in the port to pid map
pid_t getPidFromPort(uint16_t port, const std::unordered_map<uint16_t, pid_t>& portPidMap);

int main() {
    
    // Using syslog for logging, using log_daemon format, writes to /var/log/syslog app name portmon_daemon, print error to conlose
    openlog("portmon_daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
    
    NetLinkClient client;// Create Netlink client
    std::unordered_map<uint16_t, pid_t> portPidMap = ScanFiles::initializePortPidMap(); // Initialize the database of ports and PIDs
    
    // Creats the socket and bind it to SOCKET_BIND_ADRESS
    // may need to change to sharedptr later, but since i make sure main wait for threads
    // and dont change it in main its safe (same for netlink client, and portToPidMap)
    UnixSocketServer appsCommServer(SOCKET_FILE_ADRESS);
    std::thread(acceptLoop, appsCommServer);// start the thread that listen to new apps and connect them to daemon
    
    for(const auto& pair : portPidMap){// logging
        std::cout << "port " << pair.first << " pid "<< pair.second << std::endl;
    }
    
    // subscribe to kernel module messages
    if (!client.sendMessage("daemon_subscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    
    // Start the receiver thread
    std::thread listener(PacketInfoThread, std::cref(client));

    // Main loop: poll the queue for messages
    while (running) {
        const pckt_info* pckt = messageQueue.pop();
        if (!pckt) {
            // If queue is empty, wait a bit before checking again (avoid busy looping)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        // A packet was received, update port-PID map if necceccary
        if(portPidMap.find(pckt->dst_port) == portPidMap.end()) {
            if(updatePortPidMap(portPidMap, pckt)){// find the pid of the process using the port
               
                std::cout << "Port: " << pckt->dst_port << ", PID: " << portPidMap[pckt->dst_port] << " added to map"<< std::endl;// switch to log
            }
        }
        // Free the packet info structure
        client.freePacketInfo(pckt);// free the allocated memory for the message
    }

    // Stop receiver thread
    running = false;
    listener.join();

    // Unsubscribe from kernel module messages
    if (!client.sendMessage("daemon_unsubscribe")) {
        std::cerr << "Failed to send message to kernel\n";// switch to log
        return -1;
    }
    
    // Free all remaing packets in the message queue
    while (!messageQueue.empty()){
        const pckt_info* pckt = messageQueue.pop();
        if (pckt) {
            client.freePacketInfo(pckt);// free the allocated memory for the message
        }
    }

    // Join all connected app threads for clean clousure
    for(auto& appConnection : connectedApps){
        if(appConnection.second.joinable()) appConnection.second.join();
    }

    return 0;
}

// The thread thall listen and receive messages from the kernel module
void PacketInfoThread(const NetLinkClient& client){
    while (running) {
        const pckt_info* pckt = client.receivePacketInfo();// allocate memory for the packet!!!!!
        if (pckt) {
            messageQueue.push(pckt);
        }
    }
}

// Function to update the port-PID map when a new port arrives from kernel module
bool updatePortPidMap(std::unordered_map<uint16_t, pid_t>& portPidMap, const pckt_info* pckt) {
    pid_t pid = ScanFiles::scanForPidByPort(pckt->dst_port, pckt->proto);// find the pid of the process using the port
    if (pid != -1) {
        portPidMap[pckt->dst_port] = pid;// update the map
    } else {
        std::cerr << "Failed to find PID for packet: source ip " <<pckt->src_ip<< " dest ip " <<pckt->dst_ip <<" souerce ip "<< pckt->src_ip<<" for port: "<< pckt->dst_port << " protocol "<< pckt->proto <<std::endl;
        return false;
    }    
    return true;
}

// The thread for connected app, wait for messages containing ports and returns pid from the port pid map 
void handleApp(int clientFd, const UnixSocketServer& appsCommServer){
    uint16_t port;
    pid_t pid;
    while (true) {
        if(!appsCommServer.receivePort(clientFd, port)) break;// blocking, waiting to recieve message from client
        
        // Get the ports pid or -1 if unknown and send to client
        pid = getPidFromPort(port);
        appsCommServer.sendPid(clientFd, pid);
    }

    close(clientFd);

    // Remove the app from the connectedApps map
    std::lock_guard<std::mutex> lock(appsMapMutex);
    connectedApps.erase(clientFd);
    // the lock releases automatically
}

// Runs on a seperate thread, insert new clients connecting to the connectedApps map
void acceptLoop(const UnixSocketServer& appsCommServer){
    int serverFd = appsCommServer.getServerFd();
    while (true) {
        int clientFd = accept(serverFd, nullptr, nullptr);// blocking waiting for new apps to connect to daemon
        if (clientFd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "Client connected (fd=" << clientFd << ")\n";// logging

        std::lock_guard<std::mutex> lock(appsMapMutex);
        connectedApps[clientFd] = std::thread(handleApp, clientFd, appsCommServer);
    }// the lock releases automatically
}

// Find the pid in the port to pid map
pid_t getPidFromPort(uint16_t port, const std::unordered_map<uint16_t, pid_t>& portPidMap){
    if(portPidMap.find(port) != portPidMap.end()){
        return portPidMap.at(port);
    }
    return -1;
}