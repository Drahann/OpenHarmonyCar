# 分布式设备互信（软总线组网）设计 —— 按官方文档，不照搬旧 App

> 背景：`distributedDataObject`（FleetMission 黑板）跨设备同步**要求两台设备先在"可信网络"里**。旧 App 用
> `distributedDeviceManager` 自己做发现+认证，但做得不好。本文按 **OpenHarmony 官方文档**重新厘清正确做法 +
> 我们这台**无界面紫派**的特殊难点。来源见文末。

## 一、官方两条互信路径

| 路径 | 怎么建立 | App 要不要做绑定 UI | 适用 |
|---|---|---|---|
| **A 同账号** | 两台设备登录同一分布式/华为账号 + 同局域网 + 蓝牙开 → 自动互信 | **不用**（`getAvailableDeviceListSync` 直接列出可信设备） | HarmonyOS 设备、能登账号 |
| **B 账号无关** | `distributedDeviceManager.startDiscovering` → `on('discoverSuccess')` → **`bindTarget`(bindType=1 安全认证：PIN/碰一碰/扫码)** → 持久"信任标签" | **要**（发现列表 + 发起绑定 + 处理认证回调） | 跨账号 / 无账号设备 |

`distributedDataObject` 同步流程（互信建立后）：`create(context)` → `setSessionId(同一ID)` → `on('change')` → 改字段即自动同步。
权限：`ohos.permission.DISTRIBUTED_DATASYNC`（user_grant，需弹窗授权）。我们 App 侧黑板已接（`FleetMissionService` + `ControlPage`，见 feature-parity-review §6.3）。

## 二、我们的场景：平板 ↔ 紫派(RK3566 / OpenHarmony 5.0)

- 紫派是 **OpenHarmony（非 HarmonyOS）** → "同华为账号"路一般不通 → **多半走路径 B（账号无关 `bindTarget`）**。
- **🔴 卡点：`bindTarget` 是安全认证，需两端交互确认（PIN/碰一碰）。而紫派上跑的是无界面 agent，没法在车上点确认 PIN。**
  → 所以**不能指望 agent 自己完成绑定**。互信必须**预先一次性建立**（持久"信任标签"后，无界面 agent 才能直接用）。

## 三、设计（职责拆分）

1. **平板 App（我方可做对）**：做一个**设备互信 UI**——`startDiscovering` 发现同网 OH 设备 → 列表 → 选中 `bindTarget` 发起绑定 →
   处理认证回调。复刻旧 App 的"发现可用/发现新设备"两列，但按官方 API 写干净（替代旧的烂实现）。
2. **紫派侧（须 A 配合，关键）**：紫派要**可被发现**且**能接受绑定**。因 agent 无界面：
   - 方案 ①：紫派**首次配对时**有一个非无界面的"配对/确认"步骤（或开发期 hdc/系统设置里确认），建立持久信任后，agent 常驻直接用；
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
- **但能否真的把无界面紫派绑成功，取决于 A 那边怎么让紫派接受绑定**。这之前，多机黑板链路无法端到端验证 → 先用 `ControlPage` 的**「平板直发兜底」**测覆盖本身。

## 来源（官方/社区）
- distributedDeviceManager 设备发现与认证：HarmonyOS 分布式管理（segmentfault 1190000046811877）、OpenHarmony device_manager（gitee/github openharmony）。
- distributedDataObject 跨设备同步前提（同账号+同网+蓝牙、`setSessionId`、`DISTRIBUTED_DATASYNC`）：HarmonyOS 开发实践（harmonyosdev.csdn）、官方 `@ohos.data.distributedDataObject` 文档。
