# 分布式黑板：从 DDO 到 LAN Socket 的演进记录

> **日期 2026-06-13** · 状态：**✅ LAN Socket 黑板已实施完成**，替代 distributedDataObject (DDO)。
> DDO 因系统先决条件（蓝牙/超级终端/DSched/华为账号）在消费级平板+紫派组合上无法满足，
> 已切换为纯 LAN TCP Socket 方案（[`docs/lan-blackboard-plan.md`](lan-blackboard-plan.md)），**零系统权限、零互信依赖**。
> 本文记录 DDO 调试历程（§3 根因分析）与 LAN 黑板实施清单（§8）。

## 0. 当前状态

**✅ LAN Socket 黑板已上线**：平板 ↔ 紫派 agent 的 FleetMission 黑板传输从 DDO 切换为 **TCP Socket**（平板=客户端，agent=服务端 `:5003`）。
**不再需要**：distributedDeviceManager 互信 / distributedDataObject / 蓝牙 / 超级终端 / 华为账号 / DISTRIBUTED_DATASYNC 权限。
**只需**：同一 WiFi + `ohos.permission.INTERNET`（normal，安装即授）。机器人 UDP 5001 一直通就是铁证。
LAN Socket 只需设备间 TCP 可达（与 UDP 5001 同条件），不受 AP 隔离影响（TCP 走 AP 转发，不需要 P2P 直连）。

## 1. 完整链路 & 当前状态

```
平板 App ──(LAN TCP :5003 FleetMission 黑板)── 紫派 agent ──(本机 UDP 5002→5001)── udp2lcm ── Navi/轮控
            ↑ ✅ 已切 LAN Socket（替代 DDO）         ↑ ✅ 已修(端口/权限/门控)   ↑ ✅ 已对齐(cmd108 等)
```

- **导航 / UDP 链路**：已逐字段对齐（详见 audit）；A 已改 `cmd108`（master/sub 都 build 122）。
- **LAN 黑板链路**：✅ 已实施。平板 TCP 客户端连紫派 agent :5003，收发 mission/robot JSON。
- ~~**软总线 DDO 链路**~~：**已废弃**。根因分析见 §3。DDO 依赖蓝牙/超级终端/DSched/华为账号等系统先决条件，在此真机组合上无法满足。

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

### ~~首要怀疑 = AP 隔离~~ → 已排除（2026-06-13 日志分析）

日志深入分析后确认：**根因不是 AP 隔离，而是 DDO 的先决条件未满足**。

**平板 (MatePad) 失败链**：
```
GetAnonyLocalUdid Failed with ret 96929750  ← 蓝牙未开，无法获取匿名设备ID
CheckApiPermission: PermissionLevel: 2 → Check permission failed with ret: 96929750
RegisterDevStateCallback: System SA not have permission, ret: 96929750  ← 每~120ms重试
（ObjectStore 无法注册设备状态回调 → onlineDev=0 → DDO 不同步）
```

**紫派 (Purple Pi OH) 失败链**：
```
【早期会话 22:41】DSched 成功 → AbilityManagerService → Collaboration deivces size:0 ✅
【后期会话 00:42+】SAMGR proxy fail (accessToken error:7) → DSched fail ret 2097167 ❌
GetDeviceList Get collaboration events failed, error code = 29360174
onlineDev count = 0
```

**关键发现**：紫派 DSched **不是不存在，而是 IPC accessToken 失效**。早期会话 DSched 完全正常工作（只是返回 0 台协作设备）。后期 `IPCObjectProxy::SendRequest failed, error:7 desc:*.accessToken` 导致 SAMGR 代理查找失败。**重启紫派可能恢复**。

**两台设备共同结论**：当 DSched/DHDM 链路正常时，都返回 `Collaboration deivces size:0`。
**根因 = 没有通过蓝牙/超级终端建立分布式组网关系 → 系统不知道哪些设备可以协作**。

### DDO 先决条件（官方文档明确要求）

