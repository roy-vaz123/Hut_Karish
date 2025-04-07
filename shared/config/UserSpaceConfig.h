#pragma once
// include user space shared data, include multythreading shared pointers
// and Unix domain config
#include "ThreadSafeUnorderedMap.h"
#include "NetLinkClient.h"
#include "MessageQueue.h"
#include <thread>
#include <csignal> // for end program singnal
#include <atomic> // for multythread bool (running) 
#include <chrono> // To sleep the thread, avoid busy looping
#include <memory> // For shared_ptr

// Shared pointers to share data through threads, safe to use end easier to manage the global vars
using MessageQueuePtr = std::shared_ptr<MessageQueue>;
using NetLinkClientPtr = std::shared_ptr<NetLinkClient>;// not const because send messages not const
using NetLinkClientRecievePtr = std::shared_ptr<const NetLinkClient>;

// Tells the threads when to stop
static std::atomic<bool> running{true};

// Handle signal from main manu script (use to safely end the program)
static void handleSignal(int signum) {
    running = false;
}

// Free all remaing packets in the message queue
static void cleanMessageQueue(MessageQueuePtr messageQueue, NetLinkClientPtr client){
    while (!messageQueue->empty()){
    const pckt_info* pckt = messageQueue->pop();
    if (pckt) {
        client->freePacketInfo(pckt);// free the allocated memory for the message
    }
}
}