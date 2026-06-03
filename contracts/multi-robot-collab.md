# 多机协同契约 · 软总线 + 多车覆盖

**版本 v0.2（2026-06-03）** — App 侧（owner）定方案；紫派侧（成员A）需确认标 **⚠️** 项。
本契约取代旧实现"平板 `startAbility` 拉起车上 App + `Want` 一次性快照 + UDP burst"的别扭机制。

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

## 流程

1. 平板令 master 建图（UDP `cmd 0` 起 … `cmd 2` 结束建图）。
2. **master agent**：建好图 → 写 `map.ref` / `area`，`phase = dividing`。
3. **平板（规划器）**：在共享地图上划子区域 → 写 `assignments`，`phase = covering`。（交互在平板，符合"平板做划分、交互性好"）
4. **每台子车 agent**：看到自己的 `assignments[carId]` → 本机 UDP 下发"对角点1 `cmd107` + 对角点2&robot_id `cmd108`"，
   紫派据矩形规划 FullRoad 覆盖(**LCM 122**) + 分布式跟踪(**123**)；子机另经 `cmd105` 拉主机地图。
5. 各车覆盖中，心跳 `x/y/r` → agent 写回 `robots[carId].pose/progress`；平板实时渲染所有车的位置/朝向/进度。

## 定位约定（关键）：子机从 master 起点出发

地图坐标系原点锚在 **master 建图起始位姿**。**子车从 master 的同一物理起点、同一朝向出发** → 它在地图坐标系里的初始位姿**按构造 = (0, 0, 0°)**。

- 因此**无需全局重定位、无需操作员点选初始位姿、大概率无需新增"注入位姿"命令**——子机加载地图后位姿默认归零到原点即可（⚠️ 紫派确认）。
- 由此 [`map-format.md`](map-format.md) 里"子机与主机坐标系是否对齐"那条 ⚠️ **按构造消解**。
- 注意：**朝向比位置敏感**（位置差几厘米无妨，起始朝向差几度会随距离累积放大）。起点要把**位置 + 朝向都标死**（卡位/线），且**顺序出发**（master 离开起点后子车再占位）。
- 后续可叠加激光重定位（AMCL/scan-match）放宽"必须从原点出发"的约束，作为增强。

## 地图传输：两种，二选一

| 方案 | 机制 | 优点 | 代价 |
|---|---|---|---|
| **A · 走软总线**（推荐做 demo 亮点） | master agent 把 `map.text` 写入 `FleetMission` → 自动同步到各子车 agent → agent 落地为本机地图文件（紫派机器人栈读取的 `/data/test/defultMap.txt`）供规划 | "地图经软总线共享"可见、自洽、不依赖车间网络 | agent 要能把地图写进紫派栈读取的路径/方式（⚠️ 与紫派定）；~3MB 上总线（LAN 可接受） |
| **B · 走车间** | 子机 UDP `cmd105`（主机 IP 在 byte[1,2,4,6]）→ LCM `124` 直接拉图 | 复用现成、轻 | 地图不"经软总线"；软总线只载协调态 |

无论 A/B，**协调态（`area`/`assignments`/`robots`/进度）都走软总线**。

## 坐标系元数据（⚠️ 紫派定义，回写 `map-format.md`）

`frame` 三要素必须定死，App 才能正确渲染、并把平板点选的子区域顶点换算回 master 坐标系下发：

- **原点定义**（地图 (0,0) 对应现实何处）。
- **分辨率**（每格 = ? 米/毫米）。
- **0°方向与朝向单位**（`r` 用度还是弧度、零位指向）。

## 各端职责

- **平板 App（owner）**：UI、触发建图、区域划分、`FleetMission` 读写与可视化、把子区域顶点用 master 坐标系下发。
- **紫派 agent（ArkTS，需新增/集成）**：入软总线、`FleetMission` ↔ 本机 UDP 桥接、（方案A）落地地图文件、回报位姿/进度。
- **紫派机器人栈（成员A）**：SLAM/建图、覆盖规划（单机 `127/125`；**分布式矩形 `122/123`，触发自 `cmd107/108`**）、（方案B）车间拉图（`cmd124`）、子机加载图后位姿归零到原点。

## ⚠️ 待确认（找成员A）

1. **坐标系**：单位 / 原点 / 0°方向（即 `map-format.md` 三条 ⚠️）。
2. **子机"从原点出发"**：加载地图后机器人栈是否把位姿初始化到地图原点？若否，如何注入初始位姿（可能加一条命令 = 协议变更，回写 `udp-protocol.md`）。
3. **地图传输选 A 还是 B**；若 A，agent 用什么方式把地图喂给机器人栈（建议写紫派栈已读取的地图文件路径）。
4. **紫派能否常驻一个 ArkTS agent(hap)**，与机器人栈靠 localhost UDP 桥接。
5. ✅ **子区域 = 对角矩形**（2 点 + robot_id），经 `cmd107/108` 下发，触发 FullRoad 覆盖(122)+跟踪(123)（已对账）。
   剩余：子区域是否**只能矩形**、能否**多块/不规则**；以及"对角点1/2"的下发时序约束。
