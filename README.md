# OpenHarmonyCar · 智能巡检仪表读数分析机器人

基于 **OpenHarmony + Ascend YOLO** 的工业巡检机器人。机器人自主建图导航、全路径覆盖，
摄像头识别工业仪表读数，结果回传并在鸿蒙 App 上统一展示。

> 设计文档详见 [`docs/design/`](docs/design/README.md)。

---

## 系统架构（三节点 + 服务器）

```
        ┌─────────────────────┐      UDP:5001 (9字节协议) + 心跳        ┌──────────────────────────┐
        │   鸿蒙 App（平板）    │ ───────────────────────────────────▶ │   紫派 Purple Pi OH         │
        │   ArkTS / DevEco      │ ◀─── HTTP:8000 拉地图 (defultMap.txt) ─ │   RK3566 · OpenHarmony 5.0 │
        │   控制 / 取数 / 展示   │                                       │   UDP↔LCM · 轮控 · 雷达     │
        └──────────┬───────────┘                                       │   SLAM 导航 · 服务器拉起    │
                   │                                                    └────────────┬─────────────┘
                   │  读取处理后视频 + 识别结果(JSON)                                    │ LCM 信道
                   ▼                                                                   ▼
        ┌─────────────────────┐      上报视频流 + 结构化读数            ┌──────────────────────────┐
        │      服务器 (?)       │ ◀──────────────────────────────────── │   香橙派 Orange Pi          │
        │  视频转发 / 数据管理   │                                       │   昇腾 NPU · YOLOv5s+关键点 │
        └─────────────────────┘                                        └──────────────────────────┘
```

> ⚠️ **图中"服务器"的归属与部署位置尚未拍板**（云端？紫派？香橙派本机？），见
> [`contracts/server-api.md`](contracts/server-api.md) 顶部的待定项——这是开工前要先确定的第一件事。

## 分工

| 节点 | 负责人 | 技术栈 | 目录 | 职责 |
|---|---|---|---|---|
| 鸿蒙 App（平板） | **本仓库 owner** | ArkTS / DevEco Studio | [`app-harmony/`](app-harmony/) | 控制小车、取数据、地图与识别结果展示 |
| 紫派 Purple Pi OH | **成员 A** | C/C++ + Python3（OpenHarmony 5.0） | [`purplepi-control/`](purplepi-control/) | 烧录系统、LCM 通信、服务器拉起、雷达、轮控、SLAM 导航 |
| 香橙派 Orange Pi | **成员 B** | Python / C++（昇腾 ACL/MindSpore） | [`orangepi-vision/`](orangepi-vision/) | 摄像头视觉推理（YOLOv5s + 关键点）、读数计算、上报 |

## 仓库地图

| 目录 | 内容 |
|---|---|
| [`contracts/`](contracts/) | ⭐ **接口契约**——三人共同维护的"神圣区域"。改它=发 PR 并 @ 全员。 |
| [`docs/`](docs/) | 网络规划、测试计划、协作规范、设计文档。 |
| [`app-harmony/`](app-harmony/) | 鸿蒙 App（ArkTS）。 |
| [`purplepi-control/`](purplepi-control/) | 紫派主控（系统/通信/轮控/导航）。 |
| [`orangepi-vision/`](orangepi-vision/) | 香橙派视觉推理。 |
| [`tools/`](tools/) | 联调用的 Mock / 回放工具。 |

## 快速开始（明天开工）

1. 读 [`docs/onboarding.md`](docs/onboarding.md)——分支策略、提交规范、各角色第一天怎么动手。
2. 读 [`contracts/`](contracts/)——**先把你要对接的接口契约看一遍**，再写代码。
3. 读 [`docs/network.md`](docs/network.md)——把设备 IP/端口按表配好，确保在同一局域网。
4. 各角色入口：
   - App → [`app-harmony/README.md`](app-harmony/README.md)
   - 紫派 → [`purplepi-control/README.md`](purplepi-control/README.md)
   - 香橙派 → [`orangepi-vision/README.md`](orangepi-vision/README.md)
5. 测试与联调流程见 [`docs/testing.md`](docs/testing.md)。

## 核心原则

- **接缝优先**：三块代码物理隔离、互不编译依赖，集成失败几乎都来自接口不一致。所以
  `contracts/` 是事实来源，任何一方改协议先改契约、发 PR、知会全员。
- **能离线就离线测**：每个人都要能在没有另外两台设备时，用 Mock/录制数据独立开发自测。
- **构建产物不进 git**：`oh_modules/`、`build/`、交叉编译出的 `.so/.ko`、模型权重等一律忽略。
