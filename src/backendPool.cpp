#include <iostream>
#include <sys/socket.h>   // Core: sockaddr, sockaddr_storage, socket(), bind()
#include <netinet/in.h>   // Internet: sockaddr_in, sockaddr_in6, htons()
#include <arpa/inet.h>    // Munging: inet_pton(), inet_ntop()
#include "include/backendPool.hpp"
#include <shared_mutex>
#include <cstring>

using namespace std;

// stores addresses in servers vector
void BackendPool::storeNewAddress(const struct sockaddr* addr) {
    unique_lock<shared_mutex> lock(serversMutex);
    
    socklen_t addrSize;

    // get size of IP address
    if (addr->sa_family == AF_INET) {
        addrSize = sizeof(struct sockaddr_in);
    }
    else {
        addrSize = sizeof(struct sockaddr_in6);
    }

    servers.emplace_back(); // creates object directly inside vector
    Backend& backend = servers.back(); // backend is reference to newly cretaed back of servers

    // set IP address length
    backend.addr_len = addrSize;

    // copies raw socket address bytes
    memcpy(&backend.address, addr, backend.addr_len);
}

// Round Robin LB algo
Backend* BackendPool::RoundRobin() {
    shared_lock<shared_mutex> lock(serversMutex);

    if (servers.empty()) {
        return nullptr;
    }

    // get curr index
    size_t index = currInd.load();

    // find healthy server
    size_t attempts = 0;
    while (attempts < servers.size()) {
        Backend& candidate = servers[index];

        // check if healthy and return
        if (candidate.isHealthy.load() == true) {
            currInd.store((index + 1) % servers.size());
            return &candidate;
        }

        index = (index + 1) % servers.size();
        attempts++;
    }

    return nullptr;
}

// get total healthy servers
size_t BackendPool::getHealthyCount() const {
    shared_lock<shared_mutex> lock(serversMutex);
    size_t healthy = 0;
    for (size_t i = 0; i < servers.size(); i++) {
        const Backend& p = servers[i];
        if (p.isHealthy.load()) {
            healthy++;
        }
    }

    return healthy;
}

// get total unhealthy servers
size_t BackendPool::getUnhealthyCount() const {
    shared_lock<shared_mutex> lock(serversMutex);
    size_t unhealthy = 0;
    for (size_t i = 0; i < servers.size(); i++) {
        const Backend& p = servers[i];
        if (!p.isHealthy.load()) {
            unhealthy++;
        }
    }

    return unhealthy;
}

// print status
void BackendPool::printStatus() const {
    shared_lock<shared_mutex> lock(serversMutex);

    size_t healthy = 0;
    for (const auto& backend: servers) {
        if (backend.isHealthy.load()) {
            healthy++;
        }
    }
    
    cout << "TOTAL SERVERS: " << servers.size() << endl;
    cout << "Total Healthy Server: " << healthy << endl;
    cout << "Total Unhealthy Servers: " << (servers.size() - healthy) << endl;
}








