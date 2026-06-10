# 集成对接 Q&A（App ↔ 紫派，跨端异步备忘）

> 本文给**两侧的人和 AI 异步对接**用：App 侧（owner，`app-harmony/`）与紫派侧（成员A，`purplepi-control/`）。
> 谁有结论/问题就追加带日期的小节，**改了接口先改 `contracts/` 再改两侧代码**（CLAUDE.md 约定）。
> 紫派侧权威接口说明见 `purplepi-control/接口功能与对接问题说明.md`；本文是 App 侧的对账回应 + 开放问题。

---

## 状态总览（2026-06-08：A 已答复全部 Q1–Q12）

> A（紫派，`Reki-git`）于 2026-06-08 在 `main` 答复了下列全部问题（原文见文末 **§A1–A12**）。
> 本表是「现在的状态」速览，**归档下方各 Q 的「待 A」开放态**；详细问题/答复原文保留在本文档作记录。

| Q | 主题 | A 的结论（2026-06-08）| 状态 / App 行动 |
|---|---|---|---|
| Q1 | 协同避障暂停态 | UDP 心跳未暴露暂停态；建议 agent 读本机协同态写 `FleetMission.status` | ⏳ 暂不改 UDP；后续靠 agent 写黑板 |
| Q2 | 支持几车 / tie-break | 当前仅**双车**（robotId 1 继续 / 0 等待）；>2 车需多车表 + 黑板仲裁 | ✅ 已明确；App 先按 2 车联调 |
| Q3 | `roadFile.txt` | 紫派内部覆盖路径（每行 `x,y`，5cm 格）；App 无需读（除非做覆盖预览）| ✅ App 不读 |
| Q4 | 方案A 地图落地 | 写 `/data/test/defultMap.txt`，写完仍需触发 `cmd5/10` 加载 | ✅ 已确认（方案A 后置）|
| Q5 | 设备发现 | 同意新增 `0x06` 发现 ping（只回应、不记 client、不武装急停、不发 LCM）| ✅ 已同意；待定最终码 + 两端实现（协议变更）|
| Q6 | 车载 agent 前提 | 形态可行；`udp2lcm` bind `INADDR_ANY:5001`、agent 打 `127.0.0.1:5001` 不另开口；地图先方案B | ✅ 接口层确认；真机 P5 待落实 |
| Q6.2 | distributed master 独占 | distributed 阶段 master/sub 各由本机 agent 独占、平板只写黑板**不直连任何车（含 master）**；建图/单车手控仍可直连 | ✅ 确认 → App distributed 去掉对 master 的直连保活（P2）|
| Q7 | 建图实时预览 | 建议 HTTP 周期快照（`/data/test/partialMap.txt`）轮询，优于 UDP 分片 | ✅ 方向给定；属新功能、后续做 |
| Q8 | 地图就绪信号 | 无 UDP 级完成确认；短期 `cmd2` 后主动拉图可用；长期由 agent 写状态 | ✅ 短期方案获认可；长期靠 agent |
| Q9 | 软总线互信 | 方案可行（账号无关 PIN、同 WiFi、平板发起/紫派接受、HDMI 确认 PIN、agent `bundleName=com.example.carapp`）；真机待确认 3 点 | ✅ 方案确认；App 已落地 DeviceTrustPage + bundleName；真机 P5 验证 |
| Q10 | cmd105 IP 字节 | 可接受连续 `[1..4]`，但 `main` 仍按旧 `[1,2,4,6]`；近期维持旧布局 | ✅ 维持旧布局；App/agent 继续 `[1,2,4,6]`（无需改）|
| Q11 | 多车覆盖算法 | LCM122/123 **不支持**选算法（`case123` 固定 `NAVI_SetPlanFullPath(2)`）；App 多车**不显示**算法选择 | ✅ App 已符合（算法 chip 仅 fullpath 单车，DistributedOps 无）|
| Q12 | 地图格式 + x0/y0 | 7 值首行 + 空格 `-1/0`（`.txt.txt` 密排）；**x0/y0 单位=米**、=栅格最小角世界坐标、可负；`grid=(world−x0)/metersPerPixel` | ✅ 格式已确认（App 已修）；🔧 定位校正公式已给 → 见 `map-pipeline.md §5`，待落实 |

---

## 2026-06-05 · App 侧：已据 A 的文档收口（FYI，无需 A 回复）

读了 A 的《接口功能与对接问题说明.md》(`origin/purplepi-control` 的 `1608796`/`59fc335`) + 源码，App 侧据此已改：

1. **地图文件名 bug 修复**：App 之前误取 `defultMap.txt.txt`（双 .txt），会拉图失败。
   按 A 确认（`Navi/main.cpp:517/642` 实际保存 `defultMap.txt`，`.txt.txt` 启动即删 `:1094-1096`），
   已改 `MAP_FILE_NAME = 'defultMap.txt'`，URL = `http://<紫派IP>:8000/defultMap.txt`（web 根=/data/test，无前缀）。
   mock/smoke/契约一并更正。
