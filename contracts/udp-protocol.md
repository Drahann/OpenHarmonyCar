# UDP 协议契约 · App ↔ 紫派

**版本 v0.2** — 字节布局/命令语义已与**紫派实现** `purplepi-control/NewWheelCtrl/udp2lcm/` **逐字段对账**
（详见 [`udp-protocol-crosscheck.md`](udp-protocol-crosscheck.md)）。原 v0.1 的 `⚠️ 待确认` 多数已落实，余项见对账报告。

## 传输层

| 项 | 值 | 说明 |
|---|---|---|
| 协议 | UDP | App 用 `socket.UDPSocket` |
| 端口 | **5001** | App 收发同端口（`sendPort = receivePort = 5001`） |
| 包长 | **9 字节**固定 | `byteLength = 9` |
| 字节序 | **大端 (big-endian / 网络序)** | 多字节整型用大端；C 侧用 `htons/ntohs` |
| 默认目标 IP | `172.168.11.99` | App 默认连接地址，可在界面修改 |

## 发送方向：App → 紫派（9 字节）

> 来源：`MakeSendData(udpsenddata)`

| 偏移 | 字段 | 类型 | 说明 |
|---|---|---|---|
| 0 | `state` | uint8 | **命令码**（= App `robotState` 枚举值），见下表 |
| 1 | `runState` | uint8 | 运动方向：`0=stop 1=go 2=left 3=right`（仅命令 1 有意义），缺省 0 |
| 2 | `speed` | uint8 | 速度，缺省 0 |
| 3–4 | `endX` | int16 (BE) | 目标点 X（命令 3）；命令 'i'(105) 时其**低字节 byte4** = 主机 IP 第 3 段 |
| 5–6 | `endY` | int16 (BE) | 目标点 Y（命令 3）；命令 'i'(105) 时其**低字节 byte6** = 主机 IP 第 4 段 |
| 7–8 | — | — | 发送时为 0（未使用） |

> **byte1/byte2 按命令复用**：命令 1 时 = `runState`/`speed`；命令 'h'(104)/'l'(108) 时 byte1 = 房间号/robot_id；
> 命令 'i'(105) 时 byte1/byte2 = 主机 IP 前两段。**主机 IP 四段完整打包在 byte[1]、[2]、[4]、[6]**（见对账报告第五节）。

## 接收方向：紫派 → App（9 字节，心跳）

> 来源：`parseReceiveData(a)`。紫派每 **500ms** 发一次心跳（`udp.c:121`；原契约写 1s，已更正）。

| 偏移 | 字段 | 类型 | 说明 |
|---|---|---|---|
| 0 | `state` | uint8 | 心跳/状态字节。设计文档：**首字节为 3 表示本帧带当前坐标**，客户端据此更新 |
| 1–2 | — | — | 接收侧未使用 |
| 3–4 | `x` | int16 (BE) | 机器人当前 X（真实世界坐标） |
| 5–6 | `y` | int16 (BE) | 机器人当前 Y |
| 7–8 | `r` | int16 (BE) | 机器人当前朝向/角度 |

## 命令码表（byte0）

App 的 `robotState` 枚举值**刻意对齐 ASCII**，所以 byte0 既是 App 状态，也是下发给紫派的命令码
（如 `fullpath_startRoute = 102 = 'f'`）。

| byte0 | App 命令（`RobotCommand`） | 字符 | 紫派动作（✅ 已对账 `udp2lcm.c`，行号见对账报告） |
|---|---|---|---|
| 0 | beforeStart | — | 建连/心跳；经 parseCmd 时启动建图(**30**，网格 0.05m)。⚠️ 首包仅建连(发 **7** 初参+启心跳)、不进 parseCmd |
| 1 | pending | — | 遥控运动：`path.cmd=byte1, speed=byte2` → `wheel_ctrl` |
| 2 | afterEnd | — | 结束建图：停轮控；保存地图(**32**)；加载导航地图(**10**) |
| 3 | startRoute | — | 设目标点(endX,endY，÷20=米) → **20** |
| 4 | endRoute | — | 取消导航 → **23** + 多轮停车 |
| 5 | loadMap | — | **加载地图**：停车→取当前位姿→加载导航图(**10**)。（App 已补 `loadMap=5`） |
| 102 | fullpathStartRoute | 'f' | 启动全息路径规划(**127**)，iparams[0]=byte1 |
| 103 | fullpathStartover | 'g' | 取消全息路径规划(**126**) |
| 104 | fullpathSelect | 'h' | **单机**全路径选房间顶点(**125**)，byte1=房间号，坐标 byte3-6 |
| 105 | distributed | 'i' | 子机拉主机地图(**124**)；主机 IP 四段在 byte[1],[2],[4],[6] |
| 106 | distributedEnd | 'j' | 加载地图(**10**)+接收主机目标点(byte3-6) → **20** |
| 107 | distAreaCorner1 | 'k' | **分布式覆盖矩形·对角点1**(byte3-6)，仅暂存 |
| 108 | distAreaCorner2 | 'l' | **对角点2(byte3-6)+robot_id(byte1：0主/1从)**：从机规划 FullRoad 覆盖(**122**)+分布式跟踪(**123**)；主机加载图(10)+跟踪 |
| 120 | —（已弃用） | — | **紫派未实现**，落入 else 被忽略 → App 老代码死命令 |

## 坐标单位与朝向（✅ 已对账）

- **1 整数单位 = 1/20 m = 5 cm**：位姿 `pos[m]×20` 写入心跳（`udp2lcm.c:424`），接收端 `÷20` 回米。
  建图网格分辨率 0.05m（`udp2lcm.c:97`）→ **UDP 整数坐标与地图格子 1:1**（同以 5cm 为单位）。
- **朝向 `r` = 度**，范围 [-180,180]（`udp2lcm.c:429-433`）。
- 坐标系原点 = master 建图起始位姿；多机下子机"从 master 起点出发"使初始位姿按构造 (0,0,0°)，见 [`multi-robot-collab.md`](multi-robot-collab.md)。

## 连接与保活约定

- **建连**：App 向目标 IP 发命令 0；紫派**首包**用于捕获手机 IP、启动心跳线程、发 LCM **7**（初参），
  **不进 parseCmd**——故建图(LCM 30)由**后续**命令 0 触发（App 持续发 0 即可，`udp.c:31-58`）。
  App 在 `udp.on` 收到该 IP 回包即判定连接成功。
- **心跳**：紫派每 **500ms** 回一帧（`udp.c:121`）。
- **保活/安全**：App 至少每 **1s** 发一次指令（哪怕空指令）。紫派若 **3s** 未收到任何指令（`udp.c:77`），
  立即令轮控**急停**以防失控，并继续等待。⚠️ 该超时值两端务必一致。
- **遥控长按**：App 持续发命令 1（每实例节流）；松手发 `runState=stop`。
