#include <thread>
#include <ThreadSafeUnorderedMap.h>

// This class is used to manage threads in a thread safe manner, since for safety the general
// ThreadSafeUnorderedMap does not support move and join 
class AppThreadsMap : public  ThreadSafeUnorderedMap<int, std::thread> {
    public:
        void joinAll();// joins all threads in the map

        // Thread needs to be moved, not copied, so we use std::move
        // and we use rvalue and move to make sure theres no option to change
        // the thread outside the class, for safe multy threading
        void insertAppThread(int clientFd, std::thread&& t);
        
        ~AppThreadsMap();
    };
    