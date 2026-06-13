# 多机协同契约 · LAN 黑板 + 多车覆盖

**版本 v0.6（2026-06-13）** — App 侧（owner）定方案；多机覆盖主语义从“车端按一个矩形拆分 `roadFile.txt`”调整为“平板为每台车分配不同的多点队列，车端逐点执行并协同避障”。
本契约取代旧实现"平板 `startAbility` 拉起车上 App + `Want` 一次性快照 + UDP burst"的别扭机制。

> **v0.3**：App 侧落地 FleetMission 黑板模型与服务（`app-harmony/.../model/mission.ets` +
> `service/FleetMissionService.ets`），下方"共享对象"补「App 实现的具体 JSON 形状」；
> App 的 `RobotRunAbility`（旧跨端拉起目标）已删除（见 §核心原则）。
> **v0.4**：据成员A《接口功能与对接问题说明.md》+ 源码（`origin/purplepi-control`）收口所有 ⚠️——
> 坐标系/原点/0°、子机 `cmd5` 归零、地图传输=**方案B**、子区域=单矩形、agent 可行（见各节与 §已确认）；
> 新增 §双车协同避障（A commit `59fc335`，**不影响 App↔紫派 UDP 协议**）。
> **v0.5**：据 [`docs/lan-blackboard-plan.md`](../docs/lan-blackboard-plan.md) / [`docs/lan-blackboard-impl.md`](../docs/lan-blackboard-impl.md) / [`docs/agent-lan-adaptation.md`](../docs/agent-lan-adaptation.md)，DDO/设备互信路线停用；`FleetMission` 通过平板↔车 agent 的 length-prefixed JSON over TCP 传输。
> **v0.6**：据 plan9 新要求，平板不再只下发矩形子区域，而是为不同车下发不同的多个选点队列；`cmd107/108` 矩形分布式覆盖保留为旧兼容路径，新的主路径复用普通目标点导航与 `COOP_AVOID` 协同避障。

> 机器人级命令（105/107/108 等）已与紫派 `udp2lcm.c` 对账，`107/108` 的子区域机制是旧兼容路径的“对角矩形”，见 [`udp-protocol-crosscheck.md`](udp-protocol-crosscheck.md)。新多点队列不强行塞进 `107/108`。

## 场景

master 车扫描一片区域建图；完成后，其它车**从 master 的同一起点出发**并加载同一张地图。平板在地图上完成任务规划，为每台车分配不同的多个选点队列；每台车按自己的队列逐点导航，运行态和进度回传给平板。当前协同黑板用 **LAN TCP Socket** 承载，地图大文件仍走车间 HTTP/wget，不走 HarmonyOS 分布式软总线。

## 拓扑与角色

| 节点 | 设备 | 跑什么 | 职责 |
|---|---|---|---|
| 平板 | HarmonyOS 平板 | 全 UI App | 总控/规划/可视化：触发建图、划分区域、监看所有车 |
| 每台车 | 紫派 OH 5.0 | ① 机器人栈(C/C++/Python/LCM)　② 轻量 agent(ArkTS) | agent 监听 LAN TCP `:5003` + 把任务翻译成本机 UDP；机器人栈做 SLAM/导航/覆盖 |

- **LAN 黑板网络 = 平板 + 所有车的 agent**。平板主动连接每辆车的 `TCP :5003`；不需要 `bindTarget`、设备互信、同账号或 `distributedDataObject`。
- agent ↔ 本机机器人栈：走**现有 9 字节 UDP 命令协议**（[`udp-protocol.md`](udp-protocol.md)），localhost。
- agent 形态：当前可用 UIAbility 外壳启动 `com.example.carapp/AgentAbility`；后续可升级 ServiceExtensionAbility + resident 常驻。
- 紫派需把 agent(hap) 装到板上并启动，确认 `0.0.0.0:5003 LISTEN`。

## 核心原则：共享黑板，不是远程拉起

旧做法把 `startAbility` 当 RPC：每次操作**拉起** Ability、用 `Want` 塞一次性快照、页面 `aboutToAppear` 喷一串 UDP 后销毁——无实时态、Ability 生命周期被当 RPC 滥用。

