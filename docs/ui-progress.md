# App UI 进度（定时任务 · 单一事实源）

> 本文件是**定时自动推进**的状态源。每次定时 agent 运行：读本文件 → 做"下一项未完成" → 勾选 + 追加运行日志 + 标下一步。
> 目标：结合本机 `W:\Project\OpenHarmonyCar\style\` 的**五套设计融合**出 App 最终版 UI，全量实现 ArkTS（**未经 DevEco 验证**，待真机/DevEco 校验）。
> 计划详见 `docs/app-refactor-plan.md`（UI 阶段、目标架构、§连接与设备发现）。
> 定时任务（2026-06-07 再更新）：CronCreate **`55a7ea1a`**（`12 16,22,4,10 * * *`，每 6h，下次≈16:12；旧 `2a874726`/`3daa5173` 已删）。**UI U1–U11 + tools T1 已完成 → 现优先自动推进新任务「车载轻 agent」（`docs/car-agent-plan.md`，下一项 P1 设计细化）**；tools T2 低优先。**session-only**（本环境 durable 不生效）+ 约 7 天后自动过期：若本会话被 /clear 或关闭即失效，需在新会话重建（见末尾"任务自愈"）。

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
- [x] **U3 `component/MapCanvas.ets`** ✅（2026-06-07 run3）：方形 Canvas 渲染——障碍格（`parseMap`/`forEachWallRect`，左上原点）+ 叠加层（机器人 pin+朝向线+车号、目标点环+十字、大区域/子区域矩形，均 `mapToCanvas`+half 对齐中心↔左上原点差）；双指 `PinchGesture` 缩放（clamp 0.5–8）+ `PanGesture` 平移（ctx.translate/scale 变换）+ `TapGesture` 选点（逆变换→`canvasToMap`→`onPickPoint(真实坐标)`）；叠加层按 1/scale 反缩放保持视觉恒定；`side` 由 `onAreaChange` 取短边动态、解析用 `(side,side)` 接 U2。刷新约定：上层 bump `renderVersion`(@Prop+@Watch) 触发 redraw（Canvas 不自动重绘）。⚠️ 未经 DevEco。
- [x] **U4 `component/Joystick.ets`** ✅（2026-06-07 run4）：圆盘+摇杆头（iOS 质感软阴影），`PanGesture` 拖拽、限幅在可移动半径内、松手回中=stop。**每实例独立节流**：`throttleTimer` 是实例字段（非旧全局 `taskId`），按住每 `THROTTLE_INTERVAL_MS` 重发当前方向、方向切换即时补一帧、`aboutToDisappear` 清理。方向映射：上=go/左=left/右=right/中心死区+下半区=stop（协议无后退）。**解耦传输**：只产 `(MoveDirection,speed)` 经 `onDirection` 回调，由 ControlPage 路由 `RobotTransport` 多目标。⚠️ 未经 DevEco。
- [x] **U5 `component/DeviceList.ets`** ✅（2026-06-07 run5）：纯展示列表（发现行=在线点+巡检车+IP mono+连接/已连接胶囊、空态、底部"手动添加 IP"兜底、扫描中 spinner），动作经 `onScan/onConnect/onManualAdd` 注入。配套在 `service/RobotTransport` 加 `discover(onUpdate,durationMs,ping)`：开广播→周期向 `255.255.255.255:5001` 发探测→按源 IP 去重收集→自动停+返回手动停函数；**探测命令暂用 cmd0 兜底**（A 确认 Q5 的 0x06 发现 ping 前会触发未选中车 3s 急停，仅联调；确认后换码并同步 contracts）。⚠️ 未经 DevEco。
- [x] **U6 `pages/HomePage.ets`** ✅（2026-06-07 run6）：顶栏(标题+发现设备)+3 模式 chip(单机导航/全路径/多机协同)+发现车卡片列表(在线点+IP mono+进入箭头)+空态。**修旧累积 bug**：列表恒由 @State `devices` 整体重建（`discover` 已按 IP 去重回传全量），绝不在生命周期 push。点卡 → `router.pushUrl('pages/ControlPage',{mode,ip})`。⚠️ 未经 DevEco。
- [x] **U7 `pages/ControlPage.ets`** ✅（2026-06-07 run7）：单一参数化页组合 MapCanvas+Joystick+DeviceList，取代旧 4 克隆页。Google Maps 范式：全屏地图 + 顶部连接条 + 右下缩放 FAB(经 MapCanvas `viewCmd` 钩子)+ 摇杆浮层 + 按 mode 变体操作卡（astar 选点/开始(3)/取消(4)；fullpath 开始(102)/停止(103)；distributed 两点划区→107/108(byte1=robotId 经 runState)+各车进度条）。**连接避副作用**：进页不发 cmd0(=建图)，仅中性保活(pending+stop)、收心跳判在线、state==3 更新位姿并 bump 重绘。摇杆多目标下发 + setHeartbeatPayload 防节流间隙急停。⚠️ 未经 DevEco；fullpath 选房间/distributed 进度细化待真机。
- [x] **U8 `pages/SetIPPage.ets`** ✅（2026-06-07 run8）：F&F 极简表单，手填车 IP（`TextInput` mono）+ `Storage.isValidIp` 校验（无效红框+提示）+ "保存并连接"（`Storage.setRobotIp(0,ip)`→`router.replaceUrl ControlPage(astar,ip)`）。降级入口，正常仍走发现。⚠️ 未经 DevEco。
- [x] **U9 路由与入口** ✅（2026-06-07 run8）：`resources/base/profile/main_pages.json` 注册 `pages/{LoadingPage,HomePage,ControlPage,SetIPPage}`；`EntryAbility.onWindowStageCreate` `loadContent` 改指 `pages/HomePage`（替换占位 `LoadingPage`）。⚠️ 未经 DevEco。
- [x] **U10 资源与图标** ✅（2026-06-07 run9）：UI 全程 token 驱动配色 + 字形/自绘图标（↑←→/＋－⊙/〉/●），**不新增第三方 media**；既有 `resources` 已无 原神/genshin 残留、label=巡检机器人。仅把启动窗背景 `start_window_background` 由白改品牌页底 `#EDF4F1` 对齐。旧未用 media（Oreui_*/Joystick.png 等）暂留不删（避免误伤图标引用），留作后续清理。
- [x] **U11 自测与回写** ✅（2026-06-07 run9）：`verify.mjs` **17/17**（全程未动镜像算法，无需同步）；回写 `app-harmony/README.md`（架构补 component/pages/utils/screen + theme，状态改 UI 全量实现/未经 DevEco）、`docs/app-refactor-plan.md`（UI 阶段完成）、本文件。无新增待 A 问题（发现 ping 仍归 integration-qa.md Q5）。

