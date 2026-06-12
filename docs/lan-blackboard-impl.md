# LAN Socket 黑板：实施详细文档

> **日期**: 2026-06-13  
> **状态**: ✅ 已完成  
> **分支**: `app-harmony-core` (代码) / `main` (文档)  
> **前置文档**: [`lan-blackboard-plan.md`](lan-blackboard-plan.md) (设计方案) / [`distributed-bringup-status.md`](distributed-bringup-status.md) (DDO 失败根因)

---

## 1. 背景与动机

### 1.1 DDO 失败总结

鸿蒙软总线 `distributedDataObject` (DDO) 在消费级 MatePad + 紫派 OpenHarmony 组合上无法工作。8 份 HiLog 逐层分析确认根因链：

```
平板 GetAnonyLocalUdid Failed (96929750)  →  蓝牙未开 / 权限 level 不足
  ↓
紫派 DSched accessToken error (7)  →  IPC 令牌失效 / 服务不稳定
  ↓
两端 onlineDev count = 0 / Collaboration devices size = 0  →  DDO 无对端可同步
```

DDO 依赖一长串系统先决条件：
- ✅ 蓝牙开启（设备发现 + UDID 匿名化）
- ✅ 超级终端吸附（系统级组网配置）
- ✅ DSched 服务正常（分布式调度框架）
- ✅ 可能需要同华为账号（云端信任链）

这些条件在**工业紫派 + 消费平板**组合上既脆弱又难凑齐。旧 App 用 `startAbility` 走 AMS 路径完全不需要这些条件，所以能通。

### 1.2 LAN Socket 方案的优势

LAN TCP Socket 全不碰这些：
- **零系统权限**：只需 `ohos.permission.INTERNET`（normal，安装即授，无需弹框）
- **零蓝牙**：不依赖设备发现框架
- **零超级终端**：不需要系统级组网配置
- **零账号**：不需要华为账号信任链
- **已验证**：机器人 UDP 5001 一直通就是铁证——同样三方应用、同样 WiFi 环境

### 1.3 决策

**传输层替换**：把 `FleetMissionService` 的内部实现从 DDO 换成 TCP Socket，保持对外接口不变。协同语义（reconcile/cmd 时序/方案B）一行不动；紫派侧无需任何改动（黑板本就在 App/agent 之间）。

---

## 2. 架构设计

### 2.1 拓扑结构

```
┌─────────────┐         TCP :5003          ┌─────────────┐
│   平板 App  │ ◄────────────────────────► │  车 agent   │
│  (客户端)   │   mission / robot JSON     │  (服务端)   │
└─────────────┘                            └─────────────┘
      ▲                                           ▲
      │                                           │
      │ UDP 0x06 广播发现                          │ UDP 5002 → 5001
      │ (RobotTransport.discover)                  │ (agent → udp2lcm)
      │                                           │
      ▼                                           ▼
┌─────────────┐                            ┌─────────────┐
│  紫派 udp2lcm │                            │  紫派 udp2lcm │
│  (端口 5001)  │                            │  (端口 5001)  │
└─────────────┘                            └─────────────┘
```

- **星形拓扑**：平板是中枢，平板↔每辆车各一条 TCP 连接
- **车与车之间不经黑板互联**：覆盖任务里每辆车只需"从平板拿自己的 assignment + master IP"和"向平板报自己的位姿/进度"，无需车↔车
- **平板主动连车**：平板知道车的 IP（来自 UDP 0x06 广播发现或用户填写），车 agent 不知道平板 IP

### 2.2 端口规划

| 端口 | 协议 | 方向 | 用途 |
|------|------|------|------|
| 5001 | UDP | 平板→紫派 / agent→紫派 | udp2lcm 控制指令 |
| 5002 | UDP | 紫派→agent | udp2lcm 心跳回报 |
| **5003** | **TCP** | **平板→agent** | **LAN 黑板（mission/robot JSON）** |
| 8000 | HTTP | 平板→紫派 | 地图文件拉取 |

