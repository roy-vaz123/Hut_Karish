#include "ScanFiles.h"
#include "NetLinkClient.h"
#include "MessageQueue.h" 
#include <unordered_map> // Store packets by PID
#include <thread> 
#include <vector>
#include <atomic> // To share bool between threads
#include <iostream>
#include <chrono>
// To print ip address
#include <arpa/inet.h>
#include <netinet/in.h>

std::atomic<bool> running = true;
MessageQueue messageQueue;

// The thread thall listen and receive messages from the kernel module
void recvThread(const NetLinkClient& client);

// Function to update the port-PID map when a new port arrives from kernel module
void updatePortPidMap(std::unordered_map<uint16_t, pid_t>& portPidMap, const pckt_info* pkt);

int main() {
    
    NetLinkClient client;// Create Netlink client
    std::unordered_map<uint16_t, pid_t> portPidMap = ScanFiles::initializePortPidMap(); // Initialize the database of ports and PIDs
    
    
    // subscribe to kernel module messages
    if (!client.sendMessage("deamon_subscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    
    // Start the receiver thread
    std::thread listener(recvThread, std::cref(client));

    // Main loop: poll the queue for messages
    while (running) {
        const pckt_info* pkt = messageQueue.pop();
        if (!pkt) {
            // If queue is empty, wait a bit before checking again (avoid busy looping)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        // A packet was received, update port-PID map if necceccary
        if(portPidMap.find(pkt->dst_port) == portPidMap.end()) {
            updatePortPidMap(portPidMap, pkt);// find the pid of the process using the port
            ////////////////////test
            std::cout << "Port: " << pkt->dst_port << ", PID: " << portPidMap[pkt->dst_port] << "added to map"<< std::endl;
        }
        // Free the packet info structure
        client.freePacketInfo(pkt);// free the allocated memory for the message
    }

    // Stop receiver thread
    running = false;
    listener.join();

    // Unsubscribe from kernel module messages
    if (!client.sendMessage("deamon_unsubscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    
    // Free all remaing packets in the message queue
    while (!messageQueue.empty()){
        const pckt_info* pkt = messageQueue.pop();
        if (pkt) {
            client.freePacketInfo(pkt);// free the allocated memory for the message
        }
    }

    return 0;
}

// The thread thall listen and receive messages from the kernel module
void recvThread(const NetLinkClient& client){
    while (running) {
        const pckt_info* pkt = client.receivePacketInfo();// allocate memory for the packet!!!!!
        if (pkt) {
            messageQueue.push(pkt);
        }
    }
}

// Function to update the port-PID map when a new port arrives from kernel module
void updatePortPidMap(std::unordered_map<uint16_t, pid_t>& portPidMap, const pckt_info* pkt) {
    pid_t pid = ScanFiles::scanForPidByPort(pkt->dst_port);// find the pid of the process using the port
    if (pid != -1) {
        portPidMap[pkt->dst_port] = pid;// update the map
    } else {
        std::cerr << "Failed to find PID for port: " << pkt->dst_port << std::endl;
    }    
}