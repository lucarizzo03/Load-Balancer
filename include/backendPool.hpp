#include <iostream>
#include <sys/socket.h>   // Core: sockaddr, sockaddr_storage, socket(), bind()
#include <netinet/in.h>   // Internet: sockaddr_in, sockaddr_in6, htons()
#include <vector> 
#include <atomic>
#include <shared_mutex>

using namespace std;

struct Backend {
    struct sockaddr_storage address; // holds both IPv4/IPv6
    socklen_t addr_len; // socklen_t universal size for address length
    atomic<bool> isHealthy = true; 
};

class BackendPool {
public:
    // add server to backend pool 
    void storeNewAddress(const struct sockaddr* addr);

    // LB algos
    Backend* RoundRobin();

    // Pool Statistics
    size_t getHealthyCount() const;
    size_t getUnhealthyCount() const;
    void printStatus() const;


    // Pool Management
    

    
    

private:
    vector<Backend> servers;
    atomic<size_t> currInd = 0;
    mutable shared_mutex serversMutex; // protects everything below it by being accessed by other threads, Ex: "dont touch servers vector while im using it"
};