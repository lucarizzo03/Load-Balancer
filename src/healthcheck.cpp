#include <iostream>
#include "include/healthcheck.hpp"
#include <thread>
#include "include/backendPool.hpp"
#include <unistd.h> 

using namespace std;

// spawns health checker thread 
void Health::start() {
    unique_lock<shared_mutex> lock(healthMutex);
    running.store(true);
    healthThread = std::thread(&Health::healthCheckLoop, this);
}

// stops health checker thread
void Health::stop() {
    unique_lock<shared_mutex> lock(healthMutex);
    running.store(false);
    if (healthThread.joinable()) {
        healthThread.join();
    }
}

// health checker loop - runs every five seconds
void Health::healthCheckLoop() {
    shared_lock<shared_mutex> lock(healthMutex);
    while (running.load()) {
        vector<Backend> backend = pool.getBackend();

        for (int i = 0; i < 100; i++) {
            size_t randomIndex = rand() % backend.size();
            checkSingleBackend(backend[randomIndex]);
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

// checking connection on a single backend server
void Health::checkSingleBackend(Backend& backend) {
    int sock = socket(backend.address.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket single check");
        return;
    }

    // timeout to prevent hanging
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int result = connect(sock, (struct sockaddr*)&backend.address, backend.addr_len);

    if (result == 0) {
        // success
    }
    else {
        // failue
    }

    close(sock);
    return;
}
