#pragma once
#include <iostream>
#include "backendPool.hpp"
#include <thread>
#include <shared_mutex>
#include <vector>

using namespace std;

class Health {
public:

    Health(BackendPool& pool);
    ~Health();

    void start(); // spawns health checker thread 
    void stop(); // stops health checker thread 

    void healthCheckLoop(); 
    void checkSingleBackend(const Backend& backend); 

private:
    BackendPool& pool;
    thread healthThread;
    mutable shared_mutex healthMutex;
    atomic<bool> running{false}; // health check loop thread bool
};