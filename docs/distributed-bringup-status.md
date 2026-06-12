# 分布式软总线真机点火现状（调试中）

> **日期 2026-06-13** · 状态：**分布式覆盖的"代码对齐"已完成**（见 [`contracts/app-purplepi-alignment-audit.md`](../contracts/app-purplepi-alignment-audit.md) + A 的 `docs/plan6-app-purplepi-interface-change-plan.md` + 紫派 cmd108 修复 `eef8afe`），
> 现在卡在**真机把软总线 DDO 黑板真正联通**这一步。本文专记这条线的调试进展、已修项、当前阻塞与判据，供接续。
> 真机调试总入口仍是 [`docs/debug-checklist.md`](debug-checklist.md)。

## 0. 一句话现状

平板 ↔ 紫派 agent 的 **`distributedDataObject` 黑板同步建不起来**：双方已可**发现**、可**绑定(PIN)**、**`DISTRIBUTED_DATASYNC` 权限也已授**，
但 DSoftBus 报 **`onlineDev count = 0`（彼此不在线）** → 黑板不同步 → 平板下指令 agent 收不到、车不动、地图无车图标。
**当前首要怀疑 = WiFi/热点的「AP 隔离 / 客户端隔离」挡了设备间 P2P 连接。**

## 1. 完整链路 & 卡点

```
平板 App ──(软总线 DDO: FleetMission 黑板)── 紫派 agent ──(本机 UDP 5002→5001)── udp2lcm ── Navi/轮控
            ↑ 🔴 卡在这：DDO 联不通(onlineDev=0)      ↑ ✅ 已修(端口/权限/门控)   ↑ ✅ 已对齐(cmd108 等)
```

- **导航 / UDP 链路**：已逐字段对齐（详见 audit）；A 已改 `cmd108`（master/sub 都 build 122）。
- **软总线链路**：代码对齐 + 多处真机修复后，**仍卡在 DDO 的"设备在线连接"**。

## 2. 已查明 & 已修（真机逐个排掉的坑）

| # | 坑 | 现象 / 证据 | 处理 |
|---|---|---|---|
| 1 | **agent 在无头紫派活不下来** | UIAbility 启动后 ~3s `App exit code:0`；一堆 ACE/显示报错(`DisplayManager nullptr`、`EACCES`、缺 GPU 库) | 现象=需常驻；**调试期接 HDMI 让前台存活**；长期需紫派系统侧常驻(Q6.1) |
| 2 | **本机 UDP 端口互斥** | agent 绑 `0.0.0.0:5001` 与 udp2lcm 抢端口 → 平板建图/直控丢包；udp2lcm 心跳写死回 `clientIP:5001` → 回环到自身(0x03 当 cmd3) | agent 改绑 **`AGENT_LOCAL_PORT=5002`** + **仅任务期作 udp2lcm 客户端**(空闲/建图 UDP 静默)；**🔴 待 A 改 udp2lcm 心跳回"客户端源端口"**(spec 见 `contracts/integration-qa.md` 2026-06-12 条) |
| 3 | **X4 用了紫派没有的 `@ohos.net.http`** | 真机 `http.abc not existed` / `LoadNativeModule @ohos:net.http failed` | X4 退回**固定延时**(移除 http 轮询；agent 无法 HTTP，正解是 A 给 cmd124 完成信号) |
| 4 | **🔑 agent 从不申请 `DATASYNC` 权限** | agent `FleetMissionService` 注释"不做交互式申请、须预授权"，但紫派没预授 → `ObjectStore permission:-1` + `29360174` + `single device` | **agent `init` 改为主动 `requestPermissionsFromUser(DATASYNC)`**（当前 UIAbility 壳可弹框）+ 打印 `authResults`；`AgentCore.start` 串行化 `init → join`(否则 join 在 DDO 建好前跑会被静默跳过) |
| 5 | **互信"绑了却找不到、无法解绑"死循环** | 绑成功但 `getAvailableDeviceListSync=0`（它只列**在线**可信设备）→ UI 找不到 → 无法解绑再绑 | DeviceTrustPage 给**发现列表**每行加「**解绑重置**」(`unbindTarget` by id)；并认清"**互信页那个 0 是假信号**、只列在线设备、别拿它判 bind 成败" |

> **权限这道坎已实锤修复**：真机两端 `权限请求结果 authResults=[0]`、紫派 `ObjectStore permission` 从 **-1 → 0**。

## 3. 🔴 当前阻塞：`onlineDev count = 0`（设备在软总线里彼此不在线）

最新真机日志（2026-06-13 01:11，平板与紫派两端都出现）：
```
[NAdapt][SearchOnline] onlineDev count = 0
[Syncer] EnableAutoSync no online devices  /  no online standard devices
平板: GetDeviceList Collaboration deivces size:0     紫派: GetDeviceList ... 29360174 / single device
```

含义：DATASYNC 有了、能发现、能绑定，但**两台机器建不起在线 P2P 连接** → DDO 没有对端可同步。

**首要怀疑 = WiFi/热点「AP 隔离 / 客户端隔离」**：它**只挡设备间 P2P、不挡广播** → 完美解释"能发现 + 能绑定、但 `onlineDev=0`"。手机热点 / 企业 / 公共 WiFi 默认常开此项。

**待验证 / 排查（按优先级）**：
1. **换一个无隔离的普通家用路由器**（关掉 AP isolation / 客户端隔离），平板 + 紫派都连它再试。
2. 确认当前**确实处于已绑定态**（别在「解绑重置」后忘了重新「配对」→ 那样 `onlineDev=0` 是因为没信任，不是网络）。
3. 若换网 + 确认绑定后 `onlineDev` 仍为 0 → 再深挖 DSoftBus / 账号层（账号无关 PIN 信任在**同一无隔离 LAN** 上本应能在线）。

## 4. 判据速查（怎么算"通了"）

- 紫派 agent 日志：`GetDeviceList Collaboration deivces size:` **0 → ≥1** 且 **`onlineDev count > 0`**；
- DDO `status` 事件出现 **`peer <networkId> online`**；
- 平板划覆盖区域 → 紫派出现 **`黑板更新: phase=covering … → 产 N 条命令`** + `TX→ 127.0.0.1 …`；
- 平板出现第二个 toast「**车N：agent 已下发覆盖/执行命令**」+ 地图上车变色 / 移动。

## 5. 待办

- **网络**（当前首要）：排掉 AP/客户端隔离。
- **A**：`udp2lcm` 心跳回**客户端源端口**（否则覆盖时 agent 收不到位姿 + 回环；spec 见 `integration-qa.md`）。
- **长期**：agent 从 UIAbility 壳换**常驻 ServiceExtension / 系统应用**（Q6.1，需紫派系统侧）；届时 headless 弹不出框，`DATASYNC` 走**预授权**。
- 真机互信页 UX：绑成功后记本地"已配对"、显示 DDO 在线状态（待做）。

---

> 调试期所有 App/agent 改动落 `app-harmony-core`；本文（docs）按约定同步 `main`。
