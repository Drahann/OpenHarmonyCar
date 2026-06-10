# App 视觉对接计划：随时展开的仪表识别视频（香橙派直连）

> 立项 2026-06-11（owner）。成员 B 已把香橙派视觉服务**接口定稿 + 代码跑通**（见
> `contracts/vision-stream-api.md` v1.0、`contracts/server-api.md` 已定稿、`orangepi-vision/` + 分支 `orangepi_control`）。
> 本文 = App 侧对接的**单一事实源**（接续/实施前先读它 + `contracts/vision-stream-api.md`）。
> **方法**：先规划（本文）→ /clear → 分阶段实施（[[feedback-fresh-context-for-big-tasks]]）。代码→`app-harmony-core`、文档→`main`+分支。

## 0. 目标与硬约束

- **香橙派一直在推理**（摄像头常连、FastAPI 常驻），App 侧只是**按需订阅**其实时结果。
- **目标**：App 能**随时展开 / 切换到视频界面**，看实时识别（处理后视频 + 仪表读数/告警）。
- **🔴 硬约束：不打扰当前导航。** 视觉链路（WS→香橙派 `192.168.1.5`）与导航链路（UDP→紫派、HTTP 拉图）
  **完全解耦**——打开/关闭/收起视频**不得**中断导航的保活、控制、建图、地图。两条链路不同设备、不同协议、不同 service，天然隔离。
- **服务器归属已定**：香橙派**本机** FastAPI+Uvicorn（`192.168.1.5:8000`），App 直连，**无云端/独立服务器**。

## 1. 接口摘要（权威见 `contracts/vision-stream-api.md` v1.0）

基地址 `http://192.168.1.5:8000`（局域网静态 IP）。

- **实时视频流 `WebSocket /ws/video`**（核心）：连上后服务器持续推，每帧**两条消息交替**：
  1. **二进制 JPEG**（`cv2.imencode` 质量 50，**已叠加**检测框/关键点/读数/FPS）→ App 直接当图显示。
  2. **JSON 文本 `frame_meta`**：`frame_id/timestamp/fps/inference_time_ms/num_detections/detections[]/gauge_angles[]`。
     - `detections[]`：`bbox[x,y,w,h]`、`score`、`keypoints[{name,x,y,conf}]`（最多前 3 个表）。
     - `gauge_angles[]`：每个表的读数**百分比 0~100**，与 detections 一一对应。
  - 摄像头失败：先发 `{"type":"error","message":...}` 再关连接。
- **REST**：`GET /api/summary`（实时 readings[value/unit/label/status/gauge_id]）、`GET /api/video/status`、
  `GET /api/history`（最近 600 条）、`GET /api/data/stats`、`POST /api/thresholds`（low/high）、
  `GET|POST|DELETE /api/gauge/configs[/{type}]`（量程/阈值/显示名）、`POST /api/data/report`（DeepSeek 报告，**会暂停推理释放 NPU**）、`GET /api/data/reports[...]`。

### ⚠️ 对接坑（来自 `orangepi-vision/README.md`，务必记住）

1. **关键点按 `name` 字段映射，绝不按 index**：契约表记 `0=center,1=pointer_tip,2=zero_mark,3=full_mark`，
   但当前部署模型实际输出顺序是 `0=pointer_tip,1=center,2=zero,3=full`。**App 解析 `keypoints` 一律用 `name`**（center/pointer_tip/zero_mark/full_mark），不要假设数组下标。
2. **读数语义**：`gauge_angles` 是**占比百分比**（`CW(zero→pointer)/CW(zero→full)×满量程`，clamp[0,1]）；
   物理量纲（MPa 等）靠 `/api/gauge/configs` 的量程映射。App 默认显示百分比 + 可选物理量。
3. **`bbox`/`keypoints` 坐标**是**该 JPEG 帧像素系**（xywh）。B 已把框/点叠加进图，**App 最简只显示图即可**；
   若 App 想自绘更丰富叠加，需按收到的帧分辨率对齐（见 V4 风险）。

## 2. 架构（贴合现有分层，全部**新增**、不动导航代码）

