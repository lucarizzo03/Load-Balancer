#include "metrics.hpp"
#include <mutex>

using namespace std;

Metrics::~Metrics() {}

void Metrics::printMetrics() const {    
    cout << "\n========== LOAD BALANCER METRICS ==========\n";
    
    // Connection Latency Stats
    if (!connectionsLatencies.empty()) {
        uint64_t sum = 0;
        uint64_t min = connectionsLatencies[0];
        uint64_t max = connectionsLatencies[0];
        
        for (uint64_t lat : connectionsLatencies) {
            sum += lat;
            if (lat < min) min = lat;
            if (lat > max) max = lat;
        }
        
        double avg = static_cast<double>(sum) / connectionsLatencies.size();
        
        cout << "CONNECTION LATENCY:\n";
        cout << "  Total Connections: " << connectionsLatencies.size() << "\n";
        cout << "  Average: " << avg / 1000.0 << " ms (" << avg << " μs)\n";
        cout << "  Min: " << min / 1000.0 << " ms (" << min << " μs)\n";
        cout << "  Max: " << max / 1000.0 << " ms (" << max << " μs)\n";
    } else {
        cout << "CONNECTION LATENCY: No data\n";
    }
    
    cout << "\n";
    
    // Full Request Latency Stats
    if (!fullReqLatencies.empty()) {
        uint64_t sum = 0;
        uint64_t min = fullReqLatencies[0];
        uint64_t max = fullReqLatencies[0];
        
        for (uint64_t lat : fullReqLatencies) {
            sum += lat;
            if (lat < min) min = lat;
            if (lat > max) max = lat;
        }
        
        double avg = static_cast<double>(sum) / fullReqLatencies.size();
        
        cout << "FULL REQUEST LATENCY:\n";
        cout << "  Total Requests: " << fullReqLatencies.size() << "\n";
        cout << "  Average: " << avg / 1000.0 << " ms (" << avg << " μs)\n";
        cout << "  Min: " << min / 1000.0 << " ms (" << min << " μs)\n";
        cout << "  Max: " << max / 1000.0 << " ms (" << max << " μs)\n";
    } else {
        cout << "FULL REQUEST LATENCY: No data\n";
    }
    
    cout << "==========================================\n\n";
}

// record connection lat
void Metrics::recordConnectionLat(uint64_t latenteny) {
    lock_guard<mutex> lock(connectMutex);
    connectionsLatencies.push_back(latenteny);
}

// record fullreq lat
void Metrics::recordFullReqLat(uint64_t latenteny) {
    lock_guard<mutex> lock(fullreqMutex);
    fullReqLatencies.push_back(latenteny);
}