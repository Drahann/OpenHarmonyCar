# PROGRESS · OpenHarmony 工业巡检机器人 · 演示视频

STATUS: IN_PROGRESS（用户手动迭代）
更新时间: 2026-06-16（二改·忠实真机：地图改有机等高线[MAPKIT 移植 marching squares]、机器人朝向+真实运动+拖尾、覆盖改轨迹刷扫不越框、algo-trio 忠实 ControlPage 流程、PiP 复刻 VisionPip、互信两步弹窗[请求权限→PIN 638495]、03-build 机器人转向）

## 文件结构
- `film.css`；`engine.js`(setOrder + callout/infoPanel + **SPEED 常量**)；
  `scenes.js`(00标题/01首页[含手填IP小窗]/02控制台[弃用]) `scenes2`(建图/导航) `scenes3`(覆盖/多机+PiP承接,代码保留)
  `scenes4`(仪表·实机视频) `scenes5`(旧互信/手填IP,代码保留) `scenes6`(架构/协议/数字/收尾,**移出视频但保留代码**)
  `scenes7`(界面总览 ui-* + home-enter + **setOrder**) `scenes8`(**本轮新增**：PIN 互信改版 08-trust + 三平板 algo-trio)
- **地图工具箱** `scenes0_mapkit.js`（`window.MAPKIT`，文件名排序居 scenes.js 之后、scenes2 之前）：移植
  `tools/verify/verify.mjs` 的 marchingSquares/linkLoops/chaikin → **有机等高线地图**（不再画干净矩形房间）；
  `robotSVG`（圆点+白环+**朝向三角**）+ `driveRobot`（**先转向后行驶 + 加减速** → 不僵硬）+ **彗尾拖尾**；
  `boustro` 牛耕折返。被 scenes2(建图) 与 scenes8(algo-trio) 复用，与 App `MapCanvas`/`mapContour` 同源。
- `build.sh` glob 内联打包 scenes*.js → 自包含 `film.html`（`bash build.sh`）
- **校验工具**：`node check.mjs`（jsdom 烟雾测试：跑通 boot、回读场景顺序/时长、捕获运行期异常）；
  `node shot.mjs <场景id> <场景内偏移秒…>`（系统 Edge 截图到 `_tmpframes/`，用于逐帧目视核对）

## 现行顺序（10 场景 ≈ 1:52；**SPEED=1 原速**；0 报错/无章节卡/自包含）
00-title(4.5s) → ui-home → 01-home(发现设备+手填IP小窗+末尾点**设备互信**) → ui-trust → **08-trust(PIN 互信演示)**
→ home-enter(回首页点进入控制) → ui-control → 03-build(建图) → **algo-trio(三平板同时演 A*/全覆盖/分布式 → 三合一 → 点摄像头→PiP→放大)** → 07-vision(仪表·实机视频)

## 本轮已完成（用户指示）
- [x] **速度回 1×**：engine `SPEED=2→1`（节奏回作者意图；改速只调 engine.js 顶部 SPEED）
- [x] **开始页缩短**：00-title 11s→4.5s
- [x] **移出原理演示段**（保留代码、后续重构再加入）：09-arch / 09-proto / 09-nums 移出 setOrder；**删结尾页** 09-outro
- [x] **UI 总览停留减半**：ui-home/ui-control/ui-vision/ui-trust 的 calloutsAll hold 砍半 + dur 同步缩短
- [x] **三种作业模式不再模拟点击**：01-home chip 改自动轮播高亮（更快，不拖沓）
- [x] **手填 IP 章节取消**：ui-setip/08-setip 移出；改为 01-home"发现设备"时**旁开「手动连接」小窗**顺带演示
- [x] **互信改 PIN 演示**（08-trust 由 scenes8 覆盖）：资产图（紫派+显示器）；点配对→平板缩小右移、左侧出紫派+所连显示器；
      显示器黑屏弹 PIN(582047)、平板弹系统输入框(复刻参考 UI：连接 Purple Pi OH / 请输入对端设备上显示的连接码 / 6 圈 / 取消)逐位输入→成功→复原
