#pragma once

#include <queue>
#include <mutex>
#include "NetLinkConfig.h"

// Thread safe queue for pckt_info* packtets, to hold them before find pid and insert to map 
class MessageQueue {
public:
    void push(const pckt_info* pkt);         // Add packet to queue used in recv thread
    const pckt_info* pop();                  // Pop packet from queue used in main thread)
    bool empty();                      

private:
    std::queue<const pckt_info*> queue;
    std::mutex mtx;                   // Mutex protects access to the queue
};
