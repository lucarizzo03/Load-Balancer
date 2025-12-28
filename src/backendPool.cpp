#include <iostream>
#include <sys/socket.h>   // Core: sockaddr, sockaddr_storage, socket(), bind()
#include <netinet/in.h>   // Internet: sockaddr_in, sockaddr_in6, htons()
#include <arpa/inet.h>    // Munging: inet_pton(), inet_ntop()
#include "include/backendPool.hpp"

using namespace std;

void BackendPool::storeNewAddress(const struct sockaddr* addr) {
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
Backend* BackendPool::nextServer() {





}







