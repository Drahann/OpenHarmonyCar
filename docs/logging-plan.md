# 日志 / 调试方案（logging-plan）

> **状态：方案（2026-06-07）。实现由定时任务分阶段完成**（用户："先写方案，之后由定时任务完成"）。
> 动机：为方便调试 App↔紫派协议、软总线黑板、以及**无界面的车载 agent**（日志是它唯一的窗口）。
> 关联：[`car-agent-plan.md`](car-agent-plan.md)（L3 即 agent 日志）、[`app-refactor-plan.md`](app-refactor-plan.md)、`tools/` 的 mock。

## 现状（2026-06-07 盘点）

- App 侧 **30 处 ad-hoc 日志**、`console.*` 与 `hilog` **两种混用**（`EntryAbility`/`FleetMissionService` 用 hilog，
  `RobotTransport`/`MapService`/`screen`/pages/`componentUtils` 用 console.*）；**无统一 TAG/级别/开关、无线缆 trace、无文件 sink**。
- `RobotTransport` 目前只在**发送失败**时记日志——看不到真实上线的字节（集成 bug 几乎都在这）。
- `tools/mock-purplepi`、`tools/mock-app` 已有不错的 TX/RX/心跳打印（可作样式参照）。

## 目标 / 原则

1. **一个可过滤 sink**：统一到 `hilog`（固定 DOMAIN + 每模块 TAG），DevEco HiLog 窗口 / `hdc hilog` 可按 TAG 精准过滤。
2. **关键路径"线缆级"可见**：9 字节帧 TX/RX 能 hex + 解码打印。
3. **默认安静**：verbose 一律挂开关；**热路径（500ms 心跳、每帧 Canvas 渲染）不打 info 级**（否则刷屏）。
4. **App 与 agent 共用同一 log util**（属共享层，见 car-agent-plan §2「最大复用 model/service 无 UI 层」）。
5. **日志是运行时产物 → `.gitignore`，绝不提交日志文件**（符合仓库"不提交大文件/产物"约定）。

## 分阶段（定时任务按序实现）

### L1 共享日志核心 + 协议线缆 trace（最高价值，先做；属 car-agent P1 的共享基底）
- 新增 `utils/log.ets`（**共享**，App 与 agent 同用）：薄封装 `hilog`；固定 DOMAIN（如 `0xD002`）；
  `Log.scoped(tag)` 返回带 `.d/.i/.w/.e` 的记录器；全局级别 + 开关常量。
- `constants` 加调试开关：`DEBUG_WIRE`（默认 `false`）等。
- `RobotTransport`：在 `send` / 心跳 send / `on('message')` 三处，`DEBUG_WIRE` 开时 hex-dump + 解码一帧
  （命令名 / runState / speed / endX·endY 或 x·y·r / 源 IP）。复用 `model/protocol` 的枚举名，别另写一份。
- ✅ **收敛散落 `console.*`（services/pages/utils）到 `Log`**（2026-06-08 完成）：services（RobotTransport/MapService）+ utils（screen/componentUtils=TAG `Dialog`）+ pages（HomePage/ControlPage/SetIPPage/DeviceTrustPage 的路由/会话错误 → `Log.scoped(页名)`，去掉冗余 `[页名]` 前缀，TAG 已承载）。**全仓 `console.*` 归零**，verify 17/17。
  - ✅ **余尾完成（2026-06-08）**：`EntryAbility` / `FleetMissionService`（app + agent 两份）/ `EntryBackupAbility` 的裸 `hilog`（DOMAIN 0x0000/0xFF00）**全部并入 `Log`**（同一 `LOG_DOMAIN=0xD002`）。**全仓 `hilog` 仅剩 `utils/log.ets` 一处封装、`console.*` 归零 → L1 完成。**
- 验证：`node tools/verify/verify.mjs` 不受影响（纯逻辑未动）；真机在 DevEco HiLog 按 TAG 看到 TX/RX。

### L2 黑板 / 软总线 trace（distributed）
- `FleetMissionService`：每次 `distributedDataObject` change，`DEBUG_BLACKBOARD` 开时打 `phase/assignment/robots` 的 delta。
  软总线不同步是"沉默故障"，无日志不可见。

### L3 车载 agent 日志（随 agent P2/P3 实现）—— 刚需
- agent **无界面**，复用 L1 的 `Log` + 增 **滚动文件 sink**：写 app files 目录（如 `agent.log`，限大小+轮转），
  跑完用 `hdc file recv` 拉取。是真机字段调试的主手段。
- **reconciler 决策日志**（黑板变更 → 翻成哪条本机 UDP 命令、为何）+ **心跳回写日志**（localhost 心跳 → 写黑板哪些字段）。

### L4 mock 工具增强（小、可选）
- `mock-app` / `mock-purplepi`：行首加时间戳；`mock-purplepi` 加 `--quiet/--verbose`；可选 `--log FILE` tee 到文件。

## 不做

- 不引第三方日志库（`hilog` 足够）；不在热路径常开 verbose；release 关 `DEBUG_WIRE`；**不提交任何 `.log` 文件**（加 `.gitignore`）。
