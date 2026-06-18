# 服务器 API 契约 · 香橙派 → App（WiFi 直连）

**版本 v1.0（已定稿）**

> ## ✅ 已确定
> - **部署位置**：香橙派本机（FastAPI + Uvicorn，端口 8000）
> - **归属**：成员 B 实现维护，服务直接运行在香橙派上，无需独立服务器
> - **网络可达性**：局域网同网段直连，IP `192.168.1.5`
> - **详细接口文档**：见 [`vision-stream-api.md`](./vision-stream-api.md)

## 已知需求（来自设计文档 2.2.7 / 3.2.10）

- 香橙派输出：叠加了**检测框 / 关键点 / 实时读数 / 告警状态**的处理后视频帧 + 对应结构化数据。
- App：读取处理后视频与元数据，前端实时展示，与导航界面统一。
- 性能参考：端到端推理 ~73ms/帧，实际约 10 FPS；JPEG 帧均 ~17KB，带宽 ~1.4 Mbps。

## 视频流（已选定）

### 视频流（已选定）
| 方案 | 优点 | 适配性 |
|---|---|---|
| ~~RTSP~~ | 通用、低延迟 | App 端需播放器组件 |
| ~~HTTP-FLV / HLS~~ | 浏览器/网页友好 | HLS 延迟偏大 |
| **✅ WebSocket + JPEG 帧** | **简单、读数数据同通道** | **已实现，质量 50** |

```json
{
  "type": "frame_meta",
  "frame_id": 12345,
  "timestamp": 1730000000.123,
  "fps": 10.5,
  "inference_time_ms": 73.2,
  "yolo_time_ms": 21.0,
  "pose_time_ms": 8.5,
  "num_detections": 1,
  "detections": [
    {
      "bbox": [x, y, w, h],
      "score": 0.95,
      "keypoints": [
        {"name": "center", "x": 320, "y": 240, "conf": 0.99},
        {"name": "pointer_tip", "x": 280, "y": 160, "conf": 0.97},
        {"name": "zero_mark", "x": 200, "y": 300, "conf": 0.95},
        {"name": "full_mark", "x": 440, "y": 300, "conf": 0.93}
      ]
    }
  ],
  "gauge_angles": [80.5]
}
```

### REST 端点

| 端点 | 方法 | 说明 |
|---|---|---|
| `/api/summary` | GET | 实时状态（fps/latency/readings） |
| `/api/video/status` | GET | 摄像头运行状态（running/fps/frame_count） |
| `/api/history` | GET | 最近 600 条读数记录 |
| `/api/thresholds` | POST | 设置报警阈值（`{"low":N,"high":N}`） |
| `/api/gauge/configs` | GET | 仪表配置（量程/阈值/显示名） |
| `/api/data/report` | POST | 生成 DeepSeek 分析报告（暂停摄像头释放 NPU） |
| `/api/data/reports` | GET | 历史报告列表（limit/offset） |

## 联调前置

- App 侧已具备 WebSocket + HTTP 能力（`@kit.NetworkKit`），`app-harmony-core` 分支已实现完整客户端。
- 平板与香橙派须连同一 WiFi "Drahann"。
- 默认地址 `192.168.107.139:8000`，可在 App 设置页修改。
