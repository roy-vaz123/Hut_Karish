#pragma once

#include <vector>
#include <string>
#include <linux/netlink.h>
#include "NetLinkConfig.h"

// Netlink client for sending/receiving messages with kernel
class NetLinkClient {
public:
    NetLinkClient();                            // constructor: create and bind socket
    ~NetLinkClient();                           // destructor: clean up socket

    bool sendMessage(const std::string& msg);   // send string to kernel
    
    // receive packets info from kernel (for now also interpret it, see explenation on the considerations
    // and alternative approach in the implementaion of this function)
    const pckt_info* receivePacketInfo() const;  
    void freePacketInfo(const pckt_info* pkt) const; // free the packet info structure, the same allocator that allocated the memory (extra safety)
    

private:
    int sock_fd;                  // socket file descriptor
    sockaddr_nl src_addr;       // user-space address
    sockaddr_nl dest_addr;      // kernel address
};
