#include <iostream>
#include <sys/socket.h>   // Core: sockaddr, sockaddr_storage, socket(), bind()
#include <netinet/in.h>   // Internet: sockaddr_in, sockaddr_in6, htons()
#include <arpa/inet.h>    // Munging: inet_pton(), inet_ntop()
#include "include/backendPool.hpp"
#include <netdb.h>
#include <unistd.h>       // close(), read(), write()

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

    struct addrinfo hints, *res;
    int sockfd, new_fd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  // fill in IP for me

    // load up sockaddrr/error check action
    if (getaddrinfo(NULL, "8080", &hints, &res) != 0) {
        gai_strerror(getaddrinfo(NULL, "8080", &hints, &res));
        return 1;
    }

    // make a socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == - 1) {
        perror("socket");
        return 1;
    }

    // ADD SOCKET RE-USE

    // bind socket
    int bind = ::bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (bind == -1) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    // free memory or memory leak
    freeaddrinfo(res); 

    // listen for incomign connections
    if (listen(sockfd, 128) == -1) {
        perror("listen");
        close(sockfd);
        return 1;
    }

    // server loop
    while(true) {
        struct sockaddr_storage their_addr;
        socklen_t addr_size;

        addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) {
            perror("accept");
            close(sockfd);
            continue;
        }

        





    }


    





    
    


    // need to start health checker 
    // need to start Load Balancer

    return 0;
}

// build: cmake --build build
// run: ./build/LoadBalancer
