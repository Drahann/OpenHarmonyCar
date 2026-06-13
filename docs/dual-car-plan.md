# 方案A 实施单 · 平板直连双车 UDP（中央协调器）

> **状态**：2026-06-13 立项 → **App 侧 P1–P4 code-complete（未经 DevEco 编译，待真机）**。
> **设计取向（用户定）**：方案A 与 DDO 路**并存**——`distUseAgent=false`（默认）走方案A 平板直连双车；
> `distUseAgent=true` 走原 DDO 软总线黑板。**DDO/软总线/互信层全部保留**（DeviceTrustPage、FleetMissionService
> 等不删）：① 写创新点要用；② DDO 只因当前硬件凑不齐蓝牙/超级终端/账号而搁置，**功能并非不兼容**，留待日后启用。
> **单一事实源**：本文。接续开发前先读它 + `contracts/udp-protocol.md` + `contracts/multi-robot-collab.md`。
> **决策背景**：DDO 软总线、LAN socket 黑板两条路均**搁置**（工业紫派 + 消费平板凑不齐蓝牙/超级终端/账号链路；
> LAN 又把简单问题复杂化）。LAN 实现代码已 `git reset --hard` 回退到 `bd6c33e` + 只保留地图坐标修复 `997edf0`
> （force-push `app-harmony-core`；旧 HEAD 留本地 ref `backup/pre-lan-rollback-5ce70e3`）。

---

## 1. 核心思想

**平板做中央协调器，两辆车都是哑终端。** 平板对**每辆车各发 9 字节 UDP**（端口 5001），用的全是
现有单车协议；车端**无 agent、无软总线、无互信、无显示器**。这不是新协议——它复用 `contracts/` 里早已
对账过的「方案B 车间拉图」机制，只是把触发方从"车上 agent"换成"平板直发"。

```
                平板 (ControlPage = 中央协调器)
                 │   一个 UDP socket，多目标 sendto + 按源 IP 路由心跳
     ┌───────────┴────────────┐
     │ UDP :5001              │ UDP :5001
     ▼                        ▼
  主车 Purple Pi 1         从车 Purple Pi 2
   ├ 建图 'm'/cmd2          ├ cmd105 拉主机图（紫派 cmd124 wget 主车:8000）
   ├ 心跳/位姿              ├ cmd5 加载图 + 位姿归零(0,0,0) 到主车原点
   ├ 覆盖 107/108(robotId0) ├ 心跳/位姿
   └ HTTP :8000 供图        └ 覆盖 107/108(robotId1)
```

**关键不变量**：两车共享 **master 坐标系**。从车 `cmd5` 把初始位姿强制归零到主车建图起点，因此
平板在主车地图上划的区域顶点，对两车都是同一套坐标，直接下发即可（无需坐标变换）。

---

## 2. 现状盘点（重要：大半已就位）

回退后的 `app-harmony-core` 代码里，方案A 需要的多数能力**已经存在**，省去大量工作：

| 能力 | 现状 | 文件 |
|---|---|---|
| 多目标 UDP transport | ✅ **已完成**：per-IP `heartbeats` map、`startHeartbeat(ip)`/`stopHeartbeat(ip?)`/`setHeartbeatPayload(ip)`、按 `remoteInfo.address` 路由收包、`connStateOf(ip)`/`msSinceLastSeen(ip)` | `service/RobotTransport.ets` |
| 建图发 `'m'`/0x6d | ✅ **已正确**：`startBuild` 发 `forceCreateMap`（不是 cmd0） | `ControlPage.ets:421` |
| 划区域 → cmd107/108 | ✅ **已有**：`sendDistArea(ip,c1,c2,robotId)` 发 107(对角点1)+108(对角点2,byte1=robotId) | `ControlPage.ets:441` |
| 按 carId upsert 区域 + 选车 UI | ✅ **已有**：`assignDistArea`、`distActiveCarId` 选车 chip、robotId=carId≤1?0:1 | `ControlPage.ets:450` |
| 平板直发兜底路径 | ✅ **已有**：`distUseAgent=false` 时走 `sendDistArea` 直发（≈ 方案A 雏形） | `ControlPage.ets:472` |
| 按源 IP 渲染多车位姿 | ✅ **已有**：`onMessage` 按 IP 找 `RobotRuntime` 更新 x/y/r | `ControlPage.ets:181` |
| cmd105 IP 打包 byte[1,2,4,6] | ❌ **缺**（原计划由 agent 做） | 待加 `protocol.ets`/transport |
| 从车拉图编排（105→延时→5） | ❌ **缺**（原计划由 agent 做） | 待加 `ControlPage.ets` |
| 多车连接生命周期（distributed 直连双车） | ❌ **当前 distributed 模式刻意不直连任何车**（委托 agent） | 待改 `ControlPage.ets:153-168` |
| 设备发现选主/从车 | ⚠️ 有 `discover()`，但选主+加从的 UI 流程待补 | `HomePage.ets`/`ControlPage.ets` |
| DDO/软总线/互信层 | ✅ **保留**（不删，供创新点 + 未来启用）：`distUseAgent=true` 时仍走 joinSession/黑板 | `FleetMissionService`、`DeviceTrustPage` |