2. **坐标系收口**：`frame` = 单位 5cm/格、原点=建图初始位姿、`r`=度 [-180,180]、`theta=0`=+X、正角 CCW。
   写入 `app-harmony/.../model/mission.ets` 的 `Frame`/`defaultFrame()` 与 `contracts/map-format.md`。
3. **子机归零**：多机子车入场用 **`cmd 5`** 加载图即归零 (0,0,0)；App/agent 流程按此走（不用 `cmd 2/'j'/'l'`，那几个沿用当前位姿）。
4. **地图传输 = 方案 B**（`cmd124` wget 拉 `defultMap.txt`+`roadFile.txt`）。软总线方案 A 留作后续 agent 增强。
5. **子区域 = 单个轴对齐矩形**（2 对角点 + robot_id，`x1≠x2 ∧ y1≠y2`），时序 `cmd107` 先于 `cmd108`。App `Assignment` 模型与此一致。
6. **协同避障（`59fc335`）**：确认是 `Navi` 内独立 LCM 频道 `COOP_AVOID`、**未改 `udp2lcm`** → **App↔紫派 9 字节 UDP 协议不变，App 无需改协议/命令**。

契约已升版：`map-format.md` v0.2、`multi-robot-collab.md` v0.4、`udp-protocol-crosscheck.md` 补 §十。

---

## 2026-06-05 · App → 紫派：开放问题（✅ A 已于 2026-06-08 答复 → 见上「状态总览」/ 文末 §A1–A12）

### Q1【建议加协议】协同避障的"暂停"状态如何让 App 可见？

协同避障时被让车会**自主暂停**数秒（保存目标 → `PATH v=0,w=0` → 恢复）。但**当前 9 字节心跳没有"暂停/避让"标志位**
（`byte0=3`，`byte[1..2]` 保留=0，`byte[3..8]`=x/y/r）。平板仅凭心跳**无法区分**"避让暂停"与"缓行"与"卡死/掉线"，
可视化容易把正常避让误报为故障。

- **提议**：在心跳里留一个**状态字节**（如把保留的 `byte[1]` 用作运行状态码：0=正常/1=避让暂停/2=到达/3=异常…）。
  这是 **`udp2lcm` 心跳编码的小改动**（协议变更，需回写 `udp-protocol.md`）。
- 或者：紫派让 agent 能读到避让态（如 agent 订阅 `COOP_AVOID` 或 `Navi` 暴露一个查询），由 agent 写进软总线 `FleetMission.robots[].status`。
- **请 A 选一种**（或说明无需，App 就不显示避让态）。App 这边的 `RobotStatus` 已预留 `error` 等，可加 `avoiding/paused`。

### Q2【确认即可】支持几台车？协同避障的 tie-break 在 >2 车时如何处理？

避障文档按**双车**描述，互请停机时 `robotId=1` 继续、`robotId=0` 停等。
App 的 `FleetMission.assignments/robots` 支持 N 台车（每台一个矩形子区域）。

- 当前系统是否**只支持 2 车**？若支持 >2，避障 tie-break（谁让谁）如何定？
- 若暂时只支持 2 车，App 侧先按 2 车做联调即可，请确认。

### Q3【确认即可】`roadFile.txt` 是否需要 App 关心？

`cmd124` 子机会同时拉 `defultMap.txt` + `roadFile.txt`。App 目前只拉 `defultMap.txt` 做**显示**。
理解 `roadFile.txt` 是**紫派内部**的覆盖路径文件、App 无需读取/展示——请确认（若 App 也该可视化覆盖路径，请说明其格式）。

### Q4【信息同步】方案 A（软总线传地图）若要做，agent 落地路径

将来若做方案 A（demo 亮点：地图经软总线同步），agent 需把整图文本写到紫派栈读取的路径。
按现状应写 `/data/test/defultMap.txt`（紫派 `cmd10` 加载处）。请确认写文件后紫派是否需要额外触发（还是下一次 `cmd 5/10` 加载即可读到）。

### Q5【提案，待 A 确认/实现】局域网设备发现，免手动输入 IP

App 想在同一局域网/热点下**自动发现车、点击即连**，不再手填 IP。理由：`udp2lcm` 收首包才知 App IP，App 又得先知车 IP → 死循环；发现可破之。App 侧已定走 **UDP 广播/组播探测**（见 `docs/app-refactor-plan.md` §连接与设备发现）。需要 A 这边确认/配合：

