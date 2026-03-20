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
    entities: {
        slime: new Image(),
        green_slime: new Image(),
        orange_slime: new Image()
    },
    tilesets: [] 
};

assets.entities.slime.src = '/assets/textures/entities/slime_base.png';
assets.entities.green_slime.src = '/assets/textures/entities/slime_base.png';
assets.entities.orange_slime.src = '/assets/textures/entities/orange_slime.png';

let layers = []; 
const shadowEntities = new Map();

const FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
const FLIPPED_VERTICALLY_FLAG   = 0x40000000;
const FLIPPED_DIAGONALLY_FLAG   = 0x20000000;

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
    window.socket = socket;
    window.spawn = (type, x, y) => {
        if (socket.readyState === WebSocket.OPEN) {
            const cmd = `SPAWN ${type} ${Math.floor(x)} ${Math.floor(y)}`;
            socket.send(cmd);
            console.log(`%c[Oracle]%c Casted miracle: ${cmd}`, 'color: #ffd700; font-weight: bold;', 'color: inherit;');
        }
    };

    socket.onmessage = (event) => {
        const data = JSON.parse(event.data);
        
        // 1. 打上过期标记，准备清理服务器已经销毁的实体
        for (let [id, entity] of shadowEntities.entries()) {
            entity.seen = false;
        }

        data.entities.forEach(serverEntity => {
            if (!shadowEntities.has(serverEntity.id)) {
                shadowEntities.set(serverEntity.id, {
                    x: serverEntity.x, y: serverEntity.y,
                    targetX: serverEntity.x, targetY: serverEntity.y,
                    state: serverEntity.state,
                    type: serverEntity.type || 'green_slime', 
                    animSequenceIndex: 0,
                    lastAnimTime: performance.now(),
                    seen: true
                });
            } else {
                let localEntity = shadowEntities.get(serverEntity.id);
                localEntity.seen = true;

                // 【核心：网游橡皮筋重连机制】
                // 计算当前屏幕渲染坐标与服务器真实坐标的物理距离平方
                const distSq = Math.pow(serverEntity.x - localEntity.x, 2) + Math.pow(serverEntity.y - localEntity.y, 2);
                
                // 如果距离超过 2 格（平方大于 4），说明前端休眠导致了严重的时空脱节
                if (distSq > 4) {
                    localEntity.x = serverEntity.x;
                    localEntity.y = serverEntity.y;
                    console.log(`%c[Client]%c Timeline snapped for entity ${serverEntity.id}`, 'color: #ff5555;', 'color: inherit;');
                }

                localEntity.targetX = serverEntity.x;
                localEntity.targetY = serverEntity.y;
                localEntity.type = serverEntity.type || localEntity.type;
                
                if (localEntity.state !== serverEntity.state) {
                    localEntity.state = serverEntity.state;
                    localEntity.animSequenceIndex = 0;
                    localEntity.lastAnimTime = performance.now();
                }
            }
        });

        // 2. 无情清理掉未在服务器数据中出现的实体
        for (let [id, entity] of shadowEntities.entries()) {
            if (!entity.seen) {
                shadowEntities.delete(id);
            }
        }
    };
}

