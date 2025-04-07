#pragma once

#include <unordered_map> // for initilize the port-pid map
#include <sys/types.h>
#include <cstdint>
#include <memory>// for shared vars (multiple threads)

// Functions use to scan the system files, to find the PID of the process that is using a specific port
// well do that by first, find the inode of the socket using that port and then find the PID of the process using that inode
// then scaning the processes in the system trying to find the PID of the process using that inode, istead of going through all the processes
// go over all of every process socket and check if its port matches the one we are looking for
namespace ScanFiles {
    // Full scan on startup
    void initializePortPidMap(std::unordered_map<uint16_t, pid_t>& map);

    // Find port linked pid if its not already in the map
    pid_t scanForPidByPort(uint16_t port, char packetProtocol);

}
