
```
fantasy-sandbox/
├── .gitignore               # 忽略编译产物（如 node_modules, C++ 的 .o/.out 文件）
├── README.md                # 记录你的创世设定、编译指令和运行方式
├── docs/                    # 设计文档库
│   ├── world_rules.md       # 记录物理法则、生物克制关系等设计
│   └── architecture.md      # 系统架构图与 API 约定
├── assets/                  # 静态美术资源（你预先用 AI 跑好的素材）
│   ├── terrain/             # 地形图块 (草地、水、山脉等)
│   └── entities/            # 生物/角色 Sprite (史莱姆、村民等)
├── frontend/                # 网页前端 (展示层)
│   ├── index.html           # 游戏主页面 (包含一个巨大的 Canvas)
│   ├── style.css            # 简单的样式
│   └── src/
│       ├── renderer.js      # 负责把引擎传来的坐标映射到 Canvas 上
│       └── network.js       # 负责与 Node.js 建立 WebSocket 通信
├── server/                  # Node.js 中间件 (调度与通信层)
│   ├── package.json         
│   ├── index.js             # 服务器入口，处理前端连接
│   └── llm_gateway.js       # 预留：未来与大模型 API 交互的封装逻辑
└── engine/                  # C++ 核心世界引擎 (逻辑与物理层)
    ├── CMakeLists.txt       # C++ 工程构建文件
    ├── include/             # 头文件目录 (你的模板类和接口声明)
    │   ├── core/
    │   │   ├── World.h      # 世界沙盘管理类 (包含二维网格数据结构)
    │   │   └── Vector2D.h   # 基础的二维坐标结构
    │   └── entities/
    │       ├── Entity.h     # 所有生物/物体的抽象基类 (预留大模型注入接口)
    │       └── Npc.h        # 具体的生物类
    └── src/                 # C++ 实现文件
        ├── core/
        │   └── World.cpp
        ├── entities/
        │   └── Npc.cpp
        └── main.cpp         # 引擎的入口，或者编译成供 Node.js 调用的库的入口
```