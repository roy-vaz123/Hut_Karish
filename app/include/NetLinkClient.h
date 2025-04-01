#pragma once

#include <string>
#include <linux/netlink.h>
#include "NetLinkConfig.h"

// Netlink client for sending/receiving messages with kernel
class NetlinkClient {
public:
    NetlinkClient();                            // constructor: create and bind socket
    ~NetlinkClient();                           // destructor: clean up socket

    bool sendMessage(const std::string& msg);   // send string to kernel
    bool receiveMessage();                      // receive reply from kernel

private:
    int sock_fd;                  // socket file descriptor
    sockaddr_nl src_addr;       // user-space address
    sockaddr_nl dest_addr;      // kernel address
};
