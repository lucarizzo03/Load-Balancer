#include <iostream>
#include <sys/socket.h>   // Core: sockaddr, sockaddr_storage, socket(), bind()
#include <netinet/in.h>   // Internet: sockaddr_in, sockaddr_in6, htons()
#include <arpa/inet.h>    // Munging: inet_pton(), inet_ntop()
#include "backendPool.hpp"
#include <netdb.h>
#include <unistd.h>       // close(), read(), write()
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unordered_map>
#include <string>
#include <fcntl.h>
#include "healthcheck.hpp"
#include "metrics.hpp"
#include <csignal>
#include <atomic>

using namespace std;


atomic<bool> running(true);
void signalHandler(int signum) {
    cout << "\nShutting down gracefully..." << endl;
    running.store(false);
}

// Closes both ends of a proxied connection and clears all per-fd bookkeeping.
// Uses find() rather than operator[] so a stale event for an fd already torn
// down earlier in the same kevent() batch can't default-construct a bogus
// peer of 0 and close(0) (stdin) instead.
void closeConnection(int fd, unordered_map<int, int>& pairs,
                      unordered_map<int, string>& pendingWrites,
                      unordered_map<int, chrono::steady_clock::time_point>& clientRequestStartTimes) {
    auto it = pairs.find(fd);
    int peer = (it != pairs.end()) ? it->second : -1;

    close(fd);
    pairs.erase(fd);
    pendingWrites.erase(fd);
    clientRequestStartTimes.erase(fd);

    if (peer != -1) {
        close(peer);
        pairs.erase(peer);
        pendingWrites.erase(peer);
        clientRequestStartTimes.erase(peer);
    }
}

// Drains a previously buffered write for fd once it becomes writable again.
// Re-arms EVFILT_WRITE if the socket buffer fills up again before we finish.
bool drainPending(int kq, int fd, unordered_map<int, string>& pendingWrites) {
    auto it = pendingWrites.find(fd);
    if (it == pendingWrites.end()) {
        return true;
    }

    string& pending = it->second;
    ssize_t written = write(fd, pending.data(), pending.size());
    if (written == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            written = 0;
        } else {
            return false;
        }
    }

    pending.erase(0, (size_t)written);

    if (pending.empty()) {
        pendingWrites.erase(fd);
        return true;
    }

    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, NULL);
    return kevent(kq, &ev, 1, NULL, 0, NULL) != -1;
}

