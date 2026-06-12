# APP ↔ 紫派 全流程对齐审计（待人工裁决）

> **日期 2026-06-12** · 审计方：App 侧（Claude）· 范围：App(`app-harmony/`) + 车载 agent(`car-agent/`) ↔ 紫派(成员A，`origin/purplepi-control`)。
> **方法**：按 App 功能主流程**一步一步**，把 App/agent 实际代码与紫派实际代码（不是文档，是源码）逐字段/逐时序对照，
> 找出消息格式、字段、字节布局、命令码、时序先后、状态机、坐标语义上**一切可能对不齐**的地方。
> 证据均带 `文件:行/handler`。已知的分布式问题（原 `integration-qa.md` Q13）一并纳入、并补充新发现。
>
> **本文档怎么用**：每条结论留有 **🧭 以哪方为准（待裁决）** 字段——请用户在每条上勾选裁决方向；
> 裁决后，**A 读「需 A 改」条目改紫派代码 + 更新《接口功能与对接问题说明.md》**，**App 读「需 App 改」条目改 App/agent**。
> Claude 给了**建议方向**（带理由）但**不替用户拍板**。本文档是一次性审计快照，落地后相应条目可勾「已处理」。
>
> 关联契约：[`udp-protocol.md`](udp-protocol.md)、[`udp-protocol-crosscheck.md`](udp-protocol-crosscheck.md)、[`map-format.md`](map-format.md)、[`multi-robot-collab.md`](multi-robot-collab.md)、[`integration-qa.md`](integration-qa.md)、[`interface-review.md`](interface-review.md)、[`docs/map-pipeline.md`](../docs/map-pipeline.md)。

---

## 0. 速览（按严重度排序）

| # | 流程阶段 | 严重度 | 一句话 | 建议方向（待裁决） | 状态 |
|---|---|---|---|---|---|
| **X1** | 分布式覆盖 | 🔴 高 | `cmd108('l')` 仅给从机(robot_id=1) build 覆盖路径(122)，主机(0)不 build → **master 不动** | 以 **App 设计**为准 → A 改 | 真机已暴露（Q13①）|
| **X2** | 分布式覆盖 | 🔴 高 | `cmd108('l')` 内部又 load 图 + 用当前位姿，**覆盖** `cmd5` 刚归零的 (0,0,0) | 以 **App 设计**为准 → A 改 | 真机已暴露（Q13②）|
| **X3** | 分布式拉图 | 🔴 高 | agent `Reconciler` **不发 `cmd105`** 拉主机图 → 子车 `cmd5` 加载的是不存在/过期本机图 | **App/agent 改**（补 cmd105）| 新确认（Q13 提及）|
| **M1** | 地图坐标 | 🔴 高 | App **未应用 `x0/y0` 世界偏移** → 机器人 pin / 选点下发**整体偏一个常量** | 需裁决（App 落公式 *或* A 归零 x0/y0）| 新确认（Q12/§5「待落实」）|
| **X4** | 分布式拉图 | 🟠 中 | 即便补 `cmd105`，`cmd124` wget **异步无完成信号** → `cmd5` 可能早于拉图完成（race）| 双方议（agent 等待 *或* A 给信号）| 新发现 |
| **C1** | 连接/保活 | 🟠 中 | distributed 模式平板**仍直连保活 master**（`initConnection` 无 mode 判定）→ 与 master-agent 抢单 client | **App 改**（A6.2 已认可）| 旧开放（Q6.2）未落地 |
| **B3** | 建图就绪 | 🟡 低 | 「地图就绪」靠 App 端启发式（行数齐全），紫派无就绪信号 | 短期 App 自决；长期 A/agent | 已缓解（Q8）|
| **X5** | 分布式覆盖 | 🟡 低 | `cmd107→108` 两帧 + 紫派 static 暂存，WiFi 直发丢包/乱序会错配 | 维持（agent 路径已稳）| 已述（R1 §3）|
| **X7** | 分布式 | 🟡 低 | `cmd106('j')` App/agent 均不再发送，紫派仍实现 → dead/divergent | 确认废弃 → A 文档标注 | 新发现 |
| **M3** | 地图格式 | 🟡 低 | 压缩图 `ZMAP1` 行格式只在 A 的 README + App 注释，**缺契约级定义** | A 补进 `map-format.md` | 新发现（文档缺口）|
| **C4** | 连接 | 🟢 微 | 紫派 `udp.c` 只捕获一次 `clientIP`，平板换 IP/重连后心跳仍发旧 IP | A 知悉（健壮性）| 新发现（次要）|

