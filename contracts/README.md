# contracts/ — 接口契约（神圣区域）

这里是三个节点之间**所有跨设备接口的唯一事实来源**。三块代码物理隔离、互不编译依赖，
集成能否成功，取决于这里的契约是否被各端逐字节/逐字段地遵守。

## 规则

1. **改契约 = 发 PR + @ 全员**。任何一端要改协议，先改这里的文档，PR 描述里写清"影响哪一端"。
2. **契约带版本号**（见下表）。协议有不兼容变更时，升版本号，并在各端代码里记录所依赖的版本。
3. **代码以契约为准**。两端实现不一致时，以契约为仲裁；若契约本身错了，先修契约再修代码。
4. **"待确认"必须显式**。任何尚未敲定的字段/语义，用 `⚠️ 待确认` 标注，不要默默假设。

## 契约清单

| 文件 | 接缝 | 主要责任方 | 版本 |
|---|---|---|---|
| [`udp-protocol.md`](udp-protocol.md) | App ↔ 紫派（控制/心跳） | App + 紫派 | **v0.2**（已对账 udp2lcm.c） |
| [`udp-protocol-crosscheck.md`](udp-protocol-crosscheck.md) | App↔紫派 协议对账报告（逐字段+行号证据） | App | 2026-06-03 |
| [`interface-review.md`](interface-review.md) | App↔紫派 接口优雅性复审（R1：妥协盘点+联合改进建议） | App | 2026-06-08 |
| [`integration-qa.md`](integration-qa.md) | 跨端异步对接 Q&A（Q1–Q13，✅ A 已答复 Q1–Q12 于 2026-06-08；Q13/Q6.1 待 A，见「状态总览」）| App + 紫派 | 滚动 |
| [`map-format.md`](map-format.md) | 紫派 → App（地图文件/HTTP） | 紫派 + App | **v0.2**（单位/URL 已对账） |
| [`multi-robot-collab.md`](multi-robot-collab.md) | 多机协同（软总线 + 多车覆盖） | App + 紫派 | **v0.2** |
| [`lcm/`](lcm/) | 紫派内部各模块 | 紫派 | v0.1 |
| [`server-api.md`](server-api.md) | 香橙派 → App（视频流+读数，WiFi 直连） | 香橙派(B) | **v1.0（已定稿）** |
| [`vision-stream-api.md`](vision-stream-api.md) | 香橙派视觉服务完整接口（REST + WebSocket `/ws/video`） | 香橙派(B) | **v1.0** |
| [`calib.schema.json`](calib.schema.json) | 仪表量程标定 | 香橙派 | v0.1（草案） |
| [`fixtures/`](fixtures/) | 共享测试数据 | 全员 | — |

## 开工前必须先拍板的事

- [x] **服务器归属与部署位置**：**已定 = 香橙派本机**（FastAPI + Uvicorn `:8000`，无独立/云端服务器）。见 `server-api.md` v1.0。
- [x] **视频传输协议**：**已定 = WebSocket 推 JPEG 帧**（`/ws/video`）+ REST 取读数/告警。见 `vision-stream-api.md` v1.0。
- [x] UDP 命令 5 / 'i' IP 复用 / 107·108 语义：**已对账解决**（见 `udp-protocol-crosscheck.md`）。剩余：**地图文件名拼写、坐标原点定义**。
- [ ] **多机协同**（`multi-robot-collab.md`）：坐标系单位/原点/0°、子机加载图后位姿是否归零到原点、地图传输 A/B、紫派能否常驻无界面 agent、子区域顶点表达。
