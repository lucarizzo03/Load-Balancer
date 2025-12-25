const http = require('http');

const START_PORT = 3000;
const NUM_SERVERS = 10000;

let serversStarted = 0;

for (let i = 0; i < NUM_SERVERS; i++) {
    const port = START_PORT + i;
    const server = http.createServer((req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end(`Hello from server on port ${port}\n`);
    });

    server.on('error', (err) => {
        console.error(`Error on port ${port}:`, err.message);
    });

    server.listen(port, () => {
        serversStarted++;
        if (serversStarted % 1000 === 0) {
            console.log(`Started ${serversStarted} servers...`);
        }
        if (serversStarted === NUM_SERVERS) {
            console.log(`All ${NUM_SERVERS} servers are running from port ${START_PORT} to ${START_PORT + NUM_SERVERS - 1}`);
        }
    });
}

// must run with: ulimit -n 65536 && node multi_server.js

