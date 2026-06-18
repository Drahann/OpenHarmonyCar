# PROGRESS · OpenHarmony 工业巡检机器人 · 演示视频

> ⚠️ **2026-06-16（晚）：本目录（GSAP/HTML 版）重新成为主线**——用户在 `assets/产品说明书.md` 放入竞赛级说明书，
> 据其逐字重做"原理架构 + 流程图"演示段。`../demo-remotion/` 仅 3 场景样板、无建图章，未采用。
> **权威内容源 = `assets/产品说明书.md`；本轮设计见 `01-原理架构与流程图演示设计.md`。**

STATUS: IN_PROGRESS（用户手动迭代）
更新时间: 2026-06-16（三改·按说明书加原理层：开场后插"项目背景叙述 + 架构三图[图2-1/2-2/2-3]"，每段 UI 演示前插该部分"原理逻辑图"[pr-*]；建图地图改 MAPKIT 有机等高线[seed 7，与 algo-trio 同源]；07-vision 等文案核对说明书）

## 文件结构
- `film.css`；`engine.js`(setOrder + callout/infoPanel + **SPEED 常量**)；
  `scenes.js`(00标题/01首页[含手填IP小窗]/02控制台[弃用]) `scenes2`(建图/导航) `scenes3`(覆盖/多机+PiP承接,代码保留)
  `scenes4`(仪表·实机视频) `scenes5`(旧互信/手填IP,代码保留) `scenes6`(架构/协议/数字/收尾,**移出视频但保留代码**)
  `scenes7`(界面总览 ui-* + home-enter + **setOrder**) `scenes8`(**本轮新增**：PIN 互信改版 08-trust + 三平板 algo-trio)
- **地图工具箱** `scenes0_mapkit.js`（`window.MAPKIT`，文件名排序居 scenes.js 之后、scenes2 之前）：移植
  `tools/verify/verify.mjs` 的 marchingSquares/linkLoops/chaikin → **有机等高线地图**（不再画干净矩形房间）；
  `robotSVG`（圆点+白环+**朝向三角**）+ `driveRobot`（**先转向后行驶 + 加减速** → 不僵硬）+ **彗尾拖尾**；
  `boustro` 牛耕折返。被 scenes2(建图) 与 scenes8(algo-trio) 复用，与 App `MapCanvas`/`mapContour` 同源。
- **逻辑图工具箱** `scenes_diagkit.js`（`window.DIAGKIT`）：把说明书 Mermaid/ASCII 逻辑图渲染成可逐帧 seek 的发布会动画——
  `mountFlow/revealFlow`(流程图/状态机：节点卡+箭头 marker+判定菱形+`viaX/viaY` 走廊路由 loop/分支)、`mountSeq/revealSeq`(时序图：泳道+消息+self+note)。
  被 `scenes_intro`(背景+架构) 与 `scenes_principles`(各章原理) 复用。
- **原理层场景**：`scenes_intro.js`(`bg-context` 背景§1 + `arch-nodes` 图2-1 + `arch-bus` 图2-2 + `arch-flow` 图2-3 时序)；
  `scenes_principles.js`(`pr-discover` 图6-1 / `pr-trust` 图5-3 / `pr-map` 图6-2 / `pr-slam` 图4-1 / `pr-algo` 图4-3·4-5·5-2 / `pr-vision` 图3-1~3-3)。
