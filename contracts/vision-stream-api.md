# 香橙派视觉推理服务 — 视频流与数据接口文档

**版本 v1.0** · 更新时间：2026-06-10
**部署位置**：香橙派本机（Ascend 310B1 NPU）
**服务框架**：FastAPI + Uvicorn

---

## 1. 网络信息

| 项目 | 值 |
|------|------|
| 香橙派 IP | `192.168.1.5`（局域网静态） |
| 服务端口 | `8000` |
| 协议 | HTTP + WebSocket |
| 访问前提 | 平板/手机与香橙派在同一局域网内 |

> 基地址：`http://192.168.1.5:8000`

---

## 2. 实时视频流（核心接口）

### `WebSocket /ws/video`

```
ws://192.168.1.5:8000/ws/video
```

建立 WebSocket 长连接后，服务器会**持续推送**每一帧的处理结果。每帧由 **两条消息** 组成，交替发送：

#### 消息 1 — 图像帧（二进制）

| 字段 | 说明 |
|------|------|
| 类型 | WebSocket Binary Message |
| 格式 | **JPEG** |
| 编码 | `cv2.imencode('.jpg', frame, [JPEG_QUALITY, 50])` |
| 分辨率 | 摄像头原始分辨率（默认 640×480 或 1920×1080，取决于摄像头配置） |
| 内容 | 已渲染的推理结果帧——画面上叠加了检测框、关键点、读数文字、FPS |

**平板端处理方式**：直接将收到的 `bytes` 解码为图片并显示。

示例（ArkTS / TypeScript 伪代码）：
```typescript
// 收到 binary message
onMessage(data: ArrayBuffer) {
    // data 就是一帧完整的 JPEG 图片
    const imageSource = image.createImageSource(data);
    // 直接渲染到 Image 组件
}
```

示例（Python）：
```python
import websocket
import cv2
import numpy as np

ws = websocket.WebSocket()
ws.connect("ws://192.168.1.5:8000/ws/video")

while True:
    data = ws.recv()
    if isinstance(data, bytes):
        # JPEG 二进制 → OpenCV 图像
        arr = np.frombuffer(data, dtype=np.uint8)
        frame = cv2.imdecode(arr, cv2.IMREAD_COLOR)
        cv2.imshow("Gauge", frame)
        cv2.waitKey(1)
    elif isinstance(data, str):
        # JSON 元数据
        meta = json.loads(data)
        print(f"FPS={meta['fps']:.1f}, 读数={meta['gauge_angles']}")
```

#### 消息 2 — 帧元数据（JSON 文本）

| 字段 | 说明 |
|------|------|
| 类型 | WebSocket Text Message |
| 格式 | JSON |

**JSON 结构**：

```json
{
    "type": "frame_meta",
    "frame_id": 12345,
    "timestamp": 1699999999.123,
    "fps": 15.2,
    "inference_time_ms": 29.5,
    "yolo_time_ms": 21.0,
    "pose_time_ms": 8.5,
    "num_detections": 1,
    "detections": [
        {
            "bbox": [44, 525, 1252, 1083],
            "score": 0.997,
            "keypoints": [
                {"name": "center",      "x": 382.7, "y": 1338.0, "conf": 0.89},
                {"name": "pointer_tip", "x": 671.1, "y": 1083.6, "conf": 0.91},
                {"name": "zero_mark",   "x": 357.1, "y": 1370.5, "conf": 0.93},
                {"name": "full_mark",   "x": 983.0, "y": 1404.5, "conf": 0.85}
            ]
        }
    ],
    "gauge_angles": [80.5]
}
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| `type` | string | 固定为 `"frame_meta"` |
| `frame_id` | int | 帧序号，递增 |
| `timestamp` | float | Unix 时间戳（秒，精确到毫秒） |
| `fps` | float | 当前实时帧率 |
| `inference_time_ms` | float | 总推理耗时（YOLO + Pose），单位 ms |
| `yolo_time_ms` | float | YOLO 检测耗时，单位 ms |
| `pose_time_ms` | float | 关键点检测耗时，单位 ms |
| `num_detections` | int | 检测到的仪表数量 |
| `detections` | array | 检测结果列表（最多返回前 3 个） |
| `detections[].bbox` | [x,y,w,h] | 仪表检测框（像素坐标，xywh 格式） |
| `detections[].score` | float | 检测置信度（0~1） |
| `gauge_angles` | array\<float\> | 每个仪表的读数百分比（0~100），与 detections 一一对应 |

#### 连接生命周期

```
客户端                            服务器 (192.168.1.5:8000)
  |                                    |
  |--- WebSocket CONNECT /ws/video --->|
  |<-- 101 Switching Protocols --------|
  |                                    |  (初始化摄像头，等待首帧)
  |<-- Binary: JPEG frame #1 ---------|
  |<-- Text: JSON meta #1 ------------|
  |<-- Binary: JPEG frame #2 ---------|
  |<-- Text: JSON meta #2 ------------|
  |    ...持续推送...                   |
  |--- CLOSE ------------------------->|
  |                                    |
