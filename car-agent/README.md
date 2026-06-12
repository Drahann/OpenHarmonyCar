# car-agent — 车载无界面轻 agent（紫派 OpenHarmony）

常驻紫派（RK3566 / OH 5.0）的**无界面 ArkTS 节点**：入会软总线持 `FleetMission` 黑板，把协同决策翻成
本机 9 字节 UDP 下发给 `udp2lcm`（localhost），把心跳位姿写回黑板。取代旧"每车装整 CarApp + startAbility"。

> **规划/设计单一事实源**：[`../docs/car-agent-plan.md`](../docs/car-agent-plan.md)（§6 = 设计）。
> 状态：**代码已完成（code-complete）**——逻辑全部实现、自带共享层、纯逻辑测试通过。
> ⚠️ 全部 ArkTS **未经 DevEco 编译验证**；但 **DevEco 工程脚手架已就位**（2026-06-12，从 DevEco 5.1 空项目移植
> `build-profile`/`hvigor`/`resources`：模块名 `entry`（与目录同名，避免 DevEco 启动解析歧义）、bundle `com.example.carapp`、SDK 5.0.0(12)）。
> 现在可直接用 DevEco 打开 `car-agent/` 构建——剩自动签名 + 首次编译修正 + 真机联调（见下「构建与安装」「剩余」）。

## 结构（均已实现）

```
car-agent/entry/src/main/
  ets/agent/AgentCore.ets                        ✅ 运行时核心（与 ability 解耦）：bind 本机UDP+保活 / 入软总线持黑板 /
                                                    黑板变更→reconcile→发UDP / 心跳→节流写回黑板。可被任意宿主托管
  ets/agentability/AgentAbility.ets              ✅ 运行时入口=**UIAbility 外壳**托管 AgentCore（headless ServiceExtension 是系统
                                                    API、第三方公共 SDK 编不过的过渡形态；上紫派系统应用时换 ServiceExtension 壳即可，core 不动）
  ets/pages/AgentStatusPage.ets                  ✅ UIAbility 极简状态页（仅因 UIAbility 需窗口内容）
  ets/pairingability/PairingAbility.ets + pages/PairingPage.ets  ✅ 一次性配对 UIAbility（配网期拉起处理系统 PIN）
  ets/reconciler/Reconciler.ets                  ✅ 黑板→本机命令 纯决策（幂等），镜像 tools/verify/verify-reconciler.mjs(9/9)
  ets/{model,constants,utils,service}/           ✅ 自带共享层（见下）
  module.json5                                   ✅ AgentAbility(mainElement·UIAbility)+PairingAbility 声明（资源已接；deviceTypes/开机自起待 A 校验）
```

## 自带共享层（与 app-harmony 同源 · 改一处改两处）

DevEco 不可本地配 HAR，故按 car-agent-plan §6.1 的 sanctioned 退路：**精确复制** app-harmony 的 UI 无关层到本模块：

| 文件 | 来源 | 说明 |
|---|---|---|
| `model/protocol.ets`、`model/mission.ets`、`model/geometry.ets` | app-harmony 同名 | 编解码/命令枚举、黑板 schema、坐标 |
| `constants/protocol.ets`、`constants/debug.ets` | 同名 | 端口/字节布局、日志开关 |
| `utils/log.ets`、`service/RobotTransport.ets` | 同名 | 日志、UDP（agent 只对 127.0.0.1） |
| `service/FleetMissionService.ets` | **改编版** | 无界面适配：基类 `Context`、**不交互申请权限**（DISTRIBUTED_DATASYNC 须预授权） |

⚠️ 这些是**副本**——改 app-harmony 的对应文件要同步改这里。**后续目标 = 抽 shared-core HAR 去重**（在 DevEco 里做）。

## 验证

- 纯逻辑（无需 DevEco）：`node ../tools/verify/verify-reconciler.mjs`（Reconciler 9/9）、`node ../tools/verify/verify.mjs`（协议/坐标 17/17）。
- 本机闭环（待"假平板"驱动黑板）：起 `../tools/mock-purplepi` 当本机栈（127.0.0.1:5001），跑 agent↔栈；平板黑板侧可扩 `tools/mock-app`。
- 真机（P5，与成员A）：紫派部署、真 `udp2lcm`、真软总线。

## 构建与安装（DevEco / hdc）

car-agent 现在是**独立 DevEco 工程**（与 app-harmony 同级），单独打开 `car-agent/` 目录即可：

1. **打开**：DevEco Studio → Open → 选 `W:\Project\OpenHarmonyCar\car-agent`，等 `ohpm install` / Sync 完成。
2. **签名**：File → Project Structure → Signing Configs → 勾 *Automatically generate signature*。
   - bundle = `com.example.carapp`（与平板 app-harmony 同名，DDO 黑板同步硬前提）；已有该 bundle 的调试证书可复用。
   - **先把紫派设备连上**让 DevEco 登记进调试 profile，否则装机报"设备不在 profile"。
3. **编译**：Build → Build Hap(s)/APP(s)。产物 ≈ `entry/build/default/outputs/default/entry-default-signed.hap`。
   - 首次大概率有 ArkTS/装饰器/API 报错要修（同 app-harmony 首编）。
4. **装机（紫派 RK3566）**：`hdc -t <紫派connectKey> install -r <上述 .hap>`；或 DevEco 直接 Run（目标选紫派）。
5. **起服务**：`AgentAbility`（UIAbility 外壳，托管 AgentCore）需开机自起/被拉起；首配网走 `PairingAbility`（见 `../docs/archive/distributed-trust.md`）。
   - ⚠️ headless 形态(ServiceExtensionAbility)是系统 API、第三方公共 SDK 编不过 → 当前用 UIAbility 外壳过渡；上紫派**系统应用**(Full SDK+系统签名)时再换回 ServiceExtension 壳托管同一 AgentCore。见 `../contracts/integration-qa.md` Q6.1。

> 平板 App 与本 agent **同 bundle、不同设备**：平板装 app-harmony 工程的 hap、紫派装 car-agent 工程的 hap（两者模块均名 `entry`、但来自不同工程、装不同设备），靠同 bundleName + 软总线信任环同步黑板。**不要**把平板那套 hap 装到紫派。

## 剩余（真机）

1. **首次编译修正**：hvigor 首编的 ArkTS/装饰器/API 报错（同 app-harmony）。
2. **预授权**：`ohos.permission.DISTRIBUTED_DATASYNC` 在紫派侧预置授权（无界面服务弹不出框）。
3. **headless 自起**：紫派侧让 `AgentServiceAbility` 开机/按需常驻（OH5.0 service 扩展自起方式待与 A 校验）。
4. **真机联调**：前提见 `../contracts/integration-qa.md` **Q6**（OH5.0 常驻 hap、5001 端口互斥、软总线信任、地图方案B）。