> **附录 A** 另列 **11 项「已逐字段核对 = 对齐」** 的点（设备发现 0x06、9 字节布局、3s 急停、'm' 强制建图、坐标单位、A* 命令、地图首行 7 值、压缩图解码、中性保活不打断自主…），供 A 放心、避免重复排查。

---

## 1. 设备发现（0x06 广播）

### D1 ✅ 对齐 —— 0x06 发现 ping / 响应
- **App**：`RobotTransport.discover()` 向 `255.255.255.255:5001` 周期发 `byte0=0x06`；收到回包按**源 IP** 去重；`byte0=0x07` 单独判为视觉设备（香橙派/B）。
- **紫派**：`udp.c::isDiscoveryPing`（`buffer[0]==0x06`）→ `sendDiscoveryResponse`：回 `[0]=0x06,[1]=0,[2]=0,[3..8]=当前位姿`，**且 `continue`**——不记 `clientIP`、不起心跳线程、不进 `parseCmd`、不武装 3s 急停（连接前循环与命令循环里都拦截）。
- **结论**：**逐字段对齐**。App 只用 `byte0` + 源 IP，不读 `[1]/[2]`（紫派恒 0），无影响。详见附录 A-1。
- **🧭 裁决：无需**（已对齐）。

> 注：`0x07` 视觉发现响应归**香橙派(成员B)**，不在本「App↔紫派」审计范围（B 侧待实现，见 `integration-qa.md` 2026-06-12 香橙派条目）。

---

## 2. 连接 / 心跳 / 3s 急停

### C1 🟠 中 —— distributed 模式平板仍直连保活 master（应只走黑板）
- **App 现状**：`ControlPage.initConnection()`（`ControlPage.ets:160-163`）**无 mode 判定**：只要 `this.ip` 非空就 `connectTo(this.ip)` → `startHeartbeat(ip, 中性帧)`，**distributed 模式也照连**。
- **紫派现状**：`udp.c` bind `0.0.0.0:5001`，**只保存一个 `clientIP`**（首个非发现包），心跳只发这一个 client（`udpSendHandler`）。
- **不一致点**：distributed 下 master 同时有「平板直连保活」+「master-agent 本机 `127.0.0.1` 下发」两个 UDP 客户端，抢紫派的**单 client 记录**：
  - 若平板的中性帧先到 → `clientIP=平板` → master-agent 收不到本机心跳 → **agent 无法把 master 位姿写回黑板**；
  - 若 agent 先到 → `clientIP=127.0.0.1` → 平板 `onMessage` 直接收不到 master 心跳（只能经黑板间接拿）。
- **影响**：master 位姿/进度回流不稳；与 A6.2「distributed 平板不直连任何车（含 master）」相悖。
- **涉及**：`ControlPage.ets:160-163`、`car-agent/AgentCore.ets:56`、紫派 `udp.c`；契约 `multi-robot-collab.md`、`integration-qa.md` Q6.2/A6.2。
- **🧭 以哪方为准（待裁决）：** ☐ 以 App 设计为准（App 改）　☐ 以现状为准（不改）
  - **建议（Claude）：A6.2 紫派已认可「distributed 平板不直连任何车」**，故应 **App 改**：`initConnection`/进入 distributed 时**不对 master 起直连保活**，master 由其本机 agent 独占 localhost、平板只读写黑板。属 App 侧改动、不需 A 动代码。

### C2 ✅ 对齐 —— 3s 急停超时
- App `FAILSAFE_TIMEOUT_MS=3000`（`constants/protocol.ets:34`）；紫派 `udp.c` `timeoutMs = 3000 - elapsed`，超时 `wheel_ctrl` 停。**两端 3000ms 一致**。
- **🧭 裁决：无需**。

### C3 ✅ 对齐 —— 心跳 500ms / App 保活 1s
- 紫派 `udpSendHandler` `usleep(500000)`=500ms；App `HEARTBEAT_INTERVAL_MS=1000`（< 3s 即安全）。`heartBeat[0]=0x03`（`udp2lcm.c::poseHandler`），App `decodeReceive` 以 `state===3` 判 `hasPose()`。对齐。
- **🧭 裁决：无需**。

