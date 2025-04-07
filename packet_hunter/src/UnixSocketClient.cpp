#include "UnixSocketClient.h"

// Ctor inilialize Unix clint socket, and connect to server
UnixSocketClient::UnixSocketClient()
    : socketPath(SOCKET_FILE_ADRESS), sockFd(-1), isConnected(false) { connectToServer(); }// Initialize socket path and file descriptor

// Disconnect from server on destruction
UnixSocketClient::~UnixSocketClient() {
    disconnect();
}

// Connect to server (called in ctor)
bool UnixSocketClient::connectToServer() {
    // Create a unix domain socket (sock_stream is rlieable, like tcp but local)
    sockFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockFd < 0) {
        perror("socket");
        return false;
    }

    // Define the socket address
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;// set sockets adress family to unix
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);// Copy the socket file location into the struct


    // Attempt to connect to the server
    if (connect(sockFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {// failed to connect
        perror("connect"); 
        disconnect();
        return false;
    }
    std::cout << "Connected succeccfully to port monitor daemon"<< std::endl;
    isConnected = true;
    return true;
}

// Send a uint16_t port to the daemon server
bool UnixSocketClient::sendPort(uint16_t port) const {
    if (!isConnected) return false;// check if the socket is connected
    
    std::cout << "Sending PID reqest from port monitor daemon for port: "<< port << std::endl;
    // send raw uint16_t to server, if size sent matchers the size of uint16_t, sent succecfully
    ssize_t bytesSent = send(sockFd, &port, sizeof(port), 0);
    return bytesSent == sizeof(port);
}

// Recieves pid from daemon server
bool UnixSocketClient::receivePid(pid_t& pid) const {
    if (!isConnected) return false;// check if the socket is connected

    // Receive raw pid_t from server, if size sent matchers the size of pid_t, recieved succecfully
    ssize_t bytesReceived = recv(sockFd, &pid, sizeof(pid), 0);// blocking call
    return bytesReceived == sizeof(pid);
}

// Disconnect from server, used in destructor
void UnixSocketClient::disconnect() {
    if (isConnected) {
        close(sockFd);// close the socket
        sockFd = -1; // common safe practive
        isConnected = false;
    }
}

int UnixSocketClient::getSocketFd() const {
    return sockFd;
}
