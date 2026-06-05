# 多机协同契约 · 软总线 + 多车覆盖

**版本 v0.4（2026-06-05）** — App 侧（owner）定方案；标 **⚠️** 项原待成员A确认，**本版已据 A 文档全部收口**。
本契约取代旧实现"平板 `startAbility` 拉起车上 App + `Want` 一次性快照 + UDP burst"的别扭机制。

> **v0.3**：App 侧落地 FleetMission 黑板模型与服务（`app-harmony/.../model/mission.ets` +
> `service/FleetMissionService.ets`），下方"共享对象"补「App 实现的具体 JSON 形状」；
> App 的 `RobotRunAbility`（旧跨端拉起目标）已删除（见 §核心原则）。
> **v0.4**：据成员A《接口功能与对接问题说明.md》+ 源码（`origin/purplepi-control`）收口所有 ⚠️——
> 坐标系/原点/0°、子机 `cmd5` 归零、地图传输=**方案B**、子区域=单矩形、agent 可行（见各节与 §已确认）；
> 新增 §双车协同避障（A commit `59fc335`，**不影响 App↔紫派 UDP 协议**）。

> 机器人级命令（105/107/108 等）已与紫派 `udp2lcm.c` 对账，子区域机制据此**已更正为"对角矩形"**，见 [`udp-protocol-crosscheck.md`](udp-protocol-crosscheck.md)。

## 场景

master 车扫描一片区域建图；完成后，其它车**从 master 的同一起点出发**，各自领一块子区域做覆盖遍历（全路径）。
需要在多车间共享地图与任务划分，且**用 HarmonyOS 分布式软总线**承载协同。

## 拓扑与角色

| 节点 | 设备 | 跑什么 | 职责 |
|---|---|---|---|
| 平板 | HarmonyOS 平板 | 全 UI App | 总控/规划/可视化：触发建图、划分区域、监看所有车 |
| 每台车 | 紫派 OH 5.0 | ① 机器人栈(C/C++/Python/LCM)　② 轻量**无界面 agent**(ArkTS) | agent 入软总线 + 把任务翻译成本机 UDP；机器人栈做 SLAM/导航/覆盖 |

- **软总线网络 = 平板 + 所有车的 agent**（同一可信环，distributedDataObject 同一 `sessionId`）。
- agent ↔ 本机机器人栈：走**现有 9 字节 UDP 命令协议**（[`udp-protocol.md`](udp-protocol.md)），localhost。
- agent 形态：无界面常驻代理（优先 `ServiceExtensionAbility`；若三方权限受限则用"保持后台的 UIAbility"，由可用能力定）。
- ⚠️ 紫派需把 agent(hap) 装到板上并随系统常驻（旧做法是装**整个 CARApp** 当二传手；新做法只装**无界面 agent**）。

## 核心原则：共享黑板，不是远程拉起

旧做法把 `startAbility` 当 RPC：每次操作**拉起** Ability、用 `Want` 塞一次性快照、页面 `aboutToAppear` 喷一串 UDP 后销毁——无实时态、Ability 生命周期被当 RPC 滥用。

**改为**：所有节点**常驻**加入同一 distributedDataObject 会话，共享一块"任务黑板"`FleetMission`；协同 = **反应式读写共享态**。没有"拉起"，只有"共享 + 各自响应"。

> App 侧已落地：删除 `RobotRunAbility`（旧 startAbility 目标）与 `DeviceCollabService.startRemoteControl`；
> `FleetMissionService` 仅做"设备发现 + 入会话(`joinSession`) + 黑板读写(`publishMission`/`subscribeMission`)"。
> 各车 agent（紫派常驻，⚠️ 待 A）用同一 `sessionId` 常驻入会，不被远程拉起。

## 共享对象 `FleetMission`（distributedDataObject）

```
FleetMission {
  phase:        'idle' | 'scanning' | 'dividing' | 'covering' | 'done'
  frame:        { originX, originY, resolution, headingZero }   // 坐标系元数据，⚠️ 紫派定义
  map:          { ref, text? }              // ref=master 地图来源；text 可选，见"地图传输"
  area:         master 扫出的大区域边界（master 坐标系）
  assignments:  { [carId]: { corner1:{x,y}, corner2:{x,y}, robotId } }  // 子区域=对角矩形(master 坐标系)；robotId 0主/1从
  robots:       { [carId]: { networkId, online, pose:{x,y,r}, progress, status } }
}
```

- 平板与各 agent 都订阅整块对象；谁改谁广播，其余端 `onChange` 响应。
- App 侧用单一 JSON 字段（`MissionSnapshot`）承载整块，规避嵌套字段同步边界。

