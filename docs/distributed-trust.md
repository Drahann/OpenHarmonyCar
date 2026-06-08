# 分布式设备互信（软总线组网）设计 —— 按官方文档，不照搬旧 App

> 背景：`distributedDataObject`（FleetMission 黑板）跨设备同步**要求两台设备先在"可信网络"里**。旧 App 用
> `distributedDeviceManager` 自己做发现+认证，但做得不好。本文按 **OpenHarmony 官方文档**重新厘清正确做法 +
> 我们这台**无界面紫派**的特殊难点。来源见文末。

## 决策（2026-06-08 定·用户）

**保留 DDO（软总线黑板）+ 一次性账号无关配对（路径 B）。** 评估过"纯 LAN socket 黑板"替代方案
（免互信、更 headless、更好调试、几乎不依赖成员A，因 agent↔本机栈的 localhost UDP 不变、平板↔agent 两端都是我方 ArkTS）——
但**选择保留软总线**：① 反应式自动同步 + "地图经软总线共享"是 demo 亮点；② **互信是一次性、持久（跨重启）**，不是每次运行的负担；
③ 🔴 卡点已消解（见下）。**退路仍在**：若日后软总线在真机反复受阻，socket 黑板可随时切——`FleetMissionService` 的
`publishMission/subscribeMission` 本就是传输无关接口、`MissionSnapshot` 是单 JSON 整块、socket 化成本低（约一天）。

**互信怎么建（方向已定）**：
- **平板 = 发起方**（`bindTarget`）；**紫派 = 接受方**（确认 PIN）。即"平板认证车"。
- **🔴→✅ 卡点消解**：紫派**配网期接 HDMI 显示器**，在系统配对弹窗上确认 PIN——**团队既有成熟流程，此前设备认证一直这么做**。
  所以"车无界面、没法在车上点 PIN"不成立：配对**只发生一次、用显示器完成**；之后 headless agent 凭持久信任标签直接 `distributedDataObject` 同步，运行时无需任何界面。
- 因此本文 §三 的**方案①（首次配对有非无界面确认步骤）= 选定路线**（HDMI 显示器即那个"非无界面"步骤）；②dev 免 PIN / ③同账号 作备选。

**已答（2026-06-08，用户依老 App 经验）**：
- ① 走**账号无关 PIN 认证**（路径 B，`bindType=1`）；② 同一 **WiFi/热点**局域网。
- ③ 绑定时紫派**接显示器+鼠标**、PIN 在车屏可见可点——查老 App 代码证实：**本 App 不自渲染 PIN，系统弹窗代劳**
  （老 App 无 `on('uiStateChange')`/PIN UI，仅调 `bindTarget`），故车端**系统 PIN 弹窗**即可确认。
- ④ **🔑 关键约束：`distributedDataObject` 跨设备同步要求两端同 `bundleName`**。老 App 全设备同 `com.example.carapp`、
  `bindTarget` 的 `targetPkgName='com.example.carapp'`。**故我们的车载 agent 亦须打包为 `com.example.carapp`**（否则黑板不同步）；
  平板 `bindTarget` 的 `targetPkgName=BUNDLE_NAME`。

**仍待确认（真机校验 / 成员A）**：① 紫派 OH 5.0 的 DeviceManager 是否确有可点的**系统 PIN 弹窗**（用户经验=有，待真机复核）；
② 接受方是否要求**目标包(agent)在运行**才弹 PIN（不确定项——故车载 agent hap 内**另备一次性配对 UIAbility**作保险，配网时拉起，见 `car-agent-plan.md`）；
③ 紫派**被发现 + 接受绑定**所需权限/配置。

**App 侧已落地（2026-06-08）**：`pages/DeviceTrustPage.ets`（发现+配对面板，HomePage「设备互信」入口）
+ `FleetMissionService.bindDevice` 补 `targetPkgName/appOperation/customDescription`（对齐老 App 证实形参）。⚠️ 未经 DevEco。

## 一、官方两条互信路径

| 路径 | 怎么建立 | App 要不要做绑定 UI | 适用 |
|---|---|---|---|
| **A 同账号** | 两台设备登录同一分布式/华为账号 + 同局域网 + 蓝牙开 → 自动互信 | **不用**（`getAvailableDeviceListSync` 直接列出可信设备） | HarmonyOS 设备、能登账号 |
| **B 账号无关** | `distributedDeviceManager.startDiscovering` → `on('discoverSuccess')` → **`bindTarget`(bindType=1 安全认证：PIN/碰一碰/扫码)** → 持久"信任标签" | **要**（发现列表 + 发起绑定 + 处理认证回调） | 跨账号 / 无账号设备 |

