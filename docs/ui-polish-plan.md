# App UI 打磨计划：动效 + 图标 + 一致性 + 色彩升级

> 立项 2026-06-11（owner 评审结论落地）。背景：演示视频（`docs/demo-video-plan.md`）的真机录屏
> R7–R9 会怼脸拍 App 界面，且 HTML 概念段已按发布会标准设计——App 实录屏不能和它差一个时代。
> 本轮**只动视觉/动效层**：不改业务逻辑、协议、service；token 单一来源原则不变。
>
> **▶ 接续指引**：本文 = UI 打磨唯一事实源。§1 色彩（决策见 §1.4）、§2 图标、§3 动效、§4 按钮收敛、
> §5 顶栏重组、§6 空态杂项、§7 实施批次、§8 真机验收。配套预览页 `docs/palette-preview.html`
> （浏览器打开看真色）。⚠️ 本机无 hvigor，全部改动未经 DevEco 编译——真机验收清单 §8。

## 0. 评审结论（为什么做）

骨架是大厂方法论（token 单一来源 / 语义色 / 8 点网格 / Maps 范式 / 等高线地图），执行层差三类：

1. **零动效**：全 App grep 无一处 `animateTo`/`.transition()`/`.animation()`。PiP 展开收起、sheet 三态、
   设备面板、状态点变色全是硬切；摇杆松手瞬间归零。
2. **字符冒充图标**：`‹ 返回`、`＋/－/⊙`、`✕`、`📹`、`〉`、`⚠` 散落各页（SDK API 12，`SymbolGlyph` 可用未用）。
3. **一致性**：按钮高度 26/32/34/36/40/44 六种（自家 `TOUCH_MIN=44` 被自己违反）；顶栏是"按钮工具栏"
   无层级；空态一行灰字；`LoadingPage` 占位页死代码仍注册在 `main_pages.json`。

色彩另有四条硬伤（§1.1）。地图等高线方向不动（已是最强资产）。

## 1. 色彩升级

### 1.1 现状诊断（色相级问题，非玄学）

| # | 问题 | 证据 |
|---|---|---|
| 1 | **浅底温度打架**：暖品牌坐在冷背景上 | 页底 `#edf4f1` 冷薄荷(≈160°)、地图底 `#e9eef1` 冷蓝灰(≈217°)、品牌浅底 `#dfecc6` 暖黄绿(≈80°)、品牌 `#485c11` 暖橄榄(≈75°) |
| 2 | **双绿撞色** | success `#34a853`（鲜草绿）与品牌墨绿不同族，"已连接"点挨着主按钮像偶然撞色 |
| 3 | **Material 拼贴感** | Google 四色原样搬运，饱和度体系与低饱和品牌不成家族 |
| 4 | 纯黑标题偏硬 | `textTitle=#000000`，大厂浅色 UI 用带色相的墨色 |

### 1.2 三个候选方案（完整 token 见 1.3；真色对比开 `docs/palette-preview.html`）

- **A · 暖纸墨绿（推荐）**：全部中性色统一成**暖米白"纸"**（页底 `#f3f2ea`），品牌墨绿**不动**；
  语义色降饱和调暖入族；地图未知区变暖灰 → **"牛皮纸图纸"**。与演示视频的米白发布会基调同源。
  改动只在 `theme.ets` 数值层。
- **B · 冷瓷松绿**：反向修——品牌迁就冷底：墨绿 → 偏蓝松绿 `#36573e`，页底/地图底统一冷灰绿。
  实验室器械的冷静高级。**代价：换品牌色**（地图墙体、视频计划 §2.1、一切品牌物料跟着变）。
- **C · 最小修复**：只统一页底/地图底温度 + 纯黑→墨 + 明黄微降。半小时改完，但问题 2/3 仍在。

### 1.3 Token 全表（落地=覆写 `constants/theme.ets` 数值，类名/字段名/注释结构不动）

