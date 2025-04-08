#pragma once

#include "UnixSocketConfig.h"  // needed headers and the socket file path

// Thought about making this singleton, but my solution with shared pointers is easier to manage with multythreading
// Used to communicate with the packet_hunters, receive port and send pid from the map
class UnixSocketServer {
public:
    // ctor takes the path to the socket file
    UnixSocketServer();
    ~UnixSocketServer();

    // Starts the server 
    bool start();

    // Stops the server from listening for new clients(closes client/server fds, unlinks socket path)
    void stopListeningForNewClients();

    // Send a message to the connected client
    bool sendPid(pid_t pid) const;

    // Receive a message from a connected client
    bool receivePort(uint16_t& port) const;

    // Wait for client connection and connect
    bool connectToClient();

    // Returns the servers fd
    int getServerFd() const;

    // Closes the socket
    void closeSocket();

    // Disconnects client to allow new client 
    void closeClientFd();

    int getClientFd() const;

private:
    std::string socketPath;  // Path to the unix socket file
    int serverFd; 
    int clientFd;
    bool isShuttingDown; // Simple flag to prevent extra loggin when shutting down
};