> 结论：方案A 的 App 工作量集中在 **(a) `distUseAgent=false` 设为默认并据此门控连接、(b) 让 distributed 模式直连双车、
> (c) 补从车拉图编排、(d) 在页内连接从车**。线格式与覆盖 UI 基本现成；DDO 层原样保留、仅默认不走。

---

## 3. 协议复用（全部已在 `contracts/`，紫派已实现，无需新增命令）

| 阶段 | 命令 | byte 布局 | 紫派动作 |
|---|---|---|---|
| 主车建图 | `'m'`/0x6d `forceCreateMap` | byte0 | LCM30+iparams[1]=1 清旧图强制重建（cmd0 有图时只当心跳，**不能用**） |
| 主车遥控 | `1` pending | byte1=方向 byte2=速度 | wheel_ctrl |
| 主车结束建图 | `2` afterEnd | byte0 | 停轮控+存图(32)+加载导航图(10) |
| 从车拉图 | `105`/`'i'` distributed | **主机 IP 四段 = byte[1],[2],[4],[6]** | cmd124 `wget http://<主机IP>:8000/defultMap.txt`+`roadFile.txt` |
| 从车加载图 | `5` loadMap | byte0 | 加载导航图(10) + **位姿归零 (0,0,0)** 到主车原点 |
| 覆盖对角点1 | `107`/`'k'` distAreaCorner1 | byte3-6=坐标 | 暂存 |
| 覆盖对角点2 | `108`/`'l'` distAreaCorner2 | byte3-6=坐标 byte1=robotId(0主/1从) | 规划 FullRoad 覆盖(122)+分布式跟踪(123) |
| 心跳 | 紫派→App | byte0=3 带位姿；byte3-8=x/y/r | 每 500ms 一帧 |
| 设备发现 | `0x06` 广播 | — | 紫派只回 9 字节(`[0]=0x06`)，不记 client/不起心跳/不武装急停 |

坐标单位：**1 整数 = 1/20m = 5cm**，UDP 整数与地图格 1:1。划区域顶点用保留的 `model/geometry.ets`
`canvasToMap`（返回紫派世界坐标、取整给 UDP），直接作 cmd107/108 的 endX/endY——**这正是回退时
专门保留 `997edf0` 的原因**。

---

## 4. 端到端流程（方案A 目标态）

1. **发现 & 选车**：平板广播 `0x06` 发现 → 列出在线车 → 用户**选一辆作主车（建图车）**；可**再加一辆作从车**。
   两车 IP 都记入 `mission.robots`（carId：主=1、从=2）。
2. **建图（仅主车）**：平板对主车发 `'m'` 开始建图 → 摇杆遥控扫一圈 → `cmd2` 结束建图 → HTTP `:8000` 拉图显示。
   **全程只动主车**；从车此时空闲（平板对它发中性保活帧维持连接、防 3s 急停）。
3. **从车入场拉图**：用户点「让从车加入」→ 平板对从车发 **`cmd105`（打包主车 IP）** → 紫派 cmd124 wget 拉图 →
   **固定延时若干秒**（无 ack）→ 平板对从车发 **`cmd5`** 加载图并归零到主车原点。
4. **划区域 & 覆盖**：平板在主车地图上为每辆车各划一块矩形（选车 chip 切换目标车）→ 对该车 IP 发
   `cmd107`+`cmd108(byte1=robotId)`。主车 robotId=0、从车 robotId=1。
5. **监控**：平板按源 IP 同时收两车心跳 → 地图上同渲两车位姿/朝向/进度。两车保活循环各自独立（各 ≥1s 一帧）。

---

## 5. 分阶段实施（建议顺序，每阶段可独立编译/真机验证）

> 开发遵循 [[feedback-fresh-context-for-big-tasks]]：本文沉淀后 `/clear`，在新上下文照此实现。
> 代码留 `app-harmony-core`（[[feedback-docs-main-code-branch]]）。

