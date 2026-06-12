# OpenHarmony 分布式示例分析报告

> **日期**: 2026-06-13  
> **目的**: 研究官方分布式示例如何成功实现设备间通信，对比我们的 DDO 失败原因  
> **关键发现**: 官方示例使用 **startAbility + distributedDataObject 混合方案**，我们缺少 `ACCESS_SERVICE_DM` 权限

---

## 1. 官方示例概述

### 1.1 示例仓库

**仓库**: `openharmony/applications_app_samples`  
**路径**: `code/SuperFeature/DistributedAppDev/DistributedNote`  
**API 版本**: 9+  
**设备类型**: standard-system (手机/平板)

### 1.2 功能描述

分布式笔记应用：两台设备间同步笔记内容，支持增删改查，实时同步。

### 1.3 核心流程

1. 设备发现（distributedDeviceManager）
2. 设备认证（authenticateDevice）
3. **startAbility 启动远端 Ability，传递 sessionId**
4. 两端调用 `distributedObject.setSessionId(sessionId)` 建立同步
5. 监听数据变更，自动同步

---

## 2. 关键代码分析

### 2.1 权限配置（**重大发现**）

```json5
{
  "module": {
    "requestPermissions": [
      {
        "name": "ohos.permission.DISTRIBUTED_DATASYNC"
      },
      {
        "name": "ohos.permission.ACCESS_SERVICE_DM"
      }
    ]
  }
}
```

**⚠️ 我们只声明了 `DISTRIBUTED_DATASYNC`，缺少 `ACCESS_SERVICE_DM`！**

- `DISTRIBUTED_DATASYNC`: 分布式数据同步权限
- `ACCESS_SERVICE_DM`: 访问设备管理服务权限（用于设备发现、认证、在线状态查询）

### 2.2 设备发现与认证

```typescript
import deviceManager from '@ohos.distributedDeviceManager';

// 注册设备列表回调
RemoteDeviceModel.registerDeviceListCallback(() => {
  this.devices = RemoteDeviceModel.discoverDevices.length > 0 
    ? RemoteDeviceModel.discoverDevices 
    : RemoteDeviceModel.devices;
})

// 认证设备
RemoteDeviceModel.authenticateDevice(this.devices[this.selectedIndex], (device) => {
  this.startAbility(device.networkId);
})
```

### 2.3 startAbility 启动远端 Ability（**关键步骤**）

```typescript
startAbility(deviceId: string) {
  // 1. 生成 sessionId
  this.globalObject = new DistributedObjectModel();
  this.sessionId = this.globalObject.genSessionId();
  AppStorage.SetOrCreate('sessionId', this.sessionId);
  
  // 2. 启动远端 Ability，传递 sessionId
  let context = getContext(this) as common.UIAbilityContext;
  context.startAbility({
    bundleName: BUNDLE,
    abilityName: ABILITY,
    deviceId: deviceId,
    parameters: {
      sessionId: this.sessionId,  // 关键：通过 Want 传递 sessionId
    }
  })
}
```

### 2.4 分布式数据对象同步

```typescript
share() {
  // 设置数据变更回调
  this.globalObject.setChangeCallback(() => {
    this.noteDataSource.dataArray = this.globalObject.distributedObject.documents;
    this.noteDataSource.notifyDataReload();
  })
  
  // 设置设备状态回调
  this.globalObject.setStatusCallback((session, networkId, status) => {
    if (status === 'online') {
      this.isOnline = true;
    } else {
      this.isOnline = false;
    }
  })
  
  // 设置 sessionId，建立同步
  this.globalObject.distributedObject.setSessionId(this.sessionId);
  AppStorage.SetOrCreate('objectModel', this.globalObject);
}
```

### 2.5 远端 Ability 接收 sessionId

```typescript
// MainAbility.ets
onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
  let sessionId = want.parameters.sessionId;
  AppStorage.SetOrCreate('sessionId', sessionId);
  // ... 初始化 distributedObject，调用 setSessionId(sessionId)
}
```

---

## 3. 对比分析：我们的失败原因