新增常量 `FLEET_LAN_PORT = 5003`（`constants/protocol.ets`，app + car-agent 两份同步）。

---

## 3. 协议设计

### 3.1 线协议格式

每条消息 = **4 字节大端长度前缀 + UTF-8 JSON 负载**

```
┌─────────────────┬─────────────────────────────────┐
│  4 字节 (BE)    │         N 字节 (UTF-8)          │
│  长度 = N       │         JSON 负载               │
└─────────────────┴─────────────────────────────────┘
```

**必须自己按长度拆包**，处理粘包/半包（TCP 是流协议，不保证报文边界）。

### 3.2 消息类型

JSON 负载形如 `{ "t": <type>, ... }`，三种类型：

| t | 方向 | 负载 | 含义 |
|---|------|------|------|
| `"hello"` | 双向 | `{ t:"hello", session:"OpenHarmonyCarFleetV1", role:"pad"\|"car", carId? }` | 握手 + 校验 session（= `FLEET_SESSION_ID`）。不匹配则断开。 |
| `"mission"` | 平板 → 车 | `{ t:"mission", snapshot: MissionSnapshot }` | 平板下发**整块**任务黑板（phase/assignments/area/map.ref/frame/endPoints…规划真相）。 |
| `"robot"` | 车 → 平板 | `{ t:"robot", robot: RobotRuntimeDTO }` | 车回报**本车**运行态（index/ip/online/x/y/r/command/progress/status）。 |

### 3.3 连接建立流程

```
平板                              车 agent
  │                                 │
  │──── TCP connect ───────────────►│
  │                                 │ (accept)
  │◄──── TCP established ──────────│
  │                                 │
  │──── hello (role:"pad") ────────►│
  │◄──── hello (role:"car", id:1) ─│
  │                                 │
  │──── mission (当前快照) ─────────►│ (applySnapshot)
  │◄──── robot (本车状态) ──────────│ (dispatchRemoteMission)
  │                                 │
```

连接建立后：
- **平板**立刻发当前 `mission`
- **车**立刻发当前 `robot`
- 之后：`publishMission` 触发即发一条

### 3.4 合并规则（谁权威，避免互相覆盖）

**平板对规划字段权威；每辆车对自己的 robot 权威。**

- **车收到 `mission`** → `mission.applySnapshot(snapshot)` **整块覆盖**（平板是规划真相）→ 派发给监听者（`AgentCore.onBlackboard` → reconcile）
- **平板收到 `robot`** → **只把这条 `robot` 合并进 `this.mission.robots[index]`**（更新位姿/进度/状态/online），**保留**平板本地的 phase/assignments/area/map → 派发给监听者（`ControlPage.onRemoteMission` 刷新显示）

> 这样**多车也不互相覆盖**（旧 DDO 单一 missionJson 双向整覆盖会把别的车的位姿冲掉，是"最后写者胜"的 MVP 缺陷；本方案天然修掉）。

---

## 4. 代码实现

### 4.1 新增常量

`app-harmony/entry/src/main/ets/constants/protocol.ets` + `car-agent/entry/src/main/ets/constants/protocol.ets`：

```typescript
/** LAN Socket 黑板 TCP 端口（替代 distributedDataObject） */
export const FLEET_LAN_PORT: number = 5003;
```

### 4.2 平板端 FleetMissionService

`app-harmony/entry/src/main/ets/service/FleetMissionService.ets`：**整体重写**

**核心逻辑**：
- TCP 客户端：连到 `robots[].ip:5003`
- `publishMission(mission)`：遍历 `mission.robots`，对每个有 IP 的车确保连接 + 发 `{t:"mission", snapshot}`
- 收到 `{t:"robot"}` → 合并进 `this.mission.robots[index]` → 派发给监听者
- 断线重连：指数退避（1s → 2s → 4s → 8s → 16s → 30s max）
- 收到重连后立刻补发当前 mission