### P1 — 默认切方案A + 门控 DDO（保留不删）✅ 已实现
- `distUseAgent` 默认改 **false**（方案A 为默认活动路径），注释标明 DDO 路保留供未来启用。`ControlPage.ets`
- `initConnection`：DDO 的 `joinSession`/`subscribeMission` 仅在 `mode==distributed && distUseAgent` 时执行；
  否则（方案A/单机）平板直连主车保活。`FleetMissionService`/`DeviceTrustPage`/`MissionSnapshot`/`onRemoteMission` **全部保留**。
- `assignDistArea` 的 `distUseAgent` 分支保留：false→平板直发 107/108，true→写黑板（原样）。
- **验收**：方案A 默认不触发任何软总线/DATASYNC 调用（remoteListener 恒 null，aboutToDisappear 不调 DDO 清理）。

### P2 — distributed 模式直连双车（替换"不直连任何车"）✅ 已实现
- `initConnection`：仅 DDO 模式（distributed && distUseAgent）下平板不直连；方案A 下 `connectTo(master)` 起中性保活。
- 从车经设备面板连接（onConnect 在方案A 下 `connectTo(slaveIp)` 登记为车2+保活，不拉它的图）。
  保活由 transport 的 per-IP 循环负责（每车各 ≥1s 一帧，互不影响）。
- `onMessage` 已按源 IP 路由——两车心跳各落到各自 `RobotRuntime`。
- **验收（真机）**：两车都"已连接"、各自心跳；停一辆另一辆不受影响（后者 3s 后该车自行急停=预期安全）。

### P3 — cmd105 IP 打包 + 从车拉图编排（方案A 唯一全新逻辑）✅ 已实现
- **协议层** `model/protocol.ets`：新增 `encodePullMap(masterIp): ArrayBuffer`——主机 IP 四段打进
  byte[1],[2],[4],[6]，byte0=105（专用构造器，不滥用 runState/speed）。
- **传输层** `RobotTransport.ets`：新增 `sendBuffer(ip, buffer, label)`——发预构造帧 + 显式 TX 日志。
- **编排层** `ControlPage.ets`：`joinSlave(slaveIp, carId)` = 发 `encodePullMap`(cmd105) → `setTimeout(SLAVE_PULL_DELAY_MS=8s)`
  → `sendTo(cmd5)`；`pullMapForActiveSlave()` 门控（须 carId>1、有 IP、mapReady）。UI：DistributedOps 选中从车时
  显示「让车N拉主机图（105→5）」按钮，可重复点。
- **验收（真机）**：从车拉到主车图、cmd5 后位姿归零 (0,0,0) 显示在主车原点。**延时 8s 是否够需真机调** `SLAVE_PULL_DELAY_MS`。

### P4 — 从车在页内连接（复用设备面板）✅ 已实现（最小版）
- 主车 IP 由 HomePage 路由参数带入（car1）；从车在 ControlPage 设备面板里 `discover()` 后点击连接 → 成为车2。
- 选车 chip（`distActiveCarId`）候选来自 `mission.robots`，方案A 下不依赖 agent 回报。
- ⏳ **可选 polish**（未做）：HomePage 进分布式前显式"选主+选从"向导。当前页内连接已够用。
- **验收（真机）**：无需手输即可发现并把第二辆车登记为从车；切车划区域正确落到对应车 IP。

### P5 — 真机双车端到端联调（待真机）
- 建图→拉图→双车划区域→双车覆盖→双车心跳监控全链路。
- 重点验证 §6 的坑：双车保活、cmd105 延时是否够、车间 HTTP 拉图连通、坐标系一致。

---

## 6. 关键坑（点名，避免真机翻车）

1. **建图必须发 `'m'`/0x6d** —— cmd0 有图时被 `Navi.createMap` 忽略只当心跳（→ 反复拉旧图 + 定位漂移，真机实证）。
   `startBuild` 已正确（line 421），别在重构里改回 cmd0。
2. **双车各自的 3s 急停保活** —— 紫派 3s 没收到任何指令就急停。两车后**每辆车都要独立 ≥1s 一帧**。
   transport 的 per-IP `heartbeats` 已支持，确保 P2 给每车都 `startHeartbeat`。
3. **cmd105 没有 ack** —— wget 拉图是 fire-and-forget，协议无"拉完了"回执。只能延时给够（~3MB 图 + 紫派
   SLAM 负载下 `python -m http.server` 真机实测 ~47KB/s，回退普通图可能要十几秒）。优先拉压缩图 `zipedMap.txt`
   （~6× 小）能缩短。延时后**以"从车心跳开始动/位姿归零"作兜底判据**，不要把固定延时当成功保证。
4. **车间 HTTP 连通**（新前提）—— 从车 cmd124 是**从车去 wget 主车:8000**，要求**车↔车**在同一局域网能互通
   （不止平板↔车）。换路由器/网段时验证这一点。
