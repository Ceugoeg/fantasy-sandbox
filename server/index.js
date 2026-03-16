const http = require('http');
const fs = require('fs');
const path = require('path');
const WebSocket = require('ws');

const server = http.createServer((req, res) => {
    
    const index_path = path.join(__dirname, "../frontend/index.html");
    
    if (req.url === '/' || req.url === '/index.html') {
        fs.readFile(index_path, (err, data) => {
            if (err) {
                res.writeHead(500);
                res.end('Error loading index.html');
            } else {
                res.writeHead(200, { 'Content-Type': 'text/html' });
                res.end(data);
            }
        });
    } else {
        res.writeHead(404);
        res.end("Not Found in this World");
    }
});

const wss = new WebSocket.Server({ server: server });
wss.on('connection', (ws) => {
    
    console.log('A new soul has arrived...');

    ws.on('message', (message) => {
        console.log('Heard people praying...\n>> %s', message);
        ws.send('Oracel: I\'ve received your call...');
    });

    ws.send('Welcome to The Fantasy.')
});

server.listen(3000, () => {
    console.log("Isekai Gate Opened at port 3000...")
});