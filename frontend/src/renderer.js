const canvas = document.getElementById('gameCanvas');
const ctx = canvas.getContext('2d');

function resizeCanvas() {
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
    ctx.imageSmoothingEnabled = false; 
}
window.addEventListener('resize', resizeCanvas);
resizeCanvas(); 

const TILE_SIZE = 16;
let MAP_WIDTH = 50;  
let MAP_HEIGHT = 50;

const camera = {
    x: 0,        
    y: 0,        
    zoom: 3.0    
};

const ANIM_META = {
    idle: { row: 0, frames: [0, 1, 2, 1], speed: 250 },
    moving: { row: 1, frames: [0, 1, 2, 3, 4, 3, 2, 1], speed: 100 }
};

const assets = {
    slime: new Image(),
    tileset: new Image(),
};

assets.slime.src = '/assets/textures/entities/slime_base.png';
assets.tileset.src = '/assets/textures/terrain/field.png';

let mapData = []; 
const shadowEntities = new Map();

function setupInputs() {
    let isDragging = false;
    let lastMouse = { x: 0, y: 0 };

    canvas.addEventListener('mousedown', (e) => {
        isDragging = true;
        lastMouse = { x: e.offsetX, y: e.offsetY };
    });

    canvas.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const dx = e.offsetX - lastMouse.x;
        const dy = e.offsetY - lastMouse.y;
        
        camera.x -= dx / camera.zoom;
        camera.y -= dy / camera.zoom;
        
        lastMouse = { x: e.offsetX, y: e.offsetY };
    });

    window.addEventListener('mouseup', () => isDragging = false);
    
    canvas.addEventListener('wheel', (e) => {
        e.preventDefault(); 
        
        const mouseWorldX = (e.offsetX / camera.zoom) + camera.x;
        const mouseWorldY = (e.offsetY / camera.zoom) + camera.y;

        const zoomFactor = e.deltaY > 0 ? 0.9 : 1.1;
        camera.zoom *= zoomFactor;
        camera.zoom = Math.max(0.5, Math.min(camera.zoom, 10.0));

        camera.x = mouseWorldX - (e.offsetX / camera.zoom);
        camera.y = mouseWorldY - (e.offsetY / camera.zoom);
    }, { passive: false }); 
}

function setupNetwork() {
    const socket = new WebSocket('ws://localhost:3000');

    // 【新增】：造物主权杖 —— 将控制接口挂载到全局 window 对象
    window.socket = socket;
    window.spawn = (type, x, y) => {
        if (socket.readyState === WebSocket.OPEN) {
            // 构建符合 C++ 引擎解析格式的神谕字符串，确保坐标是整数
            const cmd = `SPAWN ${type} ${Math.floor(x)} ${Math.floor(y)}`;
            socket.send(cmd);
            // 在控制台打印带颜色的炫酷神谕日志
            console.log(`%c[Oracle]%c Casted miracle: ${cmd}`, 'color: #ffd700; font-weight: bold;', 'color: inherit;');
        } else {
            console.error("[Oracle] The Isekai Gate is closed or not ready.");
        }
    };

    socket.onmessage = (event) => {
        const data = JSON.parse(event.data);
        data.entities.forEach(serverEntity => {
            if (!shadowEntities.has(serverEntity.id)) {
                shadowEntities.set(serverEntity.id, {
                    x: serverEntity.x,
                    y: serverEntity.y,
                    targetX: serverEntity.x,
                    targetY: serverEntity.y,
                    state: serverEntity.state,
                    animSequenceIndex: 0,
                    lastAnimTime: performance.now()
                });
            } else {
                let localEntity = shadowEntities.get(serverEntity.id);
                localEntity.targetX = serverEntity.x;
                localEntity.targetY = serverEntity.y;
                if (localEntity.state !== serverEntity.state) {
                    localEntity.state = serverEntity.state;
                    localEntity.animSequenceIndex = 0;
                    localEntity.lastAnimTime = performance.now();
                }
            }
        });
    };
}

