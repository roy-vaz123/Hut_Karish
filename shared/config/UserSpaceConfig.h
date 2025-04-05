#pragma once
// include user space shared data, include multythreading shared pointers
// and Unix domain config
#include "ThreadSafeUnorderedMap.h"
#include "NetLinkClient.h"
#include "MessageQueue.h"
#include <thread> 
#include <chrono> // To sleep the thread, avoid busy looping
#include <memory> // For shared_ptr

// Shared pointers to share data through threads, safe to use end easier to manage the global vars
using MessageQueuePtr = std::shared_ptr<MessageQueue>;
using NetLinkClientPtr = std::shared_ptr<NetLinkClient>;// not const because send messages not const
using NetLinkClientRecievePtr = std::shared_ptr<const NetLinkClient>;
