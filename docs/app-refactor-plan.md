# 鸿蒙 App 重构计划（app-harmony）

> **状态**：功能内核 + 协议对账已落地并合并入 `main`（2026-06-03，`8dcc698`）；后续改动在分支 `app-harmony-core`。详见「下一次会话从这里开始」与「执行进度」。
> **决策**：① 多机协同改用 `@ohos.data.distributedDataObject`；② 彻底分层重构（非重写）。
> **执行方式**：建议 `/clear` 后在新上下文按本文件从 Step 0 起逐步执行；每步对照旧版（`W:\CarApp\CarApp`）行为，用 `tools/mock-purplepi` 验证。

---

## 下一次会话从这里开始（handoff · 2026-06-03）

> 按"新上下文执行大任务"的习惯：`/clear` 后读 `MEMORY.md` + 本节即可接续，不必重新长篇探查。

**当前状态**
- 功能内核 + 契约 v0.2 + 协议对账已合并进 `main`（`8dcc698`）并推送；A 已能看到平板代码。
- **后续 App 改动在分支 `app-harmony-core` 上做**（已 checkout；改完再合并 `main`）。工作区干净。
- 旧 App `W:\CarApp\CarApp` 仍作行为参照。

**可立即做（不被 A 阻塞）**
1. `service/DeviceCollabService.ets` → 重构为 `FleetMissionService`：共享黑板 `FleetMission`
   （phase/frame/map/area/assignments/robots），订阅/发布/入会话。结构见 `contracts/multi-robot-collab.md`。
2. `service/RobotTransport.ets` → 支持**多目标**（一个 App 管多台车的 UDP 链路）。
3. `model/mission.ets` → 加 `frame{originX,originY,resolution,headingZero}`、`map{ref,text?}`、
   `assignments{[carId]:{corner1,corner2,robotId}}`、`robots[].progress`。
4. 取消 `RobotRunAbility` 的"跨车拉起"语义（改作紫派常驻 agent 角色，见协同契约）。

**被 A 阻塞、待回复后收口**
- 地图文件名确切拼写（`MAP_FILE_NAME` 暂取 `defultMap.txt.txt`）。
- 坐标原点定义 + 0°方向（落到 `map-format.md` 与协同契约的 `frame`）。

**再之后**：UI 阶段（`component/` MapCanvas·Joystick·DeviceList + 参数化 `ControlPage` + 真 HomePage/SetIPPage）。

**验证**：`node tools/verify/verify.mjs`（纯逻辑）；`python tools/mock-purplepi/mock_purplepi.py`（联调）；
`entry/src/test` hypium（需 DevEco）。整体 ArkTS 构建只能在 DevEco（无 CLI hvigor）。

---

## 执行进度（2026-06-02：功能内核已落地）

> 本次决策：采用**干净功能内核**策略——不 robocopy 旧 App 原地改，而是在 `app-harmony/` 直接搭可编译
> 骨架 + 全新分层代码；旧 `W:\CarApp\CarApp` 原封作参照。**UI 留待下一阶段**（用户：先做功能再设计 UI）。

**已完成**
- 骨架：构建配置（`signingConfigs` 留空待 DevEco 自动签名，不提交密钥）、AppScope/module.json5/资源
  （清掉 原神/genshin、chuixiong 占位媒体；label 英文化）。
- `constants/`：protocol（端口/字节偏移/保活超时/地图 HTTP，对齐 contracts）、ui（节流/速度/到点/渲染色）。
- `model/`：protocol（`RobotCommand` + `encode/decodeSend`，移植 MakeData，逐字节一致）、geometry（坐标
  互逆纯函数，去全局）、mission（`@Observed` Mission/EndPoint/RobotRuntime + 可序列化快照）。
- `service/`：RobotTransport（唯一 socket + 单点 `on('message')` 分发 + 1s 保活）、MapService（HTTP+解析+
  坐标变换，去全局 context/Txt2Canvas，修「幻读」为可取消轮询）、DeviceCollabService（发现/跨端拉起改用
  **networkId** + distributedDataObject 同步 Mission）、storage（英文 key、getter 无副作用）。