```

> **注意**：如果摄像头未连接或初始化失败，服务器会先发送一条 JSON 错误消息：
> ```json
> {"type": "error", "message": "摄像头初始化失败"}
> ```
> 然后关闭连接。

---

## 3. HTTP REST 接口

### 3.1 状态查询

#### `GET /api/summary`

返回当前系统实时状态。

**响应**：
```json
{
    "fps": 15.2,
    "latency_ms": 29.5,
    "frame_id": 12345,
    "over_limit": false,
    "readings": [
        {
            "value": 80.5,
            "unit": "%",
            "label": "仪表读数",
            "status": "NORMAL",
            "gauge_id": 1
        }
    ]
}
```

| `readings[].status` | 含义 |
|---------------------|------|
| `"NORMAL"` | 读数在阈值范围内 |
| `"ALARM"` | 读数超出阈值 |
| `"UNKNOWN"` | 无法获取读数 |

#### `GET /api/video/status`

返回摄像头运行状态。

**响应**：
```json
{
    "running": true,
    "fps": 15.2,
    "frame_count": 5678
}
```

---

### 3.2 历史数据

#### `GET /api/history`

返回最近 600 条读数记录。

**响应**：
```json
{
    "history": [
        {
            "timestamp": 1699999999.0,
            "frame_id": 12340,
            "readings": [{"value": 80.5, "status": "NORMAL"}]
        }
    ]
}
```

#### `GET /api/data/stats`

获取数据统计。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `start_time` | string (ISO) | 否 | 开始时间，如 `2025-10-10T00:00:00` |
| `end_time` | string (ISO) | 否 | 结束时间 |
| `gauge_type` | string | 否 | 仪表类型筛选 |

不传时间参数则默认最近 24 小时。

---

### 3.3 报警阈值

#### `POST /api/thresholds`

设置报警阈值（百分比）。

**请求体**：
```json
{
    "low": 10.0,
    "high": 90.0
}
```

**响应**：
```json
{
    "ok": true,
    "thresholds": {"low": 10.0, "high": 90.0}
}
```

---

### 3.4 仪表配置

#### `GET /api/gauge/configs`

获取所有仪表配置。

#### `GET /api/gauge/configs/{gauge_type}`

获取指定类型仪表配置。

#### `POST /api/gauge/configs/{gauge_type}`

保存仪表配置。

**请求体**：
```json
{
    "unit": "MPa",
    "min_range": 0.0,
    "max_range": 1.6,
    "low_threshold": 0.0,
    "high_threshold": 1.4,
    "display_name": "压力表"
}
```

#### `DELETE /api/gauge/configs/{gauge_type}`

删除仪表配置（恢复默认）。

---

### 3.5 分析报告

#### `POST /api/data/report`

生成分析报告（调用 DeepSeek 大模型）。生成期间会暂停摄像头推理释放 NPU。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `start_time` | string (ISO) | 否 | 开始时间 |
| `end_time` | string (ISO) | 否 | 结束时间 |
| `hours` | int | 否 | 未指定时间时取最近 N 小时（默认 24） |

#### `GET /api/data/reports`

获取历史报告列表。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `limit` | int | 10 | 返回数量 |
| `offset` | int | 0 | 偏移量 |

#### `GET /api/data/reports/{report_id}`

获取单个报告详情。

#### `GET /api/data/reports/{report_id}/export?format=md`

导出报告为文件（支持 `md`、`txt`）。

#### `DELETE /api/data/reports/{report_id}`

删除报告。

---

### 3.6 数据管理

#### `DELETE /api/data/readings/{gauge_type}`

删除指定仪表类型的所有读数。

#### `DELETE /api/data/readings`

按时间范围删除读数。

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `start_time` | string (ISO) | 否 | 开始时间 |
| `end_time` | string (ISO) | 否 | 结束时间 |

不传参数则删除所有读数。

#### `POST /api/data/clear-all`

清空所有数据（读数 + 报告）。**危险操作**。

#### `GET /api/data/database-info`

获取数据库调试信息。

---

### 3.7 DeepSeek 大模型

#### `GET /api/llm/state`

查询 DeepSeek 模型状态。

**响应**：
```json
{
    "available": true,
    "status": "ready",
    "detail": "模型已加载",
    "model": "DeepSeek-R1-Distill-Qwen-1.5B",
    "total_queries": 5,
    "last_query_time": 3.21,
    "avg_query_time": 2.85
}
```

#### `POST /api/llm/query`

DeepSeek 对话接口（SSE 流式响应）。调用时会暂停摄像头释放 NPU。

**请求体**：
```json
{
    "message": "分析一下当前仪表读数趋势",
    "history": []
}
```

**响应**：`text/event-stream`，每个事件格式：
```
data: {"token": "分析", "done": false}
data: {"token": "结果", "done": false}
data: {"reply": "完整回复内容...", "done": true}
```

---

## 4. 推理流水线

```
USB 摄像头 (/dev/video0)
    │
    ▼
  抓帧（OpenCV, 640×480 YUYV 或 1080p MJPG）
    │
    ▼
  YOLOv5s 仪表检测（AIPP OM, 21ms）
  输入: [1, 640, 640, 3] NHWC uint8 RGB
  输出: 仪表检测框 [x, y, w, h, score]
    │
    ▼
  裁剪仪表 ROI
    │
    ▼
  Simple Baselines 关键点检测（OM, 8ms）
  输入: [1, 3, 256, 192] NCHW float32（ImageNet 归一化）
  输出: [1, 4, 64, 48] 热力图（4 个关键点）
    │
    ▼
  热力图 → 关键点坐标（亚像素精炼）
  4 个关键点: center, pointer_tip, zero_mark, full_mark
    │
    ▼
  角度计算 → 仪表读数（顺时针扫过方向）
  reading = CW(zero→pointer) / CW(zero→full) × 满量程
    │
    ▼
  渲染叠加（检测框 + 关键点 + 读数文字 + FPS）
    │
    ▼
  JPEG 编码（quality=50）→ WebSocket 推送