现有分层：`constants/ model/ service/ component/ pages/ utils/`。视觉模块照此加：

| 层 | 新文件 | 职责 |
|---|---|---|
| constants | `constants/vision.ets` | 香橙派地址（默认 `192.168.1.5:8000`、可配置）、WS/REST 端点、节流/重连/丢帧参数 |
| model | `model/vision.ets` | DTO：`FrameMeta / Detection / Keypoint / Reading / GaugeConfig` + JSON 解析（**按 name 映射关键点**） |
| service | `service/VisionService.ets` | WS 客户端（连/断/收帧/**丢帧**/自动重连）+ REST 方法 + 订阅回调；**与 RobotTransport/FleetMissionService 零耦合** |
| component | `component/VideoView.ets` | JPEG `ArrayBuffer`→`PixelMap`→`Image` 显示（只显最新帧）；`component/ReadingPanel.ets` 读数/状态/FPS/告警面板 |
| pages | `pages/VisionPage.ets` | 全屏视觉页（视频 + 读数面板 + 历史/阈值/报告入口） |

- 复用品牌主题 `constants/theme.ets`（AppColor），告警用 `danger`，与导航 UI 统一。
- 复用 `utils/log.ets`（`Log.scoped('VisionService')`）。
- **不镜像 car-agent**（agent 不涉视觉，视觉是平板独有）。

## 3. 「随时展开 / 不打扰导航」的 UX + 技术方案（本计划核心）

提供**两种形态**（可分阶段，互补）：

- **(A) 切换到全屏视频页（主推，V3）**：`HomePage` 与 `ControlPage` 顶栏/角落放「视频/识别」入口 → `router.pushUrl(VisionPage)`。
  返回导航页**无缝**（见下"不打扰"保证）。适合"专心看读数/历史/报告"。
- **(B) 可收起画中画 PiP（增强，V4 可选）**：`ControlPage` 上一个**可展开/收起的小窗**（叠在地图角落，绝对定位 + `HitTestMode` 不挡地图手势），
  实时小视频；点小窗放大到全屏 VisionPage。适合"边导航边瞥一眼仪表"。这才是字面意义的"随时展开、不离开导航"。

**🔴 不打扰导航的技术保证（每条都要在实施中守住）：**

1. **链路隔离**：`VisionService` 用**独立 WebSocket**连香橙派；与 `RobotTransport`（UDP 紫派）、`MapService`（HTTP 紫派）、
   `FleetMissionService`（软总线）**互不引用、互不共享 socket**。不同 IP/协议，天然不抢资源。
2. **导航保活不停**：进入/退出/收起视频时，`RobotTransport` 的心跳保活线程**继续运行**（不 stop、不重连）。
   视觉页只管自己的 WS，绝不碰导航 transport。
3. **只读订阅**：App 对香橙派只**收**视频/读数（WS）+ 用户主动的 REST（配置/报告）；**不发**任何控制，
   不存在与导航命令的时序/状态冲突。
4. **WS 生命周期 = 视图可见性**：视频视图**展开/进入时 `connect`、收起/离开时 `disconnect`**（省平板流量/电）。
   香橙派一直在推理、不受 App 连断影响（B 服务常驻）。重连用退避（断网/切页自动恢复）。
5. **并存（PiP）安全**：PiP 模式下 WS + UDP 两条链路**并行**；CPU/带宽在平板侧，注意 PiP 小窗用低帧率/小尺寸（见 V4）。

## 4. 视频帧渲染（性能要点，V2）

- WS `on('message')` 收到 `ArrayBuffer`（JPEG）→ `image.createImageSource(buf)` → `createPixelMap()` → `@State pixelMap` → `Image(pixelMap)`。
- **丢帧（关键）**：15FPS，平板解码/渲染可能跟不上 → **只保留最新帧**，解码进行中到来的帧直接丢，
  防积压 / 内存涨 / 延迟累积。可再**节流 UI 刷新**（如 ≤30fps）。