- 三个 Ability（init 服务、不再用 `export let` 全局）+ 占位 `LoadingPage` + `componentUtils`（保留弹窗）。
- 安全网（Step 1）：`tools/mock-purplepi/`（Python：UDP 心跳/坐标 + HTTP 地图）+ `smoke_test.py`（已跑通）；
  `contracts/fixtures/defultMap.txt`（40×40 样例）；`tools/verify/verify.mjs`（Node 镜像，**17 项断言全过**）；
  `entry/src/test` hypium 单测（协议往返/坐标互逆/地图解析）。

**待办（下一阶段）**
- UI（Step 3/4）：`component/`（MapCanvas/Joystick/DeviceList）+ 单一参数化 `ControlPage` + 真 HomePage/
  SetIPPage；动态屏幕（`display`）取代写死分辨率；把 Ability 的 loadContent 指向真实页面。
- 仪表化测试目标 ohosTest 脚手架（当前 `entry/build-profile` 只留 default 目标）。
- 分布式（Step 5，**方案已定稿**，见下「分布式方案」与 `contracts/multi-robot-collab.md`）：实现 `FleetMissionService`
  （共享黑板）+ 紫派侧无界面 agent；真机多车联调。
- 真车全链路（Step 7）。
- 回写 `contracts/`：地图首行格式、密排栅格、命令码 5/107/108/120 语义；坐标系元数据（单位/原点/0°）；子机"从原点出发"位姿初始化。

**关键决策/坑记录**
- 地图首行解析改为"取首行末两个整数"，兼容旧 `[2]/[3]` 与契约 2-数写法（`MapService.parseMap` 注释）。
- 地图栅格按**密排单字符**解析（`d[x]`），契约里空格分隔仅示意。
- distributedDataObject 用单一 `missionJson` 字段承载 `MissionSnapshot`，规避嵌套字段同步边界。
- 无 CLI hvigor：ArkTS 编译只能在 DevEco；纯逻辑已用 `verify.mjs` 即时验证，整体构建需在 DevEco 跑一次。
- 分布式：定稿为"共享黑板(distributedDataObject) + 紫派无界面 agent + 子机从 master 起点出发"，取代旧 startAbility-RPC。见下「分布式方案」。

---

## 分布式方案（定稿 2026-06-03）

> 完整跨设备契约见 [`../contracts/multi-robot-collab.md`](../contracts/multi-robot-collab.md)（三人共享）。此处只记 App 侧要点。

**场景**：master 车建图 → 其它车从 master **同一起点**出发，各领子区域做覆盖；用软总线承载协同。

**纠正旧机制**：旧实现把 `startAbility` 当 RPC（拉起车上 App + `Want` 一次性快照 + UDP burst）。改为
**所有节点常驻共享一块 `FleetMission` 黑板**（distributedDataObject），协同 = 反应式读写，**不再远程拉起**。

**拓扑**：软总线 = 平板 + 每台车的**无界面 agent**（ArkTS，装在紫派上）；agent ↔ 本机机器人栈走 localhost 的
现有 9 字节 UDP 协议。平板 = 规划器 + 总控 + 可视化。

**定位**：子车从 master 起点同位姿出发 → 初始位姿按构造 (0,0,0°)，无需重定位/点选/新命令，坐标系对齐 ⚠️
消解（朝向要标死、顺序出发）。

**地图**：协调态（area/assignments/robots）走软总线；地图位图二选一——A 经软总线同步（demo 亮点）/ B 车间 `cmd124` 拉（轻）。

