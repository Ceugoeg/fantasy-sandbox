const http = require('http');
const fs = require('fs');
const path = require('path');
const readline = require('readline');
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

    // 监听前端发来的指令，并作为神谕转发给 C++ 引擎
    ws.on('message', (message) => {
        const cmd = message.toString().trim();
        console.log(`[Oracle Gateway] Passing command to engine: ${cmd}`);
        // 必须加上 \n，因为 C++ 端是靠 std::getline 按行读取的
        if (engineProcess && !engineProcess.killed) {
            engineProcess.stdin.write(cmd + '\n');
        }
    });
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

// ============================================================================
// 【Stage 1.7 核心改造】NDJSON 粘包处理
// ============================================================================
// 旧方案：stdout.on('data') 直接 trim — 在高吞吐下必然出现粘包/截断。
// 新方案：使用 readline 按 '\n' 严格切包，每行一个完整 JSON。
// C++ 端保证每帧输出 dump() + '\n' + fflush(stdout)，二者配合消灭粘包。
const rl = readline.createInterface({
    input: engineProcess.stdout,
    crlfDelay: Infinity   // 兼容 Windows 的 \r\n 行尾
});

rl.on('line', (line) => {
    // readline 保证每次回调都是完整的一行，无需手动切割
    const trimmed = line.trim();
    if (!trimmed) return;  // 过滤空行，防止前端 JSON 解析崩溃

    // 校验 JSON 合法性后再广播，绝不把脏数据推给前端
    try {
        JSON.parse(trimmed);  // 纯校验，不存储（原始字符串直接转发更高效）
    } catch (e) {
        console.error(`[NDJSON] Malformed JSON dropped: ${trimmed.substring(0, 80)}...`);
        return;
    }

    // 广播给所有已连接的 WebSocket 前端渲染器
    wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(trimmed);
        }
    });
});

// 将引擎的错误或日志输出也引流到 Node.js 控制台，方便调试
engineProcess.stderr.on('data', (data) => {
    // 按换行符分割接收到的数据块
    const lines = data.toString().trim().split('\n');
    lines.forEach(line => {
        if (line) {
            console.error(`[Engine Log]: ${line}`);
        }
    });
});

engineProcess.on('close', (code) => {
    console.log(`Engine shut down: ${code}`);
});

// --- 3. 启动监听 ---
server.listen(3000, () => {
    console.log("Isekai Gate Opened at port 3000...");
});