**改为**：平板与每台车 agent 通过 LAN TCP `:5003` 维持一块逻辑上的"任务黑板"`FleetMission`；协同 = **平板发布 mission + 各车回报 robot + 各自响应**。没有远程拉起，也不依赖 DDO。

> App 侧已落地：删除 `RobotRunAbility`（旧 startAbility 目标）与 `DeviceCollabService.startRemoteControl`；
> `FleetMissionService` 仅做"UDP 发现车 IP + LAN 连接 + 黑板读写(`publishMission`/`subscribeMission`)"。
> 各车 agent 用同一 `sessionId` 做 hello 握手，不被远程拉起。

## 共享对象 `FleetMission`（LAN TCP JSON）

```
FleetMission {
  phase:        'idle' | 'scanning' | 'dividing' | 'covering' | 'done'
  frame:        { originX, originY, resolution, headingZero }   // 坐标系元数据，⚠️ 紫派定义
  map:          { ref, text? }              // ref=master 地图来源；text 可选，见"地图传输"
  area:         master 扫出的大区域边界（master 坐标系）
  routes:       { [carId]: { robotId, points:[{x,y}], cursor, status } } // 新主路径：平板为每车分配多点队列
  assignments:  { [carId]: { corner1:{x,y}, corner2:{x,y}, robotId } }  // 旧兼容：107/108 对角矩形
  robots:       { [carId]: { networkId, online, pose:{x,y,r}, progress, status } }
}
```

- 平板对规划字段权威；每辆车只对自己的 `robot` 运行态权威。
- 平板向各车发送 `{t:"mission", snapshot: MissionSnapshot}`；车 agent 收到后整块覆盖本地任务并响应。
- 车 agent 向平板发送 `{t:"robot", robot: RobotRuntimeDTO}`；平板只合并对应 `robots[index]`，保留本地规划字段。

**App 实现的具体 JSON 形状**（`MissionSnapshot`，各车 agent 反序列化同一份）：LAN 线协议为 4 字节大端长度前缀 + UTF-8 JSON。上面的 carId-keyed map 在 App 里用**带 id 的数组**表达（`robots[].index`、`assignments[].carId` 即 carId），`pose` **拍平**为 `x/y/r`，另带 App 内部字段 `ip/command`（agent 可忽略）；`phase/status` 为上列枚举字符串。即：

```
phase:       'idle'|'scanning'|'dividing'|'covering'|'done'
mode:        'astar'|'fullpath'|'distributed'        // App 控制模式（单机字段，agent 忽略）
mapReady:    boolean
frame:       { originX, originY, resolution, headingZero }
map:         { ref, text? }
area:        { corner1:{x,y}, corner2:{x,y} } | null
endPoints:   [{ x, y }]                               // 单机选点（agent 忽略）
routes:      [{ carId, robotId, points:[{x,y}], cursor, status }]  // 多机覆盖主路径
assignments: [{ carId, corner1:{x,y}, corner2:{x,y}, robotId }]    // 旧矩形兼容
robots:      [{ index, ip, networkId, online, x, y, r, command, progress, status }]
```

（对应 `app-harmony/entry/src/main/ets/model/mission.ets` 的 `MissionSnapshot`/`RobotRuntimeDTO`/
`AssignmentDTO`/`Rect`。若 App 侧新增 `routes` DTO，应同步本契约；LAN 只替换传输层，不改变字段语义。）

## 流程

1. 平板令 master 建图（UDP `cmd 0` 起 … `cmd 2` 结束建图）。
2. **master agent**：建好图 → 写 `map.ref` / `area`，`phase = dividing`。
3. **平板（规划器）**：在共享地图上为每台车生成不同的 `routes[carId].points[]`，`phase = covering`。平板负责全局任务拆分，车端不再根据一个矩形自行推断两车路线。
4. **每台车 agent 或平板直连控制器**：master 看到自己的 route 后直接逐点下发普通目标点；sub 先经 `cmd105` 拉主机地图，确认本机地图可用后下发 `cmd5` 归零加载，再逐点下发普通目标点。当前可复用 UDP `cmd3 -> ROBOT_CONTROL 20`，到点/失败后再下一个点。
5. 各车覆盖中，心跳 `x/y/r` → agent 或平板写回 `robots[carId].pose/progress`；平板实时渲染所有车的位置/朝向/进度。两车交汇导致短暂停车时，状态应标注为 avoiding/paused，而不是直接判故障。

