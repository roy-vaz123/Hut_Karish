#include "UnixSocketServer.h"

// takes the path to the socket file as input
UnixSocketServer::UnixSocketServer()
    : socketPath(SOCKET_FILE_ADRESS), serverFd(-1), clientFd(-1){ start(); } // initialize the socket file path and server fd

// clean up socket if still open
UnixSocketServer::~UnixSocketServer() {
    if (serverFd != -1) {
       closeSocket();
    }
}

 // Closes the socket
 void UnixSocketServer::closeSocket(){
    closeClientFd();// close the clientFd if open
    // Close the socket
    close(serverFd);
    serverFd = -1; // common safe practive
    unlink(socketPath.c_str());  // Clean up the socket file
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
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);// Copy the socket file location into the struct

    // Remove any existing file at the path (clean old sockets)
    unlink(socketPath.c_str());

    // Bind the socket to the file path
    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");// for testing change to loggong later
        return false;
    }

    // Start listening for incoming connections, somaxcon to allow maximum pending connections
    if (listen(serverFd, SOMAXCONN) < 0) {// fail to listen
        perror("listen");// logging
        closeSocket();
        return false;
    }
    return true;

}

// Send a uint16_t value to a specific client
bool UnixSocketServer::sendPid(pid_t pid) const {
    ssize_t sent = write(clientFd, &pid, sizeof(pid));// It doest matter that pid is local since we only need the value
    return sent == sizeof(pid);// If they arnt equal the was problem with sending the message
}

// Receive a port number from a specific client, check if message recieved or client disconnected (write to port memory)
bool UnixSocketServer::receivePort(uint16_t& port) const {
    ssize_t bytes = read(clientFd, &port, sizeof(port));// blocking call
    
    // Check if message recieved succecsfully
    if (bytes == 0) {// If the message is 0 bytes, the client probably disconnected
        return false;
    }
    if (bytes < 0) {// these are probably problems with the socket
        perror("read");
        return false;
    }
    if (bytes != sizeof(port)) {
        std::cerr << "Bad message (fd=" << clientFd << ")\n";
        return false;
    }

    return true;
}

// Returns the servers fd
int UnixSocketServer::getServerFd() const{
    return this->serverFd;
}

// Wait for client connection and connect
bool UnixSocketServer::connectToClient(){
    clientFd = accept(serverFd, nullptr, nullptr);// blocking waiting for the app to connect to daemon using socket accept
    if (clientFd < 0 || isShuttingDown) {
        return false;// Failed to connecto to client
    }
    std::cout << "Client connected (fd=" << clientFd << ")\n";// logging
    return true;
}

int UnixSocketServer::getClientFd() const{
    return clientFd;
}

// Disconnects client to allow new client 
void UnixSocketServer::closeClientFd(){
    if(clientFd != -1){
        close(clientFd);
        clientFd = -1;
    }
}

// Stops the server from listening for new clients(closes client/server fds, unlinks socket path)
// by sending a fake empty connection to break the accept block
void UnixSocketServer::stopListeningForNewClients(){
    this->isShuttingDown = true;
    int tmpSock = socket(AF_UNIX, SOCK_STREAM, 0);// Opens temporary socket for dummy connection
    if (tmpSock >= 0) {
        // initilize socket to simulate client
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
        
        // connect to server to break block, then disconnect
        connect(tmpSock, (sockaddr*)&addr, sizeof(addr));
        close(tmpSock);  // one-time connection
    }
}