### C4 🟢 微 —— 紫派只捕获一次 clientIP（换 App/重连不刷新）
- **紫派现状**：`udp.c::udpRecvHandler` 在**连接前循环**里 break 的第一个非发现包 `strcpy(clientIP, ...)` **仅一次**；之后命令循环虽更新 `clientAddr`（用于发现响应），但 `udpSendHandler` 的心跳目标 `clientIP` **不再更新**。
- **不一致点**：若平板换 IP（重连/换设备）后再发指令，**心跳仍发往旧 IP**，新平板收不到位姿（虽仍能控）。
- **影响**：开发期换机/断线重连偶发"能控但看不到位姿"。**纯紫派内部健壮性**，非 App 可解。
- **🧭 以哪方为准（待裁决）：** ☐ A 改（重连刷新 clientIP）　☐ 维持现状（每次连接重启 udp2lcm 规避）
  - **建议（Claude）：低优先**。可待 A 顺手把命令循环里的有效控制包来源刷新进 `clientIP`；不阻塞。

---

## 3. 建图

### B1 ✅ 对齐 —— 「开始建图」用 'm'/0x6d 强制重建
- App `startBuild()` 发 `RobotCommand.forceCreateMap=0x6d`（`ControlPage.ets:416`）；紫派 `udp2lcm.c` `buffer[0]=='m'` → LCM30 `iparams[0]=1, iparams[1]=1`(forceNewMap) → 清旧图重建。`cmd0` 在「有图 / 已请求」时只当心跳（`defaultMapExists()||mapCreateRequestedFromHeartbeat` → return）。**对齐**（Q13 历史根因已收口）。
- **🧭 裁决：无需**。

### B2 ✅ 对齐 —— 结束建图 cmd2
- App `finishBuild()` 发 `afterEnd=2`；紫派 cmd2 = 停轮控 + 存图(32) + `sleep(2)` + 加载导航图(10，当前位姿)。对齐。
- **🧭 裁决：无需**。

### B3 🟡 低 —— 「地图就绪」无紫派级信号（App 端启发式）
- **App 现状**：`finishBuild()` 后 `loadMap(ip,8,1500,true)` 轮询拉图，用 `mapLooksComplete()`（`ControlPage.ets:274`）判完整——**首行声明行数的 ≥90% 数据行**（已取代旧脆弱字节阈值 `MAP_READY_MIN_BYTES`）。
- **紫派现状**：无 UDP 级「存图完成」回执（Q8/A8）。
- **不一致点**：App 只能靠"拉到的文本行数齐全"猜就绪；建图刚结束、存图落盘中途拉到半截会重试（已用 8 次×1500ms 兜）。
- **🧭 以哪方为准（待裁决）：** ☐ 短期维持 App 启发式　☐ 长期 A/agent 给就绪信号
  - **建议（Claude）：维持现状**（A8 已认可短期方案）；长期由 agent 监听本机存图结果写 `FleetMission.robots[].status`，或紫派心跳留状态字节（属新功能，不阻塞）。

---

## 4. 导航 / 路径（单机）

### N1 ✅ 对齐 —— A* 与全路径命令
- **A* 导航**：App `startRoute=3`（`endX/endY` BE，5cm 单位）→ 紫派 cmd3 `/20=米` → LCM20；`endRoute=4` → LCM23 + 多轮停。对齐。
- **全路径（单机）**：App `fullpathSelect=104`(byte1=顶点序号, 坐标)×4 → 紫派 'h'/125；`fullpathStartRoute=102`(byte1=算法号) → 紫派 'f'/127 `NAVI_SetPlanFullPath(alg)`；`fullpathStartover=103` → 'g'/126。对齐（算法选择仅单车 127 支持，A11 确认；App 多车不显示算法 = 对齐）。
- App 在第 4 个顶点后等 `FULLPATH_START_DELAY_MS` 再发 102（对照紫派"先处理 4 个 125 再 127"）。对齐。
- **🧭 裁决：无需**。