旧 `assignments + cmd107/108` 矩形流程仍可用于兼容旧 App：`cmd107 -> cmd108(robot_id=0/1)` 触发紫派矩形覆盖规划(**LCM 122**) + 分布式跟踪(**123**)，但不再作为新的多机覆盖主设计。

## 定位约定（关键）：子机从 master 起点出发

地图坐标系原点锚在 **master 建图起始位姿**。**子车从 master 的同一物理起点、同一朝向出发** → 它在地图坐标系里的初始位姿**按构造 = (0, 0, 0°)**。

- 因此**无需全局重定位、无需操作员点选初始位姿、无需新增"注入位姿"命令**——✅ A确认：子机用 UDP **`cmd 5`** 加载地图时，紫派把初始位姿**强制设为 (0,0,0)**（给 LCM 10 传 `dparams[4..6]=0,0,0`）。
  ⚠️ 但 `cmd 2 / 'j'/106` 的加载地图逻辑**沿用当前心跳位姿、不归零**。`cmd108/'l'` 不再加载地图或重置定位，只生成并启动覆盖路径——故子机入场务必在 `cmd108` 前走 **`cmd 5`**。
- 由此 [`map-format.md`](map-format.md) 里"子机与主机坐标系是否对齐"那条 ⚠️ **按构造消解**。
- 注意：**朝向比位置敏感**（位置差几厘米无妨，起始朝向差几度会随距离累积放大）。起点要把**位置 + 朝向都标死**（卡位/线），且**顺序出发**（master 离开起点后子车再占位）。
- 后续可叠加激光重定位（AMCL/scan-match）放宽"必须从原点出发"的约束，作为增强。

## 地图传输：本轮主流程为方案 B

| 方案 | 机制 | 优点 | 代价 |
|---|---|---|---|
| **B · 走车间（本轮采用）** | 子机 UDP `cmd105`（主机 IP 在 byte[1,2,4,6]）→ LCM `124` 直接拉图 | 复用现成、与紫派 C/C++ 栈已实现能力一致 | 地图不经 LAN 黑板；LAN 只载协调态 |
| **A · 走 DDO/软总线（历史方案，不采纳）** | master agent 把 `map.text` 写入 `FleetMission` → 同步到各子车 agent → agent 落地为本机地图文件 | 可展示“地图经共享对象同步” | DDO 真机不可用，且大文本会挤占协调态通道 |

✅ **A确认现状 = 方案 B**：紫派代码已实现子机 `cmd105/'i'`→`cmd124` 用 `wget` 从主机拉地图，2026-06-13 后优先拉 **`zipedMap.txt`** 并在本机解压生成 `defultMap.txt`，压缩图不可用时回退 **`defultMap.txt`**。`roadFile.txt` 仍会拉取，但只服务旧 `107/108` 矩形兼容路径。URL 无 `/data/test` 前缀；LAN 黑板不承载地图大文件。

协调态（`area`/`assignments`/`robots`/进度）走 LAN TCP `:5003` 黑板。

## 坐标系元数据（✅ 成员A确认，回写 `map-format.md`）

`frame` 三要素（App 据此渲染、并把平板点选的子区域顶点换算回 master 坐标系下发）：

- **原点** = 紫派**建图/定位初始位姿**；多机时即 master 起点（子机 `cmd5` 归零到此）。
- **分辨率** = **0.05 m/格**（5cm，UDP 整数坐标与格 1:1）。
- **0°方向与单位**：`r` 用**度**，范围 **[-180,180]**；`theta=0` 指向 **+X**、正角 **CCW** 朝 +Y。

## 各端职责

