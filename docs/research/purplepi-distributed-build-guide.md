# 紫派 OpenHarmony 镜像编译指南：启用分布式能力

> **日期**: 2026-06-13  
> **目标**: 编译出支持 DDO（distributedDataObject）完整链路的 OpenHarmony 镜像  
> **芯片**: RK3566（紫派 Purple Pi OH）  
> **参考**: RK3568 官方示例成功配置、OpenHarmony 5.0 构建系统文档

---

## 1. 目标：让 DDO 在紫派上真正跑通

当前 DDO 在紫派上失败的根因链：

```
紫派 DSched accessToken error:7 → 在线设备列表为空
  + 蓝牙栈未启用 → DHDM 无法发现设备
  + 三方签名 → 拿不到 system_basic 权限
  = onlineDev count = 0 → DDO 不同步
```

要修的东西（按编译层面排列）：

| 序号 | 问题 | 编译层面的修法 |
|------|------|--------------|
| 1 | DSched 不稳定 | 确认 `distributedschedule` 子系统已编入镜像 |
| 2 | 蓝牙栈缺失 | 启用蓝牙子系统 + RK3566 WiFi/BT 驱动 |
| 3 | 权限不足 | 使用系统签名（`hos_system_app`）编译 car-agent |
| 4 | DSoftBus 不稳定 | 确认 `communication` 子系统中 dsoftbus 组件已编入 |

---

## 2. 产品定义文件（product.json5）

### 2.1 文件位置

```
productdefine/common/products/
├── rk3568.json5          ← 参考这个
├── rk3566.json5          ← 你们的产品文件（如果有的话）
└── ...
```

如果你们用的是 vendor 目录下的配置：

```
vendor/rockchip/rk3566/
├── ohos.build
├── product.json5         ← 产品定义
└── ...
```

### 2.2 需要确认的 subsystem_list

打开产品定义文件，找到 `subsystem_list` 数组，确认以下子系统**已包含**：

```json5
{
  "subsystem_list": [
    // ── 必须确认存在的分布式子系统 ──
    "communication",           // 包含 dsoftbus（分布式软总线）+ 蓝牙
    "distributeddatamgr",      // 分布式数据管理（DDO 的 ObjectStore + DistributedDB）
    "distributedhardware",     // 分布式硬件框架
    "distributedschedule",     // 分布式调度（DSched）—— DDO 的核心依赖
    
    // ── 以下通常已有，顺便确认 ──
    "ability",                 // AbilityManagerService（AMS，startAbility 需要）
    "security",                // 安全框架（权限管理）
    "hiviewdfx",              // 日志/诊断
    "multimedia",             // 多媒体（如果不需要可去掉）
    
    // ── 蓝牙依赖 ──
    // bluetooth 不是独立子系统，包含在 communication 中
    // 但需要确认 features 中的蓝牙开关已开启（见 §3）
  ]
}
```

### 2.3 如果缺少某个子系统

在 `subsystem_list` 数组中添加即可。例如，如果缺少 `distributedschedule`：

```json5
{
  "subsystem_list": [
    "communication",
    "distributeddatamgr",
    "distributedhardware",
    "distributedschedule",    // ← 添加这一行
    "ability",
    "security",
    // ... 其他已有的子系统
  ]
}
```

---

## 3. 蓝牙模块启用

### 3.1 蓝牙在 OpenHarmony 中的位置

蓝牙**不是独立子系统**，它包含在 `communication` 子系统中。编译时需要通过 **features** 开关启用：

```json5
{
  "subsystem_list": [
    "communication",
    // ...
  ],
  "features": [
    // ── 蓝牙相关 features ──
    "communication_bluetooth_enable = true",
    "communication_bluetooth_le_enable = true",   // BLE（低功耗蓝牙），DDO 设备发现可能需要
    
    // ── WiFi 相关（确认已开启）──
    "communication_wifi_enable = true",
    
    // ── 分布式软总线 ──
    "communication_dsoftbus_enable = true",
  ]
}
```

### 3.2 RK3566 蓝牙驱动确认

RK3566 的 WiFi/BT 模块通常是 RTL8723DS 或 AP6256。需要确认：

1. **内核设备树（DTS）中蓝牙节点已启用**：
   ```
   kernel/linux-5.10/arch/arm64/boot/dts/rockchip/rk3566-xxx.dtsi
   ```
   确认 bluetooth 节点不是 `status = "disabled"`。

2. **蓝牙固件文件已包含**：
   ```
   device/board/xxx/firmware/
   ├── rtl8723ds_fw.bin      # RTL8723DS 蓝牙固件
   └── rtl8723ds_config.bin  # 配置文件
   ```

