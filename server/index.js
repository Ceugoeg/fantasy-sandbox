const http = require('http');
const fs = require('fs');
const path = require('path');
const WebSocket = require('ws');
const { spawn } = require('child_process');

// --- MIME 类型字典 ---
const MIME_TYPES = {
    '.html': 'text/html',
    '.js': 'text/javascript',
    '.png': 'image/png',
    '.json': 'application/json',
    '.css': 'text/css'
};

// --- 1. 静态资源路由与 HTTP 服务器 ---
const server = http.createServer((req, res) => {
    let filePath = '';
    if (req.url === '/' || req.url === '/index.html') {
        filePath = path.join(__dirname, '../frontend/index.html');
    } else if (req.url.startsWith('/src/')) {
        filePath = path.join(__dirname, '../frontend', req.url);
    } else if (req.url.startsWith('/assets/')) {
        filePath = path.join(__dirname, '..', req.url);
    } else {
        res.writeHead(404);
        res.end("Not Found in this World");
        return;
    }

    const extname = path.extname(filePath);
    const contentType = MIME_TYPES[extname] || 'application/octet-stream';

    fs.readFile(filePath, (err, data) => {
        if (err) {
            console.error(`Failed at loading resources: ${filePath}: ${err.code}`);
            res.writeHead(404);
            res.end("File not found");
        } else {
            res.writeHead(200, { 'Content-Type': contentType });
            res.end(data);
        }
    });
});

// --- 2. WebSocket 服务器与 C++ 引擎桥接 ---
const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
    console.log('Connected frontend renderer');
});

// --- 【核心修正】：跨平台且兼容多配置的引擎路径探测 ---
const isWindows = process.platform === 'win32';
const engineFilename = isWindows ? 'fantasy_engine.exe' : 'fantasy_engine';

const possiblePaths = [
    path.join(__dirname, '../engine/build', engineFilename),           // Linux 默认 & Windows MinGW
    path.join(__dirname, '../engine/build/Debug', engineFilename),     // Windows MSVC Debug 模式
    path.join(__dirname, '../engine/build/Release', engineFilename)    // Windows MSVC Release 模式
];

// 寻找第一个真实存在的文件
let enginePath = possiblePaths.find(p => fs.existsSync(p));

if (!enginePath) {
    console.error("Cannot find binary file");
    console.error("Tried paths:\n", possiblePaths.join('\n'));
    process.exit(1); 
}

console.log(`Located engine: ${enginePath}`);
const engineProcess = spawn(enginePath);

// 监听引擎的 JSON 数据流并广播
engineProcess.stdout.on('data', (data) => {
    const jsonString = data.toString().trim();
    wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(jsonString);
        }
    });
});

engineProcess.stderr.on('data', (data) => {
    console.error(`Engine error: ${data.toString()}`);
});

engineProcess.on('close', (code) => {
    console.log(`Engine shut down: ${code}`);
});

// --- 3. 启动监听 ---
server.listen(3000, () => {
    console.log("Isekai Gate Opened at port 3000...");
});