## tools 阶段（UI 清单全完成后才进；用户要求**先不做多车 fleet**）
- [x] **T1 `tools/mock-app/`** ✅（2026-06-07 run10）：`mock_app.py`（PC 命令驱动器：交互/`--cmd` 发 connect/go/left/right/stop/goto/cancel/endmap/loadmap/fpstart…/corner1·2/raw、`discover` 广播 0x06、`keepalive on|off` 验证保活/急停；后台打印心跳 state/x/y/r；编解码 `>BBBhhh` 与 protocol.ets/mock-purplepi 一致）+ `smoke_test.py`（白盒编解码 + 黑盒起 mock-purplepi 跑 connect→心跳/goto→位姿推进/0x06→回身份，**8/8**）+ 重写 README。mock-purplepi smoke 仍 **6/6**。⚠️ 真机默认广播；本机自检单播兜底（广播跨虚拟网卡不回环）。发现 ping 0x06 待 A 确认(Q5)。
- [ ] **T2** 视需要补 `replay/` 等；**多车 `mock_fleet` 暂缓**（用户指示）。

## 任务自愈（若定时任务失效）
session-only 任务在 /clear 或关闭后消失。新会话若发现没有 `55a7ea1a`（`CronList` 查），按本文件顶部参数（cron `12 16,22,4,10 * * *`）用 `CronCreate` 重建，prompt 用"读 MEMORY+本进度文档+`docs/car-agent-plan.md` → 做下一项未完成（UI+T1 已完→**车载 agent P1 设计细化**）→ smoke 测 → 按 [[feedback-docs-main-code-branch]] 提交 → 收尾"的自举式提示（要点见记忆 [[app-refactor-plan]] / [[car-agent-plan]]）。

