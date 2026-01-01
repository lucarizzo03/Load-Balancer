#pragma once
#include <iostream> 
#include <vector> 
#include <mutex>

using namespace std;


class Metrics {
public:
    Metrics() = default;
    ~Metrics();

    void recordConnectionLat(uint64_t latenteny) {
        lock_guard<mutex> lock(connectionsMutex);
        connectionsLatencies.push_back(latenteny);
    }

    void recordFullReqLat(uint64_t latenteny) {
        lock_guard<mutex> lock(connectionsMutex);
        fullReqLatencies.push_back(latenteny);
    }

private:
    vector<uint64_t> connectionsLatencies; 
    vector<uint64_t> fullReqLatencies;
    mutable mutex connectionsMutex;
};

