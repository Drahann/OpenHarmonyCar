# 真机调试清单 / 新会话接续单（2026-06-08，2026-06-09 刷新：含地图 UI 重设计 + A 答复归档）

> owner 准备开**新对话专门做真机调试**。本文是新会话的**入口**：先读它，再读 `MEMORY.md` + `docs/archive/ui-progress.md`（run 日志）
> + 相关专题文档。App 代码侧主流程**已 code-complete**（每步有引导/反馈），但**全部 ArkTS 未经 DevEco**，需真机逐项验。
> 分支：代码在 `app-harmony-core`、文档同步 `main`（见记忆 [[feedback-docs-main-code-branch]]）。本机纯逻辑 `node tools/verify/verify.mjs` 应 **38/38**。

## A. 已做完（code-complete，待真机验）

- **连接动态状态** ✅：HomePage 车列表 4 态（已发现/连接中/已连接·心跳·新鲜度/丢失，`RobotTransport.connStateOf` 派生、600ms 刷新）；点「连接」起中性保活（`connectExclusive`，单连接）；`startHeartbeat` **立即发首帧**（点连接即 ping→车回心跳→秒变已连接，无需再扫描）。ControlPage 顶栏实时连接点/文案。
- **建图流程** ✅：开始建图(**`'m'`/0x6d 强制重建**,toast)→**摇杆仅建图阶段出现**→结束建图(cmd2)→**阻塞遮罩"载入地图中"**→成功转操作卡 / 失败弹「重试/关闭」。（⚠️ 旧用 cmd0，但 A 的 cmd0 有图时只当心跳不建图，已改 `'m'`）
- **地图解析** ✅（本轮大修）：真机 `defultMap.txt`=7 值首行 `range resolution height width metersPerPixel x0 y0` + 空格分隔 `-1/0`；按位置取 `parts[2]/[3]` + 自动识别空格/密排归一化 grid。详见 `docs/map-pipeline.md`。
- **命令门控** ✅：所有命令 + 地图点选按**实时心跳**门控；命令下发有 toast。
- **操作卡分步引导** ✅：astar/fullpath/distributed 三卡「下一步该干嘛」。详见 `docs/archive/op-card-guidance.md`。
- **地图 UI（2026-06-09 重设计）** ✅：浅色「建筑图纸」+ 平滑矢量墙体（marching squares→Chaikin，取代栅格方块）；底部建图/操作卡改 **bottom-docked sheet**（修溢出 + 手势透传）；通信 **TX 日志**增强（显式命令始终打印解码+hex）。详见 `docs/archive/map-ui-redesign.md`、`docs/archive/logging-plan.md` L1。真机验观感见 B7。
- **车载 agent** ✅ code-complete（含一次性配对 UIAbility、bundleName=com.example.carapp）；详见 `docs/car-agent-plan.md` §6.7。

## B. 真机调试清单（逐项验，按优先级）

