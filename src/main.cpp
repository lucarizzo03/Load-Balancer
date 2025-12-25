#include <iostream>
using namespace std;



// LB settings struct
struct LoadBalancerConfig {
    int LoadBalancerPort; // port of load balancer
    int backendStartPort; // starting port of node file
    int numServers; // total number of servers running 
};


int main(int argc, char* argv[]) {
    // checking args
    if (argc != 4) {
        cout << "not right/enough args" << endl;
        return 1;
    }

    LoadBalancerConfig config;

    try {
        config.LoadBalancerPort = stoi(argv[1]);
        config.backendStartPort = stoi(argv[2]);
        config.numServers = stoi(argv[3]);

        // runtime checks
        if (config.backendStartPort < 1 || config.backendStartPort >= 65536) {
            throw std::runtime_error("Invalid listen port (must be 1-65535)");
        }
        if (config.numServers <= 0) {
            throw std::runtime_error("Must have at least 1 backend server");
        }


        // checking args for now

        cout << "Load Balancer Config" << endl;
        cout << "LB Port: " << config.LoadBalancerPort << endl;
        cout << "Backend starting port: " << config.backendStartPort << endl;
        cout << "Total sevrers running: " << config.numServers << endl;

    }
    catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

// build: cmake --build build
// run: ./build/LoadBalancer