**关键方法**：
```typescript
private ensureConnection(ip: string): void;
private sendTo(ip: string, json: string): void;
private onReceive(conn: PadConnection, data: ArrayBuffer): void;
private processRecvBuffer(conn: PadConnection): void;  // 按长度前缀拆包
private handleMessage(ip: string, msg: FleetMsg): void;
private mergeRobot(robot: RobotRuntimeDTO): void;  // 只更新 robots[index]
```

**不再需要**：
- `distributedDeviceManager` / `distributedDataObject`
- `abilityAccessCtrl` / `requestPermissions`
- `bindDevice` / `unbindDevice` / `startDiscovering` / `listDevices`

### 4.3 车端 FleetMissionService

`car-agent/entry/src/main/ets/service/FleetMissionService.ets`：**整体重写**

**核心逻辑**：
- TCP 服务端：监听 `0.0.0.0:5003`
- 同一时刻只接受一个平板连接（多平板非目标，后连覆盖前连）
- `publishMission(mission)`：取 `mission.robots` 里**本车**那条（`index === carId`），发 `{t:"robot", robot}`
- 收到 `{t:"mission"}` → `applySnapshot` → 派发给监听者

**关键方法**：
```typescript
private startServer(): void;  // listen 0.0.0.0:5003
private onClientConnect(client: socket.TCPSocket): void;  // accept
private onReceive(conn: PadConnection, data: ArrayBuffer): void;
private processRecvBuffer(conn: PadConnection): void;  // 按长度前缀拆包
private handleMessage(msg: FleetMsg): void;
private sendRaw(conn: PadConnection, json: string): void;
```

**新增方法**：
```typescript
setSelfCarId(carId: number): void;  // AgentCore.start() 时调用
setLocalMission(mission: Mission): void;  // 初始化本地快照
```

### 4.4 AgentCore 适配

`car-agent/entry/src/main/ets/agent/AgentCore.ets`：

```typescript
start(): void {
  // ...
  const fleet = FleetMissionService.getInstance();
  fleet.init();  // 不再需要 context
  fleet.setSelfCarId(this.carId);  // 告知本车 ID
  fleet.setLocalMission(this.mission);
  fleet.subscribeMission(this.blackboardListener);
  fleet.joinSession(FLEET_SESSION_ID);  // 启动 TCP 服务端监听
  // ...
}
```

### 4.5 EntryAbility 适配

`app-harmony/entry/src/main/ets/entryability/EntryAbility.ets`：

```typescript
onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
  // ...
  FleetMissionService.getInstance().init();  // 不再传 context
  // ...
}
```

### 4.6 权限声明

两端 `module.json5`：

```json5
"requestPermissions": [
  {
    "name": "ohos.permission.INTERNET"
  }
  // DISTRIBUTED_DATASYNC 已移除
]
```

### 4.7 停用互信相关页面

- `app-harmony/.../pages/HomePage.ets`：注释掉设备互信按钮（DeviceTrustPage 入口）
- `car-agent/.../pages/AgentStatusPage.ets`：重写为纯状态页，移除配对入口

**文件保留备查**：
- `app-harmony/.../pages/DeviceTrustPage.ets`（废弃，不再路由跳转）
- `car-agent/.../pairingability/PairingAbility.ets`（废弃，不再声明）
- `car-agent/.../pages/PairingPage.ets`（废弃，不再路由跳转）

### 4.8 注释同步

`model/mission.ets`（两端同步副本）：
- 注释从 "DDO / distributedDataObject" 更新为 "LAN TCP Socket"
- `applySnapshot` 注释从 "DDO change 回调" 更新为 "LAN 黑板收到 mission 消息时调用"

---

## 5. 连接管理与可靠性

### 5.1 平板端重连

- 每辆车一条连接；断开→**指数退避重连**（1s → 2s → 4s → 8s → 16s → 30s max）
- 重连成功后立刻补发当前 mission
- 车可能重启/WiFi 抖动，平板负责主动恢复

