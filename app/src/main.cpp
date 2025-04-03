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
MessageQueue messageQueue;// queue to store messages from kernel module, waiting to be processed

// The thread that will listen and receive messages from the kernel module
void recvThread(const NetLinkClient& client);

int main() {
    
    NetLinkClient client;// Create Netlink client
    std::unordered_map<pid_t, std::vector<const pckt_info*>> packetMap; // Map to store packets by PID
    
    // subscribe to kernel module messages
    if (!client.sendMessage("app_subscribe")) {
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
        
        // Prepare for printing IP address
        struct in_addr ip_addr;
        ip_addr.s_addr = pckt->src_ip;
        
        // Print packet info
        std::cout << "Received packet: src_ip: " << inet_ntoa(ip_addr) << ", proto: " << pckt->proto << "\n";
    }

    // Stop receiver thread
    running = false;
    listener.join();

    // Unsubscribe from kernel module messages
    if (!client.sendMessage("app_unsubscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    
    // Free all stored packets
    for (auto& [pid, vec] : packetMap) {
        for (const pckt_info* pckt : vec) {
            client.freePacketInfo(pckt); // Free the allocated memory for the message
        }
    }

    return 0;
}

// The thread that will listen and receive messages from the kernel module
void recvThread(const NetLinkClient& client){
    while (running) {
        const pckt_info* pckt = client.receivePacketInfo();
        if (pckt) {
            messageQueue.push(pckt);
        }
    }
}

