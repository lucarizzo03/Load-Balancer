#include <iostream>
#include <sys/socket.h>   // Core: sockaddr, sockaddr_storage, socket(), bind()
#include <netinet/in.h>   // Internet: sockaddr_in, sockaddr_in6, htons()
#include <vector> 
#include <atomic>

using namespace std;

struct Backend {
    struct sockaddr_storage address; // holds both IPv4/IPv6
    socklen_t addr_len; // socklen_t universal size for address length
    atomic<bool> isHealthy = true; 
};

class BackendPool {
public:

    // acess for health check func

    // add server to backend pool 
    void storeNewAddress(const struct sockaddr* addr);

    // get next healthy server - Round Robin
    Backend *RoundRobin();

    // add some other LB algos
    

private:
    vector<Backend> servers;
    atomic<size_t> currInd = 0;
};