1. **`udp2lcm` 能否收子网广播**（App 发 `255.255.255.255:5001`）或加入某**组播组**？若都不行，App 只能退回手填 IP / 子网扫描。
2. **新增一个"发现 ping"**——别拿 `cmd 0` 凑合：`cmd 0` 会让车进入受控态并武装 3s 急停，广播一发就连上一片车、随后集体超时急停。提案（9 字节、大端，与现协议同框）：

   | 方向 | byte0 | byte1 | byte2 | byte3..8 | 紫派行为 |
   |---|---|---|---|---|---|
   | App→广播 发现请求 | `0x06` | 0 | 0 | 0 | 探测在线车 |
   | 车→App 发现响应 | `0x06` | `robot_id`/车号 | 状态(0=空闲…) | 可选当前位姿 x,y（或 0） | **仅回一帧标识；不记为受控客户端、不武装 3s 急停、不发 LCM** |

   **关键诉求**：发现响应**不能**让车进入"已连受控"状态（否则没被选中的车会 3s 急停）。`0x06` 仅举例（现协议 0-5、102-108 已占用，6 空闲），最终码听 A 定。

3. 若 A 更愿意走 **mDNS**（紫派跑 responder 广告 `_inspbot._udp`，App 用 `@ohos.net.mdns` 浏览）也行——与 1/2 二选一，看哪个省事。

A 确认后，把最终发现命令写进 `udp-protocol.md`（协议变更）。

### Q6【新立项·待 A 确认】车载无界面轻 agent（紫派常驻）落地前提

App 侧新立项**车载轻 agent**（无界面 ArkTS 节点，常驻紫派，入会软总线持 `FleetMission` 黑板，把协同决策翻成本机 9 字节 UDP 下发给 `udp2lcm`、把心跳位姿写回黑板）。取代旧"每车装整 CarApp + startAbility 拉起"。规划见 `docs/car-agent-plan.md`。落地前需 A 确认：

1. **OH 5.0 能否常驻一个无界面 ArkTS hap**（ServiceExtensionAbility）与你的 C/C++ 栈并存、开机自起？RK3566 资源是否吃得消？（你早前口头说"agent 可行"，这里落实形态。）
2. **本机 UDP 端口互斥**⚠️：`udp2lcm` 是否 bind `0.0.0.0:5001`（这样 agent 可打 `127.0.0.1:5001`）？只记一个 client 吗——若 **agent 与外部平板同时**对该车 5001 发指令会互抢。App 侧设计：**distributed 模式下 agent 是本机唯一 localhost 客户端，平板经黑板下发、不直连该车 UDP**；平板直连 UDP 只用于无 agent 的单车直控。需要为 agent 留独立 localhost 端口避让外部，还是同 5001 即可？
3. **软总线信任**：平板↔紫派 OH 设备认证 / `networkId` 发现 / `distributedDataObject` 可信组网，紫派侧需做什么配置？
4. 地图先走**方案B**（agent 触发 `cmd105`→你 `cmd124` wget），方案A（黑板传整图）后置——确认 OK？

这些不阻塞 agent 的本机侧开发（可先用 `mock-purplepi` 当本机栈联调），但**上真车前**需逐条落实。

### Q7【新想法·需联合开发】建图过程实时预览（边建边看地图）

**现状**：App 只在**建图完成后**经 HTTP 拉**整张** `defultMap.txt` 显示。建图过程中平板看不到地图在长什么样。
**想法**：建图时把地图**增量**推到平板，操作者能边开边看已扫区域，决定何时结束建图、哪里没扫到。
**底层已有线索**（你文档 §三）：紫派有 LCM `MAPFILE`（地图**分片**）+ `SERVICE_COMMAND`（建图/保存**状态**）。这两条目前只在紫派内部，没暴露给 App。
**需 A 评估**：能否把 `MAPFILE`（或定期的部分地图快照）经 **① 新 UDP 命令 / ② HTTP 增量端点（如 `/partialMap.txt` 周期刷新）** 暴露给 App？哪种对你改动最小？这属于**新功能**，定了接口再一起做（App 侧加"建图实时预览"）。

### Q8【新想法·需 A 确认】可靠的"地图就绪"信号（别让 App 猜文件大小）

**现状**：App 用 `pollMapUntilReady` 靠"`defultMap.txt` 字节数 ≥ 阈值(当前 324e4)"判断地图就绪——**脆**：真图若比阈值小则永远判不就绪；建图中途的半成品也可能误判。
**想法/需 A**：紫派 `SERVICE_COMMAND` 既然有"建图/保存"状态，能否在**存图完成**时给 App 一个明确信号（如心跳里留 1 个状态字节，或一条专门回包），App 收到即拉图？这样取代脆弱的大小阈值。
**短期**：App 侧已先做"`cmd2 结束建图`后主动拉一次图"绕开阈值脆性（见 `docs/feature-parity-review.md` R1/N2）。

### Q9【关键·待 A 确认】无界面紫派如何与平板建立软总线互信（distributedDataObject 同步前提）

多机协同走软总线黑板（`distributedDataObject`），**前提是平板与紫派在同一"可信网络"里**（官方）。建立可信两条路：① 同分布式账号（自动互信）；② 账号无关 `distributedDeviceManager.bindTarget`（**安全认证：PIN/碰一碰/扫码**）。详见 `docs/distributed-trust.md`。

**🔴 难点**：紫派上跑的是**无界面 agent**，`bindTarget` 的交互确认（PIN）在车上没法点 → **agent 自己绑不了**。互信必须**预先一次性建立**（建立后是持久"信任标签"，agent 常驻直接用）。需 A（紫派侧）确认/配合：