### 5.2 车端容忍

- accept 平板连接；连接断开容忍（平板会重连）
- 同一时刻通常仅一个平板连接（多平板非目标，后连覆盖前连即可）

### 5.3 拆包容错

- 用 **4 字节长度前缀**严格拆包；JSON 解析整段 `try/catch`，坏包丢弃不崩
- 处理粘包（两条消息拼一个 buffer）和半包（一条拆两个 buffer）

### 5.4 日志

- 连接建立/断开、收发 mission/robot 的条数与关键字段
- 沿用 `Log.scoped('FleetMission')`，DOMAIN 0xD002
- 便于真机判据

---

## 6. 测试与验收

### 6.1 离线测试（Node 镜像）

`tools/verify/verify.mjs` 新增 §6 "LAN 黑板 length-prefix framing"（10 项测试）：

```bash
node tools/verify/verify.mjs
```

**测试覆盖**：
- frame encode：长度 = 4 + payload
- frame decode：还原单条消息 / 无剩余字节
- 粘包：两条消息拼一个 buffer → 拆出两条
- 半包：只给前 6 字节 → 不产出消息 / 剩余 = 输入长度
- 半包补全：剩余 + 后续数据 → 还原消息 / 无剩余
- 长度前缀大端验证

**结果**：64/64 全绿（含 10 项新增 LAN framing 测试）

### 6.2 真机测试步骤

**前提**：平板 + 紫派连同一 WiFi，无客户端隔离。

**步骤**：
1. 紫派启动 udp2lcm + 安装并启动 car-agent app
2. 平板安装并启动 app-harmony
3. 平板进分布式模式，UDP 0x06 广播发现车（有车 IP）
4. 平板划覆盖区域

**预期日志**：

**紫派 agent**（hdc shell hilog | grep FleetMission）：
```
LAN 黑板就绪 OpenHarmonyCarFleetV1（TCP 服务端 :5003，等待平板连入）
TCP 服务端已启动: 0.0.0.0:5003
平板已连入
hello 握手成功: role=pad
收到 mission: phase=covering assignments=1
黑板更新: phase=covering 车1 区域=有 目标点=无 → 产 3 条本机命令
TX→ 127.0.0.1:5001 cmd=107 (0x6b) ...
```

**平板**（hdc shell hilog | grep FleetMission）：
```
连接车 agent: 192.168.43.2:5003
TCP 连接成功: 192.168.43.2
hello 握手成功: 192.168.43.2 role=car carId=1
合并 robot: index=1 pos=(120,340) status=covering
```

**验收判据**：
- 紫派 agent 日志出现 `黑板更新: phase=covering … → 产 N 条命令` + `TX→ 127.0.0.1 …`
- 平板出现 toast「车N：agent 已下发覆盖/执行命令」
- 地图上车**变色 + 随心跳移动**（agent 经 LAN 回写 robot）
- 断网/重启车 → 平板自动重连、黑板恢复
- **全程不需要 bindTarget/互信/同账号/系统权限**

---

## 7. 风险与注意

### 7.1 网络前提

**同一 WiFi 且无客户端隔离**仍是前提（socket 也要设备互通）。但这是普通 TCP，比 DDO 宽松得多：
- UDP 既然通它就通（同样三方应用、同样 WiFi）
- 不需要设备间 P2P 直连，走 AP 转发即可
- 不需要蓝牙/超级终端/华为账号

### 7.2 多车场景

- 平板对 N 辆车各一条连接
- `mission` 广播给全部，`robot` 各车只报自己
- 天然避免互相覆盖（平板合并时只更新对应 index）

### 7.3 master IP

- master IP 仍由平板写进 `mission.map.ref`
- 子车 agent 据此发 cmd105 拉图（方案B，不变）

### 7.4 传输层替换

- 这是**传输层替换**，协同语义（reconcile/cmd 时序/方案B）一行不动
- 紫派侧无需任何改动（黑板本就在 App/agent 之间）

