#include "PortToPidMap.h"
#include "NetLinkClient.h"
#include "MessageQueue.h"
#include "UnixSocketServer.h" 
#include <thread> 
#include <vector>
#include <atomic> // To share bool between threads
#include <iostream>
#include <chrono>
// To print ip address
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>// for log (no cout for daemons)


// Shared pointers to share data through threads, safe to use end easier to manage the global vars
using PortToPidMapReadPtr = std::shared_ptr<PortToPidMap>;
using PortToPidMapPtr = std::shared_ptr<PortToPidMap>;
using ManageQueuePtr = std::shared_ptr<MessageQueue>;
using NetLinkClientPtr = std::shared_ptr<NetLinkClient>;// not const because send messages not const
using NetLinkClientRecievePtr = std::shared_ptr<const NetLinkClient>;
using UnixSocketServerPtr = std::shared_ptr<const UnixSocketServer>;
using connectedAppsMutexPtr = std::shared_ptr<std::mutex>;
using ConnectedAppsMapPtr = std::shared_ptr<std::unordered_map<int, std::thread>>;// note: I could have combined this with porttopid class to thread safe template unordered map, but this is simpler and that does class contain some logic

std::atomic<bool> running = true;

// Runs on a seperate thread, insert new clients connecting to the connectedApps map
void acceptAppConnectionThread(UnixSocketServerPtr appsCommServer, PortToPidMapReadPtr portPidMap, connectedAppsMutexPtr connectedAppsMutex, ConnectedAppsMapPtr connectedApps);

// The thread for connected app, wait for messages containing ports and returns pid from the port pid map 
void appConnectionThread(int clientFd, UnixSocketServerPtr appsCommServer, PortToPidMapReadPtr portPidMap, connectedAppsMutexPtr connectedAppsMutex, ConnectedAppsMapPtr connectedApps);

// The thread thall listen and receive messages from the kernel module
void recvPacketInfoThread(NetLinkClientRecievePtr client, ManageQueuePtr messageQueue);

int main() {
    
    // Using syslog for logging, using log_daemon format, writes to /var/log/syslog app name portmon_daemon, print error to conlose
    openlog("portmon_daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
    
    NetLinkClientPtr client = std::make_shared<NetLinkClient>();// Create Netlink client
    PortToPidMapPtr portPidMap = std::make_shared<PortToPidMap>(); // Initialize the database of ports and PIDs
    UnixSocketServerPtr appsCommServer = std::make_shared<UnixSocketServer>(SOCKET_FILE_ADRESS);// Create the Unix socket server to bind with sock file adress
    ManageQueuePtr messageQueue = std::make_shared<MessageQueue>();// Create the message queue
    ConnectedAppsMapPtr connectedApps = std::make_shared<std::unordered_map<int, std::thread>>();// Map of connected apps and their threads
    
    connectedAppsMutexPtr connectedAppsMutex = std::make_shared<std::mutex>();// For thread safe access to connected apps map
    
    // start the thread that listen to new apps and connect them to daemon
    std::thread appClientConnectionListener(acceptAppConnectionThread, appsCommServer, portPidMap, connectedAppsMutex, connectedApps);

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
        if(portPidMap->getPid(pckt->dst_port) == -1) {
            if(portPidMap->addMapping(pckt->dst_port, pckt->proto)){// find the pid of the process using the port
               
                std::cout << "Port: " << pckt->dst_port << ", PID: " << portPidMap->getPid(pckt->dst_port) << " added to map"<< std::endl;// switch to log
            }
        }
        // Free the packet info structure
        client->freePacketInfo(pckt);// free the allocated memory for the message
    }

    // Stop receiver thread
    running = false;
    packetInfoListener.join();

    // Unsubscribe from kernel module messages
    if (!client->sendMessage("daemon_unsubscribe")) {
        std::cerr << "Failed to send message to kernel\n";// switch to log
        return -1;
    }
    
    // Free all remaing packets in the message queue
    while (!messageQueue->empty()){
        const pckt_info* pckt = messageQueue->pop();
        if (pckt) {
            client->freePacketInfo(pckt);// free the allocated memory for the message
        }
    }

    // Join all connected app threads for clean clousure
    for(auto& appConnection : *connectedApps) {
        if(appConnection.second.joinable()) appConnection.second.join();
    }
    appClientConnectionListener.join();// join the app connection listener thread
    
    return 0;
}

// The thread thall listen and receive messages from the kernel module
void recvPacketInfoThread(NetLinkClientRecievePtr client, ManageQueuePtr messageQueue) {
    while (running) {
        const pckt_info* pckt = client->receivePacketInfo();// allocate memory for the packet!!!!!
        if (pckt) {
            messageQueue->push(pckt);
        }
    }
}

// The thread for connected app, wait for messages containing ports and returns pid from the port pid map 
void appConnectionThread(int clientFd, UnixSocketServerPtr appsCommServer, PortToPidMapReadPtr portPidMap, connectedAppsMutexPtr connectedAppsMutex, ConnectedAppsMapPtr connectedApps) {
    uint16_t port;
    pid_t pid;
    while (true) {
        if(!appsCommServer->receivePort(clientFd, port)) break;// blocking, waiting to recieve message from client
        
        // Get the ports pid or -1 if unknown and send to client
        pid = portPidMap->getPid(port);
        appsCommServer->sendPid(clientFd, pid);
    }

    close(clientFd);

    // Remove the app from the connectedApps map
    std::lock_guard<std::mutex> lock(*connectedAppsMutex);
    if(connectedApps->find(clientFd) != connectedApps->end()) {
        connectedApps->erase(clientFd);// the app disconnected, remove it from the map
        std::cout << "Client disconnected (fd=" << clientFd << ")\n";// logging
    }
    
}// the lock releases automatically


// Runs on a seperate thread, insert new clients connecting to the connectedApps map
void acceptAppConnectionThread(UnixSocketServerPtr appsCommServer, PortToPidMapReadPtr portPidMap, connectedAppsMutexPtr connectedAppsMutex, ConnectedAppsMapPtr connectedApps) {
    int serverFd = appsCommServer->getServerFd();
    while (true) {
        int clientFd = accept(serverFd, nullptr, nullptr);// blocking waiting for new apps to connect to daemon
        if (clientFd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "Client connected (fd=" << clientFd << ")\n";// logging

        std::lock_guard<std::mutex> lock(*connectedAppsMutex);
        (*connectedApps)[clientFd] = std::thread(appConnectionThread, clientFd, appsCommServer, portPidMap, connectedAppsMutex, connectedApps);// create a new thread for the connected app
    }// the lock releases automatically
}

