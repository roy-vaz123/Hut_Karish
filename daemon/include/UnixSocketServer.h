#pragma once

#include "UnixSocketConfig.h"  // needed headers and the socket file path

// Thought about making this singleton, but my solution with shared pointers is easier to manage with multythreading
// Used to communicate with the packet_hunters, receive port and send pid from the map
class UnixSocketServer {
public:
    // ctor takes the path to the socket file
    UnixSocketServer(const std::string& socketPath);
    ~UnixSocketServer();

    // Starts the server 
    bool start();

    // Stops the server from listening for new clients(closes client/server fds, unlinks socket path)
    void stopListeningForNewClients() const;

    // Send a message to the connected client
    bool sendPid(int clientFd, pid_t pid) const;

    // Receive a message from a connected client
    bool receivePort(int clientFd, uint16_t& port) const;

    // Returns the servers fd
    int getServerFd() const;

    // Closes the socket
    void closeSocket();

private:
    std::string socketPath;  // Path to the unix socket file, will be at /tmp/portmon_daemonApp.sock
    int serverFd;            // File descriptor for listening socket
};