| 条件 | DDO 要求 | startAbility（旧App） | 当前状态 |
|---|---|---|---|
| **蓝牙** | ✅ **必须开启** | ❌ 不需要 | 🔴 未开 |
| **超级终端连接** | ✅ 控制中心"吸附"设备 | ❌ 不需要 | 🔴 未操作 |
| Wi-Fi 同局域网 | ✅ | ✅ | ✅ |
| 同华为账号 | 通常需要 | ❌ | ⚠️ 未确认 |
| DISTRIBUTED_DATASYNC 权限 | ✅ system_grant | ✅ | ✅ 已声明 |
| DSched 系统服务 | ✅ 需要 | ❌ 不需要 | 🔴 紫派缺失 |

> **核心差异**：旧 App 用 `startAbility` 走 AMS 路径，只需要同局域网 + `bindTarget`。
> 新 App 用 DDO 走 ObjectStore → DistributedDB → DSched → DHDM 长链路，每个环节都必须正常。
> **这就是旧 App 不开蓝牙、不登华为账号也能跑通的原因**。

### 下一步操作（按优先级）

1. **🔴 两台设备都开启蓝牙** → 这是 `Collaboration deivces size:0` 的最可能原因
2. **重启紫派** → 修复后期会话的 `accessToken error:7`（IPC 令牌失效）
3. **平板：控制中心 → 超级终端 → 吸附紫派设备**（建立分布式组网关系，让系统识别协作设备）
4. 确认两端**登录同一华为账号**（如果 MatePad 要求的话）
5. 以上全部满足后，启动 App → 观察日志是否出现 `Collaboration deivces size:≥1` 和 `onlineDev count > 0`
6. 若以上全做了仍不通 → 排查 AP 隔离（§3.2）或考虑回退到 startAbility 方案

## 3.1 旧 App 对比发现（2026-06-13）

> **关键事实**：旧 App（`W:\CarApp\CarApp`）**从未使用 DDO（distributedDataObject）**。
> 它的分布式 = `distributedDeviceManager`（发现+互信）+ **`startAbility` 远端拉起** + UDP 传参。
> `startAbility` 对 DSoftBus "在线" 的要求比 DDO 更低，因此旧 App 在同一网络下能跑通而新架构 DDO 卡住。

| 对比项 | 旧 App | 新 App |
|---|---|---|
| 跨设备通信 | `startAbility(want)` 远端拉起 + UDP | DDO 共享黑板 + 本地 UDP |
| bindTarget 格式 | `(deviceId, {bindType:1, targetPkgName, ...}, cb)` | **完全一致** ✅ |
| startDiscovering | `{discoverTargetType:1}, **{availableStatus:0}**` | 原缺第二参数 → **已补** ✅ |
| **蓝牙** | ❌ **不需要**（AMS 路径不走蓝牙） | ✅ **必须**（DDO 依赖蓝牙设备发现） |
| **华为账号** | ❌ **不需要** | ⚠️ 通常需要（DDO 依赖分布式组网） |
| **超级终端** | ❌ **不需要** | ✅ **需要**（控制中心吸附设备） |
| **DSched 服务** | ❌ **不需要** | ✅ **需要**（紫派可能缺失） |
| DDO / setSessionId | **未使用** | 核心机制 |

**结论**：`bindTarget` API 格式正确（与旧 App 逐字段一致）。`startDiscovering` 缺少第二参数已修复。
DDO 是**新的架构选择**（更优雅的黑板模式），但**依赖蓝牙 + 超级终端 + DSched 系统服务**——这些是旧 App 用 `startAbility` 时完全不需要的。
**当前首要任务：开蓝牙 + 超级终端吸附 + 确认紫派 DSched**。

## 3.2 网络排查手册（AP 隔离 / 客户端隔离）—— 降级为次要怀疑

> ⚠️ 日志分析后，AP 隔离已**降级为次要怀疑**。首要问题是 DDO 先决条件（蓝牙/超级终端/DSched），见 §3 顶部。
> 但如果满足所有先决条件后 `onlineDev` 仍为 0，再回来看这里。

AP 隔离只挡设备间 P2P、不挡广播 → 能发现+能绑定、但 `onlineDev=0`。

### 排查步骤

**Step 1：确认网络类型**
- [ ] 当前连的是什么网络？（手机热点 / 企业 WiFi / 家用路由器 / 便携路由器）
- [ ] 手机热点和企业 WiFi **默认开 AP 隔离** → 需换成普通家用路由器