**对 App 重构的影响**（改写原 Step 5 与目标架构里的 distributed 部分）：
- `DeviceCollabService` → `FleetMissionService`（共享黑板：phase/frame/map/area/assignments/robots；订阅发布、入会话）。
- `RobotRunAbility` 作为"跨车拉起目标"**取消**；改为紫派上的常驻**无界面 agent**（同一 hap 的 ServiceExtensionAbility，或独立瘦 hap）。
- `RobotTransport` 支持多目标（平板直管多车时）；agent 侧只管本机一台。
- 删 `robotRunPage` 的一次性 burst。
- 共享模型加字段：`frame{originX,originY,resolution,headingZero}`、`map{ref,text?}`、`assignments`、`robots[].progress`。

**待成员A确认**：坐标系单位/原点/0°；子机加载图后位姿是否归零到原点；地图传输 A/B；紫派常驻 agent 可行性；子区域顶点表达。（清单见契约文末）

---

## Context（为什么要重构）

现有 App（`W:\CarApp\CarApp`）能跑，但质量与方法都有明显问题，已全量审视确认：

- **巨型克隆页面**：6376 行 ETS 中 **73% 挤在 4 个文件**——`distributedsecondPage`(1381)、`distriFullPathPage`(1328)、`Navigationcomponents.PendingComponent`(1052)、`distributedPage`(884)。四者是同一套"地图+摇杆+选点+设备协同"界面的复制粘贴，仅算法模式/细节不同（前进速度 35 vs 20 等）。改一个控制 bug 要改 4 处。
- **全局可变单例满天飞**：`Txt2Canvas`、`context`（Pending.ets）、`taskId`（common.ets，全局唯一节流器→多控件冲突）、`dmClass`、以及 `robotRunAbility.ets` 里 `export let EndX/EndY/ip/StartX/StartY/indexEachCar`。页面间靠改写模块级全局变量传状态，脆且不可测。
- **分布式用了麻烦写法**：发现/认证用 `distributedDeviceManager`（对），跨端用 `startAbility` 拉起 `robotRunAbility`（机制对），但协同数据靠 `Want.parameters → 全局变量 → UDP` 手工拼，**未用 `@ohos.data.distributedDataObject` 自动同步**；且 `deviceId` vs `networkId` 两份克隆写法不一致（`distributedPage` 用 `item.deviceId` 几乎必拉起失败，应为 `networkId`）。
- **硬编码单一分辨率**：`Screen.ets` 写死 `2199×1533`（注释掉了 `display.getDefaultDisplaySync()`），换平板即错位。
- **状态模型与框架对着干**：因"@State 对象数组成员属性不刷新"，拆成平行数组 `EndPointX[]/EndPointY[]`——应改用 `@Observed`+`@ObjectLink`。
- **死代码与占位垃圾**：空文件 `ReceiveData.ets`/`StateConstants.ets`；几乎全注释的 `dialogBuilder.ets`、示例文件 `test.ets`；菜单 `"aa"/"bb"`；设备认证弹窗 `appName:'原神'`、应用图标 `$media:genshin`；遍地 `console.log('wzh'…)`。
- **明显 bug**：`Index.onPageShow` 每次显示都 `arr.push` 不清空→机器人列表累积重复；`Pending.getTxt` 地图"太小"就 toast「幻读了」并重试的怪启发式；getter 里有副作用（`SetIP.getIp` 每次读都 `persistProp`）。
- **接口契约散落**：UDP 9 字节协议、命令码、地图格式实际散在代码里，且出现未文档化命令 `120`、枚举无 `5`、`107/108` 语义不明。

**目标**：在**不改变对外行为**（严格遵守 `contracts/`）的前提下，把 App 重构为**分层、去全局态、组件可复用、分布式地道**的代码。这是 App 负责人的独立赛道，**不阻塞紫派/香橙派**（他们对着 `contracts/` 开发）。

---

## 目标架构（`app-harmony/entry/src/main/ets/`）

