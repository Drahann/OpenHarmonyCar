# LAN Socket 黑板传输方案（替换 distributedDataObject）

> **日期 2026-06-13** · 实施单（交子 agent 执行）。
> 目标：把多机协同的 `FleetMission` 黑板传输从 **鸿蒙软总线 `distributedDataObject`(DDO)** 换成 **平板↔车 agent 的纯 LAN TCP socket**，
> **绕开 deviceManager 系统权限 / 互信 / DDO 一整套**，让分布式覆盖在**消费版平板 + 紫派**这对真机上能真正跑通。
> 前置阅读：[`docs/distributed-bringup-status.md`](distributed-bringup-status.md)、[`contracts/multi-robot-collab.md`](../contracts/multi-robot-collab.md)、`contracts/app-purplepi-alignment-audit.md`。
> 代码落 `app-harmony-core`；本文（docs）按约定同步 `main`。

---

## 0. 为什么换（别再回头啃 DDO）

真机两端日志逐层定位（**完整根因分析见 [`docs/distributed-bringup-status.md`](distributed-bringup-status.md) §3**），结论：**DDO 共享黑板依赖一长串"消费端先决条件"，在这对真机上凑不齐 / 拿不到**——

- DDO 走 `ObjectStore → DistributedDB → DSched → DHDM` 长链路，每环都要正常；旧 App 用 `startAbility` 走 AMS 路径**完全不需要**这些。
- 关键缺口（status 文档实锤）：**蓝牙未开**（DDO 设备发现/匿名 UDID 依赖蓝牙→平板 `GetAnonyLocalUdid Failed 96929750`、`CheckApiPermission permissionLevel:2 失败`）、**未做"超级终端"吸附**、紫派后期会话 **DSched accessToken 失效**、**可能还要同华为账号**。
- 结果：两端 `[SearchOnline] onlineDev count = 0` / `Collaboration deivces size:0` → DDO **没有在线对端可同步**。
- 这些先决条件对**工业紫派 + 消费 MatePad** 这对组合既脆又难凑（消费机系统权限拿不到、紫派 DSched 不稳、还要开蓝牙/超级终端/账号）。

**LAN socket 全不碰这些**：三方应用 + WiFi TCP **只需 `ohos.permission.INTERNET`（normal，安装即授）**，零系统权限、零蓝牙、零超级终端、零账号。机器人 UDP 5001 一直通就是铁证——同样三方、同样 WiFi。

---

## 1. 方案总览

- 黑板内容不变：仍是 `MissionSnapshot`（`model/mission.ets`，单一 JSON 快照）。**不改协同逻辑、不改 `MissionSnapshot` 结构。**
- 传输层换成 **TCP**：**车载 agent = TCP 服务端**（监听），**平板 = TCP 客户端**（连到每辆车）。
- 星形拓扑，**平板是中枢**：平板↔每辆车各一条 TCP 连接；**车与车之间不经黑板互联**（覆盖任务里每辆车只需"从平板拿自己的 assignment + master IP""向平板报自己的位姿/进度"，无需车↔车）。
- **保持 `FleetMissionService` 对外接口不变**（`init/joinSession/leaveSession/publishMission/subscribeMission`），只换内部实现 → `ControlPage`/`AgentCore` 几乎不动。
- **互信/DeviceTrustPage/bindTarget/DDO/DATASYNC 全部不再需要**（黑板这条线）——见 §7 删除清单。

### 为什么 agent 当服务端、平板当客户端
平板**知道车的 IP**（来自 UDP `0x06` 广播发现 `RobotTransport.discover`，或用户填的 IP）；车 agent **不知道平板 IP**。所以**平板主动连车**；agent 从入站连接里自然拿到平板地址、在同一连接上回包，无需预先知道平板 IP。

---

## 2. 寻址 & 端口

- 新增常量 `FLEET_LAN_PORT = 5003`（`constants/protocol.ets`，**app + car-agent 两份同步**，§4 守卫）。与 udp2lcm(5001)/agent 收口(5002)/地图 HTTP(8000) 互不冲突。
- **agent**：`TCPSocketServer` bind `0.0.0.0:5003`，accept 平板连接。
- **平板**：对**每辆要协同的车**（IP 来自 `mission.robots[].ip`）各开一个 `TCPSocket` 连到 `<carIP>:5003`。

---

## 3. 线协议（length-prefixed JSON）

每条消息 = **4 字节大端长度前缀 + UTF-8 JSON 负载**（**不要**依赖 TCP 报文边界；必须自己按长度拆包，处理粘包/半包）。