1. **🔴 地图渲染**：建图后障碍能正确显示（非空气图/乱）。**排查**：看 HiLog `MapService` 的 `[parseMap] 首行="..." → height=.. width=..`——确认 height/width 是**合理正数**（不是 -45/-44）；`[mapLooksComplete]` 的 `dataRows/expected`。若仍乱，把该日志 + `defultMap.txt` 首行贴出来。
2. **🔴 地图定位（x0/y0）**：机器人 pin / 选点位置是否准。**若整体偏一个常量**→ 就是 `x0/y0` 世界偏移未校正。**✅ A 已答 Q12**：x0/y0 **单位=米**、`grid=(world−x0)/metersPerPixel`（详见 `map-pipeline.md` §5）。**需要**：parseMap 打印的首行（含 x0 y0）+ 一个已知物理点的"真实坐标 vs 屏幕显示位置"**核对符号** → 落地 §5 校正（改 geometry + MapCanvas + onPick；**公式已定，本轮未改码**）。
3. **连接状态翻转**：发现→点连接→**1–2 秒内**变"已连接·心跳正常"、不需再扫描。**排查**：开 `constants/debug.ets` 的 `DEBUG_WIRE=true` → HiLog `RobotTransport` 线缆 trace 应见 TX(连接帧)→RX(500ms 心跳)。无 RX=车没回心跳（网络/端口）；有 RX 但不翻转=UI 刷新问题（connStateOf/600ms）。
4. **建图闭环**：开始建图→摇杆能驱动车→结束建图→遮罩→拉到图转操作卡。注意 cmd2 后紫派先写 `unprobdefultMap.txt` 再优化出 `defultMap.txt`（有延迟，`loadMap` 重试 8×1.5s 兜着）。
5. **操作卡**：astar 选点→开始导航（pin 移动）；fullpath 选 4 顶点→自动启动；distributed 划区→（有 agent 才能真跑多车）。
6. **命令生效**：UDP 无 ack，toast 只表"已发出"；真实生效靠机器人动/心跳位姿/进度间接看（心跳状态字节=Q1：✅ A 答 UDP 暂未暴露，后续靠 agent 写黑板）。
7. **🆕 地图观感（2026-06-09 重设计，真机验）**：墙体是否成**顺滑矢量线**（非方块/锯齿）、白图纸 vs 灰未知区对比、泪滴目标 pin、机器人朝向圆点与品牌墨绿是否协调；底部 sheet 三态（尚无图/建图中/操作）**贴底不溢出** + 地图在 sheet 外区域手势正常（验 `HitTestMode.None` 透传）。卡顿则关 `ENABLE_MOTION` / 降 `CONTOUR_MAX_LEN`。详见 `docs/archive/map-ui-redesign.md` §7。

## C. 成员A 答复（✅ 2026-06-08 已全部答复 Q1–Q12，见 `contracts/integration-qa.md`「状态总览」）
- **Q12** 地图格式确认（7 值首行 + 空格 -1/0）；**x0/y0 单位=米**、`grid=(world−x0)/metersPerPixel` → 定位校正公式已定（`map-pipeline.md` §5），待真机标符号 + 落码。
- **Q11** 多车覆盖（LCM122）**不支持**选算法 → App 多车不显示算法（**已符合**：算法 chip 仅单车 fullpath）。 **Q10** cmd105 维持旧 `[1,2,4,6]`（App **无需改**）。 **Q9/Q6.2** 互信方案确认（账号无关 PIN / HDMI 确认 / agent `bundleName=com.example.carapp`）、distributed 阶段 master 由 agent 独占（平板不直连 master）。
- **🔴 A 留的真机尾巴（Q9）**：紫派 DM 系统 PIN 弹窗是否真可点 / 接受绑定是否要 agent 在运行 / 被发现+绑定所需权限 —— 配网期接 HDMI 时一并验。

## D. 待 owner 决策
- **P2**：distributed 覆盖阶段连接语义——(a) 纯黑板（靠 agent）/ (c) 平板直连多车（无 agent 测试）。建图阶段已定=直连单车（遥操作）。见 `docs/archive/safety-flow-review.md` §2/§4。

## E. 怎么跑
- 真机：DevEco 打开 `app-harmony` → 编译（**首次可能有 ArkTS 类型/装饰器/手势/Canvas API 报错需修**，app-harmony 早前踩过：成员名撞通用属性等）→ 签名（product 加 `signingConfig`）→ 真机装。
- 本机纯逻辑回归：`node tools/verify/verify.mjs`（38/38，含真机地图格式 + 等高线用例）、`node tools/verify/verify-reconciler.mjs`（9/9）、`python tools/mock-app/smoke_test.py`（8/8）、`python tools/mock-purplepi/smoke_test.py`（6/6）。
- 日志：HiLog DOMAIN `0xD002`，按 TAG 过滤（MapService / RobotTransport / ControlPage / FleetMission / EntryAbility）。调试开关在 `constants/debug.ets`（`DEBUG_WIRE` 线缆 / `DEBUG_BLACKBOARD` 软总线 / `LOG_MIN_LEVEL`）。
