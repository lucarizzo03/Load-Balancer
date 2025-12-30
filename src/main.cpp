#include <iostream>
#include <sys/socket.h>   // Core: sockaddr, sockaddr_storage, socket(), bind()
#include <netinet/in.h>   // Internet: sockaddr_in, sockaddr_in6, htons()
#include <arpa/inet.h>    // Munging: inet_pton(), inet_ntop()
#include "include/backendPool.hpp"
#include <netdb.h>
#include <unistd.h>       // close(), read(), write()
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unordered_map>









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
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    // bind socket
    int bindRes = ::bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (bindRes == -1) {
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

    // kqueue
    int kq = kqueue();
    // check kq init for error
    if (kq == -1) {
        perror("kq");
        return 1;
    }

    // kq event list
    struct kevent eventList[1024];

    // kevent obj
    struct kevent ev;

    // register listenign once outside
    EV_SET(&ev, sockfd, EVFILT_READ, EV_ADD, 0, 0, NULL);


    if (kevent(kq, &ev, 1, NULL, 0, NULL) == - 1) {
        perror("first kevent");
        close(kq);
        close(sockfd);
        return 1;
    }

    // maps client <---> backend servers
    unordered_map<int, int> pairs;

    // server loop
    while(true) {
        int pending = kevent(kq, NULL, 0, eventList, 1024, NULL);
        if (pending == -1) {
            perror("server loop first event");
            break;
        }

        // loop through pending events
        for (int i = 0; i < pending; i++) {
            struct kevent* event = &eventList[i];

            // process events
            if (event->ident == (uintptr_t)sockfd) {
                struct sockaddr_storage theirAddr;
                socklen_t addr_size = sizeof theirAddr;
                int newfd = accept(sockfd, (struct sockaddr*)&theirAddr, &addr_size);

                if (newfd == -1) {
                    perror("newfd");
                    continue;
                }

                Backend* backend = pool.RoundRobin();
                if (!backend) {
                    cerr << "No healthy backends" << endl;
                    continue;
                }

                int backfd = socket(backend->address.ss_family, SOCK_STREAM, 0);
                if (backfd == -1) {
                    perror("backend socket");
                    close(newfd);
                    continue;
                }

                socklen_t back_len;
                if (backend->address.ss_family == AF_INET) {
                    back_len = sizeof(struct sockaddr_in);
                }
                else {
                    back_len = sizeof(struct sockaddr_in6);
                }

                if (connect(backfd, (struct sockaddr*)&backend->address, back_len) == -1) {
                    perror("conntect");
                    close(newfd);
                    close(backfd);
                    continue;
                }

                struct kevent evPair[2];
                EV_SET(&evPair[0], newfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                EV_SET(&evPair[1], backfd, EVFILT_READ, EV_ADD, 0, 0, NULL);

                if (kevent(kq, evPair, 2, NULL, 0, NULL) == -1) {
                    perror("kevent: register client");
                    close(newfd);
                    close(backfd);
                    continue;
                }

                pairs[newfd] = backfd;
                pairs[backfd] = newfd;

                cout << "New connection: " << newfd <<  " and " << backfd << endl;
            }
            else {
                char buffer[8192];
                ssize_t bytesReadIn = read(event->ident, buffer, sizeof(buffer));

                if (bytesReadIn <= 0) {
                    if (bytesReadIn == 0) {
                        cout << "No bytes" << endl;
                    }
                    else {
                        perror("reading");
                    }

                    int peer = pairs[event->ident];
                    close(event->ident);
                    close(peer);
                    pairs.erase(event->ident);
                    pairs.erase(peer);
                }
                else {
                    int peer = pairs[event->ident];
                    ssize_t written = write(peer, buffer, bytesReadIn);

                    if (written == -1) {
                        perror("write to peer");
                        close(event->ident);
                        close(peer);
                        pairs.erase(event->ident);
                        pairs.erase(peer);
                    } else {
                        cout << "Forwarded " << bytesReadIn << " bytes: fd=" 
                                  << event->ident << " -> fd=" << peer << endl;
                    }
                }
            }
        }
    }

    // need to start health checker 
    // need to start Load Balancer

    close(kq);
    close(sockfd);

    return 0;
}

// build: cmake --build build
// run: ./build/LoadBalancer
