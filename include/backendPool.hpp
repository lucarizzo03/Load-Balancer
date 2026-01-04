#pragma once
#include <iostream>
#include <sys/socket.h>   // Core: sockaddr, sockaddr_storage, socket(), bind()
#include <netinet/in.h>   // Internet: sockaddr_in, sockaddr_in6, htons()
#include <vector> 
#include <atomic>
#include <shared_mutex>
#include <thread>

using namespace std;

struct Backend {
    struct sockaddr_storage address; // holds both IPv4/IPv6
    socklen_t addr_len; // socklen_t universal size for address length
    atomic<bool> isHealthy = true; 

    // Default constructor
    Backend() = default;
    
    // Copy constructor - manually copy atomic value
    Backend(const Backend& other) 
        : addr_len(other.addr_len), 
          isHealthy(other.isHealthy.load()) {  // Load atomic value
        memcpy(&address, &other.address, sizeof(address));
    }
    
    // Copy assignment operator
    Backend& operator=(const Backend& other) {
        if (this != &other) {
            addr_len = other. addr_len;
            isHealthy.store(other.isHealthy.load());  // Load then store
            memcpy(&address, &other.address, sizeof(address));
        }
        return *this;
    }
};

class BackendPool {
public:
    BackendPool();
    ~BackendPool();

    // add server to backend pool 
    void storeNewAddress(const struct sockaddr* addr);

    // LB algos
    optional<Backend> RoundRobin();

    // Pool Statistics
    size_t getHealthyCount() const;
    size_t getUnhealthyCount() const;
    void printStatus() const;

    // returns servers 
    vector<Backend>& getBackend();

    // protects everything below it by being accessed by other threads, Ex: "dont touch servers vector while im using it"
    mutable shared_mutex serversMutex; 
    
private:
    vector<Backend> servers;
    atomic<size_t> currInd = 0;
};