1. 紫派 OpenHarmony 5.0 走**哪条互信路**？支持分布式账号（路径①），还是只能账号无关 `bindTarget`（路径②）？
2. 若路径②：紫派怎么**接受绑定**——首次配对有没有一个**非无界面的确认步骤**（系统设置/开发期 hdc/一次性配对 App），还是能配成**开发模式同网免 PIN 自动接受**？
3. 紫派需要哪些权限/配置才能**被发现**（`startDiscovering` 的对端）+ 被绑成可信？
4. 互信建立后，无界面 agent 用 `distributedDataObject` 同 `sessionId`（`OpenHarmonyCarFleetV1`）同步，紫派侧还要做什么？

**在此之前**：平板 App 会补"设备互信 UI"（发现+发起 `bindTarget`），但**能否真把无界面紫派绑成功取决于上面**；端到端联调前，多机用平板「直发兜底」先测覆盖本身。

**↳ App 侧补记（2026-06-08，用户定）**：互信方向与车端确认手段**已定**——**平板=发起方**（`bindTarget`）、**紫派=接受方**；紫派**配网期接 HDMI 显示器**在系统配对弹窗确认 PIN（**团队既有成熟流程，此前设备认证一直这么做**）。故上面 🔴"无界面没法确认 PIN"**消解**：配对一次性、用显示器完成，之后 headless agent 凭持久信任标签直接 `distributedDataObject` 同步、运行时无需界面。传输层**保留 DDO 软总线**（评估过纯 LAN socket 退路、`FleetMissionService` 接口传输无关可随时切，留作软总线反复受阻时再用）。详见 `docs/distributed-trust.md`「决策」。

**A/用户答（2026-06-08，依老 App 经验）**：① 走**账号无关 PIN 认证**（`bindType=1`）、同一 **WiFi 局域网**；② 绑定时紫派**接显示器+鼠标**，**系统 PIN 弹窗**在车屏确认（查老 App 代码证实：本 App 不自渲染 PIN、无 `uiStateChange`，系统弹窗代劳）；③ **🔑 关键：`distributedDataObject` 同步要两端同 `bundleName`** → **车载 agent 须打包为 `com.example.carapp`**（与平板同），平板 `bindTarget` 的 `targetPkgName='com.example.carapp'`。**残留待真机/A**：紫派 DM 是否确有可点系统弹窗、接受方是否要"目标包(agent)在运行"才弹 PIN（已用"agent hap 内置**一次性配对 UIAbility**"兜底）、被发现+接受绑定的权限配置。**App 侧已落地**：发现+配对面板 `pages/DeviceTrustPage.ets`（HomePage「设备互信」入口）+ `FleetMissionService.bindDevice` 补 `targetPkgName/appOperation/customDescription`。

---

## 2026-06-08 · App → 紫派：接口优雅性复审（R1）产出的开放问题（✅ A 已答 → 见上「状态总览」/ 文末 §A1–A12）

> 来自 [`interface-review.md`](interface-review.md)（R1 复审）。R1 结论：协议骨架优雅、对齐干净，唯一值得两边一起改的是下面 Q10（非阻塞）；
> 其余脆点（发现 ping、心跳状态、地图就绪）已分别在 Q5/Q1/Q8 跟踪，地图首行"取末两个"是 A 在《接口…说明》§7 主动建议的方案、非妥协。

### Q10【建议·低优先·协议小改】cmd105 主机 IP 改**连续字节**打包

**现状**：cmd105('i') 把主机 IP 四段塞进**散列**字节 `byte[1],[2],[4],[6]`——根因是复用了 `endX/endY` 两个大端 int16 字段，
单字节 IP 落在它们的低字节（[4]/[6]），高字节 [3]/[5] 恒 0 空着（见 `udp-protocol-crosscheck.md` §五）。编解码两端都得记住"跳 [3][5]"，易错。
- **提议**：cmd105 不复用 int16 字段，IP 四段放**连续** `byte[1..4]`（[5][6] 留 0）。紫派改 `iparams[0..3] = byte[1],[2],[3],[4]`，App/agent 同步改打包。
- **优先级**：**非阻塞**——现状能工作。A 哪天动 cmd105 解析时顺手改即可；改后回写 `udp-protocol.md`（协议变更，需两端同步）。
  新写的车载 agent 触发 cmd105 时按最终布局实现（暂先按现状 [1,2,4,6]）。
- **请 A**：接受（顺手改）/ 维持现状（维持则 App 与 agent 继续按 [1,2,4,6] 打包，不阻塞）。

### Q11【接口不一致·建议】多车覆盖（LCM122）能否像单车全路径（LCM127）那样选覆盖算法？