**App 实现的具体 JSON 形状**（`MissionSnapshot`，各车 agent 反序列化同一份）：整块经
distributedDataObject 单一字符串字段 `missionJson` 同步。上面的 carId-keyed map 在 App 里用**带 id 的数组**
表达（`robots[].index`、`assignments[].carId` 即 carId），`pose` **拍平**为 `x/y/r`，另带 App 内部字段
`ip/command`（agent 可忽略）；`phase/status` 为上列枚举字符串。即：

```
phase:       'idle'|'scanning'|'dividing'|'covering'|'done'
mode:        'astar'|'fullpath'|'distributed'        // App 控制模式（单机字段，agent 忽略）
mapReady:    boolean
frame:       { originX, originY, resolution, headingZero }
map:         { ref, text? }
area:        { corner1:{x,y}, corner2:{x,y} } | null
endPoints:   [{ x, y }]                               // 单机选点（agent 忽略）
assignments: [{ carId, corner1:{x,y}, corner2:{x,y}, robotId }]
robots:      [{ index, ip, networkId, online, x, y, r, command, progress, status }]
```

（对应 `app-harmony/entry/src/main/ets/model/mission.ets` 的 `MissionSnapshot`/`RobotRuntimeDTO`/
`AssignmentDTO`/`Rect`。改这些字段须同步本契约——CLAUDE.md：跨设备通信代码与 contracts/ 逐字段一致。）

## 流程

1. 平板令 master 建图（UDP `cmd 0` 起 … `cmd 2` 结束建图）。
2. **master agent**：建好图 → 写 `map.ref` / `area`，`phase = dividing`。
3. **平板（规划器）**：在共享地图上划子区域 → 写 `assignments`，`phase = covering`。（交互在平板，符合"平板做划分、交互性好"）
4. **每台子车 agent**：看到自己的 `assignments[carId]` → 本机 UDP 下发"对角点1 `cmd107` + 对角点2&robot_id `cmd108`"，
   紫派据矩形规划 FullRoad 覆盖(**LCM 122**) + 分布式跟踪(**123**)；子机另经 `cmd105` 拉主机地图。
5. 各车覆盖中，心跳 `x/y/r` → agent 写回 `robots[carId].pose/progress`；平板实时渲染所有车的位置/朝向/进度。

## 定位约定（关键）：子机从 master 起点出发

地图坐标系原点锚在 **master 建图起始位姿**。**子车从 master 的同一物理起点、同一朝向出发** → 它在地图坐标系里的初始位姿**按构造 = (0, 0, 0°)**。

- 因此**无需全局重定位、无需操作员点选初始位姿、无需新增"注入位姿"命令**——✅ A确认：子机用 UDP **`cmd 5`** 加载地图时，紫派把初始位姿**强制设为 (0,0,0)**（给 LCM 10 传 `dparams[4..6]=0,0,0`）。
  ⚠️ 但 `cmd 2 / 'j'/106 / 'l'/108` 的加载地图逻辑**沿用当前心跳位姿、不归零**——故子机入场务必走 **`cmd 5`**（或 agent 确保传 0,0,0）。
- 由此 [`map-format.md`](map-format.md) 里"子机与主机坐标系是否对齐"那条 ⚠️ **按构造消解**。
- 注意：**朝向比位置敏感**（位置差几厘米无妨，起始朝向差几度会随距离累积放大）。起点要把**位置 + 朝向都标死**（卡位/线），且**顺序出发**（master 离开起点后子车再占位）。
- 后续可叠加激光重定位（AMCL/scan-match）放宽"必须从原点出发"的约束，作为增强。

## 地图传输：两种，二选一

| 方案 | 机制 | 优点 | 代价 |
|---|---|---|---|
| **A · 走软总线**（推荐做 demo 亮点） | master agent 把 `map.text` 写入 `FleetMission` → 自动同步到各子车 agent → agent 落地为本机地图文件（紫派机器人栈读取的 `/data/test/defultMap.txt`）供规划 | "地图经软总线共享"可见、自洽、不依赖车间网络 | agent 要能把地图写进紫派栈读取的路径/方式（⚠️ 与紫派定）；~3MB 上总线（LAN 可接受） |
| **B · 走车间** | 子机 UDP `cmd105`（主机 IP 在 byte[1,2,4,6]）→ LCM `124` 直接拉图 | 复用现成、轻 | 地图不"经软总线"；软总线只载协调态 |

✅ **A确认现状 = 方案 B**：紫派代码已实现子机 `cmd105/'i'`→`cmd124` 用 `wget` 从主机拉 **`defultMap.txt` + `roadFile.txt`**
（`NaviInterface.cpp:4799-4818`，URL 无 `/data/test` 前缀）。**方案 A** 需新增紫派 ArkTS agent 的文件写入能力，留作后续增强（demo 亮点）。