- **平板 App（owner）**：UI、触发建图、地图显示、每车多点队列规划、LAN `FleetMission` 读写与可视化，或在当前直连方案中直接向每辆车 `:5001` 下发目标点。
- **紫派 agent（ArkTS，若启用）**：监听 TCP `0.0.0.0:5003`，做 `FleetMission.routes` ↔ 本机 UDP `5002 -> 5001` 桥接、按方案 B 在 sub 覆盖前触发 `cmd105` 拉图并等待可用、逐点下发 `cmd3`、回报位姿/进度。
- **紫派机器人栈（成员A）**：SLAM/建图、普通目标点导航、旧矩形覆盖兼容（单机 `127/125`；**分布式矩形 `122/123`，触发自 `cmd107/108`**）、**方案B 车间拉图（`cmd124` 优先拉 `zipedMap.txt`，失败回退 `defultMap.txt`）**、子机 `cmd5` 加载图后位姿归零到原点、**双车协同避障（`COOP_AVOID` LCM，见下）**。

## 双车协同避障（A commit `59fc335`；⚠️ App 无需改协议）

多点队列或旧矩形覆盖执行时，两车路线可能交叠。**紫派 `Navi` 内新增独立避障通道**，与 App / LAN 黑板 / UDP **解耦**：

- 通道 = **多播 LCM** `udpm://239.255.76.67:7668`，频道 `COOP_AVOID`，类型 `robot_control_t`，**负命令号 -40..-35**
  （坐标请求/响应、停机请求/确认、恢复请求/确认）。**车↔车直连**，不经平板、不经 LAN 黑板。
- A 明确：**未改 `NewWheelCtrl/udp2lcm`** → **App↔紫派 9 字节 UDP 协议保持不变**，App 端**无需新增/改动命令**。
- 触发：A* 失败/无路径/DWA 不可达/图匹配跳变 → 协同诊断；若对方位置或预测走廊在安全半径内则判为阻挡。
- 行为：被让车**保存当前目标 → 发 `PATH v=0,w=0` 停车等待 → 恢复时重下目标**。两车互请停机时 **`robotId=1` 继续、`robotId=0` 停等**。

**对 App 的影响（仅语义/可视化，非协议）**：
- 覆盖中某车可能**自主暂停**数秒（避让）——平板可视化**不可**把"暂停"误判为"卡死/掉线"。
- 但**当前 9 字节心跳无"暂停/避让"标志位**（byte1-2 保留=0），App 仅凭心跳**无法区分**暂停与缓行。
  → 列为开放问题（见 [`integration-qa.md`](integration-qa.md)）：是否在心跳留一字节状态码，或经 LAN `robot.status` 由 agent 标注。
- 避障依赖**车间多播网络连通**（与方案B 同前提）。

## 已确认（成员A 2026-06-05，《接口功能与对接问题说明.md》+ 源码）

1. ✅ **坐标系**：单位 5cm/格、原点=建图初始位姿、`r`=度 [-180,180]、0°=+X CCW（见 §坐标系元数据）。
2. ✅ **子机从原点出发**：用 **`cmd 5`** 加载图即归零 (0,0,0)；无需新增"注入位姿"命令（见 §定位约定）。
3. ✅ **地图传输 = 方案 B**（`cmd124` 优先 wget 拉 `zipedMap.txt` 并解压，失败回退 `defultMap.txt`；`roadFile.txt` 仅旧矩形兼容使用）；DDO/软总线传图方案不采纳。
4. ✅ **紫派 agent 可行**：C/C++ 栈无需改造即可被 agent 经 localhost UDP 桥接；LAN 黑板要求 agent 监听 TCP `5003`，常驻属 HAP 部署问题（A：现有代码未含 agent，但接口边界已足够）。
5. ✅ **新多机覆盖主语义 = 平板多点队列**：平板为每台车下发不同 `routes[].points[]`，车端复用普通目标点导航逐点执行；`COOP_AVOID` 处理车间交汇。
6. ✅ **旧矩形兼容路径**：`cmd107/108` 子区域仍是单个轴对齐矩形（2 对角点 + robot_id，要求 `x1≠x2 ∧ y1≠y2`）；
   **下发时序**：先 `cmd107`(对角点1) 后 `cmd108`(对角点2+robot_id)。`robot_id=0/1` 都会生成 `122` 覆盖路径并执行 `123`；对角点不完整或 `robot_id` 非法会拒绝本次 `108`。

> **仍开放**的对接问题（App→紫派，写在 [`integration-qa.md`](integration-qa.md) 供双方 AI 异步读）：
> ① 协同避障"暂停"状态如何让 App 可见；② >2 车的避障 / tie-break；③ `roadFile.txt` 是否需 App 关心。