JSON 负载形如 `{ "t": <type>, ... }`，两种类型：

| t | 方向 | 负载 | 含义 |
|---|---|---|---|
| `"hello"` | 双向，连接建立时各发一次 | `{ t:"hello", session:"OpenHarmonyCarFleetV1", role:"pad"\|"car", carId? }` | 握手 + 校验 `session`（= `FLEET_SESSION_ID`）。不匹配则断开。 |
| `"mission"` | **平板 → 车** | `{ t:"mission", snapshot: MissionSnapshot }` | 平板下发**整块**任务黑板（phase/assignments/area/map.ref/frame/endPoints…规划真相）。 |
| `"robot"` | **车 → 平板** | `{ t:"robot", robot: RobotRuntimeDTO }` | 车回报**本车**运行态（index/ip/online/x/y/r/command/progress/status）。 |

- 连接建立后：**平板**立刻发当前 `mission`；**车**立刻发当前 `robot`（让双方拿到最新态）。
- 之后：`publishMission` 触发即发一条（见 §4 映射）。

---

## 4. 合并规则（谁权威，避免互相覆盖）

**平板对规划字段权威；每辆车对自己的 robot 权威。** 据此分方向：

- **车收到 `mission`** → `mission.applySnapshot(snapshot)` **整块覆盖**（平板是规划真相）→ 派发给监听者（`AgentCore.onBlackboard` → reconcile）。
- **平板收到 `robot`** → **只把这条 `robot` 合并进 `this.mission.robots[index]`**（更新位姿/进度/状态/online），**保留**平板本地的 phase/assignments/area/map → 派发给监听者（`ControlPage.onRemoteMission` 刷新显示）。

> 这样**多车也不互相覆盖**（旧 DDO 单一 missionJson 双向整覆盖会把别的车的位姿冲掉，是"最后写者胜"的 MVP 缺陷；本方案天然修掉）。

### `FleetMissionService` 内部映射（接口签名不变）
- **App 版** `publishMission(mission)`：对 `mission.robots[].ip` 里每个有效 IP 确保已连接（未连则发起连接），然后向各连接发 `{t:"mission", snapshot: mission.toSnapshot()}`。收到 `{t:"robot"}` → 合并进本地镜像的 `robots[index]` → 派发合并后的 `MissionSnapshot` 给 `subscribeMission` 监听者。
- **agent 版** `publishMission(mission)`：取 `mission.robots` 里**本车**那条（`index === carId`，`carId` 由 `AgentCore.setCarId` 已知——给 agent 版 `FleetMissionService` 加个 `setSelfCarId(id)`，或 `publishMission(mission, carId)`）发 `{t:"robot", robot}`。收到 `{t:"mission"}` → `applySnapshot` → 派发给 `subscribeMission` 监听者。
- `joinSession(sessionId)`：**agent** = 启动 TCP 服务端监听 5003、保存 session 作握手校验；**平板** = 记录 session（连接在 publishMission 时按 robots[].ip 惰性建立）。`leaveSession` = 关连接/停监听。

---

## 5. 要改 / 删的文件

**改（核心）**
- `app-harmony/.../constants/protocol.ets`（+ car-agent 同步副本）：加 `FLEET_LAN_PORT = 5003`。
- `app-harmony/.../service/FleetMissionService.ets`：**整体重写传输层**——删 DDO/DM/权限，改 TCP 客户端（连 `robots[].ip:5003`、发 mission、收 robot 合并、派发）。
- `car-agent/.../service/FleetMissionService.ets`：**整体重写**——删 DDO/DM，改 TCP 服务端（监听 5003、收 mission 派发、`publishMission` 发本车 robot）。
- `car-agent/.../agent/AgentCore.ets`：`publishMission` 调用处确保把 `carId` 告知 fleet（加 `fleet.setSelfCarId(this.carId)`）；其余基本不动。`start()` 里 `init`→`join` 串行化已做，保留。

**删 / 停用（互信整条线不再需要）**
- `app-harmony/.../pages/DeviceTrustPage.ets` + `HomePage` 的「设备互信」入口：**停用/删除**（LAN 黑板不需要 bindTarget 互信）。
- `car-agent/.../pairingability/PairingAbility.ets` + `pages/PairingPage.ets` + `AgentStatusPage` 的「打开配对界面」：**停用/删除**。
- 两份 `FleetMissionService` 里：删 `distributedDeviceManager`/`distributedDataObject`/`abilityAccessCtrl`/`requestPermissions`/`bindDevice`/`unbindDevice`/`startDiscovering`/`listDevices`。
- `module.json5`：黑板不再需要 `DISTRIBUTED_DATASYNC`；**确保有 `ohos.permission.INTERNET`**（socket 用，normal 权限）。

