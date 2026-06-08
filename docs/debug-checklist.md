# 真机调试清单 / 新会话接续单（2026-06-08）

> owner 准备开**新对话专门做真机调试**。本文是新会话的**入口**：先读它，再读 `MEMORY.md` + `docs/ui-progress.md`（run 日志）
> + 相关专题文档。App 代码侧主流程**已 code-complete**（每步有引导/反馈），但**全部 ArkTS 未经 DevEco**，需真机逐项验。
> 分支：代码在 `app-harmony-core`、文档同步 `main`（见记忆 [[feedback-docs-main-code-branch]]）。本机纯逻辑 `node tools/verify/verify.mjs` 应 **30/30**。

## A. 已做完（code-complete，待真机验）

- **连接动态状态** ✅：HomePage 车列表 4 态（已发现/连接中/已连接·心跳·新鲜度/丢失，`RobotTransport.connStateOf` 派生、600ms 刷新）；点「连接」起中性保活（`connectExclusive`，单连接）；`startHeartbeat` **立即发首帧**（点连接即 ping→车回心跳→秒变已连接，无需再扫描）。ControlPage 顶栏实时连接点/文案。
- **建图流程** ✅：开始建图(cmd0,toast)→**摇杆仅建图阶段出现**→结束建图(cmd2)→**阻塞遮罩"载入地图中"**→成功转操作卡 / 失败弹「重试/关闭」。
- **地图解析** ✅（本轮大修）：真机 `defultMap.txt`=7 值首行 `range resolution height width metersPerPixel x0 y0` + 空格分隔 `-1/0`；按位置取 `parts[2]/[3]` + 自动识别空格/密排归一化 grid。详见 `docs/map-pipeline.md`。
- **命令门控** ✅：所有命令 + 地图点选按**实时心跳**门控；命令下发有 toast。
- **操作卡分步引导** ✅：astar/fullpath/distributed 三卡「下一步该干嘛」。详见 `docs/op-card-guidance.md`。
- **车载 agent** ✅ code-complete（含一次性配对 UIAbility、bundleName=com.example.carapp）；详见 `docs/car-agent-plan.md` §6.7。

## B. 真机调试清单（逐项验，按优先级）

1. **🔴 地图渲染**：建图后障碍能正确显示（非空气图/乱）。**排查**：看 HiLog `MapService` 的 `[parseMap] 首行="..." → height=.. width=..`——确认 height/width 是**合理正数**（不是 -45/-44）；`[mapLooksComplete]` 的 `dataRows/expected`。若仍乱，把该日志 + `defultMap.txt` 首行贴出来。
2. **🔴 地图定位（x0/y0）**：机器人 pin / 选点位置是否准。**若整体偏一个常量（~x0 格≈2.25m）**→ 就是 `x0/y0` 世界偏移未校正（`map-pipeline.md` §5）。**需要**：parseMap 打印的首行（含 x0 y0）+ 一个已知物理点的"真实坐标 vs 屏幕显示位置"，再 + A 答 **Q12**（x0/y0 单位/符号）→ 落地 §5 校正（改 geometry + MapCanvas + onPick）。
3. **连接状态翻转**：发现→点连接→**1–2 秒内**变"已连接·心跳正常"、不需再扫描。**排查**：开 `constants/debug.ets` 的 `DEBUG_WIRE=true` → HiLog `RobotTransport` 线缆 trace 应见 TX(连接帧)→RX(500ms 心跳)。无 RX=车没回心跳（网络/端口）；有 RX 但不翻转=UI 刷新问题（connStateOf/600ms）。
4. **建图闭环**：开始建图→摇杆能驱动车→结束建图→遮罩→拉到图转操作卡。注意 cmd2 后紫派先写 `unprobdefultMap.txt` 再优化出 `defultMap.txt`（有延迟，`loadMap` 重试 8×1.5s 兜着）。
5. **操作卡**：astar 选点→开始导航（pin 移动）；fullpath 选 4 顶点→自动启动；distributed 划区→（有 agent 才能真跑多车）。
6. **命令生效**：UDP 无 ack，toast 只表"已发出"；真实生效靠机器人动/心跳位姿/进度间接看（心跳状态字节=Q1 待 A）。

## C. 待成员A（`contracts/integration-qa.md`）
- **Q12** 地图格式确认（7 值首行 + 空格 -1/0）+ **x0/y0 单位/符号**（定位校正用，最关键）。
- **Q11** 多车覆盖（LCM122）能否选算法。 **Q10** cmd105 IP 连续打包。 **Q9/Q6.2** 软总线互信 / distributed master localhost 互斥。

## D. 待 owner 决策
- **P2**：distributed 覆盖阶段连接语义——(a) 纯黑板（靠 agent）/ (c) 平板直连多车（无 agent 测试）。建图阶段已定=直连单车（遥操作）。见 `docs/safety-flow-review.md` §2/§4。

## E. 怎么跑
- 真机：DevEco 打开 `app-harmony` → 编译（**首次可能有 ArkTS 类型/装饰器/手势/Canvas API 报错需修**，app-harmony 早前踩过：成员名撞通用属性等）→ 签名（product 加 `signingConfig`）→ 真机装。
- 本机纯逻辑回归：`node tools/verify/verify.mjs`（30/30，含真机地图格式用例）、`node tools/verify/verify-reconciler.mjs`（9/9）、`python tools/mock-app/smoke_test.py`（8/8）、`python tools/mock-purplepi/smoke_test.py`（6/6）。
- 日志：HiLog DOMAIN `0xD002`，按 TAG 过滤（MapService / RobotTransport / ControlPage / FleetMission / EntryAbility）。调试开关在 `constants/debug.ets`（`DEBUG_WIRE` 线缆 / `DEBUG_BLACKBOARD` 软总线 / `LOG_MIN_LEVEL`）。
