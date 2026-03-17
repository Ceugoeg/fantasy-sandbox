# Fantasy Sandbox (异世界沙盒引擎)

本项目是一个从零构建的、带有极客精神的 2D 像素风沙盒引擎。我们的最终愿景是打造一个由 LLM (大语言模型) 深度赋能的 Agent 虚拟生态系统：模型不仅作为 NPC 的大脑，更将作为“造物主”拥有 Runtime Modding (运行时模组注入) 的极高权限，通过生成 C++ 代码或 JSON 配置动态改变世界规则。

## 核心架构设计 (The Tri-Tier Architecture)
为了实现物理计算、网络分发与视觉渲染的极致解耦，本项目采用了严苛的异构三层架构：
1. **Engine (C++ 核心引擎)**：世界的唯一真理权威。采用现代 C++17 编写，摒弃传统 OOP 庞大的继承树，利用 Policy-Based Design 模板实现零开销的实体组件抽象。引擎作为脱壳的守护进程运行，不涉足任何渲染操作，仅通过 stdout 管道以高帧率向外泵出极其纯净的 JSON 世界状态快照。
2. **Server (Node.js 通信枢纽)**：连接物质世界与展示界面的“神谕管道”。负责以子进程 (child_process) 模式静默拉起并接管 C++ 引擎的生命周期，通过挂载 WebSocket 建立全双工广播通道，将拦截到的 JSON 状态无损下发给所有连接的客户端。同时提供基础的静态资源路由服务。
3. **Frontend (Canvas 幻觉渲染器)**：极致轻量的被动视觉展示层。基于原生 HTML5 Canvas 2D API 手写。完全摒弃 `ctx.scale` 等浏览器内置仿射变换，采用严苛的整数级手动数学投影来抹杀亚像素采样带来的纹理出血。内置 Lerp 平滑插值与基于时间戳的独立序列帧状态机，负责弥合后端低 Tick 频率与屏幕 60FPS 刷新率之间的鸿沟。

## 快速启动 (Quick Start)
确保你的开发环境已安装 CMake (>=3.10)、GCC/Clang (支持 C++17) 以及 Node.js。
1. 编译核心引擎：进入 `engine/build` 目录，执行 `cmake ..` (首次) 后执行 `make` 编译出 `fantasy_engine` 二进制程序。
2. 启动服务枢纽：进入 `server` 目录，执行 `npm install` 安装 WebSocket 依赖，随后执行 `node index.js` 点火启动整个世界。
3. 观测降临：打开浏览器访问 `http://localhost:3000`，使用鼠标左键拖拽平移视口，滚轮缩放世界。

## 协作规范
* 美术资源约束：所有像素图块必须基于严谨的 Uniform Grid (如 16x16 或 32x16) 进行切分，严禁将不同逻辑类别的特效或 UI 杂糅进同一张精灵图中。
* 状态驱动原则：前端绝对禁止产生任何会影响实体坐标的物理逻辑判定，所有位置更新必须且只能依赖后端 WebSocket 推送的 targetX/Y 并进行插值逼近。
* 详细开发进度与技术细节讨论，请查阅 `docs/` 目录下的相关 Markdown 文档。

## 文件结构
注：以下文件结构将会随着 phase 推进而产生变化
```
fantasy-sandbox/
├── .gitignore                   # Git 忽略配置（忽略 C++ 编译产物和 node_modules）
├── README.md                    # 项目总入口（包含架构概览、启动说明和协作规范）
├── assets/                      # 全局静态资源中心
│   ├── maps/
│   │   └── temp.json            # Tiled 导出的世界网格数据
│   └── textures/
│       ├── terrain/
│       │   └── field.png        # 地形图块集总图
│       └── entities/
│           └── slime_base.png   # 史莱姆统一网格动画序列帧
├── docs/                        # 设计文档与开发日志
│   ├── world_rules.md           # 物理法则与生物体系设定
│   ├── architecture.md          # 核心系统架构图与 IPC 通信协议
│   └── dev_logs/                # 开发日志与里程碑规划
│       └── phase_1_genesis.md   # 第一阶段任务清单与进度追踪
├── frontend/                    # Web 前端（纯视觉层/幻觉渲染器）
│   ├── index.html               # 全屏 Canvas 挂载点
│   └── src/
│       └── renderer.js          # 手动投影摄像机、Lerp 插值与动画状态机
├── server/                      # Node.js 网关（世界通信与进程管理）
│   ├── package.json             # 后端依赖配置（WebSocket）
│   └── index.js                 # 静态资源分发、子进程保活与 JSON 广播管道
└── engine/                      # C++ 核心世界引擎（物理世界唯一真理）
    ├── CMakeLists.txt           # C++ 现代构建配置
    ├── include/
    │   ├── core/
    │   │   ├── World.h          # 空间与时间管理（网格与 Tick）
    │   │   └── EventBus.h       # [规划中] 基于多态与泛型的核心事件总线
    │   ├── events/              
    │   │   └── GameEvents.h     # [规划中] 纯数据容器的事件定义
    │   └── entities/
    │       ├── BaseEntity.h     # 实体基类（包含 FSM 基础状态数据）
    │       ├── Entity.h         # Policy-Based Design 的模板实体外壳
    │       └── policies/
    │           └── RandomWalkPolicy.h # 实体具体行为逻辑（目前包含简单的 FSM 流转）
    └── src/
        ├── core/
        │   └── World.cpp        # 核心系统实现与 JSON 快照导出
        └── main.cpp             # 引擎入口（Tick 死循环与 stdout 刷新）
```