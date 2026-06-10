# 地图 UI 重设计：浅色「建筑图纸」+ 平滑矢量墙体

> 2026-06-08 起 · 用户体感真机建图"图像还不错，但太丑太简陋"，要求**设计顶级地图 UI 并应用**。
> 数据管线本身已修好（见 `docs/map-pipeline.md`，真机解析正确、`ok=true`、1800×1800）；本轮**只改视觉渲染**，
> 不动解析/坐标变换/协议。**未经 DevEco**——下一步真机校验。

## 0. 两版与"丑的根因"

- **第一版（深色「指挥中心 HUD」）被否**（2026-06-09 用户）："深色主题一点也不搭，色彩选择很糟糕"（不贴品牌墨绿浅色），
  且"真正感觉到丑陋的地方却一点没改"——我当时只把墙体逐格 fillRect **合并成行程**，**本质还是矩形方块**。
- **丑的根因 = 把占据栅格当填充方块画**。1800 格压进 ~800vp（每格 ≈ 0.44vp）→ 亚像素碎块、锯齿、缝隙，
  无论合不合并行程都是"方块感"。
- **正解 = 不画格子**：提取自由/墙体**边界**→平滑成**矢量等高线**→描边+软填充，得到干净的楼层平面图
  （Roborock / 建筑图纸观感）。**配色回归品牌**：浅色白图纸 + 墨绿 `#485c11` 墙体 + 克制语义标记。

## 1. 设计方向（第二版，现行）

浅色楼层平面图，贴合品牌（墨绿 `#485c11` + 浅底 `#edf4f1` + 白卡）：

- **未知 / 图纸外**：极浅冷灰 `#e9eef1`，轻微纵向渐变出景深。
- **已扫地面**：白色"图纸"`#ffffff`（方形包围盒区）+ 极淡比例网格（品牌绿调，随地图缩放）+ 浅绿描边。
- **墙体**：**墨绿平滑矢量等高线**——软填充（粗格实心、`rgba(72,92,17,0.15)`）+ 描边（`#485c11`、圆角接头）。
- **目标点**：Maps 风**泪滴 pin**（品牌红 `#ea4335` + 白心 + 软投影 + 地面脉冲），对应 ui-design.md 既定"泪滴 pin"意图。
- **机器人**：带朝向三角的圆点（语义状态色、白环、**软投影非辉光**、覆盖/扫描时克制脉冲、车号）。
- **覆盖子区域**：墨绿软填充 + 实线边 + 四角直角标。
- **动效克制**：仅目标地面脉冲 + 活动车脉冲（~20fps）。**去掉**第一版的雷达扫掠/霓虹辉光。

## 2. 墙体：栅格 → 平滑矢量等高线（核心）

管线（纯几何全在 `model/mapContour.ets` + `MapService.toDisplayPool`，由 `tools/verify/verify.mjs` 镜像断言）：

1. **降采样池化** `ParsedMap.toDisplayPool(maxLen=240)`：把裁剪+正方形化栅格在 **display 朝向**（与
   `forEachWallRect` 的 y 翻转一致）**max-pool** 成粗网格 `CoarseGrid`（任一原格为墙 → 粗格为墙）。
   作用：降噪、连断点、把 1800² 降到 ≤240² → 等高线复杂度/算力可控。
2. **Marching Squares**（midpoint，`mapContour.marchingSquares`）：在粗网格上提取墙/空**边界线段**
   （对角自带 45° 倒角，比轴对齐边界更顺）。
3. **连环** `linkLoops`：按共享端点（量化键）把线段连成多段线（闭环 / 触边开线，双向延伸）。
4. **角点平滑** `chaikin`（2 迭代）：1/4·3/4 切割，把折线变顺滑曲线。
5. **映射** `extractContours`：粗格坐标 (cx,cy) → base 像素 `(cx·f·gridWidth, cy·f·gridHeight)`，与
   `forEachWallRect` 同坐标系（误差 < 1 格，肉眼不可见）。

渲染（`MapCanvas.drawWalls`）：
- **软填充**：直接填**粗网格的墙 cell**（实心、**绝不误填房间内部**，不依赖描边缠绕方向）——比"填闭合等高线 +
  nonzero 缠绕留洞"更稳（缠绕方向不保证 → 可能误填整间屋）。粗格锯齿边由平滑描边盖住。
- **平滑描边**：沿等高线 `stroke`，`lineJoin/lineCap=round` 消除棱角 → 矢量墙线。
- **回退**：等高线提取异常时退回 `forEachWallSpan` 软填充，保证有墙可见（不至空图）。

## 3. 渲染架构：双画布分层

| 层 | 画什么 | 何时重画 |
|---|---|---|
| **base**（静态） | 背景渐变 + 白图纸 + 比例网格 + 平滑墙体 | 仅地图/缩放/平移变化 |
| **overlay**（动态） | 覆盖区 + 目标 pin + 机器人 | ~20fps ticker（克制动效）+ 数据变化 |

- 手势（Tap 选点 / Pan 平移 / Pinch 缩放）绑在 overlay（顶层）；base 纯展示。
- 墙体/图纸在 base 像素空间随 `ctx` 变换缩放/平移；overlay 标记换算到**屏幕空间**后以恒定 vp 尺寸绘制
  （缩放时 pin/投影视觉大小恒定，shadow/lineWidth 不被 scale 扭曲）。