`distributedDataObject` 同步流程（互信建立后）：`create(context)` → `setSessionId(同一ID)` → `on('change')` → 改字段即自动同步。
权限：`ohos.permission.DISTRIBUTED_DATASYNC`（user_grant，需弹窗授权）。我们 App 侧黑板已接（`FleetMissionService` + `ControlPage`，见 feature-parity-review §6.3）。

## 二、我们的场景：平板 ↔ 紫派(RK3566 / OpenHarmony 5.0)

- 紫派是 **OpenHarmony（非 HarmonyOS）** → "同华为账号"路一般不通 → **多半走路径 B（账号无关 `bindTarget`）**。
- **🔴→✅ 卡点（已消解，见上「决策」）：`bindTarget` 是安全认证，需两端交互确认（PIN/碰一碰）。**
  **不能指望无界面 agent 自己完成绑定**——但互信**预先一次性建立**即可（持久"信任标签"后 agent 常驻直接用）。
  我方做法：**紫派配网期接 HDMI 显示器**，在系统配对弹窗确认 PIN（团队既有成熟流程、此前认证即如此），平板做发起方。

## 三、设计（职责拆分）

1. **平板 App（我方可做对）**：做一个**设备互信 UI**——`startDiscovering` 发现同网 OH 设备 → 列表 → 选中 `bindTarget` 发起绑定 →
   处理认证回调。复刻旧 App 的"发现可用/发现新设备"两列，但按官方 API 写干净（替代旧的烂实现）。
2. **紫派侧（须 A 配合，关键）**：紫派要**可被发现**且**能接受绑定**。因 agent 无界面：
   - **方案 ①（✅ 选定）**：紫派**首次配对时**接 **HDMI 显示器**在系统配对弹窗确认（团队既有成熟流程；亦可开发期 hdc/系统设置确认），建立持久信任后 agent 常驻直接用；
   - 方案 ②：紫派 OH 镜像配置为**开发模式下同网自动接受/免 PIN**（dev 环境）；
   - 方案 ③：若 OH 支持账号体系，走路径 A 同账号。
   - **具体走哪个、紫派侧怎么配 → 必须 A 确认（见 integration-qa Q9）。这是设备/系统配置，不是 App 代码能单方面搞定的。**

## 四、API 形态（App 侧，`FleetMissionService` 扩展；⚠️ 形参以 SDK 为准，待 DevEco/真机校验）

```ts
dm.startDiscovering({ discoverTargetType: 1 });           // 开始发现
dm.on('discoverSuccess', (data) => use(data.device));     // 发现到的新设备(DeviceBasicInfo)
dm.bindTarget(deviceId, { bindType: 1, appName: '...' }, (err, res) => {/* 认证结果 */});
dm.stopDiscovering();
dm.getAvailableDeviceListSync();                          // 已可信设备
```

## 五、结论：明天能不能测多机？

- **互信是先决条件，且依赖紫派侧配置（Q9 待 A）**。互信没建立 → `distributedDataObject` 不同步 → 黑板/多机走不通。
- 平板 App 这端：补"设备互信 UI"（路径 B 的发现+绑定）——我方做。
- 车端**接受绑定的手段已定**（HDMI 显示器配网期在系统弹窗确认，团队既有流程）——卡点不再是"车无界面"。**剩余依赖** = A 确认紫派 OH 走账号/账号无关、被发现 + 接受绑定所需权限配置（Q9）。
- 互信端到端建立前，多机黑板链路无法验证 → 先用 `ControlPage` 的**「平板直发兜底」**测覆盖本身。

## 来源（官方/社区）
- distributedDeviceManager 设备发现与认证：HarmonyOS 分布式管理（segmentfault 1190000046811877）、OpenHarmony device_manager（gitee/github openharmony）。
- distributedDataObject 跨设备同步前提（同账号+同网+蓝牙、`setSessionId`、`DISTRIBUTED_DATASYNC`）：HarmonyOS 开发实践（harmonyosdev.csdn）、官方 `@ohos.data.distributedDataObject` 文档。