| token | 现状 | A 暖纸墨绿 | B 冷瓷松绿 | C 最小修复 |
|---|---|---|---|---|
| primary | #485c11 | #485c11 | #36573e | #485c11 |
| primaryPressed | #3a4a0e | #3a4a0e | #2a4531 | #3a4a0e |
| primarySoft | #dfecc6 | #e4ebc8 | #d9e8d8 | #dfecc6 |
| pageBg | #edf4f1 | **#f3f2ea** | #eef2ef | #edf2ef |
| surface | #ffffff | #ffffff | #ffffff | #ffffff |
| surfaceMuted | #ececec | #eae8dd | #e7ebe8 | #ececec |
| divider | #e9e9e9 | #e7e5da | #e3e8e4 | #e9e9e9 |
| border | #dfe4ea | #dcd9cb | #d7ded9 | #dfe4ea |
| textTitle | #000000 | #1d2016 | #141915 | #181b12 |
| textBody | #1f2329 | #2b2f23 | #242a26 | #1f2329 |
| textSecondary | #6f7787 | #6f7461 | #68726b | #6f7787 |
| textCaption | #929292 | #9b9d8d | #959d97 | #929292 |
| info | #1a73e8 | #4678b8 | #3f78c8 | #1a73e8 |
| success | #34a853 | #3f9352 | #3a9e63 | #34a853 |
| warning | #fbbc04 | #dfa32f | #e3a52e | #eaaf1f |
| danger | #ea4335 | #d9503f | #e0503f | #ea4335 |
| robotOffline | #929292 | #9b9d8d | #959d97 | #929292 |
| mapWall | #202124 | #23261c | #1c241e | #202124 |
| mapPin / target | #ea4335 | #d9503f | #e0503f | #ea4335 |
| MapTheme.bg | #e9eef1 | **#edebe0** | #e9efeb | #eaf0ed |
| MapTheme.bgEdge | #dde5ea | #e2dfd0 | #dde6e0 | #dfe7e2 |
| MapTheme.floorEdge | #e2e9da | #e3e7d2 | #dce6dc | #e2e9da |
| MapTheme.grid | rgba(72,92,17,.055) | rgba(72,92,17,.05) | rgba(54,87,62,.055) | 不变 |
| MapTheme.wallFill | rgba(72,92,17,.15) | rgba(72,92,17,.14) | rgba(54,87,62,.15) | 不变 |
| MapTheme.wallStroke | #485c11 | #485c11 | #36573e | #485c11 |
| MapTheme.robotOnline | #2e7d32 | #357a41 | #3a9e63 | #2e7d32 |
| MapTheme.robotOffline | #9aa3ad | #a3a294 | #95a09a | #9aa3ad |
| MapTheme.markerShadow | rgba(33,42,12,.30) | rgba(40,42,28,.28) | rgba(20,32,24,.28) | 不变 |

（robotOnline/Covering/Paused/Error = success/info/warning/danger 同值映射；
MapTheme.robotCovering/Error 同 info/danger；floor/ring/label/targetInner 恒白不动。）

### 1.4 决策

- [x] **已选方案：A · 暖纸墨绿**（用户 2026-06-11 拍板，看过 palette-preview.html）。
- 连带同步：`docs/demo-video-plan.md` §2.1 色表、`tools/demo-film/shared/film.css`（建好后）。
  `docs/map-ui-redesign.md` 内的旧色值视为历史记录，不回改。

## 2. 图标系统（SymbolGlyph，API 12 已确认可用）

新建 **`constants/icons.ets`**：集中导出全部 symbol 资源名（单点），真机编译若某名不存在，
只改这一个文件（DevEco 资源面板 → Symbol 库挑近似名）。意向映射：

> **首编校验（2026-06-11 hvigor）**：下表名称除 `dot_viewfinder` 外全部通过编译；回中图标已改用
> **`sys.symbol.scope`**（十字准星，对照 SDK `ets-loader/sysResource.js` symbol 段核实存在；备选
> `local`/`circle_viewfinder`——鸿蒙定位族叫 `local*` 不是 `location*`）。同轮另修：自定义组件成员
> **不可叫 `size`**（与 CustomComponent 通用属性方法重名）→ IconButton 改名 `diameter`。

| 现状 | 位置 | 替换 symbol（意向名） |
|---|---|---|
| `‹ 返回` | ControlPage:731 / VisionPage:109 | `sys.symbol.chevron_left`（40×40 圆形白底图标钮） |
| `＋` `－` | ControlPage:891-892 | `sys.symbol.plus` / `sys.symbol.minus` |
| `⊙`（回中） | ControlPage:893 | **`sys.symbol.scope`**（已核实；`dot_viewfinder` 不存在） |
| `✕`（PiP 关） | ControlPage:818 | `sys.symbol.xmark` |
| `📹 仪表视频` | ControlPage:849 | `sys.symbol.video` + 文字「仪表视频」 |
| `视频`（顶栏） | ControlPage:770 | `sys.symbol.video` 图标钮 |
| `设备`（顶栏） | ControlPage:784 | 并入状态胶囊（§5），符号 `sys.symbol.chevron_down` 暗示可点 |
| `进入控制 〉` | HomePage:209 | 文字 +`sys.symbol.chevron_right` |
| `识别视频`/`设备互信` | HomePage:87/102 | 图标钮 `sys.symbol.video` / `sys.symbol.link`（§5 顶栏重组） |
| `⚠` | ReadingPanel:55 | `sys.symbol.exclamationmark_triangle`（或保留文字符号，错误条本身已有色块） |

