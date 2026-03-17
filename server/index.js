const http = require('http');
const fs = require('fs');
const path = require('path');
const WebSocket = require('ws');
const { spawn } = require('child_process');

const MIME_TYPES = {
    '.html': 'text/html',
    '.js': 'text/javascript',
    '.png': 'image/png',
    '.json': 'application/json',
    '.css': 'text/css'
};

const server = http.createServer((req, res) => {
    
    let filePath = '';

    if (req.url === '/' || req.url === '/index.html') {
        filePath = path.join(__dirname, "../frontend/index.html");
    } else if (req.url.startsWith('/src/')) {
        filePath = path.join(__dirname, "../frontend", req.url);
    } else if (req.url.startsWith('/assets/')) {
        filePath = path.join(__dirname, "..", req.url);
    } else {
        res.writeHead(404);
        res.end("Not Found in this World");
        return;
    }

    const extname = path.extname(filePath);
    const contentType = MIME_TYPES[extname] || 'application/octet-stream';

    fs.readFile(filePath, (err, data) => {
        if (err) {
            console.error(`[HTTP] 资源读取失败 ${filePath}: ${err.code}`);
            res.writeHead(404);
            res.end("File not found");
        } else {
            res.writeHead(200, { 'Content-Type': contentType });
            res.end(data);
        }
    });
});

const wss = new WebSocket.Server({ server: server });
wss.on('connection', (ws) => {
    
    // console.log('A new soul has arrived...');
    console.log('Frontend renderer connected...');

    // ws.on('message', (message) => {
    //     console.log('Heard people praying...\n>> %s', message);
    //     ws.send('Oracel: I\'ve received your call...');
    // });

    // ws.send('Welcome to The Fantasy.')
});

const enginePath = path.join(__dirname, '../engine/build/fantasy_engine');
console.log(`Trying to ignite the engine at: ${enginePath}`);
const engineProcess = spawn(enginePath);

engineProcess.stdout.on('data', (data) => {
    
    const jsonString = data.toString().trim();
    
    wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(jsonString);
        }
    });
});

engineProcess.stderr.on('data', (data) => {
    console.error(`[Engine Error] ${data.toString()}`);
});

engineProcess.on('close', (code) => {
    console.log(`Fantasy Engine shut down with code ${code}`);
});

server.listen(3000, () => {
    console.log("Isekai Gate Opened at port 3000...")
});