- [x] **三平板算法并演**（algo-trio）：控制 UI/建图后单平板"裂成三块"，上半部三平板**同开同停**演示 A*单机 / 全路径覆盖 / 多机分布式；
      下半部留空（算法过程+文字说明**后续按文档做**）；末尾**三合一 → 点摄像头按钮 → PiP 小窗(实机视频) → 放大** 承接仪表页
- [x] **实机仪表视频**：07-vision 主视频 + algo-trio 末尾 PiP 小窗均改用 `assets/仪表检测视频.mp4`（已内嵌检测框+4关键点+读数）；
      currentTime 绑定时间轴（确定性、可 seek、循环中段 1.5–8.5s）

## 本轮二改（忠实真机 · 用户反馈）
- [x] **地图改有机等高线**：algo-trio 三平板用 MAPKIT 渲染（栅格→marching squares→chaikin），与真机 SLAM 楼面图一致（参考用户截图 `运行中的图_隐藏控制栏…`）。
- [x] **机器人不再僵硬**：朝向三角随走向转头 + 先转向后行驶 + 加减速；建图(03-build)/导航(algo-trio)均生效。
- [x] **覆盖改轨迹刷扫**：小车牛耕折返留**刷宽拖尾**扫满**所选框内**（绝不越框）+ 细彗尾；HUD 进度由轨迹推进（=真机"通过轨迹判进度"）。
- [x] **algo-trio 忠实 ControlPage**：迷你控制页 = 顶部状态胶囊 + 缩放FAB + 覆盖HUD + 底部 peek 操作卡 + 仪表PiP；
      每台走真机流程：点模式按钮(选目标点/选顶点/选点划区域)→收起底卡→点地图选点(ripple)→执行。
- [x] **PiP 复刻 VisionPip**：实时仪表 + 放大 + ✕ 关闭，192×120 视频（实机仪表 mp4）。
- [x] **互信两步弹窗**：车屏先「请求连接·是否信任」(始终/临时/不信任) → 后「连接码 6 3 8 4 9 5」；平板逐位输入 638495（贴近用户补的紫派截图）。
- [x] **资产加载修复**（用户反馈"图片/视频不见了"）：① 资产复制为 **ASCII 文件名**(pi-board/monitor/meter/pi-pin/pi-perm，避免部分浏览器 file:// 加载中文/括号路径失败)；
      ② 仪表视频改 **autoplay loop muted**（弃 currentTime 抓帧——暂停态逐帧 seek 在实时播放时不稳定渲染→黑屏）；③ 车屏弹窗 left/top:50%+xPercent/yPercent 居中，确保稳落显示器黑屏区。
      注：autoplay 牺牲逐帧确定性导出（预览优先；如需导出再切回 currentTime 绑定）。
- [ ] **未做（可选 follow-up）**：03-build(建图) 地图仍为干净矩形（仅修了机器人转向）——如需与 algo-trio 一致改有机等高线，
      需把墙生长改成 MAPKIT 等高线路径的 dashoffset 揭示（MAPKIT 已就位，是一处定点改）。

## 结构决策（可一行回退）
- **保留 03-build**（建图）作为 algo-trio 前的单平板铺垫（先建图→三种规划同图并行）。若想直接裂三块：从 setOrder 删 '03-build'。
- **移出 ui-vision**（仪表页界面总览）：仪表页改由 algo-trio 末尾"摄像头→放大"手势直接进实时页，中间插静态总览会打断转场。若想恢复：setOrder 里 '07-vision' 前加回 'ui-vision'。
- 04-nav/05-cover/06-fleet 单平板演示已并入 algo-trio，**代码保留**（scenes2/scenes3）备用。

## 待确认 / 下一步
- [ ] 真机浏览器整片预览核对（video 渲染、各新窗像素位、节奏）——jsdom 不渲染视频/SVG，需 Edge/Chrome 目视
- [ ] algo-trio 下半部"算法过程演示 + 文字说明"按文档补做
- [ ] 原理段（架构/协议/数字）重构后再决定是否/如何加回
- [ ] 导出 mp4（record 管线，需浏览器逐帧 seek 截图——shot.mjs 已验证 Edge 可驱动 __seek）