### N2 ✅ 对齐（曾疑·已排除）—— 中性保活不打断自主运动
- **疑点**：App/agent 每 1s 发中性帧 `pending(1)+stop` → 紫派 cmd1 → `wheel_ctrl{cmd:0,speed:0}`，是否会每秒打断 A*/覆盖的自主运动？
- **核对结论 = 不会**：① 自主运动走**独立 LCM 通道 `PATH`**（`Navi/main.cpp:395` publish），手控走 `wheel_ctrl`，二者分开；② 轮控 `serial.c::parseCmd` 对 `wheel_ctrl` **幂等**——`if(curStatus==status && curSpeed==speed) return;`，中性帧 `(0,0)` 与上一帧相同即**不下发轮子**；③ `serial.c::parsePath` 仅在 `stopFlag` 时让路（cmd4 触发）。故中性保活不与 `PATH` 抢轮。**对齐，无需改**。
- **🧭 裁决：无需**（记录以备 A 放心）。

---

## 5. 地图拉取 / 格式 / 坐标变换

### M1 🔴 高 —— App 未应用 `x0/y0` 世界偏移 → 位姿 pin / 选点整体偏移
- **紫派现状（权威格式）**：地图首行 7 值 `range resolution height width metersPerPixel x0 y0`（`MapServer.cpp` LoadConfig `infile >> ... >> x0 >> y0`）。栅格 cell(col=`iin`,row=`i`) 的世界坐标 = `(iin+0.5)*metersPerPixel + x0` / `(i+0.5)*metersPerPixel + y0`（`MapServer.cpp:176-181`）。即 **`x0/y0` = 栅格最小角的世界坐标（米，可负，A12 确认）**。世界↔格公式：`grid=(world − x0)/metersPerPixel`。
- **App 现状**：`MapService.parseMap` 只取 `parts[2]=height、parts[3]=width`，**完全不解析/不传递 `x0/y0`（parts[5]/[6]）**；`ParsedMap`/`MapTransform` 无 `x0/y0` 字段；`geometry.ets::canvasToMap/mapToCanvas` 只用障碍包围盒 `startX/startY` + `txtAverSize`，**不含 `x0/y0`**。
- **不一致点**：App 把**心跳坐标**（`heartBeat x = pos[0]*20`，5cm 单位）**直接当成栅格列索引**渲染机器人 pin；但真实栅格列 = `heartBeat_x − x0*20`。两者**恒差 `(x0*20, y0*20)` 格**（`x0` 一般非零）。下发目标点（`canvasToMap`→cmd3）同样少减 `x0/y0` → 机器人会去到**偏一个常量**的位置。
- **影响**：真机「机器人 pin 整体偏一个常量 / 选的目标点偏一个常量」——正是 `map-pipeline.md §5` 预言的现象，**至今未落地修复**。
- **涉及**：`MapService.ets:309-313`(只取 height/width)、`model/geometry.ets:54-72`、`model/mapContour.ets`/`MapCanvas.ets`(pin 渲染)；紫派 `Navi/map/MapServer.cpp`；契约 `map-format.md`、`integration-qa.md` Q12/A12、`docs/map-pipeline.md §5`。
- **🧭 以哪方为准（待裁决）：** ☐ 以紫派格式为准 → **App 落 A12 公式**（推荐）　☐ 以 App 为准 → **A 把导出地图归一到 `x0=y0=0`**　☐ 双方议
  - **建议（Claude）：以紫派为准、App 改**。`x0/y0` 对 A 的规划有意义（栅格左下角世界坐标），不应强迫 A 归零。App 解析首行 `x0/y0`，在 `MapTransform` 增字段，`canvasToMap/mapToCanvas` 与 pin 渲染按 `grid = world − x0*20`（5cm 单位下 `x0_cells = round(x0/metersPerPixel)`）做平移。**纯 App 侧改动**，A 无需动代码（仅请 A 复核公式与符号）。

### M2 ✅ 对齐（代码）/ ⚠️ A 文档残留 —— 首行取 `parts[2]/[3]`
- App 按位置取 `height=parts[2]、width=parts[3]`（`MapService.ets:312-313`），与紫派 `MapServer.cpp` 写出顺序一致（crosscheck §十、A12 已确认）。**代码对齐**。
- **残留**：A 的《接口功能与对接问题说明.md》**§9 仍写「取首行末两个整数作为高宽」**——那是 LCM `MAPFILE` 4 值分片头的路径，与 HTTP 7 值文件**不同**；照它会把 `x0/y0` 当行列 → 空气图。
- **🧭 以哪方为准（待裁决）：** ☐ A 订正文档 §9（推荐）　☐ 维持
  - **建议（Claude）：请 A 删/订正 §9 的「取末两个」表述**（A 的 `README.md 0db78ed` 已写对「固定取第 3、4 字段」，§9 实已被取代）。**纯文档**，无代码。

