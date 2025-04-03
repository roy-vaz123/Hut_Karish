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
bool updatePortPidMap(std::unordered_map<uint16_t, pid_t>& portPidMap, const pckt_info* pckt);

int main() {
    
    NetLinkClient client;// Create Netlink client
    std::unordered_map<uint16_t, pid_t> portPidMap = ScanFiles::initializePortPidMap(); // Initialize the database of ports and PIDs
    //////////////test
    for(const auto& pair : portPidMap){
        std::cout << "port " << pair.first << " pid "<< pair.second << std::endl;
    }
    //////////////test
    
    // subscribe to kernel module messages
    if (!client.sendMessage("deamon_subscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    
    // Start the receiver thread
    std::thread listener(recvThread, std::cref(client));

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
                ////////////////////test
                std::cout << "Port: " << pckt->dst_port << ", PID: " << portPidMap[pckt->dst_port] << " added to map"<< std::endl;
            }
        }
        // Free the packet info structure
        client.freePacketInfo(pckt);// free the allocated memory for the message
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
        const pckt_info* pckt = messageQueue.pop();
        if (pckt) {
            client.freePacketInfo(pckt);// free the allocated memory for the message
        }
    }

    return 0;
}

// The thread thall listen and receive messages from the kernel module
void recvThread(const NetLinkClient& client){
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