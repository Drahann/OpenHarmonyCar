# orangepi-vision — 香橙派视觉推理（Python / C++）

**负责人：成员 B。** 香橙派 + 昇腾 NPU。摄像头实时识别工业仪表读数，处理后视频 + 结构化数据上报。
技术路线：**YOLOv5s 检测 → 裁剪 ROI → Simple Baselines 4 关键点 → 几何换算读数**。

## 建议目录

```
orangepi-vision/
├── detect/         # YOLOv5s 仪表检测（单类别），输出 ROI
├── pose/           # Simple Baselines 关键点（4 点：中心/指针尖/量程min/量程max）
├── reading/        # 由关键点几何换算读数 + calib.json 量程映射 + 阈值告警
├── server_client/  # 处理后视频 + 识别 JSON 上报服务器
└── models/         # 模型转换脚本/说明（ATC → .om）；权重本身不进 git
```

## 对接的契约

- [`../contracts/server-api.md`](../contracts/server-api.md) — **先和 App 一起把视频协议 + JSON 定稿**（当前 v0.0 待立项）。
- [`../contracts/calib.schema.json`](../contracts/calib.schema.json) — 仪表量程标定格式（草案，按实现定稿）。

## 模型与权重

- 模型经 **ATC** 工具转 `.om` 在昇腾 NPU 原生推理；ACL 上下文在独立子线程初始化，帧队列 + 跳帧保实时。
- 权重 / `.om` / `.onnx` / `.pt` **不进 git**（见根 `.gitignore`）；放共享盘或 Release，`models/` 只放转换脚本+说明。
- 要版本化权重再启用 Git LFS（见根 `.gitattributes`）。

## 离线开发 / 自测

用 [`../contracts/fixtures/`](../contracts/fixtures/) 里的样图/样片离线跑推理，断言读数误差（兼作精度回归）；
上报部分先 mock 掉服务器。性能目标：端到端 ~40ms，~15FPS。

---

## 实现进展（基线 v0.1 · YOLOv5s + Simple Baselines）

> 香橙派（OrangePi AI Pro 20T · 昇腾）实测跑通。**代码在分支 `orangepi_control`**，
> `.om` 权重经 Git LFS 版本化（`yolov5s_gauge_nchw_aipp.om` / `simple_baselines_256x192_bs1_fp32.om`）。

- **检测 → ROI → 关键点 → 读数** 全链路已在昇腾 NPU 上跑通（ACL / OM 原生推理）。
- 关键点模型：Simple Baselines（heatmap，输出 4 点，当前主用）；另备 RTMPose（SimCC）见 `deployment_bundle/rtmpose_om_infer.py`。
- **多表支持**：`test_gauge_reading_multi.py` 把 YOLO 置信度阈值降到 **0.10** 并加
  **长宽比（0.6–1.7）+ 面积（<85% 全图）过滤**剔除误检框（跨表大框 / 残框），可同时检测并逐表读数。
  实测一图两块压力表均正确检出并各自读数。
- 读数：几何换算指针占比 × 满量程（公式见下），物理量纲映射依赖量程标定（见 [`../contracts/calib.schema.json`](../contracts/calib.schema.json)）。

### 读数几何（对齐契约）

以圆心为原点，用 `atan2(-y, x)` 转数学坐标系，取指针相对 zero→full 的**顺时针（CW）**夹角占比，再乘满量程：

```
reading = CW(zero→pointer) / CW(zero→full) × 满量程     # 占比 clamp 到 [0, 1]
```

与 [`../contracts/vision-stream-api.md`](../contracts/vision-stream-api.md) 的角度计算方法一致。

## 上报消息格式

**以 [`../contracts/vision-stream-api.md`](../contracts/vision-stream-api.md)（v1.0 已定稿）为准** —— WebSocket（JPEG 帧 + JSON 元数据交替）+ 全部 REST 接口、`detections[]` 结构、关键点 `[{name,x,y,conf}]`、读数/告警字段均在该文档定义。本目录代码按此实现，不再在此重复 schema。

> ⚠️ **对接提醒（关键点通道顺序待核对）**：契约表格记关键点 index 为 `0=center, 1=pointer_tip, 2=zero_mark, 3=full_mark`；
> 但**当前部署的 `simple_baselines_256x192` 模型**，其 `.om` 输出通道顺序在 `test_gauge_reading.py` 中按
> `0=指针(pointer_tip), 1=圆心(center), 2=zero, 3=full` 解析（实测读数正确依赖此顺序）。
> 二者 index 不一致——上报时**以 `name` 字段为准**做映射即可，但建议与契约作者核对模型/契约的关键点顺序是否需统一。
