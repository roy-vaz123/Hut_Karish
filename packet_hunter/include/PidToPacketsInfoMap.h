#pragma once
#include "NetLinkConfig.h" // for pckt_info
#include "ThreadSafeUnorderedMap.h"
#include <vector>
class PidToPacketsInfoMap : public ThreadSafeUnorderedMap<pid_t, std::vector<const pckt_info*>> {
public:
    // Insert a packet info into the map
    void insertPacketInfo(pid_t pid, const pckt_info* packetInfo);
    
    // Check if packt exist on the map
    bool containsPacket(const pckt_info* newPacket) const;
};