来自 App 流程复审（`docs/safety-flow-review.md` §3）。**现状不一致**：单车全路径 `cmd102` 带 `byte1=algNum`（牛耕 0 / 最小生成树 1 → LCM **127**）、App 已有算法选择 UI；但多车分布式覆盖 `cmd107/108` → LCM **122**（按两对角点生成矩形覆盖路径）**协议里没有算法参数**（A 文档 §三 122 无 algNum），App 多车界面因此**没有算法选项**。
- **问**：LCM122 是否支持选覆盖算法（牛耕/最小生成树/…）？
- 若支持 → App 想在 `cmd107`（byte1/2 当前空）或 `cmd108`（byte2 当前空）带一个 `algNum`，让**单/多车算法选择一致**（DistributedOps 复用 fullpath 的算法 chip）。这是**协议变更/接口建议，需两端同步**。
- 若 122 固定算法 → App 多车就不显示算法选项（接受不一致）。请 A 定。

### Q6.2 复核【distributed 本机 localhost 互斥】

`docs/safety-flow-review.md` §2 发现：ControlPage 在 distributed 模式仍直连保活 master（`connectTo(this.ip)`），而 master 也有自己的 agent（其 udp2lcm 的唯一 localhost 客户端）→ 平板 + master-agent 两个客户端抢 master 的单 client 记录 + 3s 急停。
- **请 A 确认**（呼应 Q6.2）：distributed 模式下 master 是否应**仅由其 agent 独占** localhost、**平板不直连任何车**（含 master）、全经黑板？若是，App 将在 distributed 模式去掉对 master 的直连保活（改纯黑板）。

### Q12【关键·真机建图渲染】地图文件格式确认（已据源码自查，请 A 复核）

App 自查 `Navi/map/MapServer.cpp::saveProbMap`（详见 `docs/map-pipeline.md`）：`defultMap.txt` = 首行 `range resolution height width metersPerPixel x0 y0`（**7 值**）+ **空格分隔** `-1`(障碍)/`0`；`defultMap.txt.txt` = 同首行 + **密排** `1`/`0`。**之前 App 误按"4 值首行 + 密排 0/1"解析 → 真机首行取到 `x0 y0`(负) 当行列 + 空格数据被当密排 → 空气图/渲染乱。已修**（按位置取 `parts[2]/[3]`、自动识别空格/密排）。
1. **请 A 确认**该格式（尤其 `defultMap.txt` 是**空格分隔 -1/0**、首行 7 值），以及 App 应长期拉 `defultMap.txt` 还是 `.txt.txt`（App 已能解两种，建议统一 `defultMap.txt`）。
2. **`x0 y0` 的单位与符号**：是"栅格 [0][0] 相对世界原点的偏移"吗？单位是**米**还是**格**？符号约定？——用于**定位校正**：心跳世界坐标 → 地图数组坐标需减 x0/y0（见 `map-pipeline.md` §5）。真机若机器人 pin/选点整体偏一个常量即此因，需此信息标定。

---

## 2026-06-08 · 紫派 → App：基于当前 `main` 代码的答复

> 下面结论按当前 `main` 分支里的 `purplepi-control/` 源码确认；其中“建议”表示还没有在 `main` 代码中落地，需要两端定协议后再改。

### A1：协同避障暂停状态

当前 `main` 的紫派代码没有把协同避障/暂停状态暴露到 9 字节 UDP 心跳，也没有合入 `COOP_AVOID` 状态机；`udp2lcm` 只回传 `0x03 + x/y/sita`，`byte[1..2]` 仍是 0。因此 App 现在无法可靠区分“普通停止”和“被另一车暂停”。

建议先让车载 agent 读本机协同状态并写入 `FleetMission.robots[].status`，不要先改 9 字节 UDP；如果后续不用 agent，也可以把 `heartbeat[1]` 定义为状态字节，但这需要 `Navi -> udp2lcm` 增加状态来源，属于协议变更。

### A2：协同避障支持车辆数

当前设计和代码语义只覆盖双车：`robotId=1` 与 `robotId=0`，同时请求时按“1 继续、0 等待”的固定优先级处理。超过两台车时没有全局调度、队列或多车优先级，不能直接复用；若要扩到 N 车，需要把 `robotId`、目标点、占用走廊和停机请求改成带序号的多车表，并由 agent/黑板统一仲裁。

### A3：`roadFile.txt`

确认：`roadFile.txt` 是紫派内部覆盖路径文件，App 不需要读取。它由主机覆盖规划写出，当前工作目录在机器人脚本里是 `/data/test/` 时，落地为 `/data/test/roadFile.txt`；子机会在 `cmd124` 后用 `wget http://<master>:8000/roadFile.txt` 拉到本机同一路径。

文件内容是一行一个网格点：`x,y`，单位是地图栅格/5cm 单元，不是米。App 只有在想做“覆盖路径预览”时才需要解析它。

### A4：软总线传地图后的落地路径

方案 A 若由 agent 经软总线传整图，紫派侧应写入 `/data/test/defultMap.txt`。写文件本身不会让 `Navi` 立刻热加载地图；写完后仍需要触发当前加载流程，例如 App/agent 继续下发对应加载地图命令（现有协议里的 `cmd5/10` 路径），或者在紫派侧新增“写完即发布加载命令”的 agent 动作。