### 7.5 可逆性

- 保留 `FleetMissionService` 的"传输无关"边界
- 将来若真机环境允许，可再加一个 DDO 实现切回
- 但**当前以 LAN socket 为唯一可用实现**

---

## 8. 文件改动清单

### 8.1 核心改动

| 文件 | 改动 |
|------|------|
| `app-harmony/.../constants/protocol.ets` | 新增 `FLEET_LAN_PORT = 5003` |
| `car-agent/.../constants/protocol.ets` | 新增 `FLEET_LAN_PORT = 5003` |
| `app-harmony/.../service/FleetMissionService.ets` | **整体重写**：DDO → TCP 客户端 |
| `car-agent/.../service/FleetMissionService.ets` | **整体重写**：DDO → TCP 服务端 |
| `car-agent/.../agent/AgentCore.ets` | `fleet.setSelfCarId(carId)` + `setLocalMission()` |
| `app-harmony/.../entryability/EntryAbility.ets` | `init()` 无参 |
| `app-harmony/.../module.json5` | 移除 `DISTRIBUTED_DATASYNC` 权限 |
| `car-agent/.../module.json5` | 移除 `DISTRIBUTED_DATASYNC` 权限 |
| `app-harmony/.../pages/HomePage.ets` | 停用设备互信按钮 |
| `car-agent/.../pages/AgentStatusPage.ets` | 重写为纯状态页，移除配对入口 |
| `app-harmony/.../model/mission.ets` | 注释同步（DDO → LAN TCP Socket） |
| `car-agent/.../model/mission.ets` | 注释同步（DDO → LAN TCP Socket） |
| `tools/verify/verify.mjs` | 新增 §6 LAN framing 测试（10 项） |

### 8.2 文档更新

| 文件 | 改动 |
|------|------|
| `docs/distributed-bringup-status.md` | 完整演进记录（DDO 失败 → LAN 方案） |
| `docs/lan-blackboard-plan.md` | 设计方案（已存在） |
| `docs/lan-blackboard-impl.md` | **本文档**（实施详细） |
| `docs/agent-lan-adaptation.md` | 成员A 适配指南 |

### 8.3 废弃文件（保留备查）

- `app-harmony/.../pages/DeviceTrustPage.ets`（不再路由跳转）
- `car-agent/.../pairingability/PairingAbility.ets`（不再声明）
- `car-agent/.../pages/PairingPage.ets`（不再路由跳转）

---

## 9. 下一步

### 9.1 真机验证

按 §6.2 步骤在真机验证 LAN 黑板连通性。

### 9.2 成员A 适配

成员A 阅读 [`agent-lan-adaptation.md`](agent-lan-adaptation.md)，在紫派上部署 car-agent app。

### 9.3 后续优化（可选）

- **应用级心跳**：每 ~2s 一条空 `robot`/`mission` 或 `ping` 做保活 + 死链检测（MVP 可先靠 TCP + 重连）
- **断线重连 UI 反馈**：平板显示"正在重连车N..."
- **DDO 回退方案**：若将来真机环境允许（蓝牙/超级终端/账号全满足），可再加一个 DDO 实现切回

---

## 10. 参考资料

- [`lan-blackboard-plan.md`](lan-blackboard-plan.md)：设计方案（实施前阅读）
- [`distributed-bringup-status.md`](distributed-bringup-status.md)：DDO 失败根因分析
- [`multi-robot-collab.md`](../contracts/multi-robot-collab.md)：多机协同契约（FleetMission 黑板语义）
- [`app-purplepi-alignment-audit.md`](../contracts/app-purplepi-alignment-audit.md)：App↔紫派接口对齐审计
- [OpenHarmony Socket API](https://developer.huawei.com/consumer/cn/doc/harmonyos-references-V5/js-apis-net-socket-V5)：`@ohos.net.socket` 官方文档

---

**文档结束**