### 3.1 方案对比

| 维度 | 官方示例 | 我们的 DDO 方案 | 我们的 LAN Socket 方案 |
|------|---------|----------------|----------------------|
| **权限** | `DISTRIBUTED_DATASYNC` + `ACCESS_SERVICE_DM` | ❌ 仅 `DISTRIBUTED_DATASYNC` | ✅ `INTERNET` (normal) |
| **流程** | startAbility → DDO | ❌ 纯 DDO | ✅ 纯 TCP Socket |
| **Session ID 传递** | startAbility parameters | ❌ 两端硬编码 | ✅ 两端硬编码 |
| **设备发现** | distributedDeviceManager | ❌ 未使用 | ❌ 未使用（不需要） |
| **设备认证** | authenticateDevice | ❌ 未使用 | ❌ 未使用（不需要） |
| **数据同步** | distributedDataObject | ❌ distributedDataObject | ✅ JSON over TCP |
| **前置条件** | 同华为账号 + 蓝牙 + 超级终端 | ❌ 同华为账号 + 蓝牙 + 超级终端 | ✅ 仅同一 WiFi |

### 3.2 失败根因总结

#### 3.2.1 权限缺失

我们只声明了 `DISTRIBUTED_DATASYNC`，缺少 `ACCESS_SERVICE_DM`。后者是设备管理服务访问权限，用于：
- 设备发现（discoverDevices）
- 设备认证（authenticateDevice）
- 在线状态查询（onlineDev count）

#### 3.2.2 流程错误

我们试图**直接使用 DDO**，期望两端自动发现并同步。但官方模式是：

```
设备发现 → 设备认证 → startAbility → 传递 sessionId → DDO 同步
```

我们跳过了前三步，直接跳到 DDO，导致：
- 两端无法建立初始连接
- `onlineDev count = 0`（没有设备在线）
- `Collaboration devices size = 0`（没有协作设备）

#### 3.2.3 前置条件无法满足

DDO 依赖的系统先决条件（蓝牙、超级终端、华为账号）在消费级平板 + 工业紫派组合上无法满足：
- 平板：`GetAnonyLocalUdid Failed (96929750)` - 蓝牙未开/权限不足
- 紫派：`DSched accessToken error (7)` - 服务不稳定

### 3.3 为什么旧 App 能成功

旧 App 使用 **startAbility + UDP** 方案：
1. 设备发现（distributedDeviceManager）✅
2. 设备认证（authenticateDevice）✅
3. startAbility 启动远端 Ability ✅
4. **UDP 直接通信**（不依赖 DDO）✅

旧 App **没有使用 DDO**，所以不需要满足 DDO 的前置条件。

---

## 4. 其他官方示例

### 4.1 DistributedJotNote（跨端迁移随手记）

**路径**: `code/SuperFeature/DistributedAppDev/DistributedJotNote`

**使用的分布式能力**:
- 跨端迁移（Continuation）
- distributedDataObject
- 分布式文件（distributedFilesDir）
- ArkUI 控件状态迁移

**权限**:
```json5
{
  "requestPermissions": [
    {
      "name": "ohos.permission.DISTRIBUTED_DATASYNC"
    },
    {
      "name": "ohos.permission.READ_IMAGEVIDEO"
    }
  ]
}
```

**特殊要求**:
- **系统应用签名** (`hos_system_app`)
- API Version 12
- 仅支持 RK3568 开发板

**结论**: 需要系统签名，不适合三方应用。

### 4.2 DistributedCalc（分布式计算器）

**路径**: `code/SuperFeature/DistributedAppDev/DistributedCalc`

**使用的分布式能力**:
- startAbility
- Want 参数传递

**特点**: 不使用 DDO，纯 startAbility + 参数传递。

### 4.3 DistributedRdb（分布式关系型数据库）

**路径**: `code/SuperFeature/DistributedAppDev/DistributedRdb`

**使用的分布式能力**:
- distributedDataObject（用于同步数据库变更通知）
- relationalStore（分布式 RDB）

**特点**: 使用 RDB 而非内存对象，数据持久化。