3. **蓝牙 HCI 驱动已编译**：
   ```
   device/soc/rockchip/rk3566/bluetooth/
   ```
   确认 `ohos.build` 中有蓝牙相关的组件定义。

### 3.3 验证蓝牙是否正常工作

刷机后，通过 hdc 验证：

```bash
# 检查蓝牙设备节点
hdc shell ls /dev/hci*

# 检查蓝牙服务是否启动
hdc shell ps -ef | grep bluetooth

# 检查 hciconfig（如果有）
hdc shell hciconfig
```

---

## 4. DSched（分布式调度）确认

### 4.1 DSched 在 OpenHarmony 中的位置

DSched 属于 `distributedschedule` 子系统：

```
foundation/distributedschedule/dmsfwk/
├── services/
│   └── distributedschedsvc/    ← DSched 服务端
├── interfaces/
│   └── inner_api/              ← 内部 API
└── sa_profile/
    └── dms_fwk_sa.json         ← SA（System Ability）配置
```

### 4.2 确认 DSched SA 配置

DSched 作为系统服务（SA）运行。确认 SA 配置文件已包含在产品中：

```
vendor/rockchip/rk3566/sa_profile/
├── dms_fwk_sa.json           ← 必须有这个
└── ...
```

如果没有，从 OpenHarmony 源码中拷贝：

```bash
cp foundation/distributedschedule/dmsfwk/sa_profile/dms_fwk_sa.json \
   vendor/rockchip/rk3566/sa_profile/
```

### 4.3 验证 DSched 是否正常工作

刷机后：

```bash
# 检查 DSched 服务是否注册
hdc shell samgr | grep -i dsched
# 期望输出包含 DmsFwkService 或类似名称

# 检查 SA 进程
hdc shell ps -ef | grep dms
```

---

## 5. 系统签名（最关键的一步）

### 5.1 为什么需要系统签名

OpenHarmony 权限分三级：

| 级别 | 签名要求 | 示例权限 |
|------|---------|---------|
| **normal** | 三方签名即可 | `INTERNET` |
| **system_basic** | **系统签名** `hos_system_app` | `DISTRIBUTED_DATASYNC`、`ACCESS_SERVICE_DM` |
| **system_core** | 系统签名 + 更高信任 | 系统内部 API |

**DDO 需要的权限全是 `system_basic` 级别**。用三方签名编译的 app 即使声明了这些权限，底层系统服务（DHDM、DSched）在 IPC 调用时也会因为签名不匹配而拒绝（这就是 `accessToken error:7` 的根因）。

### 5.2 OpenHarmony 源码中的签名工具

```
developtools/hap_sign_tool/
├── hap_sign_tool.sh            ← 签名工具主脚本
├── certs/
│   ├── openharmony_sdk.cer     ← SDK 调试证书（三方签名）
│   ├── openharmony_system.cer   ← 系统证书（系统签名）  ← 要用这个
│   └── ...
└── keys/
    ├── openharmony_sdk.p12       ← SDK 密钥
    ├── openharmony_system.p12    ← 系统密钥               ← 要用这个
    └── ...
```

### 5.3 方法一：在 DevEco Studio 中配置系统签名

**步骤**：

1. 打开 `build-profile.json5`
2. 在 `signingConfigs` 中使用系统签名：

```json5
{
  "app": {
    "signingConfigs": [
      {
        "name": "system",
        "type": "HarmonyOS",
        "material": {
          "certpath": "<源码根目录>/developtools/hap_sign_tool/certs/openharmony_system.cer",
          "keyAlias": "openharmony system application debug key",
          "keyPassword": "0000001B...",  // 源码中提供的密码
          "profile": "<源码根目录>/developtools/hap_sign_tool/certs/openharmony_system.p7b",
          "signAlg": "SHA256withECDSA",
          "storeFile": "<源码根目录>/developtools/hap_sign_tool/keys/openharmony_system.p12",
          "storePassword": "0000001B..."  // 源码中提供的密码
        }
      }
    ],
    "products": [
      {
        "name": "default",
        "signingConfig": "system",  // ← 使用系统签名
        // ...
      }
    ]
  }
}
```

3. 在 `module.json5` 中添加 `app-feature`：

```json5
{
  "app": {
    "bundleName": "com.example.carapp",
    "app-feature": "hos_system_app"  // ← 声明为系统应用
  }
}
```

### 5.4 方法二：在编译镜像时预签名

如果你们自己编译镜像，可以在编译流程中直接用系统密钥签名 car-agent 的 HAP：

