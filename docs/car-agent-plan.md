# 车载轻 Agent 规划（car-agent）

> **新任务（2026-06-07 立项）**：开发**安装在车上 OpenHarmony（紫派 RK3566 / OH 5.0）的无界面轻 agent**。
> 方法：**先文档规划（本文件）→ 逐步设计 → 实现**。本文件是该任务的单一事实源，分阶段推进、每阶段收尾回写。
> 关联：[`app-refactor-plan.md`](app-refactor-plan.md) §分布式方案、[`../contracts/multi-robot-collab.md`](../contracts/multi-robot-collab.md)、
> 开放问题入 [`../contracts/integration-qa.md`](../contracts/integration-qa.md)。

## 0. 它是什么 / 为什么

**是什么**：跑在每台车紫派上的**无界面 ArkTS 节点**。加入鸿蒙分布式软总线，与平板（App）、其它车共持同一块
`FleetMission` 黑板（`@ohos.data.distributedDataObject`），并把黑板的协同决策**翻译成本机 9 字节 UDP 指令**下发给
紫派机器人栈（`udp2lcm`，localhost），同时把本机心跳位姿/进度**写回黑板**让平板可视化。

**为什么**（取代旧机制）：旧设想是"在每台车上装整个 CarApp + `startAbility` 跨端拉起 + `Want` 一次性快照 + UDP burst"，
脆且重。改为 **常驻无界面 agent + 反应式共享黑板**：平板只规划/总控/可视化，协同数据走软总线自动同步，agent 本地执行。
（见 app-refactor-plan §分布式方案"纠正旧机制"。）

**拓扑**：
```
   平板 App（规划/总控/可视化，hold FleetMission 黑板）
        │  软总线 distributedDataObject（反应式同步整块黑板）
   ┌────┴────┬───────────┐
 car1 agent  car2 agent  …        ← 本文件要做的"轻 agent"（每车一个，无界面）
   │ localhost 9字节UDP(5001)
 udp2lcm（A 的紫派栈）→ LCM → 轮控/SLAM/雷达
```

## 1. 职责（MVP 范围）

1. **入会 + 持黑板**：加入分布式会话、与平板/它车同步 `FleetMission`（`MissionSnapshot` JSON）；订阅变更。
2. **决策→本机指令**（读黑板 → 发 localhost UDP）：按本车 `index/carId` 取自己的 `Assignment`/目标：
   - 覆盖子区域 → `cmd107`(对角点1)+`cmd108`(对角点2,byte1=robotId)
   - 单点目标 → `cmd3`(startRoute, endX/endY) / 取消 `cmd4`
   - 加载图/位姿归零 → `cmd5`（子机从 master 起点出发，初始位姿构造为 0,0,0）
   - 子机拉主机图 → `cmd105`（主机 IP 打包 byte[1,2,4,6]）
3. **本机状态→黑板**（读 localhost 心跳 → 写黑板）：解析紫派 500ms 心跳（`state=3` 带 x/y/r）→ 更新本车
   `RobotRuntime.x/y/r/status`（进度 `progress` 暂由阶段/到点推算，细化见 §待定）→ 反映回黑板供平板看。
4. **保活**：对本机栈至少每 1s 发一帧（含中性帧），避免 `udp2lcm` 3s 未收指令急停。
5. **生命周期**：开机/按需起、软总线掉线重连、master/slave"同起点同朝向顺序出发"的位姿对齐编排。

**MVP 不做**（先简后繁）：多车 tie-break 仲裁、协同避障语义在 agent 侧的特殊处理（`COOP_AVOID` 不改 udp2lcm，
agent 仅按心跳状态如实回报）、地图方案A（黑板传整图）——先用方案B（`cmd124` wget）。

## 2. 形态与复用（关键设计取向）

- **形态**：独立瘦 hap，入口 = **无 Page 的 ServiceExtensionAbility / 常驻 Ability**（headless）。不含 `component/`、`pages/`。
- **最大复用 app-harmony 的 UI 无关层**——这是省力关键，app-harmony 的 `model/` + `service/` 本就无 UI 依赖：
  - `model/protocol.ets`（`encodeSend`/`decodeReceive`、命令枚举）——**逐字节同款，必须共享**。
  - `model/mission.ets`（`FleetMission`/`MissionSnapshot`/DTO）——黑板 schema，**两端反序列化同一 JSON，必须共享**。
  - `service/RobotTransport.ets`（UDP 收发 + 多目标保活）——agent 只对 **localhost 一台**。
  - `service/FleetMissionService.ets`（软总线 + distributedDataObject 黑板同步）——agent 复用其黑板部分。