### M3 🟡 低 —— 压缩图 `ZMAP1` 行格式缺契约级定义
- **App 现状**：`fetchMapPreferZiped` 优先拉 `zipedMap.txt`，`decodeZipedRow` 按 `rowBitCount wordCount word0…`、每 64 格打包成**无符号 64 位整数**、`cell0` 在 **bit63**、`BigInt` 解（`MapService.ets:277-297`）。
- **紫派现状**：`NaviInterface.cpp:143` 确实生成 `/data/test/zipedMap.txt`（建新图时 `clearGeneratedMapFilesForNewMapping` 一并清旧）。**App 能拉到、能解**（实测路径存在）。
- **不一致点**：该格式目前**只在 A 的 README §3 + App 代码注释**里描述，`contracts/map-format.md` **没有 ZMAP1 的字段级定义** → 后续任一端改打包顺序/位序无契约可依。
- **🧭 以哪方为准（待裁决）：** ☐ A 把 ZMAP1 行格式写进 `map-format.md`（推荐）　☐ 维持注释
  - **建议（Claude）：请 A（地图产出方）把 `zipedMap.txt` 的 magic/头/行编码（位序 cell0=bit63、uint64 十进制）正式写进 `map-format.md`**，App 注释与之对齐即可。**文档为主**。

### M4 ✅ 对齐 —— 数据区 `-1/0`(空格) 与 `1/0`(密排) 双格式
- App `parseRow` 自动识别：含空格→`<0` 为障碍（`-1`障碍/`0`空/`2`覆盖非障碍）；无空格→`'1'` 障碍。与紫派 `defultMap.txt`(空格 -1/0) / `defultMap.txt.txt`(密排) 一致。对齐。
- **🧭 裁决：无需**。

---

## 6. 多机分布式协同（核心问题区）

> 设计基线（App 侧，`multi-robot-collab.md` v0.4 + Q13）：**每辆车（含 master）各领一个对角矩形、统一 107/108 覆盖**；
> 子车时序 `cmd105`(拉主机图)→`cmd5`(加载+归零0,0,0)→`cmd107`→`cmd108(byte1=robotId)`；master 时序 `cmd107`→`cmd108(byte1=0)`。
> 紫派据两对角点 build FullRoad 覆盖(122)+分布式跟踪(123)。协调态走软总线黑板，**地图走方案B车间 HTTP（不走软总线）**。

### X1 🔴 高 —— `cmd108('l')` 主机(robot_id=0)不 build 覆盖路径 → master 不动
- **紫派现状**（`udp2lcm.c` `'l'` handler 逐行）：仅 `if (diag_pt1!=-1 && diag_pt2!=-1 && robot_id==1)` 分支里 `robotCtrlInit(...,122,...)` build FullRoad；`robot_id==0`(master) 分支**只 load 图(10) + 跟踪(123)，不 build 122**。
- **Navi 层佐证（已下钻确认）**（`Navi/main.cpp::RobotCtrlHandle`）：`case 122` 只做 `NAVI_SetRoomVertex(0/1/2 = 对角点A、B、当前位姿)`（设房间顶点；`NAVI_CreateFullPath` 已注释）；`case 123` = `NAVI_SetrobotId(robotId)` + `NAVI_SetPlanFullPath(2)`，**依赖 122 设好的房间顶点**才能算出覆盖路径。master 只收到 123、从未收 122 → 房间顶点未设/为旧值 → `SetPlanFullPath` 规划**空/错路径** → 原地不动。**根因在 Navi 层闭环坐实**。
- **App 设计**：master 也领一个矩形、也要覆盖（对称无主从特例）。`ControlPage.assignArea` 给 car1 推 `robotId=0`，agent/平板照发 107/108。
- **不一致点 / 现象**：master(robotId=0) 发了 107/108 却**没有 122 覆盖路径** → 123 跟踪一条空路径 → **master 原地不动**（真机：两车都画了矩形，master 不动）。
- **涉及**：紫派 `udp2lcm.c` `'l'` handler；App `ControlPage.ets:449-455`、`Reconciler.ets:70-74`；`integration-qa.md` Q13①。
- **🧭 以哪方为准（待裁决）：** ☑ **以 App 设计为准 → A 改**（用户 2026-06-12 已定）　☐ 以紫派为准
  - **A 需改**：把 `'l'` handler 的 build-122 从 `robot_id==1` 放宽到 **robot_id∈{0,1} 都按两对角点 build 122**（与从机对称），并更新《接口…说明》分布式覆盖时序。**先做这条 master 就能动。**

