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

void recvThread(const NetLinkClient& client){
    while (running) {
        pckt_info* pkt = client.receiveMessage();
        if (pkt) {
            messageQueue.push(pkt);
        }
    }
}

int main() {
    
    NetLinkClient client;// Create Netlink client
    std::unordered_map<pid_t, std::vector<const pckt_info*>> packetMap; // Map to store packets by PID
    
    // subscribe to kernel module messages
    if (!client.sendMessage("subscribe")) {
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
        
        // Prepare for printing IP address
        struct in_addr ip_addr;
        ip_addr.s_addr = pkt->src_ip;
        
        // Print packet info
        std::cout << "Received packet: src_ip: " << inet_ntoa(ip_addr) << ", proto: " << pkt->proto << "\n";
        
        // TODO: Replace with daemon-based PID lookup
        pid_t fakePid = 1000 + (pkt->src_ip % 1000); // Dummy PID logic for now
        packetMap[fakePid].push_back(pkt);

        std::cout << "Stored packet for PID " << fakePid << ", proto: " << pkt->proto << "\n";

        // Optional shutdown check
        // if (someExitCondition) running = false;
    }

    // Stop receiver thread
    running = false;
    listener.join();

    // Unsubscribe from kernel module messages
    if (!client.sendMessage("unsubscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    
    // Free all stored packets
    for (auto& [pid, vec] : packetMap) {
        for (const pckt_info* pkt : vec) {
            delete pkt;
        }
    }

    return 0;
}