写法基线：`SymbolGlyph($r('sys.symbol.xxx')).fontSize(20).fontColor([AppColor.textBody])`
（注意 fontColor 取**数组**）。图标钮统一 40×40、圆形、白底、软阴影（同 Fab 规格）。

## 3. 动效系统（从 0 到 1，参数全集中在 `constants/motion.ets`）

新建 **`constants/motion.ets`**：

```
DUR_FAST=150  DUR_BASE=250  DUR_SLOW=350           // ms
CURVE_STD   = Curve.FastOutSlowIn                   // 标准出入
SPRING_SOFT = curves.springMotion(0.4, 0.85)        // PiP/卡片形变
SPRING_SNAP = curves.springMotion(0.3, 0.65)        // 摇杆回中（快、略弹）
```

逐交互规格（全部用 `animateTo` 包状态变更，或组件 `.transition()`）：

| 交互 | 现状 | 规格 |
|---|---|---|
| 页面转场 | 系统默认 | 各 @Entry 加 `pageTransition()`：进场 `translate x:24→0 + opacity 0→1`，DUR_SLOW/CURVE_STD；返场反向 |
| PiP 胶囊⇄小窗 | if 硬切 | `pipExpanded` 赋值包 `animateTo(SPRING_SOFT)`；两分支根容器加 `.transition(TransitionEffect.OPACITY.combine(TransitionEffect.scale({x:.92,y:.92})))` |
| BottomSheet 三态 | 内容瞬换 | 建图卡/操作卡分支各加 `.transition(opacity + translate y:12)`，DUR_BASE |
| 状态点/文案变色 | 瞬变 | `refreshConn`/`onState` 中状态赋值包 `animateTo({duration:DUR_FAST*1.3, curve:CURVE_STD})` |
| 设备面板 | 瞬现 | 加全屏半透明遮罩（点击关闭）；面板 `.transition(opacity + scale .96)`，DUR_BASE |
| MapLoadOverlay | 瞬现 | `.transition(TransitionEffect.OPACITY)`，DUR_FAST |
| 摇杆回中 | 瞬归零 | 松手：`animateTo(SPRING_SNAP)` 内置 knobX/Y→0 |
| 按钮按压 | 系统默认 | 主按钮 `.stateStyles({pressed})` 背景 primaryPressed（已有色）+ 全局不加 scale（克制） |
| Home 空态⇄列表 | 瞬换 | 两分支 `.transition(opacity)`，DUR_BASE |

**禁则**（与地图重设计一致）：不加弹跳炫技、不加全屏粒子、地图 overlay ~20fps 动效已存在不叠加。

## 4. 按钮/组件收敛

三档规格（替换全部散落值）：

| 档 | 高度 | 用途 | 样式 |
|---|---|---|---|
| 主操作 | **44** (TOUCH_MIN) | 开始建图/开始导航/发现设备/连接 | filled primary，pill |
| 浮层工具 | **40** | 顶栏全部、FAB、重新建图 | 白底（图标钮圆形 40×40；文字钮 pill） |
| 卡内小钮 | **32** | PiP 头部、收起摇杆 | 白底 pill 或纯图标 |

逐点清单：HomePage 卡片按钮 36→**40**；PiP 头部 26→**32**；PiP 胶囊 34→**40**（含图标）；
收起摇杆 32 保持；TopBar 40 保持；主操作钮已是 44 保持。
按钮字号统一 `bodySm`；outline 钮去除「选中时 border:undefined」的跳变（选中态用填充，未选态恒有描边）。

## 5. 顶栏重组（去"工具栏感"，立层级）

- **HomePage**：左 = `巡检控制`(h1) + 副行 caption（扫描状态/车数）；右 = 两个 40×40 圆形**图标钮**
  （互信 `link`、视觉 `video`）+ 一个主色胶囊「发现设备」（唯一文字主按钮）。