- 大分辨率（最高 1080p）解码耗时关注；必要时请 B 调小 `JPEG_QUALITY`/分辨率，或 App 端降采样显示。
- **及时回收 PixelMap**（旧帧 `release()`），避免 native 内存泄漏。
- 备选：`XComponent` + 自绘（性能更好但复杂）——先用 `Image+PixelMap`（最简），不够再换。
- ⚠️ **ArkTS 未经 DevEco 真编**：`@ohos.net.webSocket`、`@kit.ImageKit` 的 API 形参以 SDK 为准，首次编译按报错修。

## 5. 分阶段（V0–V5）

- **V0 规划** ✅（本文档）。
- **V1 服务 + 模型层**：`constants/vision` + `model/vision`（DTO+解析，关键点按 name）+ `service/VisionService`
  （WS 连/断/收/丢帧/重连 + REST 包装 + 订阅回调）。**验证**：写 `tools/mock-orangepi/`（Python WS server 推样例 JPEG+JSON）+ Node/纯逻辑断言 JSON 解析；不依赖真机/真香橙派。
- **V2 视频显示**：`VideoView`（JPEG→PixelMap+丢帧+回收）+ `ReadingPanel`（当前读数/状态/FPS/告警）。
- **V3 视觉页 + 入口（形态 A）**：`VisionPage` + `HomePage`/`ControlPage` 入口；**验证导航不受影响**（进出视频页，导航保活/控制照常）。
- **V4 数据功能 + PiP（形态 B，可选）**：REST 接入（历史曲线/阈值设置/仪表配置/报告生成与查看/导出）；ControlPage 可收起 PiP 小窗。
- **V5 真机联调**：连真香橙派 `192.168.1.5:8000`，端到端（视频流畅度/读数正确/重连）+ **与导航并存**（边导航边开视频，互不干扰）验证。

## 6. 待成员 B 确认 / 风险（→ 写入 `contracts/integration-qa.md`，异步对接）

1. **关键点 index↔name 顺序不一致**（README 已自曝）：App 以 `name` 为准；**建议 B 把契约表 index 与模型输出统一**，免后人踩。
2. **香橙派寻址**：IP 静态 `192.168.1.5`——App 写死？做成可配置（类似 SetIPPage）？是否支持发现（mDNS）？将来多香橙派/多摄像头？
3. **WS 背压**：App 慢时服务器会积压旧帧还是**只推最新**？若服务器不丢帧，App 必须自己丢（V4 已计划）；请 B 确认服务器侧策略。
4. **`bbox`/关键点参考分辨率**：相对**这帧 JPEG** 还是摄像头原始分辨率？App 若自绘叠加需对齐（最简方案不自绘则无关）。
5. **样例数据**：请 B 在 `contracts/fixtures/` 放 1 段样例（几帧 JPEG + 对应 JSON），供 App **离线对接联调**（兼做 mock-orangepi 输入）。
6. **报告生成暂停推理**：`POST /api/data/report` 期间摄像头停推（释放 NPU）——App 要给用户明确提示"生成中，视频暂停"。
7. **读数物理量纲**：默认显示百分比，还是用 `gauge/configs` 换算成 MPa 等？默认/单位缺省怎么处理。
8. **鉴权/并发**：`/ws/video` 是否限连接数？多端（平板+调试 PC）同时连会不会互相影响？

## 7. 边界（不做）

- App **不做推理**（全在香橙派）；App 不存视频（除非用户导出报告）。
- **不动导航/建图主流程**——视觉是独立新增模块，零侵入。
- 视频流只走 **WS+JPEG**（B 已选定，不上 RTSP/HLS）。
- 不在 car-agent 镜像（agent 无视觉）。

## 8. 接续指引（实施时从这里开始）

读 `MEMORY.md` + 本文档 + `contracts/vision-stream-api.md` 即可接续，不必重新探查。
**下一步 = V1**：`constants/vision` + `model/vision` + `service/VisionService` + `tools/mock-orangepi`。
提交：代码→`app-harmony-core`、文档/契约/tools→`main`+分支（[[feedback-docs-main-code-branch]]）；跨端问题（§6）写 `contracts/integration-qa.md` 给 B。
