// A thread safe map to recive port and map them in main, and interact with packet hunter on another thread
#pragma once
#include <stdint.h>// int types 
#include <sys/types.h>       
#include <unordered_map>
#include <mutex>
#include <shared_mutex>// mutex that allows multyple reads at once (not really needed here but pretty nice)
#include "ScanFiles.h"// To search the files for port, pid
#include <iostream>

class PortToPidMap{
public:
    
    // ctor initilizes the db  with existing port to pid relations using ScanFiles
    PortToPidMap();
    
    // Adds new port to pid mapping to map, return false if cant find pid for port
    bool addPidMapping(uint16_t port, char protocol);

    // Tries to get the pid that listens to the port from the map, return -1 if not found
    pid_t getPid(uint16_t port) const;

private:
    mutable std::shared_mutex mtx;// mutable allows const methods to lock, shared mutex allows multiple readers
    std::unordered_map<uint16_t, pid_t> map;
};
