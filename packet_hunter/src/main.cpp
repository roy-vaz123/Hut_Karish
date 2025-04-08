#include "UserSpaceConfig.h"// for useful headers and shared pointers
#include "UnixSocketClient.h"// for the client
#include "PidToPacketsInfoMap.h"// for the map

#include <vector>
// To print ip address
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
   
int main() {
    
    // Capture signals to end the program
    std::signal(SIGINT, handleSignal); // For Ctrl+C
    std::signal(SIGTERM, handleSignal);// For terminate (will be sent from main menu script) 

    NetLinkClientPtr netLinkClient = std::make_shared<NetLinkClient>();// Create Netlink client
    MessageQueuePtr messageQueue = std::make_shared<MessageQueue>();// Create the message queue
    PidToPacketsInfoMap pidToPcktMap; // Map to store packets by PID
    UnixSocketClient unixClient; // Create Unix socket client
    pid_t pid; // Get the pid of the current packet
    

    // subscribe to kernel module messages
    if (!netLinkClient->sendMessage("packet_hunter_subscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    
    // Start the receiver thread, std::ref meeded to pass reference to thread 
    std::thread packetsListener(SharedUserFunctions::recvPacketInfoThread, NetLinkClientRecievePtr(netLinkClient), messageQueue, std::ref(running));

    // Main loop: poll the queue for messages
    while (running) {
        
        const pckt_info* pckt = messageQueue->pop();
        if (!pckt) {
            // If queue is empty, wait a bit before checking again (avoid busy looping)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            // Check if daemon is still alive, if its not, close the packet_hunter
            if(!unixClient.isServerAlive()) running = false;
            continue;
        }
        if(pidToPcktMap.containsPacket(pckt)) continue;// Check if the packet already exists in the map, if so, skip it
        
        // Delay a bit to allow daemon to find pid, if the port is new for it
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        //find packets dest pid and insert to map
        if(!unixClient.sendPort(pckt->dst_port)) break; // Send the packets port to the deamon
        if(!unixClient.receivePid(pid)) break; // Receive the pid from the deamon
        
        // Prepare for printing IP address
        struct in_addr ip_addr;
        ip_addr.s_addr = pckt->src_ip;
        
        // Print packet info
        if(pid != -1) {
            std::cout << "Received packet: Pid "<< pid << " src_ip: " << inet_ntoa(ip_addr) << ", proto: " << pckt->proto << "\n";
        } else {
            std::cout << "Received packet: Pid unknown src_ip: " << inet_ntoa(ip_addr) << ", proto: " << pckt->proto << "\n";
        }
        pidToPcktMap.insertPacketInfo(pid, pckt); // Insert the packet into the map    
    }

     // Unsubscribe from kernel module messages and join packet listener thread
     if (!netLinkClient->sendMessage("packet_hunter_unsubscribe")) {
        std::cerr << "Failed to send message to kernel\n";
        return -1;
    }
    packetsListener.join();
 
    // Free all stored packets in messages queue
    SharedUserFunctions::cleanMessageQueue(messageQueue, netLinkClient);

    // Free all stored packets in the map
    for (const auto& pair : pidToPcktMap.getMap()) {
        for (const pckt_info* pckt : pair.second) {
            netLinkClient->freePacketInfo(pckt); // Free the allocated memory for the message
        }
    }
    std::cout << "Packet hunter terminated "<< std::endl;
    return 0;
}