### A5：设备发现

`udp2lcm` 当前 bind `0.0.0.0:5001`，理论上能收到发往本机 5001 的单播/广播报文；但第一包会记录 `clientIP`、启动心跳线程并进入 3 秒看门狗，所以不能直接把现有控制命令当发现包用。

同意新增轻量发现命令，例如 `0x06 discovery ping`。实现要求是：收到发现包只回发现响应，不更新 `clientIP`，不启动心跳线程，不发布 LCM，不改变受控状态。这样未被选中的车不会因为发现广播进入“已连接但 3 秒无控制包”而急停。mDNS 也可行，但对当前 C 代码改动更大。

### A6：车载无界面 agent 与 UDP 端口

当前 `udp2lcm` bind `INADDR_ANY:5001`，agent 可以向 `127.0.0.1:5001` 发本机 UDP；不需要为了 localhost 另开端口。注意它只保存一个 `clientIP` 并按这一个客户端发心跳，所以 distributed 模式下必须保证“每台车只有本机 agent 是 UDP 客户端”，平板不要同时直连同一台车的 5001。

车载无界面 ArkTS agent 与 C/C++ 栈并存从接口上可行；地图先走方案 B（agent 发 `cmd105`，子机 `cmd124` 去 master `wget`）也 OK，改动最小。

### A6.2：distributed 模式下 master 是否由 agent 独占

确认建议这样做：distributed 覆盖阶段，master 和 sub 都应由各自 agent 独占本机 UDP，平板只写软总线黑板，不直连任何车，包括 master。建图/单车手动控制阶段仍可以由平板直连某一台车；进入 distributed 前应释放直连保活，避免平板和 master-agent 抢 `udp2lcm` 的单客户端记录。

### A7：建图实时预览

当前对 App 暴露的是保存后的整图 HTTP 文件；`MAPFILE` 和 `SERVICE_COMMAND` 仍是紫派内部 LCM，没有跨到 App。若做实时预览，建议优先做 HTTP 周期快照，例如 `/data/test/partialMap.txt` 或 `/data/test/defultMap.partial.txt`，App 轮询即可；这比 UDP 分片少一层重传/乱序处理，也更贴近现有 8000 文件服务。

如果追求更实时，再考虑把 `MAPFILE` 分片桥接成 UDP/软总线分片，但那是新协议。

### A8：地图就绪信号

短期 App 在 `cmd2` 结束建图后主动拉图可以先用。当前紫派没有 UDP 级“地图保存完成”确认，靠文件大小阈值确实不稳。

建议后续定义明确状态：优先由 agent 监听/判断本机保存结果后写 `FleetMission`；如果继续走裸 UDP，可以复用心跳保留字节或新增回包表示 `mapping/saving/map_ready/map_error`。在这个状态落地前，App 不应把固定文件大小当成唯一就绪条件。

### A9：软总线互信

用户确认的方案可行：账号无关 PIN 认证、同一 WiFi 局域网，平板作为发起方，紫派作为接受方；紫派配网期接 HDMI 显示器和鼠标，在系统 PIN 弹窗中确认。关键约束是车载 agent 的 `bundleName` 要与平板一致，使用 `com.example.carapp`，否则 `distributedDataObject` 同步可能不在同一应用域内。

紫派侧仍需真机确认三点：系统 PIN 弹窗是否确实可点、接受绑定时目标包/agent 是否必须在运行、以及被发现和接受绑定所需权限。

### A10：`cmd105` 主机 IP 字节布局

可以接受“连续 `byte[1..4]`”作为更清晰的新布局，但当前 `main` 代码仍按旧布局 `[1],[2],[4],[6]` 解析并下发到 `Navi`。因此近期联调请继续按旧布局打包；如果要改成连续字节，必须 App/agent 和 `udp2lcm.c` 同时改，并同步更新 `udp-protocol.md`。

### A11：多车覆盖算法选择

当前 `LCM127` 支持 `algNum`，`Navi/main.cpp` 的 `case 127` 会读取 `iparams[0]` 并调用 `NAVI_SetPlanFullPath(algNum)`。但多车覆盖这条链路不支持选择算法：`case 122` 只读取矩形顶点，`case 123` 当前固定调用 `NAVI_SetPlanFullPath(2)`，而 `CreateFullPath` 是固定矩形牛耕/分段逻辑。

所以 App 多车界面暂时不要显示算法选择。若要保持单车/多车一致，需要给 `cmd107/108` 或 LCM122/123 增加 `algNum` 字段，并把 `CreateFullPath` 或后续路径生成逻辑改成真正使用该参数。

### A12：地图格式与 `x0/y0`

确认 `defultMap.txt` 的当前格式：首行 7 个值 `range resolution height width metersPerPixel x0 y0`，后续是空格分隔的 `-1/0`，其中 `-1` 表示障碍，`0` 表示非障碍。`defultMap.txt.txt` 是同样 7 值首行，但数据区是密排 `1/0`。建议 App 长期统一拉 `defultMap.txt`，`.txt.txt` 只作为兼容格式。

