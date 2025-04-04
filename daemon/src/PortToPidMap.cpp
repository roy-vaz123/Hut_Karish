#include "PortToPidMap.h"

// ctor initializes the portToPidMap with the data from ScanFiles
PortToPidMap::PortToPidMap():  portToPidMap(ScanFiles::initializePortPidMap()),  mtx(){}

bool PortToPidMap::addMapping(uint16_t port, char packetProtocol) {
    std::unique_lock lock(mtx);
    pid_t pid = ScanFiles::scanForPidByPort(port, packetProtocol);// find the pid of the process using the port
    
    if(pid != -1) {// if pid is found
        portToPidMap[port] = pid;// update the map
        return true;
    } else {// logging
        std::cerr << "Failed to find PID in files for port: " << port << std::endl;
    }
    return false;

}// Unlocks automatically when going out of scope

pid_t PortToPidMap::getPid(uint16_t port) const {
    std::shared_lock lock(mtx); // Use shared lock for read-only access
    
    if (portToPidMap.find(port) != portToPidMap.end()) {
        return portToPidMap.at(port); // Return the PID if found
    }
    
    return -1; // Return -1 if PID not found
}// Unlocks automatically when going out of scope

const std::unordered_map<uint16_t, pid_t>& PortToPidMap::getMap() const {
    std::shared_lock lock(mtx); // Use shared lock for read-only access
    return portToPidMap; // Return the entire map
}