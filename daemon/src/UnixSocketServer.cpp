#include "UnixSocketServer.h"
#include <sys/socket.h> // socket functions (accept, bind) and constants
#include <sys/un.h>// unix domain socket adress structure (sockaddr_un)
#include <cstring> // string functions
#include <iostream>// debug, might move later

// takes the path to the socket file as input
UnixSocketServer::UnixSocketServer(const std::string& socketPath)
    : socketPath(socketPath), serverFd(-1){}

// clean up socket if still open
UnixSocketServer::~UnixSocketServer() {
    stop();
}

// Starts the unix socket server. return true on success and false on failure
bool UnixSocketServer::start() {
    // Create a unix domain socket (sock_stream is rlieable, like tcp but local)
    serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverFd < 0) {
        perror("socket");// for testing change to loggong later
        return false;
    }

    // Define the socket address
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;// set sockets adress family to unix
    strncpy(addr.sun_path, socketPath_.c_str(), sizeof(addr.sun_path) - 1);// Copy the socket file location into the struct

    // Remove any existing file at the path (clean old sockets)
    unlink(socketPath_.c_str());

    // Bind the socket to the file path
    if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");// for testing change to loggong later
        return false;
    }

    // Start listening for incoming connections, somaxcon to allow maximum pending connections
    if (listen(serverFd, SOMAXCONN) < 0) {
        perror("listen");
        close(serverFd);
        return false;
    }

    std::cout << "Server socket setup complete. Waiting for clients...\n";// logging
    return true;

}

void UnixSocketServer::stop() {
    if (serverFd != -1) {
        close(serverFd);
        serverFd = -1; // common safe practive
        unlink(socketPath.c_str());  // Clean up the socket file
    }
}

// Send a uint16_t value to a specific client
bool UnixSocketServer::sendPid(int clientFd, pid_t pid) const {
    ssize_t sent = write(clientFd, &pid, sizeof(pid));// It doest matter that pid is local since we only need the value
    if(sent == sizeof(pid)) return true;
    return false;// If they arnt equal the was problem with sending the message
}

// Receive a port number from a specific client, check if message recieved or client disconnected (write to port memory)
bool UnixSocketServer::receivePort(int clientFd, uint16_t& port) const {
    ssize_t bytes = read(clientFd, &port, sizeof(port));
    
    // Check if message recieved succecsfully
    if (bytes == 0) {// If the message is 0 bytes, the client disconnected
        std::cout << "Client disconnected (fd=" << clientFd << ")\n";
        return false;
    } else if (bytes < 0) {// these are probably problems with the socket
        perror("read");
        return false;
    } else if (bytes != sizeof(port)) {
        std::cerr << "Bad message (fd=" << clientFd << ")\n";
        return false;
    }

    return true;
}
int UnixSocketServer::getServerFd() const{
    return this->serverFd;
}