function render(timestamp) {
    requestAnimationFrame(render);
    ctx.imageSmoothingEnabled = false; 
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    const startCol = Math.max(0, Math.floor(camera.x / TILE_SIZE));
    const endCol = Math.min(MAP_WIDTH - 1, Math.floor((camera.x + canvas.width / camera.zoom) / TILE_SIZE));
    const startRow = Math.max(0, Math.floor(camera.y / TILE_SIZE));
    const endRow = Math.min(MAP_HEIGHT - 1, Math.floor((camera.y + canvas.height / camera.zoom) / TILE_SIZE));

    const tilesetCols = assets.tileset.width ? (assets.tileset.width / TILE_SIZE) : 1;

    // --- A. 手动投影绘制地形 ---
    for (let r = startRow; r <= endRow; r++) {
        for (let c = startCol; c <= endCol; c++) {
            const tileId = mapData[r * MAP_WIDTH + c];
            if (tileId > 0 && assets.tileset.width > 0) {
                const localId = tileId - 1; 
                const sx = (localId % tilesetCols) * TILE_SIZE;
                const sy = Math.floor(localId / tilesetCols) * TILE_SIZE;

                const screenX = Math.round((c * TILE_SIZE - camera.x) * camera.zoom);
                const screenY = Math.round((r * TILE_SIZE - camera.y) * camera.zoom);
                const nextScreenX = Math.round(((c + 1) * TILE_SIZE - camera.x) * camera.zoom);
                const nextScreenY = Math.round(((r + 1) * TILE_SIZE - camera.y) * camera.zoom);
                
                const drawW = nextScreenX - screenX;
                const drawH = nextScreenY - screenY;

                ctx.drawImage(
                    assets.tileset,
                    sx, sy, TILE_SIZE, TILE_SIZE,
                    screenX, screenY, drawW, drawH
                );
            }
        }
    }

    // --- B. 手动投影绘制实体 ---
    shadowEntities.forEach(entity => {
        const lerpFactor = 0.02;
        entity.x += (entity.targetX - entity.x) * lerpFactor;
        entity.y += (entity.targetY - entity.y) * lerpFactor;

        if (entity.x < startCol - 2 || entity.x > endCol + 2 || 
            entity.y < startRow - 2 || entity.y > endRow + 2) {
            return; 
        }

        const currentAnim = ANIM_META[entity.state] || ANIM_META['idle'];
        if (timestamp - entity.lastAnimTime > currentAnim.speed) {
            entity.animSequenceIndex = (entity.animSequenceIndex + 1) % currentAnim.frames.length;
            entity.lastAnimTime = timestamp;
        }

        const frameCol = currentAnim.frames[entity.animSequenceIndex];

        const worldPxX = entity.x * TILE_SIZE;
        const worldPxY = entity.y * TILE_SIZE;
        
        const drawScreenX = Math.round((worldPxX - 8 - camera.x) * camera.zoom);
        const drawScreenY = Math.round((worldPxY - camera.y) * camera.zoom);

        const drawW = Math.round(32 * camera.zoom);
        const drawH = Math.round(16 * camera.zoom);

        ctx.drawImage(
            assets.slime,
            frameCol * 32, currentAnim.row * 16,
            32, 16,
            drawScreenX, drawScreenY,
            drawW, drawH
        );
    });
}

async function bootstrap() {
    try {
        const response = await fetch('/assets/maps/temp.json');
        const mapJson = await response.json();
        
        MAP_WIDTH = mapJson.width;
        MAP_HEIGHT = mapJson.height;
        mapData = mapJson.layers[0].data;
        
        setupInputs();
        setupNetwork();
        requestAnimationFrame(render);
    } catch (e) {
        console.error("Failed to boot engine:", e);
    }
}

bootstrap();