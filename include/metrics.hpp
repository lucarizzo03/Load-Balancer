#pragma once
#include <iostream> 
#include <vector> 
#include <mutex>

using namespace std;


class Metrics {
public:
    Metrics() = default;
    ~Metrics();

    void recordConnectionLat(uint64_t latenteny);

    void recordFullReqLat(uint64_t latenteny);

    void printMetrics() const;

    vector<uint64_t> connectionsLatencies; 
    vector<uint64_t> fullReqLatencies;
    mutable mutex connectMutex;
    mutable mutex fullreqMutex;
};