- **ControlPage**：左 = 返回圆形图标钮；随后**一条状态胶囊**（dot + IP mono + mode 名，**点击 = 展开设备面板**，
  右端 chevron_down 暗示）→ 删掉独立「设备」按钮；右 = 「重新建图」outline（有图时）+ 视频圆形图标钮。
  顶栏从 5 个胶囊 → 3 个元素。
- **VisionPage**：返回钮图标化；其余结构已合理（标题+副标+状态点）。

## 6. 空态与杂项

1. **Home 空态**：80vp 同心圆环（border 两圈 primarySoft/border）+ 中心 `magnifyingglass` symbol；
   `scanning` 时外环 `rotate` 循环动画（2s linear）；主文案「未发现车辆」/「正在搜索…」+
   副文案（现有文字降级为 caption）+「发现设备」主按钮。两分支带 transition。
2. **VideoView 占位区**：视频区背景白→**近黑 `#15170f`**（letterbox，视频观感专业、JPEG 边缘不刺眼），
   占位文字/图标用 onPrimary 70%。VisionPage 视频 Stack 同步。
3. **LoadingPage 清理**：`EntryAbility` 已直载 HomePage（EntryAbility.ets:31），LoadingPage 是死代码
   → 从 `main_pages.json` 移除 + 删 `pages/LoadingPage.ets`。
4. **DevicePanel 包装**：全屏 40% 黑遮罩（点击关闭）+ 居中卡片（Radius.lg + Elevation.card）。
5. 摇杆/速度滑块的 `y:'44%'/'50%'/'62%'` 百分比魔数：**本轮不动布局**（涉及与 sheet 避让，真机调），
   仅记录：终态应锚定左下、与 BottomSheet 同容器排布。

## 7. 实施批次（本会话按此顺序直接改）

> **状态（2026-06-11）：批 1–6 全部 code-complete**（含 SetIPPage 顺手图标化+转场；DeviceTrustPage 无字符图标问题未动）。
> ⚠️ 全部未经 DevEco 编译——下一步走 §8 真机验收。

| 批 | 内容 | 文件 |
|---|---|---|
| 1 | 色彩 A 方案覆写 + motion/icons 常量 | theme.ets（数值）、新建 motion.ets / icons.ets |
| 2 | LoadingPage 清理 + 按钮收敛 | main_pages.json、删 LoadingPage.ets、HomePage/ControlPage 高度 |
| 3 | 图标化 | ControlPage / HomePage / VisionPage / ReadingPanel |
| 4 | 动效 | 同上 + Joystick（回中弹簧）、页面转场 ×3 |
| 5 | 顶栏重组 + 空态 + VideoView letterbox + DevicePanel 遮罩 | HomePage / ControlPage / VisionPage / VideoView |
| 6 | 同步 demo-video-plan §2.1 色表 | docs |

提交粒度建议：批 1+2 一笔（`ui: 色彩A+收敛`）、批 3+4 一笔（`ui: 图标+动效`）、批 5 一笔（`ui: 顶栏+空态`）。

## 8. 真机验收清单（DevEco，全部改动未经本机编译）

- [ ] 编译过：`SymbolGlyph` 资源名逐个存在（报错则改 `icons.ets` 单点）；`curves`/`TransitionEffect`/
      `pageTransition` API 12 形参以 SDK 为准。
- [ ] 每页截图对照：HomePage（空态/列表/扫描中）、ControlPage（建图/操作/PiP 开关/设备面板）、VisionPage。
- [ ] 动效：PiP 展开收起是否顺滑；sheet 三态切换；页面转场；摇杆回中弹簧；与地图 20fps overlay 并存不卡。
- [ ] 色彩：暖纸底下白卡对比是否足够（卡阴影已有）；正文对比度 body/pageBg ≈ 11:1、secondary ≈ 4.6:1（达标）；
      地图"牛皮纸图纸"观感 vs 旧冷灰（重点体感项）。
- [ ] 触控：可点区 ≥40（工具钮）/ ≥44（主操作）。
- [ ] 录屏检查：按演示视频 R7–R9 流程过一遍，看有无露怯处（占位文字/瞬切/裸 IP 等）。

*关联：评审背景见会话记录；视频计划 `docs/demo-video-plan.md`（P0 完成后才拍 R7–R9）；
地图视觉权威 `docs/map-ui-redesign.md`（本轮只动其色值 token，管线/几何不动）。*