---

## 5. 关键结论

### 5.1 官方推荐的分布式模式

**混合方案**: `startAbility + distributedDataObject`

1. **startAbility 是必须的**: 用于建立初始连接、传递 sessionId、启动远端 Ability
2. **DDO 是数据层**: 在 startAbility 建立连接后，用于实时数据同步
3. **两个权限都需要**: `DISTRIBUTED_DATASYNC` + `ACCESS_SERVICE_DM`
4. **前置条件**: 同华为账号 + 蓝牙 + 超级终端（消费级设备）

### 5.2 我们的决策验证

**LAN Socket 方案是正确的选择**:

1. **简化架构**: 不需要 startAbility、设备发现、设备认证等复杂流程
2. **零系统依赖**: 只需要 `INTERNET` 权限（normal），不需要任何系统级权限
3. **跨平台兼容**: 不依赖华为账号、蓝牙、超级终端
4. **可控性强**: 完全自主实现，不依赖系统服务稳定性
5. **旧 App 验证**: 旧 App 使用类似方案（UDP）成功运行

### 5.3 如果未来要回到 DDO

必须满足以下条件：

1. **添加 `ACCESS_SERVICE_DM` 权限**
2. **使用 startAbility + DDO 混合方案**:
   - 设备发现（distributedDeviceManager）
   - 设备认证（authenticateDevice）
   - startAbility 启动远端 Ability，传递 sessionId
   - 两端调用 `setSessionId(sessionId)` 建立 DDO 同步
3. **确保前置条件**:
   - 两端登录同一华为账号
   - 两端开启蓝牙
   - 两端在超级终端中互相吸附
   - DSched 服务正常

**当前不推荐**: 前置条件在消费级平板 + 工业紫派组合上无法满足。

---

## 6. 参考资料

### 6.1 官方示例仓库

- **DistributedNote**: https://gitcode.com/openharmony/applications_app_samples/tree/master/code/SuperFeature/DistributedAppDev/DistributedNote
- **DistributedJotNote**: https://gitcode.com/openharmony/applications_app_samples/tree/master/code/SuperFeature/DistributedAppDev/DistributedJotNote
- **DistributedCalc**: https://gitcode.com/openharmony/applications_app_samples/tree/master/code/SuperFeature/DistributedAppDev/DistributedCalc
- **DistributedRdb**: https://gitcode.com/openharmony/applications_app_samples/tree/master/code/SuperFeature/DistributedAppDev/DistributedRdb

### 6.2 官方文档

- **分布式数据对象指南**: https://developer.huawei.com/consumer/cn/doc/harmonyos-guides-V5/distributed-data-object-guidelines-V5
- **distributedDataObject API**: https://developer.huawei.com/consumer/cn/doc/harmonyos-references-V5/js-apis-data-distributeddataobject-V5
- **distributedDeviceManager API**: https://developer.huawei.com/consumer/cn/doc/harmonyos-references-V5/js-apis-distributeddevicemanager-V5

### 6.3 项目内部文档

- **DDO 失败根因分析**: `docs/distributed-bringup-status.md`
- **LAN Socket 实施文档**: `docs/lan-blackboard-impl.md`
- **紫派适配指南**: `docs/agent-lan-adaptation.md`

---

## 7. 附录：权限说明

### 7.1 DISTRIBUTED_DATASYNC

- **用途**: 允许应用使用分布式数据同步能力
- **级别**: system_basic
- **授权方式**: system_grant
- **适用场景**: distributedDataObject、distributedRdb、distributedKvStore

### 7.2 ACCESS_SERVICE_DM

- **用途**: 允许应用访问设备管理服务
- **级别**: system_basic
- **授权方式**: system_grant
- **适用场景**: 设备发现、设备认证、在线状态查询
- **API**: distributedDeviceManager

### 7.3 INTERNET

- **用途**: 允许应用使用网络
- **级别**: normal
- **授权方式**: user_grant（安装时自动授予）
- **适用场景**: TCP/UDP Socket、HTTP 请求

---

**文档结束**