**Step 2：路由器设置检查**（如使用路由器）
- [ ] 登录路由器管理页面（通常 192.168.1.1 或 192.168.0.1）
- [ ] 找到「AP 隔离」/「客户端隔离」/「Station Isolation」/「Wireless Isolation」→ **关闭**
- [ ] 确认「WMM」/「QoS」未限制设备间通信

**Step 3：设备连通性验证**
```bash
# 在平板上（或通过 hdc shell 进入紫派）：
ping <对方IP>
# 能 ping通 ≠ P2P 通（ICMP 可能走 AP 转发，但 mDNS/P2P 被隔离）
# 进一步验证：
# 平板和紫派各开 HiLog，看 deviceStateChange 事件是否出现对端
```

**Step 4：确认设备状态**
- [ ] 两端**同 WiFi SSID**（不是 2.4G 和 5G 两个 SSID 各连一个——部分路由器隔离频段）
- [ ] 平板和紫派的**分布式开关已开启**（设置 → 超级终端 / 多设备协同 → 开启）
- [ ] 紫派 agent 正在运行（`hdc shell ps -ef | grep carapp`）
- [ ] 两端**互信已绑定**（DeviceTrustPage 显示「已信任」，非"解绑重置"后忘重新配对）

**Step 5：HiLog 诊断（关键日志）**
```bash
# 平板端：
hdc shell hilog | grep -E "FleetMission|deviceStateChange|DDO|onlineDev"

# 紫派端：
hdc shell hilog | grep -E "FleetMission\(agent\)|deviceStateChange|DDO|onlineDev"
```

**期望看到**（按出现顺序）：
1. `✅ 全部已授权` — DATASYNC 权限 OK
2. `deviceStateChange: ...` — DM 层感知到对端
3. `已信任在线设备: ≥1 台` — 互信 + 在线
4. `[DDO] peer status: ... status=online ✅` — DDO 层对端在线

**若只看到 1、2，没有 3、4** → AP 隔离（设备间 P2P 不通）。
**若连 2 都没有** → 网络不在同一子网 / 分布式开关未开 / agent 未运行。

**Step 6：换网验证**（最直接的方法）
- [ ] 用一个**普通家用路由器**（确认无 AP 隔离），平板 + 紫派都连它
- [ ] 重新走一遍 Step 4 + Step 5
- [ ] 若此时通了 → 确认原网络的 AP 隔离是根因

## 4. 判据速查（怎么算"通了"）

- 紫派 agent 日志：`GetDeviceList Collaboration deivces size:` **0 → ≥1** 且 **`onlineDev count > 0`**；
- DDO `status` 事件出现 **`peer <networkId> online`**；
- 平板划覆盖区域 → 紫派出现 **`黑板更新: phase=covering … → 产 N 条命令`** + `TX→ 127.0.0.1 …`；
- 平板出现第二个 toast「**车N：agent 已下发覆盖/执行命令**」+ 地图上车变色 / 移动。

## 5. 已修（2026-06-13 代码修正）

### DDO 调试期修正（已随 LAN 方案上线而废弃，保留作为历史记录）

| 项 | 改动 |
|---|---|
| `startDiscovering` 补第二参数 | `{availableStatus: 0}` 与旧 App 对齐 |
| `deviceStateChange` 事件监听 | DM 层设备上下线跟踪 |
| DDO `status` 回调增强 | 明确记录 `online`/`offline` + 中文诊断 |
| `diagnoseConnectivity()` 诊断方法 | 两端 FleetMissionService 均增加 |

### LAN Socket 黑板实施（当前生效）

| 项 | 改动 |
|---|---|
| **FleetMissionService 整体重写** | DDO → TCP Socket（平板=客户端，agent=服务端 :5003） |
| **constants/protocol.ets** | 新增 `FLEET_LAN_PORT = 5003`（两端同步） |
| **module.json5** | 移除 `DISTRIBUTED_DATASYNC` 权限 |
| **DeviceTrustPage / PairingAbility** | 停用（LAN 方案不再需要互信） |
| **verify.mjs** | 64/64 通过（含 10 项新增 LAN framing 测试） |

## 6. 已查明（2026-06-13 日志分析 · 8 份 HiLog）

### 根因链路

