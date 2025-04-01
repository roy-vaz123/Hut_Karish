#include "MessageQueue.h"

void MessageQueue::push(const pckt_info* pkt) {
    std::lock_guard<std::mutex> lock(mtx); // Lock while modifying queue
    queue.push(pkt);
}// Automatically unlocks when going out of scope

const pckt_info* MessageQueue::pop(){
    std::lock_guard<std::mutex> lock(mtx); // Lock while accessing queue
    if (queue.empty()) return nullptr;

    const pckt_info* pkt = queue.front();
    queue.pop();
    return pkt;
}

bool MessageQueue::empty() {
    std::lock_guard<std::mutex> lock(mtx);
    return queue.empty();
}