## 运行日志（每次追加一行）
- 2026-06-06 20:1x（设定·本会话）：建定时任务 `3daa5173`（session-only，每 6h，首次≈+5h=06-07 01:12）+ 写本进度文档。**下一步 = U1 风格融合**。
- 2026-06-07 ~01:12（自动·run1）：完成 **U1 风格融合**——`constants/theme.ets`（统一 token）+ `docs/ui-design.md`（五套融合决策/各屏/组件/导航）；顺手 `.gitignore` 补忽略 `.claude/` 运行文件并 untrack `scheduled_tasks.lock`。`verify.mjs` 仍 17/17。**下一步 = U2 动态屏幕（display.getDefaultDisplaySync 工具）**。
- 2026-06-07（手动·run2，用户在场）：完成 **U2 动态屏幕**——新增 `app-harmony/entry/src/main/ets/utils/screen.ets`（动态取屏 + 懒加载缓存 + `refreshScreen` + 方形地图画布边长/半视口派生 + 失败回退），取代旧写死 2199×1533 与模块加载期硬派生的面板常量；未沿用旧 `screenWidth9/3/1` 面板布局数（新设计为全屏方形地图，布局交各组件）。`verify.mjs` 仍 17/17（未动镜像算法，display/px2vp 不可在 Node 跑、不镜像）。**下一步 = U3 `component/MapCanvas.ets`**（地图渲染/缩放平移/选点/多车位姿；用 U2 的 `mapCanvasSideVp`+`halfMapViewportVp` 接 MapService/geometry）。
- 2026-06-07（手动·run3，连续推进）：完成 **U3 `component/MapCanvas.ets`**——见 U3 勾选项。复用 `MapService.parseMap`/`forEachWallRect` + `geometry.mapToCanvas`/`canvasToMap`（中心原点 vs 左上原点差以 +half 统一）；缩放/平移走 Canvas 2D ctx 变换、选点用逆变换还原；叠加层 1/scale 反缩放。`verify.mjs` 仍 17/17（未动镜像算法）。⚠️ ArkUI 装配未经 DevEco；@Prop 数组深拷贝/`renderVersion` 刷新模型待 DevEco 真编校验。**下一步 = U4 `component/Joystick.ets`**（每实例独立节流的摇杆遥控）。
- 2026-06-07（手动·run4，连续推进）：完成 **U4 `component/Joystick.ets`**——见 U4 勾选项。核心=每实例 `throttleTimer`（修旧全局 taskId 抢占）；解耦传输（产方向回调，传输路由交 ControlPage）。`verify.mjs` 仍 17/17。**下一步 = U5 `component/DeviceList.ets`**（方案B 广播发现+点击连接+手填兜底入口）。
- 2026-06-07（手动·run5，连续推进）：完成 **U5 `component/DeviceList.ets`** + `RobotTransport.discover()`——见 U5 勾选项。`verify.mjs` 仍 17/17（未动镜像算法；RobotTransport 非镜像）。⚠️ ArkUI 未经 DevEco；广播发现待真机/A 确认。**下一步 = U6 `pages/HomePage.ets`**（机器人列表+模式选择，修旧 onPageShow 累积 bug）。
- 2026-06-07（手动·run6+7，连续推进）：完成 **U6 `pages/HomePage.ets`** + **U7 `pages/ControlPage.ets`**（耦合：Home 路由 → Control）——见 U6/U7 勾选项。给 MapCanvas 加了 `viewCmd`(@Prop+@Watch) 缩放钩子供 ControlPage FAB 驱动。`verify.mjs` 仍 17/17。⚠️ 路由待 U9 注册后可达；ArkUI 未经 DevEco。**下一步 = U8 `pages/SetIPPage.ets`**（手填 IP 兜底，走 service/storage）。
- 2026-06-07（手动·run8，连续推进）：完成 **U8 `pages/SetIPPage.ets`** + **U9 路由与入口**——见 U8/U9 勾选项。`main_pages.json` 注册 4 页、入口改指 HomePage（页面路由现自洽可达）。`verify.mjs` 仍 17/17。⚠️ ArkUI 未经 DevEco。**下一步 = U10 资源与图标**（多为 token 驱动+字形图标，预计轻量）。
- 2026-06-07（手动·run9，连续推进收尾）：完成 **U10 资源与图标** + **U11 自测与回写**——见 U10/U11 勾选项。**🎉 UI 清单 U1–U11 全部完成。** `verify.mjs` 终检 17/17。⚠️ 全部 ArkUI 未经 DevEco 真编，下一阶段务必先在 DevEco 构建一遍修类型/装饰器/手势 API 偏差，再真机校验。**下一步 = tools 阶段 T1 `tools/mock-app/`**（PC 命令驱动器，先不做多车 fleet）。本会话连续推进 U2→U11（用户要求做到额度~90% 后转定时任务）。
- 2026-06-07（手动·run10，连续推进）：完成 **tools T1 `tools/mock-app/`**——见 T1 勾选项。mock-app smoke 8/8、mock-purplepi smoke 6/6、verify.mjs 17/17。**新增任务（用户 2026-06-07）：开发车载 OpenHarmony 轻 agent**，按"先文档规划→逐步设计→实现"——本轮已出规划文档 `docs/car-agent-plan.md`（见下条）。**下一步 = 车载 agent 设计细化**（或 tools T2 视需要）。