### X2 🔴 高 —— `cmd108('l')` 内部重 load 图 + 用当前位姿，覆盖 `cmd5` 的归零
- **紫派现状**：`'l'` handler 里（robot_id==1 与 ==0 两分支）都先 `robotCtrlInit(...,10,...)` 加载图、并把 `dparams[4,5,6]` 设为**当前 heartBeat 位姿**（`x/20,y/20,sita`），再 build/跟踪。
- **Navi 层佐证（已下钻确认）**：`Navi/main.cpp case 10` 用 `dparams[4,5,6]` 作 `initPos` 传给 `NAVI_LoadMapAndLoc`（重定位初始位姿；`initRange` 硬编码 0.2/0.2/15）。cmd5 设 `dparams[4,5,6]=0,0,0`=归零；cmd108('l') 内部 load-10 设 `dparams[4,5,6]=当前心跳位姿` → **把 initPos 重定位回当前位姿**，覆盖 cmd5 的零点。
- **App 设计**：子车入场已先发 `cmd5`（加载图 + 位姿**归零 (0,0,0)**，因「子车从 master 起点出发」）；`cmd108` 应**只 build 122+123**。
- **不一致点**：`cmd108` 内部的「load 10 + 当前位姿」**覆盖**了 `cmd5` 刚设的 (0,0,0) → 子车坐标系原点错位 → 覆盖矩形落在错误位置。
- **涉及**：紫派 `udp2lcm.c` `'l'` handler；App `Reconciler.ets:67`(cmd5)、`AgentCore.ets`；`integration-qa.md` Q13②。
- **🧭 以哪方为准（待裁决）：** ☑ **以 App 设计为准 → A 改**　☐ 以紫派为准
  - **A 需改**：让 `cmd108('l')` **只 build 122 + 123**，**移除内部的 load-map(10)/位姿重置**；地图加载与位姿归零交给前置的 `cmd5`（子车）或沿用当前定位（master）。

### X3 🔴 高 —— agent `Reconciler` 不发 `cmd105`，子车从不拉主机图
- **App/agent 现状**：`Reconciler.reconcile`（`Reconciler.ets:65-76`）covering 时只产 `cmd5 → cmd107 → cmd108`，**没有 `cmd105`**；`AgentCmd` 接口也无"主机 IP"字段，`AgentCore.toPayload` 无法把 IP 打进 `byte[1,2,4,6]`。
- **紫派现状（已就绪）**：`'i'`/105 → LCM124 `iparams[0..3]=byte[1],[2],[4],[6]` → `NaviInterface` `wget http://<主机>:8000/defultMap.txt(+roadFile.txt)`（方案B，A 已实现）。
- **不一致点 / 现象**：子车没拉过主机图 → `cmd5` 加载的是**本机不存在/过期**地图（A 开机即清图）→ 覆盖落空。
- **黑板已可承载 master IP（已核对）**：`model/mission.ets` `RobotRuntimeDTO.ip/networkId` 已随快照同步、`MapRef.ref` 亦可专门承载 master 地址；agent 可按 `assignment.robotId==0` 定位 master 的 `carId` → 取 `robots[carId].ip`。**缺口纯在 App/agent 侧**，紫派 `cmd124` wget 已就绪（方案B）。
- **涉及**：`Reconciler.ets`、`AgentCore.toPayload`(`AgentCore.ets:156-163`)、`model/protocol.encodeSend`、平板需把 master IP 写进黑板（`ControlPage.assignArea` → `robots[].ip`/`map.ref`）；紫派 `'i'` handler + `NaviInterface.cpp`(case 124 `NAVI_SubGetMapFromMain(ip)`)；`multi-robot-collab.md`「方案B」、Q13。
- **🧭 以哪方为准（待裁决）：** ☑ **App/agent 改**（紫派 124 已就绪）　☐ 其它
  - **App/agent 需改**：子车 covering 时在 `cmd5` **之前**补 `cmd105`（主机 IP 四段打进 `byte[1,2,4,6]`，IP 从黑板 `robots[master].ip` 取）；`AgentCmd`/`encodeSend` 增 IP 支路。**需配合 X4 的拉图完成时机。**