**平板 (MatePad 11.5"S)**：蓝牙未开 → `GetAnonyLocalUdid` 失败 (ret `96929750`) → ObjectStore 无法注册 DevStateCallback → onlineDev=0 → DDO 不同步

**紫派 (Purple Pi OH)**：DSched 系统服务不存在 → `GetDSchedEventInfo` 失败 (ret `2097167`) → `GetDeviceList` 协作事件失败 (`29360174`) → onlineDev=0 → DDO 不同步

### 关键发现

1. **`DISTRIBUTED_DATASYNC` 是 system_grant 权限**（APL=normal，三方可用）——权限声明本身没问题
2. **`96929750` = DHDM 权限拒绝**——不是应用层权限问题，是底层 DHDM 依赖蓝牙/设备组网状态
3. **旧 App 不用 DDO**——用 `startAbility` + UDP，不走 ObjectStore/DSched/DHDM 长链路，所以不需要蓝牙/超级终端/DSched
4. **DDO 官方要求**：蓝牙开启 + 超级终端吸附 + 同华为账号 + DSched 服务——这些全部是新架构额外的依赖
5. **AP 隔离已排除为首要原因**（降级为次要怀疑）

## 7. 待办

> DDO 路径的排查项（蓝牙/超级终端/华为账号等）已因 LAN Socket 方案上线而**不再需要**。保留作为历史记录。

1. **真机验证 LAN 黑板** → 装 App 到平板+紫派，同一 WiFi，验证 TCP :5003 连通 + mission 下发 + robot 回写
2. **A**：`udp2lcm` 心跳回**客户端源端口**（否则覆盖时 agent 收不到位姿 + 回环；spec 见 `integration-qa.md`）
3. **长期**：agent 从 UIAbility 壳换**常驻 ServiceExtension / 系统应用**（Q6.1，需紫派系统侧）

## 8. LAN Socket 黑板实施完成（2026-06-13）

### 方案选择理由

DDO 根因分析（§3）确认：DDO 依赖蓝牙 + 超级终端 + DSched + 华为账号等一长串系统先决条件，
在消费级平板 + 紫派 OpenHarmony 组合上**凑不齐**。旧 App 用 `startAbility` 走 AMS 路径，
**完全不需要这些**，所以能通。LAN TCP Socket 走和旧 App 同样的思路——纯网络层，不依赖系统分布式框架。

### 已实施的改动清单

| 文件 | 改动 |
|---|---|
| `app-harmony/.../constants/protocol.ets` + car-agent 副本 | 新增 `FLEET_LAN_PORT = 5003` |
| `app-harmony/.../service/FleetMissionService.ets` | **整体重写**：DDO → TCP 客户端（连车 :5003，发 mission，收 robot 合并） |
| `car-agent/.../service/FleetMissionService.ets` | **整体重写**：DDO → TCP 服务端（监听 5003，收 mission 派发，发本车 robot） |
| `car-agent/.../agent/AgentCore.ets` | 加 `fleet.setSelfCarId(carId)` + `setLocalMission()`；init 不再需 context |
| `app-harmony/.../entryability/EntryAbility.ets` | `init()` 不再传 context |
| 两端 `module.json5` | 移除 `ohos.permission.DISTRIBUTED_DATASYNC` 权限声明 |
| `app-harmony/.../pages/HomePage.ets` | 停用设备互信按钮（DeviceTrustPage） |
| `car-agent/.../pages/AgentStatusPage.ets` | 重写为纯状态页，移除配对入口 |
| `tools/verify/verify.mjs` | BUNDLE_NAME 检查改为 FLEET_LAN_PORT + FLEET_SESSION_ID；新增 §⑥ LAN framing 测试（10 项） |

### 线协议

4 字节大端长度前缀 + UTF-8 JSON。消息类型：
- `{t:"hello", session, role, carId}` — 连接建立时双向发送
- `{t:"mission", snapshot}` — 平板 → 车，整块任务黑板
- `{t:"robot", robot}` — 车 → 平板，本车运行态

### 合并规则

平板对规划字段权威（phase/assignments/area/map）；每辆车对自己的 robot 权威。
收到 `{t:"robot"}` → 只更新 `robots[index]`，保留平板本地规划字段。**天然修掉 DDO 的"最后写者胜"缺陷**。

### 验证状态

- `verify.mjs`：**64/64 通过**（含 10 项新增 LAN framing 测试：encode/decode + 粘包 + 半包）
- 真机验证：待装 App + 同一 WiFi 后测试

### 不再需要的代码/功能（已停用，文件保留备查）

- `DeviceTrustPage.ets` — 设备互信（bindTarget/unbindTarget）
- `PairingAbility.ets` + `PairingPage.ets` — 车载配对界面
- `distributedDeviceManager` / `distributedDataObject` / `abilityAccessCtrl` 相关代码
- `DISTRIBUTED_DATASYNC` 权限

## 9. 官方示例研究（2026-06-13 补充发现）

### 9.1 关键发现：缺少 ACCESS_SERVICE_DM 权限

**研究官方 DistributedNote 示例后发现的重大遗漏**：

官方示例 `module.json5` 声明了**两个权限**：

```json5
{
  "requestPermissions": [
    { "name": "ohos.permission.DISTRIBUTED_DATASYNC" },
    { "name": "ohos.permission.ACCESS_SERVICE_DM" }  // ← 我们完全没声明！
  ]
}
```

**权限说明**：
- `DISTRIBUTED_DATASYNC`：分布式数据同步权限（我们声明了 ✅）
- `ACCESS_SERVICE_DM`：访问设备管理服务权限（我们**完全没声明** ❌）
  - 用途：设备发现、设备认证、在线状态查询
  - 对应 API：`distributedDeviceManager`

**这可能是 DDO 失败的另一个关键因素**：即使权限全部授予，缺少 `ACCESS_SERVICE_DM` 也会导致设备管理服务无法正常工作，进而影响 `onlineDev` 计数。

详细分析见 [`docs/research/openharmony-distributed-examples-analysis.md`](research/openharmony-distributed-examples-analysis.md)。

### 9.2 官方推荐模式：startAbility + DDO 混合方案

**官方 DistributedNote 示例的完整流程**：

```
1. distributedDeviceManager.discoverDevices()  → 发现设备
2. distributedDeviceManager.authenticateDevice() → 认证设备（需要 ACCESS_SERVICE_DM）
3. startAbility(deviceId, parameters: {sessionId}) → 启动远端 Ability，传递 sessionId
4. 远端 Ability 接收 sessionId → 调用 distributedObject.setSessionId(sessionId)
5. 两端建立 DDO 同步 → 监听变更 → 自动同步
```

**关键差异**：
- 官方使用 **startAbility 先建立连接**，再用 DDO 做数据同步
- 我们试图**直接使用 DDO**，跳过了设备发现→认证→startAbility 的前置步骤
- 旧 App 使用 startAbility + UDP（不用 DDO），所以能成功

### 9.3 如果未来要回到 DDO

必须满足以下条件：

1. **添加 `ACCESS_SERVICE_DM` 权限**到 `module.json5`
2. **使用 startAbility + DDO 混合方案**：
   - 设备发现（distributedDeviceManager）
   - 设备认证（authenticateDevice）
   - startAbility 启动远端 Ability，传递 sessionId
   - 两端调用 `setSessionId(sessionId)` 建立 DDO 同步
3. **确保前置条件**：
   - 两端登录同一华为账号
   - 两端开启蓝牙
   - 两端在超级终端中互相吸附
   - DSched 服务正常

**当前不推荐**：前置条件在消费级平板 + 工业紫派组合上无法满足。LAN Socket 方案更简单可靠。

### 9.4 结论：LAN Socket 是正确选择

| 维度 | 官方 DDO 方案 | 我们的 LAN Socket 方案 |
|------|-------------|----------------------|
| **权限** | `DISTRIBUTED_DATASYNC` + `ACCESS_SERVICE_DM` | `INTERNET` (normal) |
| **前置条件** | 同华为账号 + 蓝牙 + 超级终端 + DSched | 仅同一 WiFi |
| **流程复杂度** | 设备发现→认证→startAbility→DDO | 直接 TCP 连接 |
| **可靠性** | 依赖多个系统服务 | 仅依赖网络连通性 |
| **跨平台** | 仅鸿蒙设备 | 任何支持 TCP 的平台 |

**LAN Socket 方案的优势**：
- 零系统权限依赖
- 零系统服务依赖
- 流程简单直接
- 完全可控可调试
- 旧 App UDP 方案已验证可行性

---

> 调试期所有 App/agent 改动落 `app-harmony-core`；本文（docs）按约定同步 `main`。