- **复用方式（待 §3 定）**：① 抽成共享 **HAR**（`shared-core/`）被 app-harmony 与 car-agent 同时依赖（最干净，避免漂移）；
  ② 或暂时**精确复制**这几个文件并加"与 app-harmony 同源、改一处改两处"注释（快但易漂移）。**倾向①**，但需评估 DevEco HAR 配置成本。
- **代码位置**：新建顶层 `car-agent/`（ArkTS hap，sibling 于 `app-harmony/`）。**不放** `purplepi-control/`（A 的 C/C++）、**不放** `app-harmony/`（平板 UI）。部署在紫派。

## 3. 阶段计划（逐步设计 → 实现）

- **P0 规划**（本文件）✅ 2026-06-07。
- **P1 设计细化**：定 ① 复用策略（HAR vs 复制）+ 目录骨架；② **reconciler 状态机**（黑板 `phase/assignment` → 本机命令序列；
  幂等：黑板未变不重发、变了才发；到点/超时迁移）；③ localhost 传输配置（端口/与外部 App 直连路径的**互斥**，见 §4 风险）；
  ④ 黑板写回节流（心跳 500ms，写黑板别太频）；⑤ **先落地共享日志** `utils/log.ets`（[`logging-plan.md`](logging-plan.md) **L1**，
  App 与 agent 同用、属共享层基底）。产出：本文件「设计」节 + 必要时序图。
- **P2 脚手架**：`car-agent/` hap（`module.json5` 声明 ServiceExtensionAbility、权限：分布式 datasync / 软总线 / UDP；无 page）；
  接好共享 `model`/`service`；能起、入会、打印黑板。
- **P3 实现核心**：reconciler（黑板变更 → localhost UDP）+ 心跳回写（localhost 心跳 → 黑板 RobotRuntime）+ 保活
  + **agent 日志**（[`logging-plan.md`](logging-plan.md) **L3**：无界面刚需——复用 `Log` + 滚动文件 sink + reconciler 决策/回写日志）。
- **P4 测试**：纯映射逻辑（assignment→命令序列）进 `tools/verify` 镜像；起 `mock-purplepi` 当本机栈、
  另起一个"假平板"（可扩 `tools/mock-app` 或新 `mock-tablet` 写黑板）跑 **agent↔mock-purplepi** 闭环；
  软总线双机用两实例桩。
- **P5 上车**：与成员A 在真紫派部署联调（OH 5.0 跑 hap、真 `udp2lcm`、真软总线 trust）。

## 4. 边界 / 风险 / 待成员A 确认（→ integration-qa.md）

- **本机 UDP 端口互斥**⚠️：`udp2lcm` 收首包记一个 client。若 **agent 与外部平板同时**对紫派 5001 发指令会互抢。
  设计取向：**distributed 模式下 agent 是本机唯一 localhost 客户端**，平板经黑板下发（不直连该车 UDP）；平板直连 UDP 只用于
  无 agent 的单车直控。需 A 确认 `udp2lcm` 是否 bind `0.0.0.0:5001`（agent 可打 `127.0.0.1:5001`）、是否需独立 localhost 端口避让外部。
- **OH 5.0 能否常驻无界面 ArkTS hap**：A 早前口头"agent 可行"，需落实 ServiceExtensionAbility 常驻 + 开机自起 + 资源占用（RK3566）。
- **软总线信任**：平板↔紫派 OH 设备认证/`networkId` 发现、`distributedDataObject` 可信组网门槛。
- **位姿对齐**：子机"从 master 同物理起点同朝向、顺序出发 → 构造位姿 (0,0,0)"的编排谁来保证（agent 收到 `cmd5` 即归零，已确认）。
- **地图**：先方案B（agent 触发 `cmd105`→紫派 `cmd124` wget）；方案A（黑板传整图）后置。
- 这些汇总成 **integration-qa.md 新问题块（Q6 起）**，供 A 异步答。

## 5. 验收（MVP）

平板在黑板上给某车划一个覆盖矩形 → 该车 agent 读到 → 发 `107/108` 给本机 `mock-purplepi`/真栈 → 车开始"覆盖"移动 →
agent 把心跳位姿写回黑板 → 平板地图上看到该车 pin 移动 + 进度。全程不在车上装 CarApp、不靠 startAbility 拉起。

## 6. 设计（P1，进行中）

> 本节是 P1 产出。已落地：**⑤ 共享日志**——`constants/debug.ets` + `utils/log.ets`（`Log.scoped(tag)`）+
> `RobotTransport` 的 `DEBUG_WIRE` 线缆 trace（logging-plan L1，commit `6137efc`）。以下为其余设计决策。

