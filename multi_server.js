const http = require('http');

const START_PORT = 3000;
const NUM_SERVERS = 1000;

let serversStarted = 0;
const totalServers = NUM_SERVERS * 2; // IPv4 + IPv6

for (let i = 0; i < NUM_SERVERS; i++) {
    const port = START_PORT + i;
    
    const handler = (req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end(`Hello from server on port ${port}\n`);
    };

    // IPv4 server
    const server4 = http.createServer(handler);
    server4.listen(port, '127.0.0.1', () => {
        serversStarted++;
        checkComplete();
    });

    // IPv6 server (same port)
    const server6 = http.createServer(handler);
    server6.listen(port, '::1', () => {
        serversStarted++;
        checkComplete();
    });
}

function checkComplete() {
    if (serversStarted % 1000 === 0) {
        console.log(`Started ${serversStarted}/${totalServers} servers...`);
    }
    if (serversStarted === totalServers) {
        console.log(`All servers running on ports ${START_PORT}-${START_PORT + NUM_SERVERS - 1} (IPv4 + IPv6)`);
    }
}