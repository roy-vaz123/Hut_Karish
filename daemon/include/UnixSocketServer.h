#pragma once
#include <string>
#include <unistd.h>// for socket functions 
#include "UnixSocketConfig.h"  // Shared struct definition


class UnixSocketServer {
public:
    // ctor takes the path to the socket file
    UnixSocketServer(const std::string& socketPath);
    ~UnixSocketServer();

    // Starts the server 
    bool start();

    // Stops the server (closes client/server fds, unlinks socket path)
    void stop();

    // Send a message to the connected client
    bool sendPid(int clientFd, pid_t pid) const;

    // Receive a message from a connected client
    bool receivePort(int clientFd, uint16_t& port) const;

    // returns the servers fd
    int getServerFd() const;

private:
    std::string socketPath;  // Path to the unix socket file, will be at /tmp/portmon_daemonApp.sock
    int serverFd;            // File descriptor for listening socket
};