`x0/y0` 单位是米，含义是地图栅格左下角/最小 `x,y` 的世界坐标，不是建图起点，也不一定为正。代码中的世界坐标转栅格公式是：

```text
grid_x = (world_x - x0) / metersPerPixel
grid_y = (world_y - y0) / metersPerPixel
```

若 App 心跳坐标使用 5cm 栅格单位，则需要先把 `x0/y0` 除以 `metersPerPixel` 转成格，再做偏移；反向从地图点下发目标时，也要先用 `world = grid * metersPerPixel + x0/y0` 还原到世界米制坐标，再转成 UDP 使用的 5cm 单元。

---

## 2026-06-10 · App → 紫派：A 接口文档 §9「地图首行取末两个」请订正（非阻塞·文档一致性）

> 起因：A 在 `purplepi-control` 分支 `接口功能与对接问题说明.md` **§9**（commit `fb0bafe`）写道
> 「地图首行**建议 App 继续按"取首行最后两个整数作为 height/width"解析**」。这与 **A 自己的源码 + 上面 A12 矛盾**，
> App 据源码核对后**不采纳"取末两个"**，特此说明并请 A 订正其文档（避免后人照此再踩坑）。

- **App 实拉的 HTTP `defultMap.txt` 首行 = 7 值** `range resolution height width metersPerPixel x0 y0`
  —— A 源码 `Navi/map/MapServer.cpp::saveProbMap` 逐字段实证：`outFile << range<<' '<<resolution<<' '<<height<<' '<<width<<' '<<metersPerPixel<<' '<<x0<<' '<<y0`（A12 亦确认）。
- 故 **height/width = `parts[2]/[3]`（第 3、4 位）**，**"末两个" = `x0/y0`（世界偏移、常为负）**。若 App 真"取末两个"
  会把 x0/y0 当行列 → 空气图（**正是 2026-06 真机渲染 bug 根因**）。**App 已按位置 `parts[2]/[3]` 解析，与源码一致，不会改回"取末两个"。**
- 推测 §9「取末两个」实指 **LCM `MAPFILE` 分片头**（按 `range resolution height width` **4 值**，末两个恰为 height/width），
  **与 App 拉的 7 值 HTTP 文件是两条不同路径**。**请 A 把 §9 订正为**：HTTP `defultMap.txt` 按位置取 `parts[2]/[3]`；"取末两个"仅适用于 4 值分片头（若该路径仍在用）。
- **结论·非阻塞**：App 解析对 2 值(fixture)/4 值/7 值首行均鲁棒（按位置取 + 数据推断回退），**无需 A 侧代码改动**，仅请订正文档表述。
- **✅ 补注（2026-06-10）**：A 同日推送 `purplepi-control/README.md`（`main` `0db78ed`）已写明「解析首行 7 字段，`height/width` 固定取第 3、4 字段」——**与 App 一致**，本 §9 实已被该 README 取代，仅建议 A 删除分支旧文档 §9 的「取末两个」残留表述。

---

## 2026-06-10 · ⚠️ 待 A：`0x06` 发现 ping「README 已写、udp2lcm 代码未实现」

> 核对 A 2026-06-10 README 时发现的**文档≠代码**，避免 App 照 README 切换后发现失效。

- README §1/§5 称「收到 `0x06` 发现探测时立即回 9 字节发现响应，不建立心跳会话，也不触发导航命令」。
- 但**实际 `NewWheelCtrl/udp2lcm/udp2lcm.c::parseCmd` 无 `buffer[0]==6` 分支**——实现的是 `0..5` 与 `'f''g''h''i''j''k''l'`，其余落 `else`；
  且 `udp.c` 收包即**无条件** `parseCmd`，首包按 README 还会记 clientIP + 起心跳 + 武装 3s 看门狗。
- 故 App **暂不能切到 `0x06` 发现**（发出去无专门响应；即便有也可能仍被当控制首包武装急停）→ **App 维持 `cmd0` 广播兜底**（`RobotTransport.discover`，与原 Q5 一致）。
- **请 A**：在 `parseCmd` 加 `buffer[0]==0x06` 分支，**只回发现响应、不记 clientIP / 不起心跳 / 不发 LCM / 不武装急停**；并**定 9 字节发现响应字节布局**
  （建议沿用 Q5：byte0=`0x06`、byte1=`robot_id`、byte2=状态、byte3..=可选位姿）。实现 + 同步 `udp-protocol.md` 后，App 改 `discover` 的 ping 码一行即可切换。
- **✅ 已解决（2026-06-10 晚）**：A 同日 `8b4af4b` 已在 `udp.c` 实现 `0x06`（`isDiscoveryPing`→`sendDiscoveryResponse`，只回包、不武装急停）；App 已切 `RobotCommand.discoveryPing=0x06`（见上「设备发现」）。

---

