# docs/archive/ — 已完成工作的归档文档

这里收纳**工作已完成、不再驱动后续开发**的文档：一次性复审、被实现取代的进度/设计/引导单。
它们作为**历史记录**保留（决策与根因可追溯），但**不是接续入口**。

> **当前活跃入口**：真机调试 → [`../debug-checklist.md`](../debug-checklist.md)；架构/重构参照 →
> [`../app-refactor-plan.md`](../app-refactor-plan.md)；车载 agent → [`../car-agent-plan.md`](../car-agent-plan.md)；
> 视觉对接 → [`../vision-integration-plan.md`](../vision-integration-plan.md)。契约一律以 [`../../contracts/`](../../contracts/) 为准。

## 清单（为什么归档 / 成果落在哪）

| 文档 | 性质 | 成果已落地到 |
|---|---|---|
| `feature-parity-review.md` | 测试前功能对等性复审（一次性） | 建图卡 / fullpath 选算法+选顶点 / 摇杆速度等已补回 `ControlPage`；结论见 `debug-checklist.md` A |
| `safety-flow-review.md` | 流程/连接安全复审（一次性） | P1 连接门控已修（实时心跳判据）；剩 P2/P4 决策入 `contracts/integration-qa.md` |
| `op-card-guidance.md` | astar/fullpath/distributed 操作卡分步引导设计 | 已实现于 `ControlPage` 三类操作卡文案 |
| `map-ui-redesign.md` | 地图 UI 重设计（浅色建筑图纸 + 平滑矢量墙体） | 已实现 `model/mapContour.ets` + `MapCanvas`（真机观感验收并入 `debug-checklist.md` B7） |
| `logging-plan.md` | 日志/调试方案（hilog 统一 + 线缆 trace） | L1 已实现（`utils/log.ets` + `RobotTransport` `DEBUG_WIRE`）；全仓 console 归零 |
| `ui-design.md` | UI 五套融合设计（U1 产出） | token 落地 `constants/theme.ets`；后续以 `ui-polish-plan.md` 为现行视觉权威 |
| `ui-progress.md` | UI 阶段进度单（曾由定时任务推进） | U1–U11 全部完成；定时任务已取消；进度并入 `debug-checklist.md` |
| `distributed-trust.md` | 软总线设备互信方案（DDO + 一次性配对）决策记录 | 决策已定并实现（平板发起 `bindTarget` + 紫派 HDMI 确认 PIN + agent `bundleName=com.example.carapp`） |

> 归档不改这些文档的内文；如需查其细节直接打开对应文件。其中的旧色值/旧接口描述按"历史记录"理解，**不回改**，现行以契约与活跃文档为准。
