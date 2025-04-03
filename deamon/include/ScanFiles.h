#pragma once

#include <sys/types.h>
#include <unordered_map>
#include <cstdint>

// Functions use to scan the system files, to find the PID of the process that is using a specific port
// well do that by first, find the inode of the socket using that port and then find the PID of the process using that inode
// then scaning the processes in the system trying to find the PID of the process using that inode, istead of going through all the processes
// go over all of every process socket and check if its port matches the one we are looking for
namespace ScanFiles {

    using PortPidMap = std::unordered_map<uint16_t, pid_t>;

    // Full scan on startup
    PortPidMap initializeDB();

    // Real-time scan for one port
    pid_t scanForPidByPort(uint16_t port);

}
