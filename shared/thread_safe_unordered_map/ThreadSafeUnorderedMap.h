#pragma once
#include <unordered_map>
#include <mutex>
#include <shared_mutex>

template <typename Key, typename Value>
class ThreadSafeUnorderedMap {
public:
    
    // Inserts or updates a value
    void insertOrAssign(const Key& key, const Value& value) {
        std::unique_lock<std::shared_mutex> lock(mtx);// Unique lock for changing the map
        map[key] = value;
    }

    // Tries to get a value, returns true if found
    const Value* get(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(mtx);
        if (map.find(key) != map.end()) {
            return &(map.at(key));
        }
        return nullptr; // didnt find the key
    }

    // Removes a key
    void erase(const Key& key) {
        std::unique_lock<std::shared_mutex> lock(mtx);
        map.erase(key);
    }

    // Clears the map
    void clear() {
        std::unique_lock<std::shared_mutex> lock(mtx);// Unique lock for changing the map
        map.clear();
    }

    // Returns const ref of the map
    const std::unordered_map<Key, Value>& getMap() const {
        std::shared_lock<std::shared_mutex> lock(mtx);
        return map;
    }
    

protected:
    mutable std::shared_mutex mtx;// mutable allows const methods to lock, shared mutex allows multiple readers
    std::unordered_map<Key, Value> map;
};