```bash
# 编译 HAP 后，用系统签名重签
java -jar developtools/hap_sign_tool/hap-sign-tool.jar sign \
  -keyAlias "openharmony system application debug key" \
  -signAlg "SHA256withECDSA" \
  -mode "local" \
  -profile developtools/hap_sign_tool/certs/openharmony_system.p7b \
  -keystore developtools/hap_sign_tool/keys/openharmony_system.p12 \
  -keystorePwd "123456" \
  -keyPwd "123456" \
  -signCode "1" \
  -apk car-agent/entry/build/default/outputs/default/entry-default-unsigned.hap
```

### 5.5 方法三：镜像编译时预装并预授权

如果 car-agent 要作为系统应用预装到镜像中：

1. 将 car-agent 的 HAP 放入镜像的 `system/app/` 或 `vendor/app/` 目录
2. 在 `vendor/rockchip/rk3566/` 的配置中添加预装声明
3. 系统启动时自动安装并授予所有 `system_grant` 权限

---

## 6. DSoftBus（分布式软总线）确认

### 6.1 DSoftBus 在 OpenHarmony 中的位置

```
foundation/communication/dsoftbus/
├── core/                    ← 核心框架
├── adapter/                 ← 平台适配层
├── interfaces/              ← API
└── sa_profile/
    └── dsoftbus_sa.json     ← SA 配置
```

### 6.2 确认 DSoftBus 编译开关

在产品定义的 features 中确认：

```json5
{
  "features": [
    "communication_dsoftbus_enable = true",
    "communication_dsoftbus_auth_enable = true",   // 认证功能
    "communication_dsoftbus_trans_enable = true",  // 传输功能
  ]
}
```

### 6.3 验证 DSoftBus 是否正常工作

```bash
# 检查 DSoftBus SA 进程
hdc shell ps -ef | grep dsoftbus

# 检查日志
hdc shell hilog | grep -i "dsoftbus\|softbus"
```

---

## 7. 完整编译步骤清单

### 7.1 修改产品定义

编辑 `vendor/rockchip/rk3566/product.json5`（或对应路径）：

```json5
{
  "product_name": "rk3566",
  "device_company": "rockchip",
  "device_board": "rk3566",
  "type": "standard",
  "subsystem_list": [
    // 分布式核心（必须全部存在）
    "communication",
    "distributeddatamgr",
    "distributedhardware",
    "distributedschedule",
    
    // 基础系统（通常已有）
    "ability",
    "security",
    "hiviewdfx",
    "multimedia",
    "startup",
    "update",
    "web",
    // ... 其他已有的子系统保持不变
  ],
  "features": [
    // 蓝牙（关键！）
    "communication_bluetooth_enable = true",
    "communication_bluetooth_le_enable = true",
    
    // 分布式软总线
    "communication_dsoftbus_enable = true",
    "communication_dsoftbus_auth_enable = true",
    "communication_dsoftbus_trans_enable = true",
    
    // WiFi
    "communication_wifi_enable = true",
    
    // ... 其他已有的 features 保持不变
  ]
}
```

### 7.2 确认 SA 配置文件

```bash
# 检查 DSched SA 配置
ls vendor/rockchip/rk3566/sa_profile/dms_fwk_sa.json
# 如果不存在，从源码拷贝
cp foundation/distributedschedule/dmsfwk/sa_profile/dms_fwk_sa.json \
   vendor/rockchip/rk3566/sa_profile/

# 检查 DSoftBus SA 配置
ls vendor/rockchip/rk3566/sa_profile/dsoftbus_sa.json
# 如果不存在，从源码拷贝
cp foundation/communication/dsoftbus/sa_profile/dsoftbus_sa.json \
   vendor/rockchip/rk3566/sa_profile/
```

### 7.3 确认蓝牙驱动

```bash
# 检查设备树
grep -r "bluetooth\|hci" device/soc/rockchip/rk3566/
# 确认蓝牙节点 status = "okay"

# 检查蓝牙固件
ls device/board/xxx/firmware/*bt* device/board/xxx/firmware/*bluetooth*
# 确认固件文件存在
```

### 7.4 编译 car-agent 使用系统签名

修改 `car-agent/build-profile.json5`：

```json5
{
  "app": {
    "signingConfigs": [
      {
        "name": "system",
        "type": "HarmonyOS",
        "material": {
          "certpath": "<OpenHarmony源码>/developtools/hap_sign_tool/certs/openharmony_system.cer",
          "keyAlias": "openharmony system application debug key",
          "keyPassword": "<源码中提供的密码>",
          "profile": "<OpenHarmony源码>/developtools/hap_sign_tool/certs/openharmony_system.p7b",
          "signAlg": "SHA256withECDSA",
          "storeFile": "<OpenHarmony源码>/developtools/hap_sign_tool/keys/openharmony_system.p12",
          "storePassword": "<源码中提供的密码>"
        }
      }
    ],
    "products": [
      {
        "name": "default",
        "signingConfig": "system"
      }
    ]
  }
}
```

