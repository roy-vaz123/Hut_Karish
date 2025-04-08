#include "PortToPidMap.h"

// Fills the map with the current port to pid relations in the system (pid listens to the port)
PortToPidMap::PortToPidMap(){
    
    ScanFiles::initializePortPidMap(map);// Scan the files to fill map
    
    syslog(LOG_INFO, "Port Pid Mapping When Daemon Started:");
    for(const auto& pair : map){// logging
        syslog(LOG_INFO, "port %u pid %d", pair.first, pair.second);
    }
    
}

// Adds new port to pid mapping to map, return false if cant find pid for port
bool PortToPidMap::addPidMapping(uint16_t port, char protocol){
    std::unique_lock<std::shared_mutex> lock(mtx);// Unique lock for changing the map
    
    pid_t pid = ScanFiles::scanForPidByPort(port, protocol);// find the pid of the process using the port
    if(pid == -1) {
        syslog(LOG_ERR, "Failed to find PID for port %u packet type: %c", port, protocol);
        return false; // failed to find pid
    }
    
    // update the port pid map with the new pid from files 
    map[port] = pid;
    return true;
}

// Tries to get the pid that listens to the port from the map, return -1 if not found
pid_t PortToPidMap::getPid(uint16_t port) const {
    std::shared_lock<std::shared_mutex> lock(mtx);// Lock that allows multyple reading
    
    if (map.find(port) != map.end()) {
        return map.at(port);
    }
    return -1;
}