// Writes to peer immediately if possible; otherwise buffers whatever the
// kernel wouldn't accept and arms EVFILT_WRITE to finish later. Bytes are
// always appended behind any already-buffered data so forwarded order is
// preserved across multiple short writes.
bool queueWrite(int kq, int peer, const char* data, size_t len, unordered_map<int, string>& pendingWrites) {
    auto it = pendingWrites.find(peer);
    if (it != pendingWrites.end() && !it->second.empty()) {
        it->second.append(data, len);
        return true; // already waiting on EVFILT_WRITE; new data just queues behind it
    }

    ssize_t written = write(peer, data, len);
    if (written == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            written = 0;
        } else {
            return false;
        }
    }

    if ((size_t)written == len) {
        return true;
    }

    string& pending = pendingWrites[peer];
    pending.append(data + written, len - (size_t)written);

    struct kevent ev;
    EV_SET(&ev, peer, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, NULL);
    return kevent(kq, &ev, 1, NULL, 0, NULL) != -1;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);   
    signal(SIGTERM, signalHandler);  
    
    BackendPool pool;

    // init for IPv4 (3000 - 3499)
    for (int i = 0; i < 500; i++) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        addr.sin_port = htons(3000 + i);
        pool.storeNewAddress((struct sockaddr*)&addr);
    }

    // init for IPv6 (3500 - 3999)
    for (int i = 0; i < 500; i++) {
        struct sockaddr_in6 addr;
        addr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, "::1", &addr.sin6_addr);
        addr.sin6_port = htons(3500 + i);
        pool.storeNewAddress((struct sockaddr*)&addr);
    }

    cout << "completed init" << endl;

    struct addrinfo hints, *res;
    int sockfd, new_fd;

    memset(&hints, 0, sizeof hints); // sets all the memory bytes of hints to 0
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

    int sockfdFlags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, sockfdFlags | O_NONBLOCK);

    // ADD SOCKET RE-USE -- look this up
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

    // health checker object
    Health health(pool);
   
    // spawns health checker thread
    health.start();

    // metrics obj;
    Metrics metric;

    // Track when each client connection started (for full request latency)
    unordered_map<int, chrono::steady_clock::time_point> clientRequestStartTimes;

    // Track when backend connections started (for connection setup latency)
    unordered_map<int, chrono::steady_clock::time_point> backendConnectStartTimes;

    // Buffers the unwritten tail of a short write until the peer fd is writable again
    unordered_map<int, string> pendingWrites;

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
            // NEW CLIENT CONNECTION
            if (event->ident == (uintptr_t)sockfd) {
                struct sockaddr_storage theirAddr;
                socklen_t addr_size = sizeof theirAddr;
                
                // client socket
                int newfd = accept(sockfd, (struct sockaddr*)&theirAddr, &addr_size);

                if (newfd == -1) {
                    perror("newfd");
                    continue;
                }

                int newfdFlags = fcntl(newfd, F_GETFL, 0);
                fcntl(newfd, F_SETFL, newfdFlags | O_NONBLOCK);

                optional<Backend> backend = pool.RoundRobin();
                if (!backend) {
                    cerr << "No healthy backends" << endl;
                    continue;
                }

                // retrieve Backend obj
                Backend back = backend.value();

                // backend socket
                int backfd = socket(back.address.ss_family, SOCK_STREAM, 0);
                if (backfd == -1) {
                    perror("backend socket");
                    close(newfd);
                    continue;
                }

                int flags = fcntl(backfd, F_GETFL, 0);
                fcntl(backfd, F_SETFL, flags | O_NONBLOCK);

                socklen_t back_len;
                if (back.address.ss_family == AF_INET) {
                    back_len = sizeof(struct sockaddr_in);
                }
                else {
                    back_len = sizeof(struct sockaddr_in6);
                }

                auto start = chrono::steady_clock::now();

                if (connect(backfd, (struct sockaddr*)&back.address, back_len) == -1) {
                   if (errno != EINPROGRESS) {
                        perror("connect");
                        close(newfd);
                        close(backfd);
                        continue;
                    }
                    backendConnectStartTimes[backfd] = start;
                }
                else {
                    auto end = chrono::steady_clock::now();
                    auto connectionsLatency = chrono::duration_cast<chrono::microseconds>(end - start).count();
                    metric.recordConnectionLat(connectionsLatency);
                }

                struct kevent evPair[2];
                EV_SET(&evPair[0], backfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                EV_SET(&evPair[1], backfd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, NULL);

                if (kevent(kq, evPair, 2, NULL, 0, NULL) == -1) {
                    perror("kevent: register client");
                    close(newfd);
                    close(backfd);
                    backendConnectStartTimes.erase(backfd);
                    continue;
                }

                pairs[newfd] = backfd;
                pairs[backfd] = newfd;
                clientRequestStartTimes[newfd] = chrono::steady_clock::now();
            }
            else if (event->filter == EVFILT_WRITE) {
                if (backendConnectStartTimes.count(event->ident) > 0) {
                    auto connectEnd = chrono::steady_clock::now();
                    auto connectStart = backendConnectStartTimes[event->ident];
                    auto connectionLatency = chrono::duration_cast<chrono::microseconds>(connectEnd - connectStart).count();
                    metric.recordConnectionLat(connectionLatency);
                    backendConnectStartTimes.erase(event->ident);

                    auto clientIt = pairs.find((int)event->ident);
                    if (clientIt != pairs.end()) {
                        struct kevent clientEv;
                        EV_SET(&clientEv, clientIt->second, EVFILT_READ, EV_ADD, 0, 0, NULL);
                        if (kevent(kq, &clientEv, 1, NULL, 0, NULL) == -1) {
                            perror("kevent: add client read");
                        }
                    }
                }
                else {
                    // Backpressure drain: peer's send buffer was full earlier, now writable again
                    if (!drainPending(kq, (int)event->ident, pendingWrites)) {
                        closeConnection((int)event->ident, pairs, pendingWrites, clientRequestStartTimes);
                    }
                }
            }
            else { // DATA FROM CLIENT OR BACKEND
                char buffer[65536];
                ssize_t bytesReadIn = read(event->ident, buffer, sizeof(buffer));

                if (bytesReadIn < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue; // spurious readiness notification, nothing to do yet
                    }
                    perror("reading");
                    closeConnection((int)event->ident, pairs, pendingWrites, clientRequestStartTimes);
                    continue;
                }

                if (bytesReadIn == 0) {
                    closeConnection((int)event->ident, pairs, pendingWrites, clientRequestStartTimes);
                    continue;
                }

                auto pairIt = pairs.find((int)event->ident);
                if (pairIt == pairs.end()) {
                    continue; // stale event for a connection already torn down this batch
                }
                int peer = pairIt->second;

                if (!queueWrite(kq, peer, buffer, (size_t)bytesReadIn, pendingWrites)) {
                    closeConnection(peer, pairs, pendingWrites, clientRequestStartTimes);
                    continue;
                }

                if (clientRequestStartTimes.count(peer) > 0) {
                    auto reqEnd = chrono::steady_clock::now();
                    auto reqStart = clientRequestStartTimes[peer];
                    auto fullReqLatency = chrono::duration_cast<chrono::microseconds>(reqEnd - reqStart).count();
                    metric.recordFullReqLat(fullReqLatency);
                    clientRequestStartTimes.erase(peer);
                }
            }
        }
    }

    health.stop();
    close(kq);
    close(sockfd);

    pool.printStatus();
    metric.printMetrics();

    return 0;
}

// build: cmake --build build
// run: ./build/LoadBalancer