修改 `car-agent/entry/src/main/module.json5`：

```json5
{
  "module": {
    // ... 其他配置
    "app-feature": "hos_system_app"  // ← 添加这一行
  }
}
```

### 7.5 编译镜像

```bash
# 进入 OpenHarmony 源码目录
cd /path/to/openharmony

# 配置产品
hb set
# 选择 rk3566 产品

# 编译
hb build -f

# 产物
ls out/rk3566/packages/phone/images/
```

### 7.6 刷机验证

```bash
# 刷入新镜像
# （按你们现有的刷机流程）

# 验证蓝牙
hdc shell hciconfig
# 期望看到 hci0 设备

# 验证 DSched
hdc shell samgr | grep -i dms
# 期望看到 DmsFwkService

# 验证 DSoftBus
hdc shell ps -ef | grep dsoftbus
# 期望看到 dsoftbus 进程

# 安装系统签名的 car-agent
hdc install car-agent-signed.hap

# 启动 agent
hdc shell aa start -a AgentAbility -b com.example.carapp

# 检查权限
hdc shell hilog | grep "authResults"
# 期望看到 authResults=[0]（所有权限已授予）

# 检查 DDO
hdc shell hilog | grep "onlineDev count"
# 期望看到 onlineDev count > 0（不再是 0！）
```

---

## 8. 验证判据：怎么算"分布式通了"

### 8.1 紫派侧日志

```bash
hdc shell hilog | grep -E "FleetMission|onlineDev|Collaboration|DSched"
```

**期望看到**：
```
✅ RegisterDevStateCallback completed          ← DSched 回调注册成功
✅ Collaboration devices size: ≥1              ← 有协作设备
✅ onlineDev count = 1                         ← 有在线设备
✅ [DDO] peer status: ... status=online        ← DDO 对端在线
```

### 8.2 平板侧日志

```bash
hdc shell hilog | grep -E "FleetMission|onlineDev|peer status"
```

**期望看到**：
```
✅ GetAnonyLocalUdid success                   ← 蓝牙正常
✅ onlineDev count = 1                         ← 紫派在线
✅ [DDO] peer status: ... status=online        ← DDO 对端在线
```

### 8.3 端到端验证

1. 平板划覆盖区域
2. 紫派 agent 日志出现 `黑板更新: phase=covering → 产 N 条命令`
3. 平板收到 `合并 robot: index=1 pos=(...)`
4. 地图上车变色 + 移动

---

## 9. 如果还是不通怎么办

### 9.1 逐步排查

| 检查项 | 命令 | 期望结果 | 如果失败 |
|--------|------|---------|---------|
| 蓝牙设备 | `hdc shell hciconfig` | 看到 hci0 | 检查设备树和驱动 |
| DSched 服务 | `hdc shell samgr \| grep dms` | 看到服务 | 检查 SA 配置文件 |
| DSoftBus 进程 | `hdc shell ps -ef \| grep dsoftbus` | 看到进程 | 检查 features 开关 |
| 权限 | `hdc shell hilog \| grep authResults` | 全 0 | 检查系统签名 |
| 在线设备 | `hdc shell hilog \| grep onlineDev` | count > 0 | 蓝牙 + DSched + 签名都要对 |

### 9.2 回退方案

如果镜像编译后仍有问题，LAN Socket 方案仍然是可靠的备选：
- 零系统依赖
- 已验证 64/64 测试通过
- 文档齐全（`docs/lan-blackboard-impl.md`）

两套方案可以共存：DDO 作为首选，LAN Socket 作为降级方案。代码中 `FleetMissionService` 的接口不变，可以按条件切换实现。

---

## 10. 参考资料

- **OpenHarmony 产品定义**: `productdefine/common/` 仓库
- **系统签名工具**: `developtools/hap_sign_tool/` 目录
- **DSched 源码**: `foundation/distributedschedule/dmsfwk/`
- **DSoftBus 源码**: `foundation/communication/dsoftbus/`
- **蓝牙子系统**: `foundation/communication/bluetooth/`
- **RK3568 BSP**: `device/soc/rockchip/rk3568/`
- **官方分布式示例**: `openharmony/applications_app_samples/code/SuperFeature/DistributedAppDev/`
- **本项目 DDO 调试记录**: `docs/distributed-bringup-status.md`
- **本项目 LAN Socket 方案**: `docs/lan-blackboard-impl.md`

---

**文档结束**
