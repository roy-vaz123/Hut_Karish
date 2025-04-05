#include "AppThreadsMap.h"

void AppThreadsMap::joinAll() {
    std::unique_lock<std::shared_mutex> lock(this->mtx); // Unique lock for changing the map
    for (auto& pair : this->map) {
        if (pair.second.joinable()) {
            pair.second.join();
        }
    }
    this->map.clear(); // For cleaness, clear the map after joining
}

// Inserts a new appthread into the map, using std::move to avoid copying
void AppThreadsMap::insertAppThread(int clientFd, std::thread&& t){
    std::unique_lock<std::shared_mutex> lock(this->mtx); // Unique lock for changing the map
    this->map[clientFd] = std::move(t);
}

// for cleaner destructor
AppThreadsMap::~AppThreadsMap() {
    joinAll(); 
}