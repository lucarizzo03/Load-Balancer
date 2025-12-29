#include <iostream>
#include <sys/socket.h>   // Core: sockaddr, sockaddr_storage, socket(), bind()
#include <netinet/in.h>   // Internet: sockaddr_in, sockaddr_in6, htons()
#include <arpa/inet.h>    // Munging: inet_pton(), inet_ntop()
#include "include/backendPool.hpp"

using namespace std;










int main(int argc, char* argv[]) {
    BackendPool pool;

    // init for IPv4 (8000 - 12999)
    for (int i = 0; i < 5000; i++) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        addr.sin_port = htons(8000 + i);
        pool.storeNewAddress((struct sockaddr*)&addr);
    }

    // init for IPv6 (13000 - 17999)
    for (int i = 0; i < 5000; i++) {
        struct sockaddr_in6 addr;
        addr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, "::1", &addr.sin6_addr);
        addr.sin6_port = htons(13000 + i);
        pool.storeNewAddress((struct sockaddr*)&addr);
    }


    // need to start health checker 
    // need to start Load Balancer

    return 0;
}

// build: cmake --build build
// run: ./build/LoadBalancer
