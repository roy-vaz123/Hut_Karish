#include "PidToPacketsInfoMap.h"


// Compare the packet info structures
bool comparePacketInfo(const pckt_info* a, const pckt_info* b) {
    return (a->src_ip == b->src_ip && a->dst_ip == b->dst_ip &&
            a->src_port == b->src_port && a->dst_port == b->dst_port &&
            a->proto == b->proto);
}

// Finds if packet already exists in the map, to avoid unnecessary insertions and daemon requests
bool PidToPacketsInfoMap::containsPacket(const pckt_info* newPacket) const{
    // Iterate through the map
    for (const auto& pair : this->map) {
        for(const auto& packet : pair.second) {
            // Compare the packet info structures
            if (comparePacketInfo(packet, newPacket)) {
                return true; // Packet info found
            }
        }
    }
    return false; // Packet info not found
}

// Insert a packet info into the map
void PidToPacketsInfoMap::insertPacketInfo(pid_t pid, const pckt_info* newPacket) {
    if(pid == -1) return; // Dont insert packets with unknown pid
    
    // If the pid is not in the map, create a new vector for it
    if(this->map.find(pid) == this->map.end()) {
        this->map[pid] = std::vector<const pckt_info*>();
    }
    this->map[pid].push_back(newPacket);// Add the packet info to the vector of that pid
}

// Returns a const ref to the map
const pidToPcktMap& PidToPacketsInfoMap::getMap() const{
    return map;
}
