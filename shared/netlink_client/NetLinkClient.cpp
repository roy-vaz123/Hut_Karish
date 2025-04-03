#include "NetLinkClient.h" // Header file for NetLinkClient class
#include <sys/socket.h> // socket, bind
#include <cstring> // memset, strncpy
#include <unistd.h> // getpid, close
#include <cstdlib> // malloc, free
#include <iostream>// Printing, debugging


// Constructor create socket and bind to this process (src_addr)
NetLinkClient::NetLinkClient() {
    
    // Create a Netlink socket: nrtlink socket family, raw socket type, NETLINK_USER protocol
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0) {
        std::cerr << "Error: Failed to create Netlink socket\n";
        return;
    }

    // Fill in source address (this user-space process)
    memset(&src_addr, 0, sizeof(src_addr));// Clear the structure memory before initilization
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();   // This process' PID
    src_addr.nl_groups = 0;       // Not joining any multicast groups, for 1:1 communication


    // Bind the socket to the source address
    if (bind(sock_fd, reinterpret_cast<sockaddr*>(&src_addr), sizeof(src_addr)) < 0) {
        std::cerr << "Error: Failed to bind Netlink socket\n";
        close(sock_fd);
        sock_fd = -1;
    }

    
    // Fill in destination address (the kernel)
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;         // 0 = kernel
    dest_addr.nl_groups = 0;      
}

// Destructor only closes the socket
NetLinkClient::~NetLinkClient() {
    if (sock_fd != -1) {
        close(sock_fd);
    }
}

// Sends a string message to the kernel
bool NetLinkClient::sendMessage(const std::string& msg) {
    if (sock_fd < 0) return false;// The socket is not created or bound

    std::cout << "Sending message to kernel: " << msg << std::endl;

    // Allocate memory for the Netlink message (header + payload) static_cast for type safety
    nlmsghdr* nlh = static_cast<nlmsghdr*>(malloc(NLMSG_SPACE(MAX_PAYLOAD)));
    if (!nlh) return false;

    // Fill in the Netlink message header
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));// NLMSG_SPACE returns the size of the Netlink message header + payload
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    // Fill in the payload with the message
    char* data = reinterpret_cast<char*>(NLMSG_DATA(nlh)); // NLMSG_DATA returns a pointer to the payload area of the Netlink message
    strncpy(data, msg.c_str(), MAX_PAYLOAD - 1); // copy the message to the payload
    data[MAX_PAYLOAD - 1] = '\0';  // ensure null-termination


    // {} to clean memory, iovec is a structure used to describe a vector of memory blocks (the netlink send format)
    iovec iov{};
    iov.iov_base = reinterpret_cast<void*>(nlh);
    iov.iov_len = nlh->nlmsg_len;

    // using the iov to create the message
    msghdr message{};
    message.msg_name = &dest_addr;
    message.msg_namelen = sizeof(dest_addr);
    message.msg_iov = &iov;
    message.msg_iovlen = 1;

    // Send the message to the kernel and clean up
    ssize_t sent = sendmsg(sock_fd, &message, 0);
    free(nlh);
    if (sent < 0) {
        std::cerr << "Error: Failed to send message to kernel\n";
        return false;
    }

    return true;
}


// Receives a reply from the kernel, allocate memory
const pckt_info* NetLinkClient::receivePacketInfo() const {
    char buffer[MAX_PAYLOAD];
    struct nlmsghdr* nlh = reinterpret_cast<struct nlmsghdr*>(buffer);

    int len = recv(sock_fd, nlh, MAX_PAYLOAD, 0);
    if (len < 0) {
        perror("recv");
        return nullptr;
    }

    size_t payloadLen = nlh->nlmsg_len - NLMSG_HDRLEN;
    if (payloadLen != sizeof(pckt_info)) {
        std::cerr << "Unexpected payload size: " << payloadLen << std::endl;
        return nullptr;
    }

    //   This copy is negligible for small packets like pckt_info.
    //   For large or structured messages, ill need to consider queueing the full Netlink
    //   buffer and parsing later in the main thread (to make sure this thread returns to listen quickly).  
    pckt_info* pckt = new pckt_info;
    std::memcpy(pckt, NLMSG_DATA(nlh), sizeof(pckt_info));
    return pckt;
}

// Free the packet info structure
void NetLinkClient::freePacketInfo(const pckt_info* pckt) const {
    delete pckt;
}