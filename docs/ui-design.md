# App UI 设计（五套融合）· OpenHarmonyCar 巡检控制平板

> 2026-06-07 · U1 产出。token 落地见 `app-harmony/entry/src/main/ets/constants/theme.ets`。
> 设计参考：本机 `style/` 五套（不入 main，太大 71.8MB；仅 app-harmony-core 留存 + 本机引用）。
> 场景：**工业巡检机器人控制平板**——地图为中心、状态一目了然、手指操作、数据型克制风。

## 一、融合取向（各取所长）

| 来源 | 取什么 | 用在哪 |
|---|---|---|
| `new/`（F&F 极简） | 干净中性的骨架、留白、克制 chrome、字号尺度 | 全局基底、列表/设置屏 |
| `google-maps-mobile-2024` | 地图屏范式：全屏地图 + 浮层搜索条 + 缩放 FAB + 底部操作卡 + 胶囊 chip + 泪滴 pin + 语义色 | **ControlPage / MapCanvas（核心）** |
| `area-161-designs`（绿色数据产品） | 墨绿主色(#485c11)、淡绿页底(#edf4f1)、8 点网格、数据感 | 品牌主色/主按钮/选中态/页底 |
| `plants-ecommerce-ios` | iOS 圆角卡片、薄荷点缀、舒适触控、Satoshi 字味 | 卡片/按钮质感、圆角 |
| `medical-mobile-app` | 应用骨架：顶栏 + 卡片列表 + 底部 Tab + 搜索 | HomePage/列表结构 |
| `tokens.css` 的 Colin | 高饱和色仅作语义状态点缀（绿/黄/蓝/橙） | 机器人状态、提示徽标 |

**一句话**：F&F 极简打底 + Google Maps 地图范式 + Area 墨绿品牌 + iOS 圆角 + 高饱和语义色点状态。

## 二、设计 token（已落地 constants/theme.ets）

- 颜色 `AppColor`：品牌 primary `#485c11`/primarySoft `#dfecc6`；表面 pageBg `#edf4f1`/surface `#fff`；
  文字 title/body/secondary `#6f7787`；语义 info`#1a73e8`/success`#34a853`/warning`#fbbc04`/danger`#ea4335`；
  机器人状态 robotOnline/Covering/Paused/Error/Offline；地图 mapWall/mapTarget/mapPin。
- 字号 `FontSize`：display32/h1 24/h2 20/h3 18/body16/caption12 + **遥测等宽 dataLg28/dataMd18/dataSm14**。
- 字体 `FontFamily`：sans=系统(留位 Instrument Sans)、mono=系统等宽(留位 Roboto Mono)。⚠️ 自定义字体需 `font.registerFont`。
- 间距 `Space` 4/8/16/24/32/48；圆角 `Radius` sm10/md16/lg20/pill999；阴影 `Elevation`；触控 `TOUCH_MIN=44`。

## 三、导航结构

```
EntryAbility → HomePage（机器人列表/连接 + 模式选择）
                   │ 点某车 + 选模式
                   └→ ControlPage(mode, carIds)   // 单一参数化页，取代旧 4 克隆页
                          ├ astar       单机 A* 导航
                          ├ fullpath    单机全路径覆盖
                          └ distributed 多机协同（划区域+监看）
            HomePage ┄┄(高级/兜底)┄┄→ SetIPPage（手填 IP）
```
底部不做重 Tab（控制类 App 以地图全屏为主）；HomePage 顶栏 + 卡片列表（medical 风），ControlPage 走 Maps 浮层范式。

## 四、各屏设计

### HomePage（medical 骨架 + F&F + Area 主色）
- 顶栏：标题"巡检控制" + 右上"添加/发现设备"按钮（主色胶囊）。
- 机器人卡片列表（surface 卡、Radius.md、Elevation.card）：每卡 = 车名/车号 + 状态徽标（robot* 色）+ IP（mono dataSm）+ 在线点 + 进入箭头。点卡进 ControlPage。
- 模式选择：卡内或进入时用 3 个 chip（astar/fullpath/distributed）。
- 修旧 bug：列表用 @State 数组重建，不在 onPageShow 累积 push。

### ControlPage（核心 · Google Maps 范式 · 参数化 mode）
- 底层：`MapCanvas` 全屏。
- 顶部浮层：搜索/连接条（圆角 pill、白底软阴影）——显示当前车/连接态；右侧头像位放"设备列表"入口。
- 右下：缩放 FAB 簇（+/−/回中）。
- 左下/底部：操作卡（Maps route-card 风，Radius.md）——按 mode 变体：
  - astar：选点导航（显示目标点坐标 mono）+ 开始/取消按钮。
  - fullpath：选房间/起停全路径。
  - distributed：划矩形子区域（对角点1/2）+ 各车进度条 + robotId 标。
- 摇杆：`Joystick` 浮层（右下或可收起），手动遥控。
- 顶部/角落：状态条（连接、心跳、急停提示用 danger）。

### MapCanvas（component）
> 注（2026-06-09）：地图渲染已**重设计**为浅色「建筑图纸」+ **平滑矢量墙体**（marching squares→Chaikin，取代栅格方块），
> 配色走品牌墨绿浅色（`MapTheme`，非下述 `mapWall`）。**权威见 `docs/map-ui-redesign.md`**；下面为 U1 初版描述，仅存档。
- Canvas 渲染：`MapService.parseMap` → `forEachWallRect` 画障碍（mapWall），空旷透明/白。
- 叠加：机器人 pin（泪滴，mapPin/robot* 色 + 朝向小箭头）、目标点（mapTarget）、分布式矩形子区域（半透明主色框）。
- 交互：双指缩放 + 拖拽平移；点选 → `geometry.canvasToMap` 得真实坐标回调上层。
- 多车：遍历 `mission.robots` 各画一个 pin（车号角标）。

### Joystick（component）
- 圆形底盘 + 摇杆头（iOS 质感、软阴影）；输出方向→`MoveDirection`(go/left/right/stop)+speed。
- **每实例独立节流** `THROTTLE_INTERVAL_MS`（不再全局 taskId）；松手回中=stop。

### DeviceList（component）
- 发现列表：方案B 广播发现（A 确认 Q5 前用 cmd0 广播 / 0x06 ping 兜底，见 integration-qa.md）。
- 每行：车号/名 + IP（mono）+ 信号/在线点 + "连接"胶囊按钮（连上变主色）。
- 底部："手动添加 IP"→ SetIPPage。空态："同一热点/局域网下自动发现车辆"。

### SetIPPage（F&F 极简表单）
- 高级/兜底：手填 IP（`storage.isValidIp` 校验）、车号；保存走 `service/storage`。

## 五、组件规范（复用）

- **卡片**：surface 底、Radius.md、padding Space.md、Elevation.card 阴影。
- **主按钮**：primary 底、onPrimary 字、Radius.pill、高 ≥ TOUCH_MIN；按下 primaryPressed。
- **次按钮/chip**：白底或 primarySoft、border 描边、Radius.pill；选中=主色/info 底。
- **搜索/连接条**：白底、Radius.pill、软阴影、左图标右内容。
- **FAB**：圆形白底、软阴影、≥ TOUCH_MIN。
- **状态徽标**：小圆点/胶囊，用 robot*/语义色 + caption 文案。
- **遥测数字**：FontFamily.mono + FontSize.data*（坐标/IP/进度/读数）。

## 六、字体与图标

- 字体：先用系统（HarmonyOS Sans）跑通；后续可 `font.registerFont` 注册 Instrument Sans(标题)/Roboto Mono(数据) 提质感（字体文件放 resources、注意版权）。
- 图标：优先用 ArkUI 内置/symbol 或自绘；`style/medical-mobile-app/assets/*.svg`、`google-maps` 的 search/zoom/定位等可作参照重绘（勿直接搬第三方品牌图标如 Google 配色 pin 的商标元素）。

## 七、待定 / 风险

- 自定义字体注册与版权：暂用系统字体，提质感时再评估。
- 真机分辨率/缩放：U2 用 `display.getDefaultDisplaySync()` 动态取屏，所有尺寸用 vp、关键处按屏宽派生。
- **全部 ArkUI 代码未经 DevEco 编译**——本设计为实现依据，最终以 DevEco 构建 + 真机校验为准。
- 仪表识别结果展示（ResultPanel/VisionService）留到对接 `server-api.md` 后再加，不在本轮。
