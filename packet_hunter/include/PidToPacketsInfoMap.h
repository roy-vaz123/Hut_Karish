#pragma once
#include "NetLinkConfig.h" // for pckt_info
#include <unordered_map>
#include <vector>

using pidToPcktMap = std::unordered_map<pid_t, std::vector<const pckt_info*>>;

class PidToPacketsInfoMap {
public:
    // Insert a packet info into the map
    void insertPacketInfo(pid_t pid, const pckt_info* packetInfo);
    
    // Check if packt exist on the map
    bool containsPacket(const pckt_info* newPacket) const;

    // Returns a const ref to the map
    const pidToPcktMap& getMap() const;
private:
    pidToPcktMap map;
};