**不动**
- `model/mission.ets`（`MissionSnapshot`/`RobotRuntimeDTO` 复用）、`reconciler/Reconciler.ets`、UDP 那条链路（`RobotTransport`/端口 5002/引擎门控）、地图、紫派侧。
- 平板发现车 IP 仍走现成的 UDP `0x06` 广播（`RobotTransport.discover`）——**LAN 黑板复用它给的 IP**，不需要 DM 发现。

---

## 6. 连接管理 / 可靠性

- **平板**：每辆车一条连接；断开→**指数退避重连**（车可能重启/WiFi 抖动）。重连成功后立刻补发当前 mission。
- **agent**：accept 平板连接；连接断开容忍（平板会重连）；同一时刻通常仅一个平板连接（多平板非目标，后连覆盖前连即可）。
- 用 **4 字节长度前缀**严格拆包；JSON 解析整段 `try/catch`，坏包丢弃不崩。
- 可选 app 级心跳（每 ~2s 一条空 `robot`/`mission` 或 `ping`）做保活 + 死链检测；MVP 可先靠 TCP + 重连。
- 日志：连接建立/断开、收发 mission/robot 的条数与关键字段（沿用 `Log.scoped('FleetMission')`，DOMAIN 0xD002），便于真机判据。

---

## 7. ArkTS / API 注意

- 用 `@ohos.net.socket`：客户端 `socket.constructTCPSocketInstance()`；服务端 `socket.constructTCPSocketServerInstance()`（API 10+，平板 HarmonyOS5 / 紫派 OH5 均有）。
- TCPSocketServer：`listen({address:'0.0.0.0', port:5003})` → `on('connect', (client) => {...})`；每个 client 上 `on('message')` 收、`send()` 发。
- 权限：`ohos.permission.INTERNET`（module.json5；normal，安装即授，**无需弹框、无需系统签名**）。
- ArkTS 严格类型：消息对象用**已声明 interface**（如 `interface FleetMsg { t: string; snapshot?: MissionSnapshot; robot?: RobotRuntimeDTO; ... }`），避免 `arkts-no-untyped-obj-literals`。
- ⚠️ 本仓多文件未经 DevEco 全量编译，注意 SDK 签名（如回调 arity、类型导出位置）——编译报错按提示逐个修。

---

## 8. 测试 & 验收

**离线（Node 镜像，放 `tools/verify/`）**
- length-prefix framing 的 encode/decode（含粘包/半包：两条消息拼一个 buffer、一条拆两个 buffer，都要正确还原）。
- 合并规则：平板收 `robot` 只更新 `robots[index]`、保留 planner 字段；车收 `mission` 整覆盖。
- §4 共享层守卫：`constants/protocol.ets` 改了 → 同步 car-agent 副本，跑 `node tools/verify/verify.mjs` 53+ 项全绿。

**真机（平板 + 紫派，同一 WiFi，无需互信/同账号）**
1. 平板进分布式模式、发现到车（有车 IP）。
2. 平板划覆盖区域 → **紫派 agent 日志出现 `黑板更新: phase=covering … → 产 N 条命令`** + `TX→ 127.0.0.1 …`。
3. 平板出现第二个 toast「车N：agent 已下发覆盖/执行命令」；地图上车**变色 + 随心跳移动**（agent 经 LAN 回写 robot）。
4. 断网/重启车 → 平板自动重连、黑板恢复。

**验收判据（算通了）**：上面 2/3/4 全绿，且**全程不需要 bindTarget/互信/同账号/系统权限**。

---

## 9. 风险 / 注意

- **同一 WiFi 且无客户端隔离**仍是前提（socket 也要设备互通；但这是普通 TCP，比 DDO 宽松得多，UDP 既然通它就通）。
- 多车：平板对 N 辆车各一条连接；`mission` 广播给全部，`robot` 各车只报自己——已天然避免互相覆盖。
- master IP 仍由平板写进 `mission.map.ref`（子车 agent 据此发 cmd105 拉图，方案B，不变）。
- 这是**传输层替换**，协同语义（reconcile/cmd 时序/方案B）一行不动；紫派侧无需任何改动（黑板本就在 App/agent 之间）。
- 保留 `FleetMissionService` 的"传输无关"边界：将来若真机环境允许，可再加一个 DDO 实现切回——但**当前以 LAN socket 为唯一可用实现**。
