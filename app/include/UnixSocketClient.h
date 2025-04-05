#pragma once

#include "UnixSocketConfig.h"


// UnixSocketClient handles client-side communication with the daemon.
// It connects to a UNIX socket, sends port numbers, and receives PIDs in response.
class UnixSocketClient {
public:
    // Constructor: takes path to the UNIX socket file (e.g., /tmp/portmon_daemonApp.sock)
    UnixSocketClient();
    ~UnixSocketClient();

    // Connects to the daemon server
    bool connectToServer();

    // Sends a port number to the server
    bool sendPort(uint16_t port) const;

    // Receives the associated PID for the previously sent port
    bool receivePid(pid_t& pid) const;

    // Closes the socket
    void disconnect();

    // Returns the socket file descriptor
    int getSocketFd() const;

private:
    std::string socketPath; // Path to the socket
    int sockFd;             // File descriptor for the socket
    bool isConnected;       // Tracks connection status
};