```

**端到端性能**：

| 阶段 | 耗时 |
|------|------|
| YOLO 检测 | ~21 ms |
| 关键点检测 | ~8 ms |
| 预/后处理 | ~3 ms |
| JPEG 编码 | ~2 ms |
| **总计** | **~34 ms（约 29 FPS）** |

---

## 5. 4 个关键点定义

| 编号 | 名称 | 含义 | 用途 |
|------|------|------|------|
| 0 | `center` | 表盘旋转中心（轴心） | 角度计算参考原点 |
| 1 | `pointer_tip` | 指针尖端 | 当前读数方向 |
| 2 | `zero_mark` | 零刻度标记 | 量程起点（角度 0%） |
| 3 | `full_mark` | 满刻度标记 | 量程终点（角度 100%） |

```
        0.8
       /   \
     0.4    1.2     ← 表盘
     |       |
     0  [C]  1.6    C = center
      \  |  /       P = pointer_tip
       \ P /        Z = zero_mark (0位置)
        [Z]---[F]   F = full_mark (1.6位置)
```

---

## 6. 平板端接入指南

### 6.1 最简接入（仅显示视频）

1. 建立 WebSocket 连接到 `ws://192.168.1.5:8000/ws/video`
2. 收到 `Binary` 消息 → 解码为 JPEG 图片 → 显示
3. 收到 `Text` 消息 → 解析 JSON → 提取 `gauge_angles` 显示读数
4. 连接断开时自动重连

### 6.2 完整接入

1. 轮询 `GET /api/summary` 获取实时状态
2. WebSocket 视频流用于实时画面
3. `GET /api/history` 获取历史数据绘制趋势图
4. `POST /api/thresholds` 设置报警阈值
5. `GET /api/gauge/configs` 获取仪表量程配置

### 6.3 错误处理

| 场景 | 处理方式 |
|------|---------|
| WebSocket 断开 | 3 秒后自动重连 |
| 收到 `{"type":"error"}` | 显示错误提示，等待重连 |
| HTTP 返回 5xx | 服务器异常，稍后重试 |
| 读数为 `NaN` | 显示"无法识别" |
| `status: "ALARM"` | 触发报警 UI（闪烁/声音） |

---

## 7. 模型文件（不入 Git）

| 文件 | 大小 | 路径（香橙派） |
|------|------|----------------|
| YOLOv5s 检测 | 15 MB | `~/simple_baselines/deployment_bundle/yolo/yolov5s_gauge_nchw_aipp.om` |
| Simple Baselines 关键点 | 66 MB | `~/simple_baselines/deployment_bundle/simple_baselines/simple_baselines_256x192_bs1_fp32.om` |
| CRNN 文字识别 | 16 MB | `~/simple_baselines/new_yolo_crnn/CRNN/infer/model/crnn.om` |

---

## 8. 启动方式

```bash
# SSH 到香橙派
ssh HwHiAiUser@192.168.1.5

# 启动推理服务
cd ~/simple_baselines/web
bash 启动服务.sh

# 或手动启动
cd ~/simple_baselines/web/backend
export PATH=/usr/local/miniconda3/bin:$PATH
source /usr/local/miniconda3/etc/profile.d/conda.sh
conda activate base
source /usr/local/Ascend/ascend-toolkit/set_env.sh
python3 main.py
# 服务监听 0.0.0.0:8000
```