function drawTile(ctx, rawId, c, r) {
    if (rawId === 0) return;

    const flipped_h = (rawId & FLIPPED_HORIZONTALLY_FLAG) !== 0;
    const flipped_v = (rawId & FLIPPED_VERTICALLY_FLAG) !== 0;
    const flipped_d = (rawId & FLIPPED_DIAGONALLY_FLAG) !== 0;
    
    const realId = rawId & 0x1FFFFFFF;

    const tsData = assets.tilesets.find(ts => realId >= ts.firstgid);
    if (!tsData || !tsData.img.width) return;

    const localId = realId - tsData.firstgid;
    const tilesetCols = Math.floor(tsData.img.width / TILE_SIZE);
    if (localId < 0 || localId >= tilesetCols * Math.floor(tsData.img.height / TILE_SIZE)) return;

    const sx = (localId % tilesetCols) * TILE_SIZE;
    const sy = Math.floor(localId / tilesetCols) * TILE_SIZE;

    const screenX = Math.round((c * TILE_SIZE - camera.x) * camera.zoom);
    const screenY = Math.round((r * TILE_SIZE - camera.y) * camera.zoom);
    const nextScreenX = Math.round(((c + 1) * TILE_SIZE - camera.x) * camera.zoom);
    const nextScreenY = Math.round(((r + 1) * TILE_SIZE - camera.y) * camera.zoom);
    
    const drawW = nextScreenX - screenX;
    const drawH = nextScreenY - screenY;

    ctx.save();
    ctx.translate(screenX, screenY);

    let paintW = drawW;
    let paintH = drawH;

    if (!flipped_d) {
        let scaleX = 1, scaleY = 1;
        let tx = 0, ty = 0;
        if (flipped_h) { scaleX = -1; tx = drawW; }
        if (flipped_v) { scaleY = -1; ty = drawH; }
        ctx.transform(scaleX, 0, 0, scaleY, tx, ty);
    } else {
        let a = 0, b = 1, c = 1, d = 0;
        let tx = 0, ty = 0;
        
        if (flipped_h && flipped_v) {
            a = 0; b = -1; c = -1; d = 0;
            tx = drawW; ty = drawH;
        } else if (flipped_h) {
            a = 0; b = 1; c = -1; d = 0;
            tx = drawW; ty = 0;
        } else if (flipped_v) {
            a = 0; b = -1; c = 1; d = 0;
            tx = 0; ty = drawH;
        } else {
            a = 0; b = 1; c = 1; d = 0;
            tx = 0; ty = 0;
        }
        ctx.transform(a, b, c, d, tx, ty);
        paintW = drawH;
        paintH = drawW;
    }

    ctx.drawImage(
        tsData.img,
        sx, sy, TILE_SIZE, TILE_SIZE,
        0, 0, paintW, paintH
    );

    ctx.restore();
}

function render(timestamp) {
    requestAnimationFrame(render);
    ctx.imageSmoothingEnabled = false; 
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    const startCol = Math.max(0, Math.floor(camera.x / TILE_SIZE));
    const endCol = Math.min(MAP_WIDTH - 1, Math.floor((camera.x + canvas.width / camera.zoom) / TILE_SIZE));
    const startRow = Math.max(0, Math.floor(camera.y / TILE_SIZE));
    const endRow = Math.min(MAP_HEIGHT - 1, Math.floor((camera.y + canvas.height / camera.zoom) / TILE_SIZE));

    layers.forEach(layer => {
        if (layer.type !== 'tilelayer') return;
        const data = layer.data;
        
        for (let r = startRow; r <= endRow; r++) {
            for (let c = startCol; c <= endCol; c++) {
                drawTile(ctx, data[r * MAP_WIDTH + c], c, r);
            }
        }
    });

    shadowEntities.forEach((entity, id) => {
        // 【关键修复：提升插值阻尼系数】
        // 0.15 能让它在后端分配的 0.6~1.0 秒内极其平滑且不拖泥带水地滑入下一个格子
        const lerpFactor = 0.15;
        entity.x += (entity.targetX - entity.x) * lerpFactor;
        entity.y += (entity.targetY - entity.y) * lerpFactor;

        // 视椎体剔除，提升渲染性能
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

        const entityImage = assets.entities[entity.type] || assets.entities.green_slime;

        ctx.drawImage(
            entityImage,
            frameCol * 32, currentAnim.row * 16,
            32, 16,
            drawScreenX, drawScreenY,
            drawW, drawH
        );
    });
}

async function bootstrap() {
    try {
        const response = await fetch('/assets/maps/temp2.json');
        const mapJson = await response.json();
        
        MAP_WIDTH = mapJson.width;
        MAP_HEIGHT = mapJson.height;
        layers = mapJson.layers; 
        
        mapJson.tilesets.forEach(ts => {
            const imgName = ts.source.split('/').pop().replace('.tsx', '.png');
            const img = new Image();
            img.src = `/assets/textures/terrain/${imgName}`;
            
            assets.tilesets.push({
                firstgid: ts.firstgid,
                img: img
            });
        });
        
        assets.tilesets.sort((a, b) => b.firstgid - a.firstgid);

        setupInputs();
        setupNetwork();
        requestAnimationFrame(render);
    } catch (e) {
        console.error("Failed to boot engine:", e);
    }
}

bootstrap();