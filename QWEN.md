# QWEN.md — OpenHarmonyCar 项目指南

## 项目概述

基于 **OpenHarmony + Ascend YOLO** 的工业巡检仪表读数分析机器人。三节点协作：
- **鸿蒙 App（平板）** — ArkTS / DevEco Studio，控制小车、地图展示、识别结果展示
- **紫派 Purple Pi OH** — C/C++ + Python3（OpenHarmony 5.0），UDP↔LCM 桥、轮控、雷达、SLAM 导航
- **香橙派 Orange Pi** — Python / C++（昇腾 ACL），YOLOv5s 视觉推理、读数计算、视频流推送

三块代码运行在不同设备、不同语言、**互不编译依赖**。集成靠 `contracts/` 中的接口契约统一。

## 仓库结构

| 目录 | 语言/框架 | 职责 |
|---|---|---|
| `app-harmony/` | ArkTS, DevEco Studio 5.0 | 鸿蒙平板 App（控制/取数/展示） |
| `car-agent/` | ArkTS, DevEco Studio 5.0 | 车载无界面轻 agent（常驻紫派，软总线黑板↔本机 UDP 桥） |
| `purplepi-control/` | C/C++ + Python3 | 紫派主控（通信/轮控/导航），成员 A 负责 |
| `orangepi-vision/` | Python / C++（昇腾） | 香橙派视觉推理，成员 B 负责 |
| `contracts/` | 文档 | ⭐ 接口契约（神圣区域），改协议必须先改这里并发 PR |
| `docs/` | 文档 | 设计文档、网络规划、测试计划、协作规范 |
| `tools/` | Python / Node.js | Mock / 回放 / 验证工具 |

## 构建与运行

### app-harmony（鸿蒙 App）
- **IDE**: DevEco Studio（必须），SDK `5.0.0(12)`，runtimeOS = HarmonyOS
- **构建**: 在 DevEco Studio 中打开 `app-harmony/` 目录，使用菜单 Build → Build Hap(s)
- **依赖管理**: `oh-package.json5`（使用 `@ohos/hypium` 测试框架 + `@ohos/hamock`）
- **测试**: DevEco Studio 内置 hypium 测试框架（`ohosTest`）
- **签名**: 本地调试证书，不提交密钥到仓库
- **Lint**: `code-linter.json5` 配置

### car-agent（车载 agent）
- **IDE**: DevEco Studio，结构与 app-harmony 相同
- **部署**: 常驻紫派设备，无 UI

### purplepi-control / orangepi-vision
- 交叉编译/本地编译在各自设备上完成，构建产物不进 git

### Mock 工具（本地开发无需真机）
```bash
# 假紫派（PC 端模拟）
python tools/mock-purplepi/mock_purplepi.py --id 1 [--gen-map 1800x1800]

# 纯逻辑验证
node tools/verify/verify.mjs

# 冒烟测试
python tools/mock-purplepi/smoke_test.py
```

## 开发约定

### 分支策略（trunk-based）
- `main` 始终可集成
- 特性分支: `feat/<area>-<desc>`，area ∈ {app, pi, vision, contracts, docs}
- 修复: `fix/<area>-<desc>`
- 通过 PR 合回 main；**改 `contracts/` 的 PR 必须 @ 受影响的另一端**

### 提交信息规范
```
<scope>: <简述>

scope ∈ app | pi | vision | contracts | docs | tools
例：app: 地图 Canvas 渲染支持缩放
    contracts: UDP 协议补充命令 'i' 的 IP 字段说明
```

### 代码风格
- 与所在子项目既有代码风格保持一致
- **文档与注释用中文，代码标识符用英文**
- 脚本优先跨平台；必须区分时 `.ps1`(Windows) / `.sh`(Linux)
- 构建产物不进 git：`oh_modules/`、`build/`、`.hvigor/`、`.preview/`、`.so/.ko`、模型权重

## 关键通信协议

| 链路 | 协议 | 端口 | 契约文档 |
|---|---|---|---|
| App ↔ 紫派（控制/心跳） | UDP 自定义 9 字节二进制 | 5001 | `contracts/udp-protocol.md` |
| 紫派 → App（地图） | HTTP（web 根=`/data/test`） | 8000 | `contracts/map-format.md` |
| 紫派内部 | LCM 信道 | — | `contracts/lcm/` |
| 香橙派 → App（视频+读数） | WebSocket JPEG 帧 + JSON | 8000 | `contracts/vision-stream-api.md` |
| App ↔ car-agent（多车协同） | 鸿蒙软总线 `distributedDataObject` | — | `contracts/multi-robot-collab.md` |

### 重要细节
- **bundleName 必须为 `com.example.carapp`**（App 和 car-agent 两端都要一致）
- App `robotState` 枚举值对齐 ASCII（byte0 既是状态也是命令码）
- 建图命令必须发 `'m'`/`0x6d`（强制重建），`cmd0` 在已有图/导航态时被紫派忽略
- 设备发现：App 广播 `0x06` ping，紫派只回包；香橙派以 `0x07` 回应
- 地图首选压缩格式 `zipedMap.txt`（首行 `ZMAP1`），回退 `defultMap.txt`（首行 7 值元数据）
- 视觉链路 = 香橙派本机 FastAPI，App WebSocket 直连，**无独立/云端服务器**

## 测试策略

三层测试，原则是**每人能独立离线开发**：
1. **单测**（每天）：Mock 对端 + 本机验证
2. **两两联调**（模块完成后）：两台设备一组
3. **全系统联调**（联调日）：全员 + 真车 + 同一局域网

测试数据/录制放 `contracts/fixtures/`，一次录制长期复用。
