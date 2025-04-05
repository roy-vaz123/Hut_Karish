#include "UserSpaceConfig.h"// for useful headers and shared pointers
#include "UnixSocketClient.h"// for the client
#include "PidToPacketsInfoMap.h"// for the map
#include <vector>
// To print ip address
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>

using UnixSocketClientPtr = std::shared_ptr<const UnixSocketClient>;
using PidToPacketsMapPtr = std::shared_ptr<PidToPacketsInfoMap>;

// The thread that will listen and receive messages from the kernel module
void recvThread(NetLinkClientRecievePtr client, MessageQueuePtr messageQueue);

int main() {
    
    NetLinkClientPtr netLinkClient = std::make_shared<NetLinkClient>();// Create Netlink client
    MessageQueuePtr messageQueue = std::make_shared<MessageQueue>();// Create the message queue
    PidToPacketsMapPtr pidToPacketsMap = std::make_shared<PidToPacketsInfoMap>(); // Map to store packets by PID
    UnixSocketClientPtr unixClient = std::make_shared<UnixSocketClient>(); // Create Unix socket client
    pid_t pid; // Get the pid of the current packet
    
    // subscribe to kernel module messages
    if (!netLinkClient->sendMessage("app_subscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    
    // Start the receiver thread
    std::thread listener(recvThread, NetLinkClientRecievePtr(netLinkClient), messageQueue);

    // Main loop: poll the queue for messages
    while (true) {
        
        const pckt_info* pckt = messageQueue->pop();
        if (!pckt) {
            // If queue is empty, wait a bit before checking again (avoid busy looping)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        if(pidToPacketsMap->containsPacket(pckt)) continue;// Check if the packet already exists in the map, if so, skip it
        
        //find packets dest pid and insert to map
        if(!unixClient->sendPort(pckt->dst_port)) break; // Send the packets port to the deamon
        if(!unixClient->receivePid(pid)) break; // Receive the pid from the deamon
        // Prepare for printing IP address
        struct in_addr ip_addr;
        ip_addr.s_addr = pckt->src_ip;
        
        // Print packet info
        if(pid != -1) {
            std::cout << "Received packet: Pid "<< pid << " src_ip: " << inet_ntoa(ip_addr) << ", proto: " << pckt->proto << "\n";
        } else {
            std::cout << "Received packet: Pid unknown src_ip: " << inet_ntoa(ip_addr) << ", proto: " << pckt->proto << "\n";
        }
        pidToPacketsMap->insertPacketInfo(pid, pckt); // Insert the packet into the map    
    }

    // Stop receiver thread
    listener.join();

    // Unsubscribe from kernel module messages
    if (!netLinkClient->sendMessage("app_unsubscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    
    // Free all stored packets
    for (const auto& pair : pidToPacketsMap->getMap()) {
        for (const pckt_info* pckt : pair.second) {
            netLinkClient->freePacketInfo(pckt); // Free the allocated memory for the message
        }
    }

    return 0;
}

// The thread that will listen and receive messages from the kernel module
void recvThread(NetLinkClientRecievePtr client, MessageQueuePtr messageQueue) {
    while (true) {
        const pckt_info* pckt = client->receivePacketInfo();
        if (pckt) {
            messageQueue->push(pckt);
        }
    }
}

