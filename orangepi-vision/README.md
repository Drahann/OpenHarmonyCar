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