```
ets/
├── entryability/EntryAbility.ets        # 入口：初始化 RobotTransport / DeviceCollabService / 权限
├── robotrunability/RobotRunAbility.ets  # 跨端拉起目标：改为从 distributedDataObject 读协同状态（删除 export let 全局）
├── model/
│   ├── protocol.ets        # UDPSendData/UDPReceiveData + 命令枚举（对齐 contracts/udp-protocol.md）
│   ├── mission.ets         # @Observed Mission/EndPoint/RobotStatus（协同共享状态）
│   └── geometry.ets        # 坐标/画布纯类型
├── constants/
│   ├── protocol.ets        # 端口 5001 / byteLength 9 / 命令码（单一来源，对齐 contracts）
│   └── ui.ets              # 颜色/尺寸/节流间隔等（替换 5e2、速度、324e4 等魔法数）
├── service/                # ⭐ 业务服务层（无 UI、可单测）
│   ├── RobotTransport.ets       # 唯一 UDP socket：收发 + 心跳循环 + 3s 保活急停 + on('message') 单点分发
│   ├── MapService.ets           # HTTP 拉图 + 解析 + 坐标换算（整合 Pending + canvas2map/map2canvas，去全局 context/Txt2Canvas）
│   ├── DeviceCollabService.ets  # 设备发现/认证(distributedDeviceManager) + distributedDataObject 协同同步
│   └── storage.ets              # 持久化（英文 key，取代 SetIP 的中文 key + getter 副作用）
├── component/              # ⭐ 可复用 UI（无业务耦合）
│   ├── MapCanvas.ets       # 地图渲染 + 缩放/平移 + 选点（取代 4 处克隆画布逻辑）
│   ├── Joystick.ets        # 摇杆遥控（取代 4 处克隆的 CarControlBuilder；每实例独立节流）
│   └── DeviceList.ets      # 设备发现/认证/拉起列表（取代 3 处克隆）
├── pages/
│   ├── LoadingPage.ets
│   ├── SetIPPage.ets       # 瘦身，走 storage/RobotTransport
│   ├── HomePage.ets        # 原 Index：机器人列表 + 算法选择（修 onPageShow 累积 bug）
│   └── ControlPage.ets     # ⭐ 单一参数化页面，mode∈{单机A* | 全路径 | 分布式} 组合 MapCanvas+Joystick+DeviceList
└── utils/componentUtils.ets # 保留 PromptActionClass（弹窗）
```

**删除**：`pages/distributedPage.ets`、`pages/distributedsecondPage.ets`、`pages/distriFullPathPage.ets`、`components/Navigationcomponents.ets`、`components/dialogBuilder.ets`、`components/test.ets`、`utils/ReceiveData.ets`、`constants/StateConstants.ets`、`Common/Screen.ets`（写死分辨率，改为动态）。

预计从 ~6400 行降到 ~2000–2500 行。

---

## 关键方法升级（"是不是用麻烦方式做的"——是，下面是地道写法）

| 旧写法（麻烦/有坑） | 新写法（地道） |
|---|---|
| 多机协同：`Want.parameters → export let 全局 → UDP` 手工拼，一次性快照 | **`distributedDataObject`** 自动同步 `Mission`（选点/任务分配/各车位姿状态）到可信设备，各端响应式更新 |
| 跨端 `startAbility` 用 `item.deviceId`（一份克隆写错） | 统一用 `item.networkId`；拉起仅负责"启动远端控制能力"，状态走分布式对象 |
| `@State EndPointX[]/EndPointY[]` 平行数组绕过刷新限制 | `@Observed` 类 + 子组件 `@ObjectLink`，对象数组成员属性可响应式更新 |
| `Screen.ets` 写死 `2199×1533` | `display.getDefaultDisplaySync()` 动态取屏，常量按比例派生 |
| 全局唯一 `taskId` 节流（多控件互相清定时器） | 每个 `Joystick` 实例自带节流器 |
| 每个页面各自 `udp.on/off('message')`（监听泄漏/抢占） | `RobotTransport` 持有唯一 socket，注册一次，按类型分发给订阅者 |
| 4 份克隆页面 | 1 个参数化 `ControlPage` + 3 个可复用组件 |
| 命令码/端口/字节布局散落 + 未文档化命令 `120` | 收敛到 `constants/protocol.ets`，与 `contracts/udp-protocol.md` 双向对齐（顺带定 5/107/108/120 语义） |

