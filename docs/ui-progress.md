# App UI 进度（定时任务 · 单一事实源）

> 本文件是**定时自动推进**的状态源。每次定时 agent 运行：读本文件 → 做"下一项未完成" → 勾选 + 追加运行日志 + 标下一步。
> 目标：结合本机 `W:\Project\OpenHarmonyCar\style\` 的**五套设计融合**出 App 最终版 UI，全量实现 ArkTS（**未经 DevEco 验证**，待真机/DevEco 校验）。
> 计划详见 `docs/app-refactor-plan.md`（UI 阶段、目标架构、§连接与设备发现）。
> 定时任务：CronCreate `3daa5173`（`12 1,7,13,19 * * *`，每 6h，首次≈2026-06-07 01:12）。**session-only**：若本会话被 /clear 或关闭即失效，需在新会话重建（见末尾"任务自愈"）。

## 执行约定（每次必守）
- App 代码（`app-harmony/**`）→ **只**提交 `app-harmony-core` 并 push。
- 共享文档（`docs/`、`contracts/`、`CLAUDE.md`、`tools/`，以及 `style/` 若决定入库）→ **同步 main + app-harmony-core** 并 push（流程见记忆 [[feedback-docs-main-code-branch]]：先提交分支→`git switch main`→`git checkout app-harmony-core -- <改的文档>`→commit→push→切回）。
- ArkTS 本机不可编译 → 新代码顶部标注"未经 DevEco 验证"；可跑的纯逻辑用 `node tools/verify/verify.mjs`，协议用 `python tools/mock-purplepi/smoke_test.py`。
- 需成员A（紫派）确认的问题 → 写 `contracts/integration-qa.md`。
- 每次收尾：勾选完成项 + 追加"运行日志"一行 + 更新记忆 [[app-refactor-plan]] + 按约定提交 push。
- **每次只做一个会话能稳妥完成的量**，做完即收尾，把状态写清楚留给下次；切勿一次硬做完全部。

## 风格（交给 agent 自行判断融合）
通读 `style/` 五套子目录，自行判断融合（参考定位：`new`=F&F 极简基底、`google-maps-mobile-2024`=地图屏、`medical-mobile-app`=应用骨架、`area-161-designs`=绿色数据产品配色、`plants-ecommerce-ios`=iOS 组件/字体）。先在 U1 定稿统一主题 token 与 `docs/ui-design.md`，后续各屏遵循它。

## UI 清单（按序做"下一项未完成"）
- [x] **U1 风格融合** ✅（2026-06-07 run1）：`constants/theme.ets`（AppColor/FontSize/FontFamily/Space/Radius/Elevation 统一 token）+ `docs/ui-design.md`（融合决策 + 各屏风格来源 + 组件规范 + 导航结构）。融合=F&F极简底+Maps地图范式+Area墨绿主色+iOS圆角+高饱和语义色。
- [x] **U2 动态屏幕** ✅（2026-06-07 run2）：`utils/screen.ets`——`getScreen()`（懒加载+缓存）/`refreshScreen()`（旋转/分屏后重算）/`ScreenMetrics`（px+vp 双单位，派生 `halfWidthVp`/`halfHeightVp`/`shorterEdgeVp`/`longerEdgeVp`/`mapCanvasSideVp(inset)`/`halfMapViewportVp(inset)`），取代旧 `Screen.ets` 写死 2199×1533；display 查询失败/返回非正值回退兜底。**成对喂法**：`MapService.parseMap(text, side, side)` + `geometry.canvasToMap/mapToCanvas(..., halfMapViewportVp)`（同一方形画布、各向同性）。
- [ ] **U3 `component/MapCanvas.ets`**：地图渲染（`MapService.parseMap`/`forEachWallRect`）+ 缩放/平移 + 选点（`geometry.canvasToMap`）+ 多车位姿/朝向叠加（`mission.RobotRuntime`）。
- [ ] **U4 `component/Joystick.ets`**：摇杆遥控，**每实例独立节流**（`THROTTLE_INTERVAL_MS`），输出 `MoveDirection`+speed 经 `RobotTransport`（多目标）。
- [ ] **U5 `component/DeviceList.ets`**：设备发现列表（方案B 广播发现+点击连接；A 确认 Q5 前用 `cmd0` 广播 / `0x06` ping 兜底，见 integration-qa.md）+ 连接态 + 手填 IP 兜底入口。
- [ ] **U6 `pages/HomePage.ets`**：机器人列表 + 模式选择（修旧 `Index.onPageShow` 每次 push 不清空的累积 bug）→ 路由 `ControlPage(mode)`。
- [ ] **U7 `pages/ControlPage.ets`**：**单一参数化页**（mode∈{astar|fullpath|distributed}）组合 MapCanvas+Joystick+DeviceList，取代旧 4 个克隆页（distributedPage/distributedsecondPage/distriFullPathPage/PendingComponent）。
- [ ] **U8 `pages/SetIPPage.ets`**：高级/兜底手填 IP，走 `service/storage`（英文 key、无 getter 副作用）。
- [ ] **U9 路由与入口**：`resources/.../main_pages.json5` 注册新页；`EntryAbility.onWindowStageCreate` 指向真实 `HomePage`（替换占位 `LoadingPage`）。
- [ ] **U10 资源与图标**：按融合风格补齐/替换 media 与颜色资源（沿用 bundle `com.example.carapp`）。
- [ ] **U11 自测与回写**：`verify.mjs` 仍过（若动了镜像算法同步它）；回写 `app-harmony/README.md` 架构、`docs/app-refactor-plan.md` 进度、本文件；A-问题入 `integration-qa.md`。

## tools 阶段（UI 清单全完成后才进；用户要求**先不做多车 fleet**）
- [ ] **T1 `tools/mock-app/`**：PC 命令驱动器（交互/脚本发 0-5 / 102-108、打印心跳、保活/急停验证），落地其 README 规格；兼给成员A 用。
- [ ] **T2** 视需要补 `replay/` 等；**多车 `mock_fleet` 暂缓**（用户指示）。

## 任务自愈（若定时任务失效）
session-only 任务在 /clear 或关闭后消失。新会话若发现没有 `3daa5173`（`CronList` 查），按本文件顶部参数用 `CronCreate` 重建（prompt 见记忆 [[app-refactor-plan]] 或沿用"读本进度文档→做下一项→收尾"的自举式提示）。

## 运行日志（每次追加一行）
- 2026-06-06 20:1x（设定·本会话）：建定时任务 `3daa5173`（session-only，每 6h，首次≈+5h=06-07 01:12）+ 写本进度文档。**下一步 = U1 风格融合**。
- 2026-06-07 ~01:12（自动·run1）：完成 **U1 风格融合**——`constants/theme.ets`（统一 token）+ `docs/ui-design.md`（五套融合决策/各屏/组件/导航）；顺手 `.gitignore` 补忽略 `.claude/` 运行文件并 untrack `scheduled_tasks.lock`。`verify.mjs` 仍 17/17。**下一步 = U2 动态屏幕（display.getDefaultDisplaySync 工具）**。
- 2026-06-07（手动·run2，用户在场）：完成 **U2 动态屏幕**——新增 `app-harmony/entry/src/main/ets/utils/screen.ets`（动态取屏 + 懒加载缓存 + `refreshScreen` + 方形地图画布边长/半视口派生 + 失败回退），取代旧写死 2199×1533 与模块加载期硬派生的面板常量；未沿用旧 `screenWidth9/3/1` 面板布局数（新设计为全屏方形地图，布局交各组件）。`verify.mjs` 仍 17/17（未动镜像算法，display/px2vp 不可在 Node 跑、不镜像）。**下一步 = U3 `component/MapCanvas.ets`**（地图渲染/缩放平移/选点/多车位姿；用 U2 的 `mapCanvasSideVp`+`halfMapViewportVp` 接 MapService/geometry）。
