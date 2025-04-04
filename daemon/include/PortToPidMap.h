#include "ScanFiles.h"
#include <unordered_map> // Store packets by PID
#include <mutex> // mutex for thread-safe access
#include <shared_mutex>// mutex that allows multiple reads simutanesly
#include <iostream>

class PortToPidMap {
public:
    
    // ctor initializes the portToPidMap with the data from ScanFiles
    PortToPidMap();

    // Add a new port pid mapping
    bool addMapping(uint16_t port, char packetProtocol);

    // Get the PID for a given port
    pid_t getPid(uint16_t port) const;

    // Get a const ref of the entire map of ports to PIDs
    const std::unordered_map<uint16_t, pid_t>& getMap() const;


private:
    std::unordered_map<uint16_t, pid_t> portToPidMap; // Map of ports to PIDs
    mutable std::shared_mutex mtx; // Mutex for thread-safe access, mutable allows const methods to lock

};