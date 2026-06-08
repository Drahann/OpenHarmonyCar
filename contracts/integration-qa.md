# 集成对接 Q&A（App ↔ 紫派，跨端异步备忘）

> 本文给**两侧的人和 AI 异步对接**用：App 侧（owner，`app-harmony/`）与紫派侧（成员A，`purplepi-control/`）。
> 谁有结论/问题就追加带日期的小节，**改了接口先改 `contracts/` 再改两侧代码**（CLAUDE.md 约定）。
> 紫派侧权威接口说明见 `purplepi-control/接口功能与对接问题说明.md`；本文是 App 侧的对账回应 + 开放问题。

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

## 2026-06-05 · App → 紫派：开放问题（请 A 侧确认/答复）

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

## 2026-06-08 · App → 紫派：接口优雅性复审（R1）产出的开放问题

> 来自 [`interface-review.md`](interface-review.md)（R1 复审）。R1 结论：协议骨架优雅、对齐干净，唯一值得两边一起改的是下面 Q10（非阻塞）；
> 其余脆点（发现 ping、心跳状态、地图就绪）已分别在 Q5/Q1/Q8 跟踪，地图首行"取末两个"是 A 在《接口…说明》§7 主动建议的方案、非妥协。

### Q10【建议·低优先·协议小改】cmd105 主机 IP 改**连续字节**打包

**现状**：cmd105('i') 把主机 IP 四段塞进**散列**字节 `byte[1],[2],[4],[6]`——根因是复用了 `endX/endY` 两个大端 int16 字段，
单字节 IP 落在它们的低字节（[4]/[6]），高字节 [3]/[5] 恒 0 空着（见 `udp-protocol-crosscheck.md` §五）。编解码两端都得记住"跳 [3][5]"，易错。
- **提议**：cmd105 不复用 int16 字段，IP 四段放**连续** `byte[1..4]`（[5][6] 留 0）。紫派改 `iparams[0..3] = byte[1],[2],[3],[4]`，App/agent 同步改打包。
- **优先级**：**非阻塞**——现状能工作。A 哪天动 cmd105 解析时顺手改即可；改后回写 `udp-protocol.md`（协议变更，需两端同步）。
  新写的车载 agent 触发 cmd105 时按最终布局实现（暂先按现状 [1,2,4,6]）。
- **请 A**：接受（顺手改）/ 维持现状（维持则 App 与 agent 继续按 [1,2,4,6] 打包，不阻塞）。