无论 A/B，**协调态（`area`/`assignments`/`robots`/进度）都走软总线**。

## 坐标系元数据（✅ 成员A确认，回写 `map-format.md`）

`frame` 三要素（App 据此渲染、并把平板点选的子区域顶点换算回 master 坐标系下发）：

- **原点** = 紫派**建图/定位初始位姿**；多机时即 master 起点（子机 `cmd5` 归零到此）。
- **分辨率** = **0.05 m/格**（5cm，UDP 整数坐标与格 1:1）。
- **0°方向与单位**：`r` 用**度**，范围 **[-180,180]**；`theta=0` 指向 **+X**、正角 **CCW** 朝 +Y。

## 各端职责

- **平板 App（owner）**：UI、触发建图、区域划分、`FleetMission` 读写与可视化、把子区域顶点用 master 坐标系下发。
- **紫派 agent（ArkTS，需新增/集成）**：入软总线、`FleetMission` ↔ 本机 UDP 桥接、（方案A）落地地图文件、回报位姿/进度。
- **紫派机器人栈（成员A）**：SLAM/建图、覆盖规划（单机 `127/125`；**分布式矩形 `122/123`，触发自 `cmd107/108`**）、**方案B 车间拉图（`cmd124` 拉 `defultMap.txt`+`roadFile.txt`）**、子机 `cmd5` 加载图后位姿归零到原点、**双车协同避障（`COOP_AVOID` LCM，见下）**。

## 双车协同避障（A commit `59fc335`；⚠️ App 无需改协议）

覆盖遍历时两车路线可能交叠。**紫派 `Navi` 内新增独立避障通道**，与 App / 软总线 / UDP **解耦**：

- 通道 = **多播 LCM** `udpm://239.255.76.67:7668`，频道 `COOP_AVOID`，类型 `robot_control_t`，**负命令号 -40..-35**
  （坐标请求/响应、停机请求/确认、恢复请求/确认）。**车↔车直连**，不经平板、不经软总线。
- A 明确：**未改 `NewWheelCtrl/udp2lcm`** → **App↔紫派 9 字节 UDP 协议保持不变**，App 端**无需新增/改动命令**。
- 触发：A* 失败/无路径/DWA 不可达/图匹配跳变 → 协同诊断；若对方位置或预测走廊在安全半径内则判为阻挡。
- 行为：被让车**保存当前目标 → 发 `PATH v=0,w=0` 停车等待 → 恢复时重下目标**。两车互请停机时 **`robotId=1` 继续、`robotId=0` 停等**。

**对 App 的影响（仅语义/可视化，非协议）**：
- 覆盖中某车可能**自主暂停**数秒（避让）——平板可视化**不可**把"暂停"误判为"卡死/掉线"。
- 但**当前 9 字节心跳无"暂停/避让"标志位**（byte1-2 保留=0），App 仅凭心跳**无法区分**暂停与缓行。
  → 列为开放问题（见 [`integration-qa.md`](integration-qa.md)）：是否在心跳留一字节状态码，或经软总线由 agent 标注。
- 避障依赖**车间多播网络连通**（与方案B 同前提）。

## 已确认（成员A 2026-06-05，《接口功能与对接问题说明.md》+ 源码）

1. ✅ **坐标系**：单位 5cm/格、原点=建图初始位姿、`r`=度 [-180,180]、0°=+X CCW（见 §坐标系元数据）。
2. ✅ **子机从原点出发**：用 **`cmd 5`** 加载图即归零 (0,0,0)；无需新增"注入位姿"命令（见 §定位约定）。
3. ✅ **地图传输 = 方案 B**（`cmd124` wget 拉 `defultMap.txt`+`roadFile.txt`）；方案 A 待 agent 增强。
4. ✅ **紫派 agent 可行**：C/C++ 栈无需改造即可被 agent 经 localhost UDP 桥接；常驻属 HAP 部署问题（A：现有代码未含 agent，但接口边界已足够）。
5. ✅ **子区域 = 单个轴对齐矩形**（2 对角点 + robot_id，要求 `x1≠x2 ∧ y1≠y2`），**不支持多块/不规则**；
   **下发时序**：先 `cmd107`(对角点1) 后 `cmd108`(对角点2+robot_id)，否则从机无法生成 `122` 覆盖路径。

> **仍开放**的对接问题（App→紫派，写在 [`integration-qa.md`](integration-qa.md) 供双方 AI 异步读）：
> ① 协同避障"暂停"状态如何让 App 可见；② >2 车的避障 / tie-break；③ `roadFile.txt` 是否需 App 关心。
