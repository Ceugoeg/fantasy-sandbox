# Fantasy Sandbox (异世界沙盒引擎)

本项目是一个从零构建的、带有极客精神的 2D 像素风沙盒引擎。我们的最终愿景是打造一个由 LLM (大语言模型) 深度赋能的 Agent 虚拟生态系统：模型不仅作为 NPC 的大脑，更将作为“造物主”拥有 Runtime Modding (运行时模组注入) 的极高权限，动态改变世界规则。

## 核心架构设计 (The Quad-Tier Architecture)
为了实现物理计算、意图决策、网络分发与视觉渲染的极致解耦，本项目演进为严苛的四层异构微服务集群：
1. **Engine (C++ 核心引擎)**：世界的唯一真理权威。采用现代 C++17 编写，利用 Policy-Based Design 模板与 EventBus 事件总线实现实体组件的零开销抽象与防级联分发。引擎作为脱壳的守护进程，不涉足渲染与复杂意图推演，仅通过标准输入流接收结构化指令，并以固定 Tick Rate 向外泵出增量 JSON 状态快照。
2. **AI Service (Python 智能中枢)**：大模型的思考载体（开发中）。作为独立的异步微服务，接管所有 LLM 推理调度。通过 Agent Tool Calling 机制主动感知世界局部状态，利用 Pydantic/Hygenar 进行极其严格的幻觉过滤与 JSON 契约校验，将自然语言降维为底层的宏观意图。
3. **Gateway (Node.js 通信枢纽)**：连接物质世界与展示界面的“神谕管道”。管理 C++ 引擎子进程，通过 WebSocket 桥接前端渲染器与 Python AI 服务，采用 `readline` 严格切包消灭高吞吐下的 NDJSON 粘包问题，负责全双工的数据下发与指令路由。
4. **Frontend (Canvas 幻觉渲染器)**：极致轻量的被动视觉展示层。基于原生 HTML5 Canvas 2D API 手写。采用严苛的整数级纯数学仿射矩阵变换彻底抹杀纹理出血，内置 Lerp 平滑插值与网游级“橡皮筋重连”机制，弥合后端低频运算与屏幕 60FPS 刷新率之间的鸿沟。

## AI 赋能架构规划 (LLM Integration Roadmap)
为了兼顾运行效率与大模型的无限创造力，我们将 LLM 的功能干预权限解耦为三种渐进的耦合模式：
* **模式一：原子化意图数据驱动 (JSON Data-Driven)**。针对常规的高频交互逻辑，我们在 C++ 引擎底层预埋 `EventDrivenPolicy` 与高维指令集。大模型仅需输出标准化的 JSON 意图（如“目标寻路”、“引起种群仇恨”、“修改某地块的图块 ID”），交由引擎原子接口直接解析执行。此模式开销最小，足以覆盖绝大多数对象行为与地貌微调。
* **模式二：Lua VM 动态挂载 (Disposable Runtime Scripts)**。针对即用即抛的突发性、全局性事件（例如：“魔王降下诅咒，所有非魔族生物全属性下降 50%”），原子化指令难以穷尽所有可能。此时允许大模型实时编写一段 Lua 逻辑并挂载到引擎内置的轻量级 VM 中，调用引擎事先暴露的底层 API，事件结算完毕后立刻销毁，兼顾灵活性与安全性。
* **模式三：WASM 静态热重载 (Abstract System Injection)**。针对玩家提出的高度抽象且庞大的全新玩法系统（例如：“添加一个带有 50 种材料的炼金系统”、“复杂的烹饪工艺体系”），大模型生成的 C++/Rust 逻辑将直接被编译为 WebAssembly (WASM) 二进制文件。引擎在运行时将其静态热注入并重载，实现极客级别的深层系统模组无缝扩展。

## 快速启动 (Quick Start)
确保你的开发环境已安装 CMake (>=3.10)、GCC/Clang (支持 C++17) 以及 Node.js。
1. 编译核心引擎：进入 `engine/build` 目录，执行 `cmake ..` (首次) 后执行 `make` 编译出 `fantasy_engine` 二进制程序。
2. 启动服务枢纽：进入 `server` 目录，执行 `npm install` 安装 WebSocket 依赖，随后执行 `node index.js` 点火启动整个世界。
3. 观测降临：打开浏览器访问 `http://localhost:3000`，使用鼠标左键拖拽平移视口，滚轮缩放世界。

## 协作规范
* 美术资源约束：所有像素图块必须基于严谨的 Uniform Grid 进行切分，严禁将不同逻辑类别的特效或 UI 杂糅进同一张精灵图中。
* 状态驱动原则：前端绝对禁止产生任何会影响实体坐标的物理逻辑判定，所有位置更新必须且只能依赖后端 WebSocket 推送的数据进行插值逼近。
* 详细开发进度与技术细节讨论，请查阅 `docs/` 目录下的相关 Markdown 文档。

## 文件结构 (Phase 1 完工快照)
```text
fantasy-sandbox/
├── .gitignore                   # Git 忽略配置
├── LICENSE                      # 开源协议
├── README.md                    # 项目总入口
├── assets/                      # 全局静态资源中心
│   ├── maps/                    # 地图 JSON 数据与预览图 (如 temp2.json)
│   ├── textures/                # 纹理图集 (实体精灵图与地形图集)
│   └── tilesets/                # Tiled 导出的 TSX 图块集配置
├── docs/                        # 设计文档与开发日志
│   ├── dev_logs/                # 里程碑规划 (含 phase_1_genesis.md)
│   └── dev_Q&A/                 # 核心技术决策 Q&A 归档 (01.md ~ 03.md)
├── engine/                      # C++ 核心真理引擎
│   ├── CMakeLists.txt           # 引擎构建脚本
│   ├── include/                 # 核心头文件 (EventBus, CommandQueue, Entity 模板)
│   ├── src/                     # 核心实现 (World 仲裁与主循环 main.cpp)
│   └── third_party/             # 第三方单头文件库 (nlohmann/json)
├── frontend/                    # Web 前端幻觉渲染器
│   ├── index.html               # 页面挂载点
│   └── src/                     # 视觉逻辑 (renderer.js: 整数矩阵投影与 Lerp)
└── server/                      # Node.js 通信枢纽
    ├── index.js                 # HTTP 静态路由与 WebSocket 双工网关
    ├── package.json             # 后端依赖配置
    └── package-lock.json        # 依赖锁