---

## 复用清单（移植已验证逻辑，不要从零重写）

- `Common/MakeData.ets` 的 `MakeSendData/parseReceiveData` → `model/protocol.ets` + `RobotTransport`（字节布局已正确）。
- `Common/common.ets` 的 `canvas2map/map2canvas` → `MapService`（坐标数学是硬知识，保留，只去全局化 `Txt2Canvas`）。
- `utils/Pending.ets` 的 `getTxt/MakeCanvas` → `MapService`（保留解析+包围盒裁剪；修「幻读」启发式与魔法数；去全局 `context`）。
- `Common/distributed.ets` 设备发现 → `DeviceCollabService`（保留 API 用法，叠加 `distributedDataObject`）。
- `utils/componentUtils.ets` `PromptActionClass` → 保留作弹窗。
- `utils/SetIP.ets` 持久化 → `service/storage.ets`（英文 key、去 getter 副作用）。

---

## 执行步骤（渐进式，每步可独立验证、对照旧版行为）

0. **迁移基线**：`robocopy CarApp → app-harmony/`（排除 oh_modules/build/.hvigor/.preview/.idea），`ohpm install`，确认能编能跑、对接 `mock-purplepi` 正常。提交基线（用于行为对照）。
1. **安全网**：实现 `tools/mock-purplepi`（UDP 心跳+坐标、HTTP 托管示例地图）+ 录 `contracts/fixtures`（示例地图、抓包）。记录旧版基准行为。
2. **抽服务层（不改 UI 行为）**：抽 `RobotTransport / MapService / storage`；现有页面改为调用它们，逐步删全局可变量。每步对照 mock 验证一致。
3. **修状态模型 + 动态屏幕**：`@Observed/@ObjectLink` 取代平行数组；动态 `display` 取代写死分辨率。
4. **合并克隆**：实现 `MapCanvas/Joystick/DeviceList`，建参数化 `ControlPage`，`HomePage` 路由到 `ControlPage(mode)`；删除 4 个克隆页面。逐 mode 对照 mock 验证。
5. **分布式重构**：`DeviceCollabService` + `distributedDataObject` 同步 `Mission`；修 `deviceId→networkId`；**两台真机联调**验证协同。
6. **清理**：删死文件；清 `wzh/hjx` 日志、`原神/genshin`/`aa·bb` 占位；英文化组件 ID/存储 key；魔法数入常量；命令码对齐 `contracts` 并定稿 5/107/108/120。
7. **回归**：mock + 真车 + 双平板全链路；若澄清了协议细节，回写 `contracts/`。

---

## 验证

- **构建/静态**：`hvigorw assembleHap` 通过；ArkTS linter（仓库已有 `code-linter.json5`）零新增告警。
- **单测（hypium，`entry/src/ohosTest`）**：
  - 协议编解码往返：`parse(make(x)) === x`（逐字节核对 `contracts/udp-protocol.md`）。
  - 坐标换算互逆：`map2canvas(canvas2map(p)) ≈ p`。
- **行为对照**：用 `tools/mock-purplepi`，每条指令产生的 UDP 字节、心跳坐标渲染与旧版一致（重构前后 A/B）。
- **分布式**：两台平板做发现/认证/协同状态同步与跨端拉起（Step 5）。
- **真车联调日**：建图→选点导航→全路径→（后续）仪表识别全链路。

---

## 风险与边界

- **保留旧 `W:\CarApp\CarApp` 不动**作参照；重构只在 `app-harmony/`。
- 每步都在 `contracts/` 之下→紫派/香橙派不受影响，可并行。
- **先重构、后叠新功能**：仪表识别 UI（视频+读数展示）留到重构完成后，作为新增 `component/ResultPanel` + `service/VisionService`（对接 `contracts/server-api.md`）接入，不在旧克隆页面上加。
- `distributedDataObject` 与跨端拉起需双平板验证，单机阶段先以 mock/桩跑通逻辑。