- **硬件层场景** `scenes_hw.js`（`hw-overview` 整车全貌 + `hw-explode` 六层拆解）：参数核对《产品说明书》§2.7 + 用户「小车结构图」；
  16 张透明底部件图(01_*~16_*.png)与六层一一对应，全貌真机照压缩为 ASCII 名 `car_full.jpg`(~128KB，避免 file:// 中文路径)。素材白名单见 `.gitignore`。
- **总片序 setOrder 单一来源 = `scenes7.js`**（勿在别处再调 setOrder）。ui-control/03-build 地图改 MAPKIT(seed 7)，三章同一张图。
- `build.sh` glob 内联打包 scenes*.js → 自包含 `film.html`（`bash build.sh`）
- **校验工具**：`node check.mjs`（jsdom 烟雾测试：跑通 boot、回读场景顺序/时长、捕获运行期异常）；
  `node shot.mjs <场景id> <场景内偏移秒…>`（系统 Edge 截图到 `_tmpframes/`，用于逐帧目视核对）

## 现行顺序（23 场景 ≈ 4:52；**SPEED=1 原速**；0 报错/自包含；每段 UI 前插"原理可视化"，pr-* 多为实物级动画非流程框）
> 互信链新增 `pr-share`（设备互信之后：车入会软总线 + 改黑板数据自动广播 onChange），插在 08-trust 之后。
> `pr-slam` ↔ `03-build` **不再合并**（用户对合并效果不满意，已撤销）——pr-slam 恢复为独立章，03-build 恢复纯建图演示。
> **硬件章新增**（背景之后、架构之前）：`hw-overview`(整车全貌真机照 car_full.jpg + 全栈自研叙述 + 规格胶囊) +
> `hw-explode`(自顶向下六层爆炸：左分层堆叠[随部件缩略图]，右逐层焦点卡=部件图+规格+文字说明)；见 `scenes_hw.js`，素材=assets/01_*~16_*.png(透明底) + car_full.jpg。
> **片尾实机演示章新增** `demo-real`（见 `scenes_demo.js`）：整屏播放真车演示视频 `assets/demo_real.mp4`(53.9s，由用户 `实机演示.mp4` 转 ASCII 名+faststart+去音轨)；
> **video.currentTime 绑定主时间轴**(确定性/可 __seek/shot 可截，非 autoplay)；顶部标题条 5s 后淡出、右上「实机录制」红点徽标、底部进度条。
> ⚠ `demo_real.mp4`(~29MB) 与源 `实机演示.mp4`(32MB) **未进 git**(超大文件，遵 CLAUDE.md 走外部/Release)，预览需本地有此文件。总时长控制在 **<5min**(现 4:52)。
00-title(4.5) → **bg-context(背景§1)** → **hw-overview(整车全貌)** → **hw-explode(六层拆解)** → **arch-nodes(图2-1)** → **arch-bus(图2-2)** → **arch-flow(图2-3 时序)**
→ **pr-discover(图6-1 连接状态机)** → ui-home → 01-home
→ **pr-trust(图5-3 互信时序)** → ui-trust → 08-trust → home-enter
→ **pr-map(图6-2 三坐标系)** → ui-control(地图 MAPKIT)
→ **pr-slam(图4-1 实时 SLAM：激光扫描+等高线生长+位姿图)** → 03-build(纯建图演示)
→ **algo-trio(上半三平板 + 下半三算法实时动画 · 中间平板放大到 1.85)** → **pr-vision(图3-1~3-3 两段式 + 右侧端到端流程图)** → 07-vision
→ **demo-real(片尾实机演示视频 53.9s)**
> **实物替换**：`pr-discover`(心跳章)画的小车 → 真车透明图 `assets/car_node.png`；`pr-vision`(仪表章)画的表盘 → 真表盘透明图 `assets/gauge_real.png`
> + 自绘检测框/4 关键点(圆心·指针尖 247°·零刻 140°·满刻 40°)/几何射线/读数弧，实测读数 **0.40 MPa·40%**（见 scenes_principles.js 注释里的源图像素坐标）。
> **导出**：`node export_record.mjs` → 系统 Edge 实时录屏(playwright recordVideo) + ffmpeg 转 H.264 → `film_preview.mp4`(1080p30·292s·~49MB，自动裁掉加载前导)。该 mp4 未进 git(大文件)。
> 加粗=新增"原理/背景/架构/硬件/实机"场景（DIAGKIT 渲染，文案逐字出说明书）。已 build+check(0 错) + shot 逐场目视核对。

## 六改（用户反馈 · 节奏/合并/放大/补图）
- [x] **削减呼吸感停顿压时长**（保留呼吸感、只砍死等）：arch-nodes 五链路连贯一气连完(不再等 香橙派 线)、arch-flow/pr-trust 箭头出齐后字幕立刻跟上；各原理场景缩尾部 hold。全片 3:53 → **3:23**。
- [x] **pr-slam 并入 03-build**：移出 setOrder；03-build 摇杆探索时加 `scanRing` 激光扫描脉冲 + 侧栏 FX 注解(激光/里程计→定位、轮廓累加→建图、关键帧)，标题改"先建一张图·图优化扫描匹配 SLAM"。
- [x] **algo-trio 中间平板放大不够**：结尾 `moveCam` 1.1 → **1.85**(≈1036px 居中)。
- [x] **07-vision 右侧空白补 mermaid**：DIAGKIT 竖向流程图(摄像头帧→YOLO检测→4关键点→几何读数+告警→WS推App·DeepSeek)，与左侧实机演示同时进行。
- [ ] **可继续**：呼吸感/时长再微调；多样入场铺到既有 UI 演示场；连贯转场/配乐。

## 七改（用户反馈 · 节奏回调/合并要像前章/补图换位/放大去糊）
- [x] **呼吸感砍太狠 → 回调**：3:23 → **3:32**（各场景尾部 hold 加回 ~1s；仍比初版 3:53 短）。
- [x] **03-build 合并要像 pr-slam（前一章）**：之前只加注解、偏 03-build 本身；现在平板里**激光雷达扇随车旋转扫描 + 位姿图关键帧逐个落 + 边**（`.lidar` 注入 robot 组、`.kfn/.kfe` 随探索揭示），= pr-slam 的扫描建图感。
- [x] **mermaid 加错章**：从最后一章 07-vision 撤掉，挪到**倒数第二章 pr-vision**——表盘左、流程图右，流程图节点随表盘每步(检测框/关键点/几何/读数)同步点亮。
- [x] **三平板放大很糊**：CSS `.cam{will-change:transform}` 使放大后用 GPU 位图上采样→糊。放大到位后 `tl.set(M.cam,{willChange:'auto'})` 取消层缓存→按终态重栅格化。

## 四改（用户反馈 · 2026-06-16）
- [x] **arch-nodes 第四根线(软总线)错位**：节点卡未给固定高 → 内容撑不满、线落卡片外。已给 `.ax-node` `height:n.h` + flex 列(模块沉底)，五链路均接卡片边。
- [x] **pr-algo 并入 algo-trio**（用户要求平板下方放原理）：删 setOrder 的 'pr-algo'；algo-trio 上半三平板缩放上移(cam 0.72)，**下半三块画布实时演算法原理**——
      `A*`(A* 波前扩散 order + 回溯最短路 + 机器人沿路) / `牛耕 BCD`(障碍切 cell + 弓字刷扫 brush) / `生成树 STC`(最小生成树 BFS 生长 + 螺旋覆盖)。算法均**真算**(JS 内 A*/BFS/spiral)，确定性可 seek。
- [x] **动效不止渐入 + 运镜**：algo-trio 方向性视差入场(M 升/L 左/R 右) + 焦点下移(平板淡出让位算法 + 面板放大)；
      pr-slam/pr-vision/pr-discover 加 **camera 引导**(`F.camTo/camReset` 推进关键节点：定位→建图 / 流水线→几何 / 已连接⇄已丢失)，替换原静态 ken-burns。
## 五改（用户反馈 · 动效/可视化/文字大改）
- [x] **删"假运镜"**：之前给流程框做的"放大-缩小 camTo"被否决，已全删。
- [x] **出现动画不再单一渐入**：新增 `scenes_fx.js`(`window.FX`)——`enter()` 提供 rise/fall/left/right/spring/pop/zoomBlur/wipeX/draw 多种入场 + `chars()` 逐字；各原理场景按内容选用。
- [x] **流程框图 → 实物级可视化**（保留逻辑/说明性，参照 algo-trio 模型）：
      `pr-discover`=心电监护(ECG 心跳 + 四态徽章，flatline=丢失)；`pr-map`=取点穿坐标系(平板地图上点→飞到真实世界图落 pin，反向渲染位姿)；
      `pr-slam`=实时 SLAM(激光雷达扇扫 + 等高线随扫描生长 + 位姿图关键帧)；`pr-vision`=真表盘(检测框 draw-on → 4 关键点 spring → 几何射线/夹角弧 → 读数卡)。
      仅 `pr-trust` 保留时序图(握手最贴切，非"流程框")；arch-* 本就是节点图/分层/时序，非流程框。
- [x] **文字不再困在底部 + 不重叠**：`FX.note(tl,pos,dur,x,y,html,{style,anchor,enter,w,accent})` 把说明放屏幕任意处，**挂到场景根(`FX.scene(s.root)`)** 随场景淡出、不泄漏到后续场景(修了"软总线"等标题串场 bug)；引擎 `caption()` 改为**单条不重叠**(新字幕出现即清上一条)。
## 六改（用户反馈 · 节奏/可视化/文字/入场）
- [x] **时序图箭头分批出**（图2-3 arch-flow / 图5-3 pr-trust）：`DK.revealSeq` 按 msg 的 `g` 分组，同组一次性出 → 节奏快、省时（替代一支一支太拖沓）。
- [x] **心跳示意改"两边发包"**：pr-discover 删 ECG，改 平板↔车 **命令包(绿)/心跳包(蓝)双向飞包**；**下半同时演状态机 mermaid**（上下并演，填满空白），丢失=心跳包停+红叉，四态徽章。
- [x] **互信之后加 `pr-share`**：车2 joinSession 入软总线 → 平板改 assignments / 车2 回写 robots 进度 → **脉冲沿连线广播 + 卡片闪烁(onChange)**，演 DDO 共享黑板。
- [x] **algo-trio 节奏 + 结尾转场**：平板出现 ~1s 即变淡、下半算法即开始（不等上面演完）；三算法 `TOT` 参数化 **同开同停**；结尾**中间平板放大移到正中 + 两侧平板/三面板移出** → 点摄像头 → 放大进下一章。
- [x] **FX 文字加大/加粗/上色**：`.fxn-*` 字号 +20~30% + `font-weight` + `var(--ac)` 上色。
- [x] **多样入场铺到既有场**：架构三节点(左滑/右滑/缩放模糊) + UI 总览平板(zoomBlur/rise/fall 轮换)。⚠ scenes7 排序在 scenes_fx 前 → `enterIn` 内用 `window.FX`（不能 load 期捕获 FX）。
- [ ] **可继续**：场景间连贯转场(元素承接/morph)；配乐卡点；更多既有组件(chips/cards)的入场多样化。

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