- 刷新：`onMapChanged`→重解析+重算等高线→重画 base+overlay；`redraw`(renderVersion)→只重画 overlay；
  `onViewCmd`/手势→两层都重画。

## 4. token（`constants/theme.ets` 的 `MapTheme`，替代被否的 `MapDark`）

浅色键：`bg/bgEdge`、`floor/floorEdge/grid`、`wallFill/wallStroke`、`robot*`(online/covering/error/offline/ring/label)、
`target/targetInner`、`region`、`markerShadow`。与浅色品牌 `AppColor` 同源（墙体/区域直接用品牌 `#485c11`）。

## 5. 坐标/点选未动（回归安全）

`geometry.mapToCanvas/canvasToMap` 与 `parseMap` 几何**逐字未改**；点选逆变换、世界↔画布换算、包围盒/正方形化
全部沿用。`verify.mjs` **38/38**（30 原有 + 8 新等高线用例：MS 产段/连环/顶点贴边/Chaikin 增密/映射 base px/含洞不崩）。
**x0/y0 定位偏移**（`map-pipeline.md` §5 / Q12）仍是待真机标定的独立问题，与本轮视觉无关。

## 6. 文件改动

- `model/mapContour.ets`（新）：`marchingSquares / linkLoops / chaikin / extractContours` + `CoarseGrid/Contour`。
- `service/MapService.ets`：`ParsedMap.toDisplayPool(maxLen)`（降采样池化，display 朝向）。`forEachWallSpan` 保留作回退。
- `constants/theme.ets`：`MapDark` → `MapTheme`（浅色图纸）。
- `component/MapCanvas.ets`：整体重写为浅色双画布 + 平滑墙体 + 克制标记/动效。
- `pages/ControlPage.ets`：底部建图/操作卡 → **bottom-docked sheet**（修溢出 + 去突兀，见 §8）。
- `tools/verify/verify.mjs`：镜像 mapContour + ⑤ 等高线用例。

## 7. 待真机校验（DevEco）

- ArkTS/ArkUI **未编译**。重点看：① 两块 `Canvas` 叠放 + 各自 `onReady` 时序；② `quadraticCurveTo`/`createLinearGradient`/
  `shadowBlur`/`clip` 在目标 SDK 支持（均标准 API）；③ `setInterval` 动效在大图上的流畅度（必要时关 `ENABLE_MOTION`）；
  ④ `toDisplayPool` 在真机 1800² 的耗时（一次性、≤240² 输出）。
- 体感重点：**墙体是否成顺滑矢量线（非方块/锯齿）**、白图纸 vs 灰未知区对比、泪滴目标 pin、机器人朝向圆点与品牌墨绿是否协调。
- 若墙体仍偏"硬"：调 `CONTOUR_MAX_LEN`（小=更平滑但丢细节）/ `CHAIKIN_ITERS`（大=更顺）/ `WALL_STROKE`。
- 若性能吃紧：先关 `ENABLE_MOTION`；再降 `CONTOUR_MAX_LEN`（粗格更小、cell 填充与 MS 更省）。

## 8. 底部建图/操作卡 → bottom sheet（2026-06-09，用户反馈）

> 用户："建图按钮所在的 UI 有点突兀，而且在全覆盖时会**超出界面**。"

- **根因**：旧 `BuildCard`/`OpCard` 用绝对定位 `.position({ x:'4%', y:'80%'/'84%' })`——**top 锚点固定**，卡片向**下**生长；
  全覆盖(fullpath)/多机(distributed) 内容多 → 底部溢出屏幕外。且浮在 84% 处看着"突兀"。
- **改为 bottom-docked sheet**（`ControlPage.BottomSheet`）：
  - 全屏透明 `Column`（`width/height 100%` + `justifyContent(FlexAlign.End)` + `padding bottom`），把卡片**docked 到底部**，
    内容**向上**生长 → **永不超出界面底部**。
  - `hitTestBehavior(HitTestMode.None)`：包裹层自身不响应触摸 → **空白区把触摸透传给底层地图**（缩放/平移/选点不被挡），
    仅卡片本体（子节点）拦截。这是"全屏浮层不挡底层手势"的标准做法。
  - 顶部**小抓手** + 圆角(`Radius.lg`) + 软阴影 → Maps/iOS 底部卡观感，消除"浮在半空"的突兀感。
  - `constraintSize({ maxHeight: '62%' })` 兜底封顶（极端高内容也不顶出屏幕）；常规内容按需自适应高度。
- **拆分**：`BuildCard`/`OpCard` → 纯内容 `BuildCardContent`/`OpCardContent`（去掉定位与卡片 chrome，chrome 移到 `BottomSheet`）。
  `AstarOps/FullpathOps/DistributedOps` 不变。
- **未动**：摇杆/速度滑块/缩放 FAB 仍各自绝对定位（仅建图阶段显示的摇杆与 sheet 在极矮屏可能轻微重叠，非本轮问题，暂留）。
- ⚠️ 未经 DevEco；真机重点看：底部 sheet 三态(尚无图/建图中/操作)是否都贴底不溢出、地图在 sheet 以外区域手势是否正常（验证 `HitTestMode.None` 透传）。
  若极端多机内容仍超 62% 封顶，把 `BottomSheet` 内容包一层 `Scroll` 即可。
