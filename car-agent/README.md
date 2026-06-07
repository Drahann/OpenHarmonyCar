# car-agent — 车载无界面轻 agent（紫派 OpenHarmony）

常驻紫派（RK3566 / OH 5.0）的**无界面 ArkTS 节点**：入会软总线持 `FleetMission` 黑板，把协同决策翻成
本机 9 字节 UDP 下发给 `udp2lcm`（localhost），把心跳位姿写回黑板。取代旧"每车装整 CarApp + startAbility"。

> **规划/设计单一事实源**：[`../docs/car-agent-plan.md`](../docs/car-agent-plan.md)（§6 = 设计）。
> 状态：**代码已完成（code-complete）**——逻辑全部实现、自带共享层、纯逻辑测试通过。
> ⚠️ 全部 ArkTS **未经 DevEco 验证**；剩 DevEco 工程集成 + 编译修正 + 真机联调（见下「剩余」）。

## 结构（均已实现）

```
car-agent/entry/src/main/
  ets/serviceextability/AgentServiceAbility.ets  ✅ 无界面常驻入口：bind 本机UDP+保活 / 入软总线持黑板 /
                                                    黑板变更→reconcile→发UDP / 心跳→节流写回黑板 / 生命周期
  ets/reconciler/Reconciler.ets                  ✅ 黑板→本机命令 纯决策（幂等），镜像 tools/verify/verify-reconciler.mjs(9/9)
  ets/{model,constants,utils,service}/           ✅ 自带共享层（见下）
  module.json5                                   🚧 ServiceExtensionAbility 声明（deviceTypes/自起/权限资源 待 DevEco+A 校验）
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

## 剩余（DevEco / 真机）

1. **DevEco 工程集成**：把 car-agent 作为模块（或独立工程）纳入构建；补 `resources/`（含 `module.json5` 引用的 `$string:perm_datasync` 等）；签名。
2. **编译修正**：首次 hvigor 构建必有 ArkTS/装饰器/API 报错要修（同 app-harmony）。
3. **预授权**：`ohos.permission.DISTRIBUTED_DATASYNC` 在紫派侧预置授权（无界面服务弹不出框）。
4. **真机联调**：前提见 `contracts/integration-qa.md` **Q6**（OH5.0 常驻 hap、5001 端口互斥、软总线信任、地图方案B）。