### X4 🟠 中 —— `cmd105→cmd124` wget 异步、无完成信号 → `cmd5` 可能早于拉图完成
- **现状**：紫派 `cmd124` 的 `wget` 是**异步**拉图，**无回执/完成事件**给 App/agent。
- **不一致点**：补了 `cmd105` 后，agent 紧接着发 `cmd5` 加载图；若 wget 未完成，`cmd5` 加载到**半截/旧**图。仅靠 `CMD_DISPATCH_GAP_MS=300ms` 间隔**不足以**保证 ~MB 级图在 RK3566 单线程 server 上拉完。
- **涉及**：`AgentCore`(命令间隔)、紫派 `cmd124`；关联 Q8 就绪信号。
- **🧭 以哪方为准（待裁决）：** ☐ App/agent 轮询本机 `defultMap.txt` 出现再发 cmd5　☐ A 给 124 完成信号　☐ 双方议
  - **建议（Claude）：双方各做一点**——短期 **agent 侧**在 `cmd105` 后**轮询/延时确认本机 `/data/test/defultMap.txt` 落地**再发 `cmd5`（agent 难直接读紫派文件 → 退化为足够长延时）；更稳的是 **A 给 `cmd124` 一个完成信号**（LCM/心跳位/或 124 同步阻塞到 wget 完成）。请裁决由哪侧承担。

### X5 🟡 低 —— `cmd107→108` 两帧 + 紫派 static 暂存的乱序脆性
- 紫派 `'k'`/107 把 `diag_pt1` 存进 **static**，`'l'`/108 才据两点 build；WiFi **直发**丢包/乱序会用到上一轮残留的 corner1。
- **现状缓解**：distributed 优先走 **agent**（本机 localhost 可靠 + `CMD_DISPATCH_GAP_MS` 间隔）；平板直发(`distUseAgent=false`)仅兜底。
- **🧭 以哪方为准（待裁决）：** ☐ 维持（agent 路径已稳）　☐ 后续给 107 加序号/回执
  - **建议（Claude）：维持**（R1 §3 已评估为帧长固有约束）；真要加固再给 107 回执，低优先。

### X6 ✅ 对齐（非阻塞建议）—— `cmd105` 主机 IP 散列字节 `[1,2,4,6]`
- App/agent 按 `byte[1],[2],[4],[6]` 打包（复用 endX/endY 低字节，跳 [3][5]），紫派 `'i'` `iparams[0..3]=byte[1],[2],[4],[6]`。**对齐**（A10：近期维持旧布局）。Q10 建议改连续 `[1..4]` 为**非阻塞**。
- **🧭 裁决：维持**（A 哪天动 `'i'` 解析时顺手改、两端同步即可）。

### X7 🟡 低 —— `cmd106('j')` distributedEnd 已 dead/divergent
- **App/agent 现状**：`RobotCommand.distributedEnd=106` 枚举仍在，但 **App(`ControlPage`) 与 agent(`Reconciler`) 均不再发送 106**（grep 确认；分布式已统一走 107/108）。
- **紫派现状**：`'j'`/106 handler 仍在（load 图 + 当前位姿 + 接收主机目标点 → LCM20）。
- **不一致点**：106 成为**两端都不走的旧路径**；语义（106=加载+主机目标点）与新 107/108 覆盖模型不再对应。
- **🧭 以哪方为准（待裁决）：** ☐ 确认废弃 → A 文档标注 106 已弃用 / App 删枚举　☐ 保留作他用
  - **建议（Claude）：确认废弃**——App 保留枚举不发即可，请 A 在文档标注 106 现状（避免后人以为分布式靠 106）。无代码必要改动。

### X8 ✅ 对齐 —— robotId 主从约定（0=主/1=从）
- App `assignArea` `robotId = carId<=1?0:1`，写进黑板 `assignment.robotId`；agent `toPayload` 把 `robotId`→`byte1`；紫派 `'l'` `robot_id=buffer[1]`。**布局对齐**。（注：master `robotId=0` 正是 X1 暴露"主机不 build 122"的输入——X1 修好后此约定即闭环。）
- **🧭 裁决：无需**（依赖 X1 修复）。

