#include "ScanFiles.h"
// Files handling 
#include <filesystem>
#include <fstream>
// Extract information from the files
#include <regex>
#include <string>
#include <sstream>
#include <vector>// Keep vector of (inode, port) pairs
#include <cstddef> // For size_t
#include <iostream>// debugging for now (maybe remove later)

// to clean the code a bit
namespace fs = std::filesystem;
using sockInodePortVec = std::vector<std::pair<std::string, uint16_t>>;

// Parse /proc/net/tcp or /proc/net/udp and extract, inode, port for sockets that are listening or bound.
sockInodePortVec parseListeningSockets(const std::string& path, uint16_t filterPort = 0) {
        
    std::ifstream sockFile(path);// open the file as stream (divided by lines)
    
    std::string line;// buffer for each line
    std::string portHex;// hex port string
    std::string stateHex;// hex state string
    std::smatch match;// regex match result, match each part of the regex in () to a cell
    
    sockInodePortVec result;

    // Match format: sl  local_address rem_address   st (the format of the sockets in the proc/net/tcp and udp files)
    std::regex sockRegex(R"(\s*\d+:\s[0-9A-F]{8}:(\w{4})\s[0-9A-F]{8}:\w{4}\s(\w{2}))");

    // Go over each line in the file, try to match the regex
    while (std::getline(sockFile, line)) {
        if (std::regex_search(line, match, sockRegex)) {// extracting the port and state from the line
            portHex = match[1];
            stateHex = match[2];

            // Convert hex port string to uint16 (the format of the port sent from the kerne module)
            // and check if it is the port we are looking for (or 0 for initialization)
            uint16_t port = static_cast<uint16_t>(std::stoul(portHex, nullptr, 16));
            if (filterPort != 0 && port != filterPort){
                continue;
            } 

            // Extract the inode of the socket from the line
            std::istringstream iss(line);// Treat the line as a stream (seprate tokens by spaces)
            std::string token;
            int field = 0;
            while (iss >> token) {// Go over each token in the line
                if (field == 9) {// in the 10th socket creates the pair (token, port) in the result vector next available cell
                    // Skip if inode is "0" or malformed
                    if (token != "0" && std::all_of(token.begin(), token.end(), ::isdigit)) {
                        result.emplace_back(token, port);
                    } 
                    break;
                }
                field++;
            }
        }
    }

    return result;
}

// Given a socket inode ( a socket we now is active and tied to relevant port), return the PID that owns it by scanning /proc/[pid]/fd/* links
pid_t findPidBySockInode(const std::string& sockInode) {
    
    std::string pidStr;// pid folder name (heave to be a number)
    std::string fdPath;// pid folder + /fd, the file descriptor folder well look for the socket in
    std::error_code ec;// error code for the read_symlink function
    std::string link;// symlink to the socket
    
    // Go over proc directory (contains all the processes in the system)
    for (const auto& dir : fs::directory_iterator("/proc")) {
        
        if (!dir.is_directory()) continue; // Ignore files (we want to search process folders)
        
        pidStr = dir.path().filename();// Get folder name, ignore if not a number 
        if (!std::all_of(pidStr.begin(), pidStr.end(), ::isdigit)) continue;
        
        fdPath = dir.path().string() + "/fd";// get pid file descriptor folder, ignore if not exist (kernel tasks or zombies)
        if (!fs::exists(fdPath)) continue;

        // Go over each file descriptor in the pid/fd folder
        for (const auto& fd : fs::directory_iterator(fdPath)) {
            
            link = fs::read_symlink(fd.path(), ec).string();// If fails, continue to next fd
            if (ec) continue;

            // We looking for symlinks in format: socket:[123456], continue if not found
            std::size_t start = link.find("socket:[");
            if (start == std::string::npos) continue;

            // Get the inode of the socket from the symlink and compare it to the one we are looking for
            if (link.substr(8, link.size() - 9) == sockInode) {
                return std::stoi(pidStr);// If found, return the pid 
            }
        }
    }

    return -1; // not found
}

namespace ScanFiles {
    // Intialize the map of ports to PIDs by scanning the system files
    void initializePortPidMap(std::unordered_map<uint16_t, pid_t>& map) {
        std::string inode;
        uint16_t port;
        pid_t pid;
        
        // Get all TCP sockets
        sockInodePortVec tcpSockets = parseListeningSockets("/proc/net/tcp");

        // Get all UDP sockets
        sockInodePortVec udpSockets = parseListeningSockets("/proc/net/udp");

        // For each TCP socket, find its owning PID and map it
        for (const auto& socket : tcpSockets) {
            inode = socket.first;
            port = socket.second;
        
            // Find the pid of the process using the socket inode, and update the map
            pid = findPidBySockInode(inode);
            if (pid != -1) {
                map[port] = pid;
            }
        }

        // For each UDP socket find PID only if not already mapped by TCP
        for (const auto& socket : udpSockets) {
            inode = socket.first;
            port = socket.second;

            if (map.find(port) != map.end()) continue;// give priority to tcp port on first scan  
            
            // Find the pid of the process using the socket inode, and update the map
            pid = findPidBySockInode(inode);
            if (pid != -1) {
                map[port] = pid;
            }
        }
    }

    // Find new port linked pid if its not already in the map
    pid_t scanForPidByPort(uint16_t port, char protocol) {
        // sockInodePortVec pair members
        std::string inode;
        pid_t pid;
        

        if(protocol == 'T'){
            // search port in TCP LISTEN sockets
            sockInodePortVec tcpSockets = parseListeningSockets("/proc/net/tcp", port);
            for (const auto& socket : tcpSockets) {
                inode = socket.first;

                syslog(LOG_INFO, "found tcp socket port %u for socket inode %s", socket.second, inode.c_str());
                
                // Find the pid of the process using the socket inode, and update the map
                pid = findPidBySockInode(inode);
                return pid;
            }
        }else{
            // search port in UDP sockets
            sockInodePortVec udpSockets = parseListeningSockets("/proc/net/udp", port);
            for (const auto& socket : udpSockets) {
                inode = socket.first;
                
                syslog(LOG_INFO, "found udp socket port %u for socket inode %s", socket.second, inode.c_str());
                
                // Find the pid of the process using the socket inode, and update the map
                pid = findPidBySockInode(inode);
                return pid;
            }
        }

        return -1; // not found
    }
}
