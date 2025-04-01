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
    pckt_info* receiveMessage() const;                     

private:
    int sock_fd;                  // socket file descriptor
    sockaddr_nl src_addr;       // user-space address
    sockaddr_nl dest_addr;      // kernel address
};
