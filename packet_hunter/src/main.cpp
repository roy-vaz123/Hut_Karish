#include "UserSpaceConfig.h"// for useful headers and shared pointers
#include "UnixSocketClient.h"// for the client
#include "PidToPacketsInfoMap.h"// for the map
#include <fstream> // to Save the map
#include <vector>
// To print ip address
#include <arpa/inet.h>
#include <netinet/in.h>

// To find project folder for saving map
#include <filesystem>
namespace fs = std::filesystem;

// Format and print the packet information
void printPacketInfo(const pckt_info* pckt, pid_t pid);

// Ask user to save the packet map and write it to a file (default: hut_karish/packets.log)
void savePacketMapToFile(const PidToPacketsInfoMap& pidToPcktMap, NetLinkClientRecievePtr client);

   
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
        
        printPacketInfo(pckt, pid);

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

    // Asks the user if he wants to save the captured packets
    char saveChoice;
    std::cout << "Do you want to save the packets hunted to a file? (y/n): ";
    std::cin >> saveChoice;

    if (saveChoice == 'y' || saveChoice == 'Y'){
        savePacketMapToFile(pidToPcktMap, netLinkClient);
    }
    else{
        // Free all stored packets in the map
        for (const auto& pair : pidToPcktMap.getMap()) {
            for (const pckt_info* pckt : pair.second) {
                netLinkClient->freePacketInfo(pckt); // Free the allocated memory for the message
            }
        }
    }
    
    std::cout << "Packet hunter terminated "<< std::endl;
    return 0;
}

// Prints the full packets info of incoming packet
void printPacketInfo(const pckt_info* pckt, pid_t pid) {
    struct in_addr src, dst;// to properly print ip address
    src.s_addr = pckt->src_ip;
    dst.s_addr = pckt->dst_ip;
    
    // Write full protocol name
    std::string proto;
    if (pckt->proto == 'T'){
        proto = "TCP";
    } 
    else if (pckt->proto == 'U') {
        proto = "UDP";
    }else{
        proto = "other";
    }

    if (pid != -1) {
        std::cout << "PID: " << pid << " | " << "Proto: " << proto << " | "<< "Src: " << inet_ntoa(src) << ":" << pckt->src_port << " → "<< "Dst: " << inet_ntoa(dst) << ":" << pckt->dst_port << std::endl;
    } else {
        std::cout << "PID: unknown  | " << "Proto: " << proto << " | "<< "Src: " << inet_ntoa(src) << ":" << pckt->src_port << " → "<< "Dst: " << inet_ntoa(dst) << ":" << pckt->dst_port << std::endl;
    }
}

// Ask user to save the packet map and write it to a file (default: hut_karish/packets.log)
void savePacketMapToFile(const PidToPacketsInfoMap& pidToPcktMap, NetLinkClientRecievePtr client) {
    // This gives you the actual directory where the binary lives, so save log in project folder
    fs::path exePath = fs::canonical("/proc/self/exe");
    fs::path logPath = exePath.parent_path() // packet_hunter/
                            .parent_path() // build/
                            .parent_path() // hut_karish/  
                            / "packets.log"; 

    std::string path = logPath.string(); // final usable string
    std::string input;
    std::cout << "Enter file path [default: " << path << "]: ";
    std::cin.ignore(); // Flush leftover newline from previous cin
    std::getline(std::cin, input);
    if (!input.empty()) path = input;

    // Try to open the file (will be created if it doesn't exist)
    std::ofstream out(path);
    if (!out) {
        std::cerr << "Error: failed to open file for writing: " << path << std::endl;
        return;
    }

    // Write packet data to file
    for (const auto& pair : pidToPcktMap.getMap()) {
        pid_t pid = pair.first;
        for (const pckt_info* pckt : pair.second) {
            struct in_addr src, dst;
            src.s_addr = pckt->src_ip;
            dst.s_addr = pckt->dst_ip;
            std::string proto = (pckt->proto == 'T') ? "TCP" :
                                (pckt->proto == 'U') ? "UDP" : "Other";

            out << "PID: " << pid
                << " | Proto: " << proto
                << " | Src: " << inet_ntoa(src) << ":" << pckt->src_port
                << " → Dst: " << inet_ntoa(dst) << ":" << pckt->dst_port
                << std::endl;
            
            client->freePacketInfo(pckt);// Frees packet info using client (the same allocator tha allocate it for safety)
        }
        
    }

    out.close();// Close file
    std::cout << "Saved packet map to: " << path << std::endl;
}