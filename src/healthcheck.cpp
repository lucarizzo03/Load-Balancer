#include <iostream>
#include "healthcheck.hpp"
#include <thread>
#include "backendPool.hpp"
#include <unistd.h> 
#include <future>

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

// health checker loop - runs every five seconds - parralell health checking
void Health::healthCheckLoop() {
   const size_t NUM_WORKER_THREADS = 50;
    
    while (running.load()) {
        const vector<Backend>& backends = pool.getBackend();
        
        // Divide backends among worker threads
        size_t backendsPerThread = backends.size() / NUM_WORKER_THREADS; // 200 per thread
        vector<future<void>> futures;
        
        for (size_t i = 0; i < NUM_WORKER_THREADS; i++) {
            size_t start = i * backendsPerThread;
            size_t end = (i == NUM_WORKER_THREADS - 1) ? backends.size() : start + backendsPerThread;
            
            // Launch async health checks 
            futures.push_back(async(launch::async, [this, &backends, start, end]() {
                for (size_t j = start; j < end; j++) {
                    if (!running.load()) return;  // early exit if stopping
                    checkSingleBackend(backends[j]);
                }
            }));
        }
        
        // Wait for all health checks to complete
        for (auto& fut : futures) {
            fut.wait();
        }

        // Print summary instead of individual results
        cout << "Health check complete: " << pool.getHealthyCount() 
             << " healthy, " << pool.getUnhealthyCount() << " unhealthy" << endl;
        
        std::this_thread::sleep_for(std::chrono:: seconds(5));
    }
}

// checking connection on a single backend server
void Health::checkSingleBackend(const Backend& backend) {
    int sock = socket(backend.address.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket single check");
        return;
    }

    // timeout to prevent hanging
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int result = connect(sock, (struct sockaddr*)&backend.address, backend.addr_len);

    /*
    if (result == 0) {
       cout << "success" << endl;
    }
    else {
        cout << "failure fucker" << endl;
    }
     */

    close(sock);
    return;
}

// constructor
Health::Health(BackendPool& backendpool) : pool(backendpool), running(false) {};

// descrtuctor
Health::~Health() {
    if (running.load()) {
        stop();
    }
};