### 6.1 复用策略（①）——定：抽共享 HAR `shared-core`
- **目标形态**：DevEco 同一工程内一个 HAR 模块 `shared-core`，`entry`(平板 App) 与 `car-agent`(hap) 都依赖它。
- **首批迁入（agent 必需、UI 无关）**：`model/protocol`、`model/mission`、`model/geometry`、`constants/protocol`、
  `constants/debug`、`utils/log`、`service/RobotTransport`、`service/FleetMissionService`（黑板部分）。
- **迁移方式（避免大爆炸）**：P2 第一步把上述文件移入 `shared-core`，`app-harmony` 改为从 HAR import（路径替换、
  不改逻辑，`verify.mjs` 仍应 17/17）。**期间若 HAR 配置受阻**，退而求其次：`car-agent` 先**精确复制**这几个文件并加
  "与 app-harmony 同源，改一处改两处"注释，HAR 化作为后续；但首选 HAR（杜绝漂移）。

### 6.2 reconciler 状态机（②）——黑板 → 本机 UDP，幂等
- **输入**：FleetMission 黑板变更（订阅）+ 本机 localhost 心跳。**本车** = 按 `index/carId` 在 `robots/assignments` 里定位自己。
- **派生状态**（由黑板 `phase` + 本车 `assignment` + 本机位姿推导，不另存权威态）：
  - `idle`：无分配 / `phase∈{idle,scanning}` → 只发中性保活（`pending+stop`）。
  - `loading`：刚分到任务且未归零 → 发一次 `cmd5`（加载图+位姿归零 0,0,0，子机从 master 起点出发）。
  - `covering`：有覆盖矩形 `assignment` 且 `phase=covering` → 依次发 `cmd107`(对角点1)、`cmd108`(对角点2,byte1=robotId)。
  - `nav`：有单点目标 → `cmd3`(endX,endY)；目标清除/`cmd4` 取消 → 回 `idle`。
  - `done`：`phase=done` 或分配撤销 → `cmd4`/`idle`。
- **幂等（关键，别每 tick 重发）**：维护 `lastDispatched` = 本车相关黑板切片的签名（如 `assignment` 的 JSON/hash）。
  仅当签名变化才推进状态、发命令；黑板其它字段变化（别的车位姿）不触发本车下发。
- **保活**：无论状态如何，对本机栈 ≥1/s 发当前帧（`RobotTransport.startHeartbeat(127.0.0.1, payload)` + 状态切换时 `setHeartbeatPayload`）。

### 6.3 localhost 传输与外部平板直连的互斥（③）
- agent 用共享 `RobotTransport`，**目标固定 `127.0.0.1:5001`**（本机 `udp2lcm`）。
- **distributed 模式下 agent 是本机唯一 localhost 客户端**；平板**不**直连该车 5001，改经黑板下发（平板的多目标直连
  仅用于"无 agent 单车直控"）。否则两个 client 抢 `udp2lcm` 的单 client 记录 + 各自 3s 急停判定会打架。
- 待 A 确认 `udp2lcm` 是否 bind `0.0.0.0:5001`、是否需给 agent 留独立 localhost 端口（**integration-qa Q6.2**）。

### 6.4 黑板写回节流（④）
- 本机心跳 500ms 一帧，但**别每帧写黑板**（软总线会被刷爆）。策略：合并 `pose+progress+status` 到一次
  `MissionSnapshot` 更新（沿用"单一 `missionJson` 字段"），**最小写回间隔**（如 ≥300ms）或"位姿显著变化才写"二选一。
- `progress` MVP：覆盖模式按"已扫面积/子区域面积"粗估，或先留 0，P3 细化（心跳无 progress 字段，靠 agent 推算）。

### 6.5 `car-agent/` 目录骨架（②给 P2 预览）
```
car-agent/
  entry/src/main/ets/
    serviceextability/AgentServiceAbility.ets   # 无界面常驻入口（ServiceExtensionAbility）
    reconciler/Reconciler.ets                   # 6.2 状态机：订阅黑板 → 发本机 UDP；心跳 → 写回黑板
  entry/src/main/module.json5                   # 声明 ServiceExtensionAbility；权限：分布式 datasync/软总线/Internet(UDP)；无 page
  （依赖 HAR shared-core：model/ + service/RobotTransport + service/FleetMissionService + utils/log + constants/）
```
- **日志**（logging-plan L3）：agent 复用 `Log` + 加滚动文件 sink（`agent.log`，`hdc file recv` 拉）+ reconciler 决策/回写日志。

### 6.6 P1 剩余 / 交接
- 收尾共享日志 L1 的其余 `console.*` 收敛（pages/MapService/screen/componentUtils 共 12 处）。
- P1 设计已够 P2 起步（脚手架 + HAR 抽取）。**下一步实操 = P2**（建 `shared-core` HAR + `car-agent` 骨架）。