## 2026-06-10 · ✅ 建图根因：cmd0「有图时只当心跳」→ App「开始建图」改发 `'m'`/0x6d

> 真机现象：开始建图后反复拉到/渲染**同一张旧图**（建图前那张）+ 结束建图后**坐标漂移**。逐层定位 = **接口语义没对齐**（非 App 渲染、也非 A 写错）。

- **根因**：A 2026-06-10 `8b4af4b` 把建图分两档，**README §四已列、App 此前漏登记**：
  - `cmd0`（App 旧「开始建图」）→ LCM30 `niparams=1`、`forceNewMap=false` → `Navi.createMap` 在**已有图/导航态时直接忽略**（日志 `Ignore create map ... force=0`）、**沿用旧图**；
  - `'m'`/109（A 新增「Force Create Map」）→ LCM30 `iparams[1]=1`、`forceNewMap=true` → **清空旧图、从头建**。
- **现象链**：cmd0 被忽略 → Navi 一直用上次旧图 → 摇杆在旧图上定位（**漂移**）→ cmd2 存的还是旧图 → App 反复拉到同一张旧图。
- **为何排除 App 渲染**：App `usingCache:false`、每次真 GET（log 每次 `RespCode:200` 实传）、`mapText` 为 `@State` 变即重绘——无「反复渲染缓存旧图」机制。
- **App 已改（统一落地）**：`RobotCommand.forceCreateMap=0x6d`；`ControlPage.startBuild` 改发 `'m'`（取代 `cmd0`）；`udp-protocol.md` 命令表补 `0x6d` + 修正 cmd0 语义 + cmd5 归零；app↔agent `protocol.ets` 逐字节同步。
- **顺带统一的不一致**（A README §四 vs 旧 contracts）：① cmd0 语义 = 「心跳/兼容建图，有图时只当心跳」；② cmd5 初始位姿 = **归零 (0,0,0)**（旧 contracts 误写"取当前位姿"）。
- **建议 A**：在 README/接口说明里**显式标注「App 开始建图请用 `'m'`、cmd0 不保证新建图」**，避免下个对接者再踩。

---

## 2026-06-11 · 视觉/视频流对接（→ 成员B / 香橙派）· App 侧 V1–V4 已实施

> App 已按 `contracts/vision-stream-api.md` v1.0 对接香橙派视觉（`docs/vision-integration-plan.md`，分支 `app-harmony-core`）。
> 以下为对接中需 B 确认/统一的问题（来自 plan §6），异步处理即可：

1. **🔴 关键点 index ↔ name 顺序不一致（B 的 README 已自曝）**：契约表 §5 记 `0=center,1=pointer_tip,2=zero_mark,3=full_mark`，
   但部署模型实际输出顺序 `0=pointer_tip,1=center,2=zero,3=full`。**App 已一律按 `name` 取点（绝不按 index）**，不受影响。
   **请 B**：把契约表 §5 的 index 与模型真实输出对齐（或在契约里显式标注"按 name、index 仅示意"），免后人踩。
2. **WS 背压**：App 慢时服务器是**积压旧帧**还是**只推最新**？App 端已做"只显最新帧 + 解码进行中丢帧"，
   但若服务器侧也积压，端到端延迟仍会累积。**请 B 确认服务器推送策略**（最好只推最新 / 有界队列）。
3. **关键点 / bbox 参考分辨率**：坐标是相对**该 JPEG 帧像素**还是摄像头原始分辨率？App 最简方案只显已叠加的图、不自绘，
   故暂不受影响；若 App 后续自绘叠加需按帧分辨率对齐——**请 B 在契约注明坐标参考系**。
4. **样例 fixtures**：**请 B 在 `contracts/fixtures/` 放一段样例**（几帧 JPEG + 对应 `frame_meta` JSON），供 App 离线对接联调
   （现 App 用自建 `tools/mock-orangepi` 顶替，真实样例能校准）。
5. **读数物理量纲**：`gauge_angles` 是占比百分比；App 默认显示 `%`，可经 `/api/gauge/configs` 量程换算物理量（MPa 等）。
   **请 B 确认** `/api/summary` 的 `readings[].unit/value` 在已配置量程时是否已是物理量（避免双重换算）。
6. **报告暂停推理**：`POST /api/data/report`（及 `/api/llm/query`）期间摄像头停推、视频会停——App 已给"生成中、视频暂停"提示；
   **请 B 确认**生成期间 WS 是断开还是保持（仅无帧）、完成后是否自动恢复推流。
7. **香橙派寻址**：IP 静态 `192.168.1.5`，App 做成可配置（持久化，默认此值）。是否需要发现（mDNS）/将来多香橙派多摄像头？暂记。
8. **鉴权/并发**：`/ws/video` 是否限连接数？多端（平板 + 调试 PC）同时连是否互相影响？暂记。

> ⚠️ 注：此条目落在 `app-harmony-core`，按 [[feedback-docs-main-code-branch]] 应 cherry-pick 同步到 `main`。