5. **坐标系统一** —— 划区域顶点经 `canvasToMap` 得主车世界坐标，对两车通用（从车已 cmd5 归零到主车原点）。
   别引入任何"从车坐标变换"——按构造就是同一套。
6. **单 clientIP 模型** —— 紫派 udp2lcm 只记一个 clientIP 并向它发心跳。方案A 下平板是每辆车的**唯一**直连
   客户端（无 agent 竞争），与单车手控同构，天然不冲突。**这反转了旧 Q6.2/A6.2 的"平板不直连任何车"约定**
   （那是 agent 共存时代的约束）——见 §7。

---

## 7. 对成员A（紫派）的影响 —— 待 P5 后定稿为独立交接文档

> 用户要求：**我们这边开发完后**，给 A 一份"紫派代码该怎么改"的文档。A 当前分布式逻辑仍是 DDO/agent 时代的预期。
> 此处先记录**已知 delta**，开发中如有新增再补，最终单独成文交接。

**A 几乎不用改 C++ 代码**——方案A 复用的全是他已实现并经 Q13 对齐（`eef8afe`）的 handler：
- ✅ **保留不动**：cmd105 wget 拉图（cmd124）、cmd5 加载图归零、cmd107/108 覆盖（122/123）、心跳、0x06 发现。
  这些与 DDO/agent **无关**，方案A 原样复用。

**需要告诉 A 的（主要是"撤回"和"确认"，不是"新增"）**：
1. **架构变更通知**：弃软总线/DDO/agent/互信；平板现在**直连每辆车 5001**（双车），每辆车把平板当作其**唯一**
   UDP 客户端——与单车手控同构。**因此 Q6.2/A6.2「distributed 模式平板不直连任何车」约定作废**。
2. **撤回 agent 期请求**：
   - 「udp2lcm 心跳回客户端**源端口**」(原为 agent 绑 5002 共存而提) —— 不再需要，平板就在 5001。
   - 「agent 常驻 / BOOT_COMPLETED 静态订阅 / 预装系统 hap / DATASYNC 预授权」(Q6/Q9) —— 全不需要，车上不跑 agent。
   - 软总线互信配置 (Q9) —— 不需要。
3. **请 A 确认/验证（真机双车）**：
   - 主车 `:8000` 在自身跑 SLAM/空闲时，能否稳定供从车 wget 拉图（车↔车 HTTP，非仅平板↔车）。
   - cmd105 后从车 cmd124 拉图的**典型耗时**，好让平板把延时设够；能否暴露任何"拉完/加载完"信号（可选增强）。
   - 双车同网段 :8000 互访无防火墙/隔离阻挡。

> 落地方式：开发完成后在 `contracts/integration-qa.md` 写"方案A 架构变更 → A"，并把上述 1/2/3 整理成
> 一份独立交接文档（A 通过其分支异步读）。**这是协议/架构变更，需另一端同步**（CLAUDE.md 约定）。

---

## 8. 验收判据（方案A 打通）

平板发现两车 → 选主车建图 → 拉图显示 → 让从车加入（cmd105→cmd5、从车归零到原点）→ 为两车各划一块覆盖矩形
→ 两车各自规划覆盖并移动 → 平板地图实时同渲两车位姿/进度；**全程无软总线/无互信/无 agent/无系统权限**。

---

## 9. 文件改动清单（速查）

| 文件 | 改动 | 状态 |
|---|---|---|
| `service/FleetMissionService.ets` | ✅ **保留**（DDO 黑板，distUseAgent=true 时启用；创新点 + 未来用） | 不动 |
| `pages/DeviceTrustPage.ets` | ✅ **保留**（互信页；创新点 + 未来 DDO 用） | 不动 |
| `model/protocol.ets` | ➕ `encodePullMap(masterIp)`（cmd105 IP 打包 byte[1,2,4,6]） | ✅ 已加 |
| `service/RobotTransport.ets` | ➕ `sendBuffer(ip, buffer, label)`（发预构造帧 + TX 日志）；多目标已就位 | ✅ 已加 |
| `pages/ControlPage.ets` | distUseAgent 默认 false + 门控 DDO；distributed 直连双车；`joinSlave`/`pullMapForActiveSlave`；onConnect 方案A 登记从车；DistributedOps 加拉图钮 + 重标注 | ✅ 已改 |
| `model/mission.ets` | 不动（`MissionSnapshot` 等保留供 DDO 路） | 不动 |
| `pages/HomePage.ets` | 不动（从车在 ControlPage 页内连接；显式选主/从向导列为可选 polish） | 不动 |
| `model/geometry.ets` | 不动（`canvasToMap` 已保留 `997edf0` 修复，覆盖坐标依赖它） | 不动 |
