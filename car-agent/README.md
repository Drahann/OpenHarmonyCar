# car-agent — 车载无界面轻 agent（紫派 OpenHarmony）

常驻紫派（RK3566 / OH 5.0）的**无界面 ArkTS 节点**：入会软总线持 `FleetMission` 黑板，把协同决策翻成
本机 9 字节 UDP 下发给 `udp2lcm`（localhost），把心跳位姿写回黑板。取代旧"每车装整 CarApp + startAbility"。

> **规划/设计单一事实源**：[`../docs/car-agent-plan.md`](../docs/car-agent-plan.md)（§6 = 设计）。
> ⚠️ 全部 ArkTS **未经 DevEco 验证**；本模块尚在 P2 脚手架阶段。

## 结构

```
car-agent/entry/src/main/
  ets/reconciler/Reconciler.ets          ✅ 黑板→本机命令 纯决策（幂等），Node 镜像 tools/verify/verify-reconciler.mjs(9/9)
  ets/serviceextability/AgentServiceAbility.ets  🚧 无界面常驻入口（订阅黑板→reconcile→发UDP；心跳→写回黑板）
  module.json5                           🚧 ServiceExtensionAbility 声明（schema 待 DevEco/A 校验）
```

## 依赖共享层（shared-core HAR，待抽取，见 car-agent-plan §6.1）

复用 app-harmony 的 UI 无关层（**勿各写一份**）：`model/protocol`(编解码/命令枚举)、`model/mission`(黑板 schema)、
`model/geometry`、`service/RobotTransport`(目标=127.0.0.1)、`service/FleetMissionService`(黑板)、`utils/log`、`constants/`。
P2 第一步把这些抽成 HAR `shared-core`，`app-harmony` 与 `car-agent` 同依赖。**抽取前** Reconciler 的类型/命令码本地占位。

## 验证

- 纯逻辑：`node ../tools/verify/verify-reconciler.mjs`（Reconciler 映射/幂等，9/9）。
- 本机联调（待 glue 接好）：起 `../tools/mock-purplepi` 当本机栈（127.0.0.1:5001），跑 agent↔栈闭环；
  平板侧黑板可由"假平板"驱动（待建，或扩 `tools/mock-app`）。
- 真机（P5，与成员A）：紫派部署 hap、真 `udp2lcm`、真软总线信任。前提见 `contracts/integration-qa.md` Q6。

## 待成员A（紫派）—— integration-qa.md Q6

OH5.0 能否常驻无界面 hap 并存其 C++ 栈/开机自起；本机 5001 端口 agent 与外部平板互斥；软总线设备信任配置；地图先方案B。
