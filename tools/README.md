# tools — 联调 / Mock 工具

让每个人在缺少其它设备时也能独立开发自测。这些工具跨平台（建议 Python，PC 上即可跑）。

| 工具 | 给谁用 | 作用 |
|---|---|---|
| [`mock-purplepi/`](mock-purplepi/) | App | PC 端假紫派：按 UDP 9 字节协议回心跳/坐标 + HTTP 托管示例地图 |
| [`mock-app/`](mock-app/) | 紫派 | 假 App：按命令码表发各类指令，验证 UDP↔LCM 桥与轮控 |
| `replay/`（待建） | 全员 | LCM 日志 / 视频回放封装 |

> 这些目前是规格说明（README）。需要时可让 Claude 按 `contracts/udp-protocol.md` 直接生成可运行实现。