---

## 7. 视觉链路（香橙派/B）—— 不在本审计范围

App↔视觉走**香橙派(成员B)** FastAPI/WS，与紫派(A)的 UDP/HTTP 链路**解耦**。相关对齐项（detection `score` 缺字段、keypoint index↔name、热点动态 IP 0x07 发现…）属 **App↔B**，已在 `integration-qa.md`（2026-06-11/06-12）与 `contracts/vision-stream-api.md` 跟踪，**本「App↔紫派」文档不重复**。

---

## 附录 A：已逐字段核对 = 对齐（无需改，供 A 放心 / 避免重复排查）

1. **0x06 发现**：ping/响应字节、`continue` 不武装急停、不记 client、不进 parseCmd——`udp.c::isDiscoveryPing/sendDiscoveryResponse` ↔ `RobotTransport.discover`。
2. **9 字节布局 + 大端 + 端口 5001**：`encodeSend/decodeReceive` ↔ `udp2lcm.c swapEndian`/`heartBeat[]`。
3. **首包语义**：紫派首个非发现包 = 捕获 IP + 启心跳 + LCM7，不进 parseCmd；App 首帧（中性 `pending+stop`）被"吃掉"——对齐。
4. **3s 急停 / 500ms 心跳 / 1s 保活**：数值与方向一致。
5. **'m'/0x6d 强制重建 + cmd0 有图只当心跳**：Q13 历史根因已收口。
6. **坐标单位**：1 整数 = 1/20m = 5cm = 1 格；`r`=度[-180,180]。`poseHandler pos*20` ↔ App `COORD_UNITS_PER_METER=20`。
7. **A* / 全路径命令族（3/4/102/103/104）**：命令码、byte1 语义、坐标 BE、启动延时——对齐。
8. **地图首行 7 值、height/width=parts[2]/[3]**：代码两端一致（A 文档 §9 残留待订正，见 M2）。
9. **压缩图能拉能解**：`zipedMap.txt` 紫派生成、App `ZMAP1`/BigInt 解码（格式契约缺定义见 M3）。
10. **数据区 -1/0(空格) 与 1/0(密排) 双格式**：App `parseRow` 自动识别。
11. **中性保活不打断自主**：`PATH` 独立通道 + `serial.c::parseCmd` 幂等——已排除（N2）。
12. **Navi `ROBOT_CONTROL` 分发无错误 fall-through**：`case 10/20/23/30/122/123/124…` 均以 `break` 收尾（曾疑 `case10→30` 漏 break，逐行**contiguous**核对为**误报**、已排除）；`case 123` 固定 `NAVI_SetPlanFullPath(2)`（算法不可选，与 A11 一致）；`case 124` `NAVI_SubGetMapFromMain(ip)` 拉图清晰。

---

## 附录 B：裁决落地清单（裁决后照此分工）

| # | 若裁决「以 App 为准」→ **A 改紫派** | 若裁决「以紫派为准 / App 改」→ **App/agent 改** |
|---|---|---|
| X1 | `'l'` handler：`robot_id==0` 也 build 122 | —（建议 A 改）|
| X2 | `'l'` handler：去掉内部 load10/位姿重置，只 build 122+123 | —（建议 A 改）|
| X3 | —（紫派 124 已就绪）| agent 补 `cmd105`：IP→byte[1,2,4,6]，黑板带 master IP |
| X4 | （可选）`cmd124` 给完成信号 | （短期）agent `cmd105` 后延时/确认再 `cmd5` |
| M1 | （备选）导出图归一 `x0=y0=0` | （推荐）App 落 A12 公式：`grid=world−x0`（×20 格）|
| C1 | — | App：distributed 不直连保活 master，纯黑板 |
| M2 | 订正《接口…说明》§9「取末两个」表述 | —（App 已对）|
| M3 | 把 ZMAP1 行格式写进 `map-format.md` | —（App 已实现）|
| X7 | 文档标注 106 废弃 | （可选）App 删 `distributedEnd` 枚举 |
| C4 | （低优先）重连刷新 clientIP | — |

> **改了协议/接口的任何一侧，务必同步 `contracts/` 对应文档 + 通知另一端**（CLAUDE.md 约定）。
> 本审计落在 `app-harmony-core`；按 [[feedback-docs-main-code-branch]] 文档应 cherry-pick 同步 `main`。
