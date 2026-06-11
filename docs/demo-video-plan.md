# 演示视频制作计划：实拍 + HTML 概念演示 + Seedance 转场

> 立项 2026-06-11（owner）。目标：一支**发布会级**演示片（参照 Apple / OpenAI / Anthropic 新品片的
> 节奏与质感）——实拍证明"真的能跑"，HTML 渲染的概念段呈现"它是怎么做到的"，Seedance 2 生成转场
> 与氛围镜头，最后剪成 1:45–2:05 成片。
>
> **▶ 接续指引 / 怎么用本文**
> - **给 Sonnet（HTML+录制）**：只需读 **§2 视觉语言 + §4 全部**。开工提示词见 **§4.9**，逐字粘贴即可。
> - **给拍摄者（用户）**：§6 实拍清单、§5 Seedance 操作、§7 剪辑。
> - **总览/排期**：§1 分镜表、§8 工序。
> - 文案、色值、协议数字**全部以本文为准**（已与 `contracts/` 和 `theme.ets` 核对）；Sonnet **不得自创文案/配色**。

---

## 0. 成片定位

| 项 | 决定 |
|---|---|
| 风格基准 | Apple 产品页/keynote 片段：浅色大留白、超大字号、克制丝滑动效、一屏一个主语 |
| 总时长 | **2:05 标准版**；赶时间砍 S6 段 → 1:45 精简版（见 §1 表注） |
| 画幅/帧率 | 16:9 横屏、成片 1080p60（HTML 段按 4K 渲染、剪辑时可推近裁切） |
| 三种素材 | ① **实拍**（真机+真 App 录屏，证明力）② **HTML 概念段**（讲原理，全片视觉锚点）③ **Seedance 2**（实拍质感的转场/氛围，HTML 做不了的部分） |
| 配色立场 | **浅色品牌系**（米白 + 墨绿 `#485c11`）。⚠️ 此前地图 UI 深色霓虹方案已被否决（见 `docs/map-ui-redesign.md` §0），**全片同样禁止深色底/霓虹辉光**。 |
| 名称 | 文案中用占位符 **{NAME}**。候选：① 直接用「OpenHarmony 工业巡检机器人」（默认，诚实学术风）② 代号「青巡」（贴墨绿品牌）③「巡迹」。**开工前由用户定一个**，Sonnet 全局替换。 |
| 主口号 | **「让巡检自己跑起来。」**（S1/S8 用）备选：「看见每一处，读懂每一块表。」 |

---

## 1. 总分镜表（Master Storyboard）

素材列代号：**S\*** = HTML 概念段（§4）、**R\*** = 实拍（§6）、**T\*** = Seedance（§5）。

| 时码 | 段 | 画面内容 | 素材 | 音乐情绪 |
|---|---|---|---|---|
| 0:00–0:06 | 冷开场 | 米白底，{NAME} 字标逐字浮现 + 主口号 | **S1** | 静→第一声钢琴 |
| 0:06–0:11 | 命题 | 清晨厂房空镜，叠两行小字（剪辑加字幕：「工厂巡检，枯燥、重复、不能漏。」） | **T2** | 铺底律动进 |
| 0:11–0:15 | 启动 | 上电、指示灯亮、激光雷达起转特写 | **R2** | 节拍点亮 |
| 0:15–0:20 | 亮相 | 机器人 hero 环绕推近（AI 增强运镜） | **T3** | 第一个小高潮 |
| 0:20–0:26 | 亮相 | 实拍低机位环绕半圈 | **R1** | 持续 |
| 0:26–0:40 | 架构 | 三块板子卡片落位 + 四条链路连线动画 | **S2** | 回落、讲述感 |
| 0:40–0:45 | 建图 | 机器人在走廊建图行驶，侧跟拍 | **R3** | 律动 |
| 0:45–0:50 | 建图 | 平板录屏：点「开始建图」（发 'm'），地图在屏上长出来 | **R7** | 律动 |
| 0:50–1:00 | 地图概念 | 栅格→平滑等高线 morph + 选点导航动画（S4 截选核心 10s） | **S4** | 小高潮 |
| 1:00–1:04 | 转场 | 等高线图纸"立体化"渐变为真实车间 | **T1** | 过门 |
| 1:04–1:09 | 遥控 | 同框：前景手持平板推摇杆，背景车同步动 | **R5** | 律动 |
| 1:09–1:19 | 协议概念 | 9 字节显微镜：逐字节标注 + 包飞向 :5001（S3 截选 10s） | **S3** | 硬核段 |
| 1:19–1:23 | 视觉 | 香橙派镜头对着真实压力表盘 | **R10** | 回落 |
| 1:23–1:29 | 视觉 | 平板录屏：VisionPage 实时画面+读数、PiP 收放 | **R9** | 律动 |
| 1:29–1:39 | 视觉概念 | 表盘识别 + WS 双消息节拍 + PiP morph（S5 截选 10s） | **S5** | 小高潮 |
| 1:39–1:49 | 协同概念 | 一次配对→开机互信→多机共享地图 *（精简版砍掉本行）* | **S6** | 讲述感 |
| 1:49–1:57 | 数据墙 | 8 个硬核数字 count-up | **S7** | 收束推进 |
| 1:57–2:05 | 收尾 | {NAME} + 口号回扣 + 三人署名 + Built on OpenHarmony | **S8** | 终止式、渐弱 |

> 剪辑自由度：S3/S4/S5 的 HTML 成片各 12–16s，分镜里只截 10s 核心段——**多渲染的部分是剪辑余量，不是浪费**。

---

## 2. 统一视觉语言（实拍 / HTML / Seedance 三者都遵守）

### 2.1 配色（与 `app-harmony/.../constants/theme.ets` 同源，不得偏离）

> 2026-06-11 更新为色板**方案A「暖纸墨绿」**（与 App 同步换色，决策见 `docs/ui-polish-plan.md` §1）。

| 用途 | 值 | 来源 token |
|---|---|---|
| 品牌主色（标题强调/墙体/主图形） | `#485c11` | AppColor.primary |
| 品牌浅底（标签底/高亮块） | `#e4ebc8` | AppColor.primarySoft |
| 页面底色（暖米白纸） | `#f3f2ea` | AppColor.pageBg |
| 卡片/图纸白 | `#ffffff` | AppColor.surface |
| 地图未知区暖灰纸 | `#edebe0` | MapTheme.bg |
| 墙体软填充 | `rgba(72,92,17,.14)` | MapTheme.wallFill |
| 正文墨 | `#2b2f23` | AppColor.textBody |
| 次要文字 | `#6f7461` | AppColor.textSecondary |
| 信息蓝（链路/检测框，钢蓝） | `#4678b8` | AppColor.info |
| 成功绿（在线/正常） | `#3f9352` | AppColor.success |
| 警告琥珀 | `#dfa32f` | AppColor.warning |
| 品牌红（目标 pin/告警/急停，砖红） | `#d9503f` | AppColor.danger |

**禁则**：深色背景、霓虹辉光（glow/neon）、高饱和渐变大面积铺底、彩虹配色。阴影一律"软投影"
（低透明度大模糊），不发光。

### 2.2 字体

| 角色 | 字体 | 获取 |
|---|---|---|
| 标题/正文（中文） | **HarmonyOS Sans SC**（Regular/Medium/Bold） | 华为开发者官网"设计资源"页下载，放 `assets/fonts/`（已 gitignore） |
| 回退链 | `'MiSans','Noto Sans SC',sans-serif` | MiSans：hyperos.mi.com/font；Noto 可走 Google Fonts CDN（录制机需联网） |
| 数据/字节/遥测 | **Roboto Mono**（与 App `theme.ets` 既定意向一致） | Google Fonts 下载放本地 |

数字一律 `font-variant-numeric: tabular-nums`（计数器不抖动）。中文标题用真 Bold 字重文件，
**禁止 faux-bold**（浏览器仿粗）。

### 2.3 动效原则（HTML 段的"语法"，Seedance prompt 也照此描述运动）

1. **一屏一个主语**：同一时刻只有一个视觉焦点在动；其余元素已 settle 或还没进场。
2. **快进慢停**：入场 0.7–1.0s、`power4.out`（快速启动、长尾减速）；镜头级移动 1.6–2.4s、`power2.inOut`。
3. **进场三件套**：位移（y 24–32px↑）+ 透明度 + **模糊 12px→0**。这是"发布会质感"的核心配方。
4. **停留可读**：每段文案 settle 后保持 ≥1.2s 静止再切下一动作；场景末帧静止 hold ≥1.2s（剪辑余量）。
5. **克制**：无弹跳（bounce/elastic）、无 3D 翻转炫技、无粒子乱飞。允许的"活力"上限 = 轻微 overshoot（`back.out(1.2)` 仅用于小元素如 pin 落下）。
6. **隐形运镜**：每个场景的 `#stage` 整体做 1.00→1.03 的匀速缓推（`ease:'none'` 全场贯穿），模拟 keynote 摄像机。

### 2.4 构图

- 设计稿固定 **1920×1080**；左右安全边距 96px；上下 72px；关键文字再内收 5%（动作安全区）。
- 大标题字号 96–128px；一行不超过 12 个汉字；标点用全角「。」收尾（Apple 中文文案习惯）。

---

## 3. 全片文案总表

所有屏上文字逐字见 §4.6 各场景规格。需要用户**开工前定稿**的只有三处：

1. **{NAME}**（见 §0 候选）；
2. **署名**（S8）：`App · ___　紫派 · ___　香橙派 · ___`（三人姓名/ID）；
3. S7 的「推理 __ ms」：用真机实测的 `inference_time_ms` 中位数回填（联调前可先用 mock 值 38）。

---

## 4. HTML 概念演示 —— 给 Sonnet 的实施规范

### 4.0 任务一句话

在 `tools/demo-film/` 用**纯静态 HTML + GSAP** 实现 §4.6 的 8 个场景（S1–S8），每个场景是一条
**可被逐帧 seek 的暂停时间轴**，用 §4.3 的 Playwright 管线渲染成 **4K 60fps PNG 序列**，ffmpeg 合成
mp4。完成定义：8 个 mp4 + 指定首末帧 PNG，全部通过 §4.8 验收清单。
**不得改动 `app-harmony/`、`contracts/`、`purplepi-control/`、`orangepi-vision/` 下任何文件。**

### 4.1 目录结构与技术选型

```
tools/demo-film/
├── scenes/
│   ├── s1-title.html      s2-arch.html      s3-protocol.html   s4-map.html
│   ├── s5-vision.html     s6-fleet.html     s7-numbers.html    s8-ending.html
├── shared/
│   ├── film.css           # 设计系统（§4.4，全场景唯一样式来源）
│   ├── film.js            # 场景运行时契约 + 工具（§4.2，逐字照抄）
│   ├── contour.js         # S4 用：marching squares + chaikin（来源见 S4 实现要点）
│   └── vendor/
│       ├── gsap.min.js            # GSAP 3 core（jsdelivr 下载一次放本地，离线可录）
│       └── MotionPathPlugin.min.js
├── assets/
│   ├── data/map40.js      # 真实地图 fixture 转 JS（生成方式见 S4）
│   └── fonts/             # woff2/ttf（gitignored，下载说明见 §2.2）
├── record/
│   ├── record.mjs         # 录制脚本（§4.3，逐字照抄）
│   ├── package.json       # { "type":"module", "dependencies": { "playwright": "^1" } }
│   └── README.md          # 安装/录制/合成命令速查
├── frames/   out/   stills/        # 产物目录（全部 gitignore）
└── README.md              # 指回本文档
```

技术决定（不要偏离）：

- **无构建步骤、无框架、无 ES Module**——普通 `<script>` 全局变量，保证 `file://` 直开可用
  （Playwright 用 `file://` 加载，省去起服务器）。
- **动画只用 GSAP 时间轴**；**排版用 DOM/SVG，不用 canvas**（离线渲染无性能压力，SVG 在 4K 下更锐，
  且天然随 seek 确定性渲染）。
- 需要的 GSAP 插件仅 MotionPathPlugin（S4 小车沿路径）。**不用 SplitText**（付费插件），中文逐字
  拆分用 film.js 的 `splitChars`。
- `.gitignore` 追加（在仓库根 `.gitignore` 末尾加一段）：

```gitignore
# tools/demo-film 产物
tools/demo-film/frames/
tools/demo-film/out/
tools/demo-film/stills/
tools/demo-film/assets/fonts/
tools/demo-film/record/node_modules/
```

> 备注：这些场景文件天然可日后改造成项目展示网页（scroll 版），但**本轮只做视频，不做滚动交互**。

### 4.2 场景运行时契约（确定性的根基，逐字照抄到 `shared/film.js`）

每个场景 = 一条 **paused 的 GSAP 主时间轴**。录制器通过 `window.__seek(t)` 把时间轴打到任意时刻
截图，因此**任何不经过时间轴的动效都会导致帧间不一致**。铁律：

- ❌ 禁止：`requestAnimationFrame` 循环、`setInterval/setTimeout` 驱动动画、`Math.random()`、
  `Date.now()/performance.now()`、CSS `transition`/`animation`（CSS 动画不响应 seek）。
- ✅ 一切随时间变化的东西（包括数字滚动、指针摆动、帧号自增）都做成 timeline 上的 tween。
- ✅ 初始态写法：**CSS 里写好隐藏初值 + `tl.to()` 显示**，或用 `tl.fromTo()`；**禁止裸 `tl.from()`**
  （from 在暂停时间轴上的 immediateRender 行为易产生首帧闪烁/不确定）。
- ✅ 偶发"随机感"（如散点抖动）用 `seededRand(固定种子)`。

```js
// shared/film.js — 场景运行时契约 + 公共工具（所有场景引入）
window.__ready = false;
window.__duration = 0;

/** 场景注册：build(tl) 里把全部动画挂到传入的 paused 主时间轴上 */
window.defineScene = function ({ duration, build }) {
  (async () => {
    await document.fonts.ready;            // 字体就绪后再排版/搭时间轴，避免错位
    gsap.defaults({ ease: 'power4.out', duration: 0.9 });
    const tl = gsap.timeline({ paused: true });
    build(tl);
    tl.progress(1).progress(0);            // 预热：强制求值全部终态再回零，锁定确定性
    window.__tl = tl;
    window.__duration = duration;
    window.__seek = (t) => tl.time(Math.min(t, duration), false); // false=回调照常触发（计数器靠它）
    window.__ready = true;
    if (new URLSearchParams(location.search).has('play')) {       // 浏览器预览：?play 自动播
      tl.play();
      addEventListener('keydown', (e) => { if (e.code === 'Space') tl.restart(); });
    }
  })();
};

/** 确定性伪随机（全片禁 Math.random） */
window.seededRand = function (seed = 1) {
  let s = seed >>> 0;
  return () => ((s = (s * 1664525 + 1013904223) >>> 0) / 4294967296);
};

/** 中文逐字拆分（替代付费 SplitText）：清空元素并回填 span.char，返回 span 数组 */
window.splitChars = function (el) {
  const text = el.textContent; el.textContent = '';
  return [...text].map((ch) => {
    const s = document.createElement('span');
    s.className = 'char'; s.textContent = ch; el.appendChild(s); return s;
  });
};

/** 数字滚动：在 tl 的 at 时刻把 0→target tween 进 el（fmt 控制格式） */
window.counterTo = function (tl, el, target, { at = 0, dur = 1.2, fmt = (v) => Math.round(v) } = {}) {
  const o = { v: 0 };
  tl.to(o, { v: target, duration: dur, ease: 'power2.out', onUpdate: () => (el.textContent = fmt(o.v)) }, at);
};
```

场景 HTML 模板（每个场景照此骨架）：

```html
<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8">
<title>S2 · 三节点架构</title>
<link rel="stylesheet" href="../shared/film.css">
<script src="../shared/vendor/gsap.min.js"></script>
<script src="../shared/vendor/MotionPathPlugin.min.js"></script>
<script src="../shared/film.js"></script>
</head>
<body>
<div id="stage">
  <!-- 1920×1080 固定画布；元素绝对定位或内部 flex/grid -->
</div>
<script>
defineScene({
  duration: 14,
  build(tl) {
    // 全部动画挂 tl；遵守 §4.2 铁律
  }
});
</script>
</body></html>
```

### 4.3 录制管线（Playwright 逐帧渲染 → ffmpeg 合成）

为什么不用 OBS 直录：屏录会掉帧、受机器负载影响；逐帧 seek + 截图**按构造无掉帧**，且可
无限重录到逐字节一致。OBS 仅作应急 fallback（Chrome 全屏 `?play` + OBS 60fps，见 §4.8 注）。

`record/record.mjs`（逐字照抄）：

```js
// record/record.mjs — 把场景逐帧渲染为 PNG 序列（确定性录制）
// 用法:
//   node record.mjs ../scenes/s2-arch.html             # 录全部帧 → ../frames/s2-arch/
//   node record.mjs ../scenes/s4-map.html --stills     # 只导首/末帧 → ../stills/（给 Seedance）
//   可选: --fps 60   --scale 2(默认,出 4K; 磁盘紧张可 1.5)
import { chromium } from 'playwright';
import { mkdirSync } from 'node:fs';
import { resolve, basename, dirname } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const args = process.argv.slice(2);
const file = args.find((a) => !a.startsWith('--'));
if (!file) { console.error('用法: node record.mjs <scene.html> [--stills] [--fps N] [--scale N]'); process.exit(1); }
const opt = (k, d) => { const i = args.indexOf('--' + k); return i >= 0 ? Number(args[i + 1]) : d; };
const FPS = opt('fps', 60), SCALE = opt('scale', 2), STILLS = args.includes('--stills');
const name = basename(file).replace(/\.html$/, '');
const outDir = resolve(here, STILLS ? '../stills' : `../frames/${name}`);
mkdirSync(outDir, { recursive: true });

const browser = await chromium.launch();
const page = await browser.newPage({
  viewport: { width: 1920, height: 1080 },
  deviceScaleFactor: SCALE,                       // 2 → 3840×2160 PNG
});
await page.goto(pathToFileURL(resolve(here, file)).href);
await page.waitForFunction('window.__ready === true', null, { timeout: 30000 });
const duration = await page.evaluate('window.__duration');
const total = Math.round(duration * FPS);
console.log(`${name}: ${duration}s × ${FPS}fps = ${total + 1} 帧 → ${outDir}`);

const seek = async (t) => {
  await page.evaluate((tt) => window.__seek(tt), t);
  // 双 rAF：确保 GSAP 写入的样式已布局+合成完毕再截图
  await page.evaluate(() => new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r))));
};

if (STILLS) {
  await seek(0);        await page.screenshot({ path: `${outDir}/${name}-first.png` });
  await seek(duration); await page.screenshot({ path: `${outDir}/${name}-last.png` });
} else {
  for (let f = 0; f <= total; f++) {
    await seek(f / FPS);
    await page.screenshot({ path: `${outDir}/${String(f).padStart(5, '0')}.png` });
    if (f % 120 === 0) console.log(`  ${f}/${total}`);
  }
}
await browser.close();
console.log('done');
```

安装与合成（写进 `record/README.md`）：

```powershell
# 一次性安装
cd tools/demo-film/record
npm install                       # playwright
npx playwright install chromium
winget install Gyan.FFmpeg        # 若无 ffmpeg

# 录制（每场景 ~3–6 分钟）
node record.mjs ../scenes/s2-arch.html

# 合成 4K 母版（剪辑用这个，可二次推近）
ffmpeg -y -framerate 60 -i ../frames/s2-arch/%05d.png -c:v libx264 -preset slow -crf 14 `
  -pix_fmt yuv420p -color_primaries bt709 -color_trc bt709 -colorspace bt709 `
  -movflags +faststart ../out/s2-arch-4k.mp4

# 可选 1080p 轻量版
ffmpeg -y -framerate 60 -i ../frames/s2-arch/%05d.png -vf "scale=1920:1080:flags=lanczos" `
  -c:v libx264 -preset slow -crf 16 -pix_fmt yuv420p -movflags +faststart ../out/s2-arch-1080.mp4

# 确认 mp4 无误后可删帧序列（每场景 4K 帧约 1.5–3 GB）
Remove-Item -Recurse -Force ../frames/s2-arch
```

需要导出给 Seedance 的静帧（§5 用）：S4 末帧、S2 末帧、S5 末帧、S1/S8 首末帧——
对这些场景各跑一次 `--stills`。

### 4.4 设计系统 `shared/film.css`（核心节选，按此扩展）

```css
/* shared/film.css — 值与 app-harmony theme.ets 同源（§2.1），不得新增色值 */
:root{
  --brand:#485c11; --brand-soft:#e4ebc8; --brand-press:#3a4a0e;
  --bg:#f3f2ea; --paper:#ffffff; --bg-map:#edebe0;
  --ink:#2b2f23; --ink-2:#6f7461; --ink-3:#9b9d8d; --line:#dcd9cb;
  --info:#4678b8; --ok:#3f9352; --warn:#dfa32f; --danger:#d9503f;
  --wall-fill:rgba(72,92,17,.14); --wall-stroke:#485c11;
  --shadow-card:0 12px 36px rgba(60,64,67,.16);
  --shadow-soft:0 4px 16px rgba(0,0,0,.08);
  --r-sm:14px; --r-md:24px; --r-lg:32px; --r-pill:999px;
  --sans:'HOS','HarmonyOS Sans SC','MiSans','Noto Sans SC',sans-serif;
  --mono:'Roboto Mono',Consolas,monospace;
}
@font-face{font-family:'HOS';src:url('../assets/fonts/HarmonyOS_Sans_SC_Regular.ttf');font-weight:400}
@font-face{font-family:'HOS';src:url('../assets/fonts/HarmonyOS_Sans_SC_Medium.ttf');font-weight:500}
@font-face{font-family:'HOS';src:url('../assets/fonts/HarmonyOS_Sans_SC_Bold.ttf');font-weight:700}

html,body{margin:0;width:1920px;height:1080px;overflow:hidden;background:var(--bg)}
#stage{position:absolute;inset:0;font-family:var(--sans);color:var(--ink);
  -webkit-font-smoothing:antialiased;text-rendering:optimizeLegibility}
.char{display:inline-block;will-change:transform,filter,opacity}

/* 字阶（1920 设计稿绝对 px） */
.display{font-size:128px;font-weight:700;line-height:1.18;letter-spacing:.01em}
.h1{font-size:84px;font-weight:700;line-height:1.2}
.h2{font-size:48px;font-weight:500}
.body{font-size:30px;font-weight:400;color:var(--ink-2);line-height:1.6}
.caption{font-size:24px;color:var(--ink-3)}
.eyebrow{font-size:24px;font-weight:500;letter-spacing:.4em;color:var(--brand)}
.data{font-family:var(--mono);font-variant-numeric:tabular-nums}

.card{background:var(--paper);border-radius:var(--r-md);box-shadow:var(--shadow-card)}
.chip{display:inline-block;padding:8px 22px;border-radius:var(--r-pill);
  background:var(--brand-soft);color:var(--brand);font-size:24px;font-weight:500}
```

布局通则：标题区贴左上（x=96,y=96 起），主图形居中或居右；遥测/注释类 mono 小字放右下角。
所有位移动画用 transform（`x/y`），不要动 layout 属性。

### 4.5 动效词汇表（场景里反复使用的"乐句"）

| 名称 | 用途 | GSAP 写法（示意） |
|---|---|---|
| 字浮现 | 标题/口号 | `tl.to(splitChars(el), {y:0,opacity:1,filter:'blur(0px)',stagger:.05,duration:.8}, at)`（CSS 初值 `y:28px;opacity:0;blur(12px)`） |
| 卡落位 | 设备卡/读数卡 | `tl.fromTo(card,{y:36,opacity:0,scale:.97},{y:0,opacity:1,scale:1,duration:.9}, at)` |
| 线生长 | 链路/等高线/路径 | `stroke-dasharray=L; tl.fromTo(path,{strokeDashoffset:L},{strokeDashoffset:0,duration:1.2,ease:'power2.inOut'}, at)` |
| 包点流动 | UDP/WS 数据流 | 小圆 `MotionPathPlugin` 沿连线 path，3–5 个点等距 `stagger:.18`、`repeat` 数次（次数固定写死，不用 `repeat:-1`，保证时长确定） |
| 数字滚动 | 读数/数据墙 | `counterTo(tl, el, 67.4, {at, fmt:v=>v.toFixed(1)+'%'})` |
| 翻牌 | 字节高亮 | `tl.to(cell,{rotateX:90,duration:.25,ease:'power2.in'}).set(cell,{textContent:'…'}).to(cell,{rotateX:0,duration:.35,ease:'power2.out'})` |
| 擦除切换 | 栅格→图纸 | 上层 `clip-path:inset(0 100% 0 0)` → `inset(0 0 0 0)`，`duration:1.6,ease:'power2.inOut'` |
| 缓推运镜 | 每场景贯穿 | `tl.fromTo('#stage',{scale:1},{scale:1.03,duration:全场,ease:'none'},0)`（transform-origin 设画面焦点） |
| pin 落下 | 目标点 | `tl.fromTo(pin,{y:-90,opacity:0},{y:0,opacity:1,duration:.5,ease:'back.out(1.2)'},at)` + 地面脉冲圈 `scale:0→2.6,opacity:.5→0` |

### 4.6 场景规格 S1–S8（文案逐字执行，含标点）

> 每场景表头：**[时长 | 背景色 | 录制输出]**。时间轴表中 `t=秒`。所有英文/数字用 `.data`(mono) 或
> Inter 风格保持克制；中文为主、英文小注为辅。

---

#### S1 冷开场标题 — `s1-title.html` [6s | `--bg` | 4K60 + 首末帧]

居中竖排三行：eyebrow「OPENHARMONY · 三端协同」/ display「{NAME}」/ h2 口号「让巡检自己跑起来。」

| t | 动作 |
|---|---|
| 0.0–0.8 | 纯底色静帧（剪辑淡入余量） |
| 0.8–1.6 | eyebrow 整行淡入（无位移，letter-spacing .55em→.4em 微收） |
| 1.6–3.0 | {NAME} **逐字浮现**（字浮现乐句，stagger .07） |
| 3.0–4.0 | 口号整行浮现（y 24→0 + blur） |
| 4.0–6.0 | 全静止 hold + 缓推运镜收尾 |

---

#### S2 三节点架构 — `s2-arch.html` [14s | `--bg` | 4K60 + 末帧]

标题区：h1「三块板子，一个闭环。」
主体：三张设备卡横排（每张 460×420，圆角 `--r-md`，卡内：简笔线框图标 + 名称 + 职责 + 技术 chip）：

- 卡1「鸿蒙平板 · App」/ 职责「指挥与呈现」/ chips：`ArkTS`
- 卡2「紫派 · 导航主控」/ 职责「建图 · 定位 · 运动」/ chips：`OpenHarmony 5.0` `C/C++ · Python` `LCM`
- 卡3「香橙派 · 视觉推理」/ 职责「识别仪表，读出数字」/ chips：`昇腾 NPU` `FastAPI`

卡间连线（SVG 弧线 + 流动包点 + mono 标签）：

- 卡1↔卡2：「UDP :5001 · 9 字节控制」（双向，墨绿）
- 卡2→卡1：「HTTP :8000 · 地图」（信息蓝）
- 卡3→卡1：「WS /ws/video · 视频 + 读数」（信息蓝）
- 卡1 上方自环弧：「分布式软总线 · 多机」（浅绿）

收束句（底部 body）：「互不依赖编译，只共享契约。」

| t | 动作 |
|---|---|
| 0.0–1.2 | 标题字浮现 |
| 1.2–3.4 | 三卡依次落位（stagger .35），图标线稿同步"线生长" |
| 3.4–5.0 | UDP 线生长 + 3 个包点流动 + 标签淡入 |
| 5.0–6.4 | HTTP 线 + 标签 |
| 6.4–8.0 | WS 线 + 帧脉冲点（节拍快一点，stagger .12）+ 标签 |
| 8.0–9.4 | 软总线弧 + 标签 |
| 9.4–11.4 | 收束句浮现；包点持续流动（固定循环 2 次后停） |
| 11.4–14.0 | hold + 缓推 |

---

#### S3 协议显微镜 — `s3-protocol.html` [12s | `--bg` | 4K60]

标题：h1「9 个字节，足够指挥一台车。」
主体：居中一条 9 格字节带（每格 150×170 白卡，mono 大字显示 hex，格下小字标注字段）。
**字段标注逐字照抄**（与 `contracts/udp-protocol.md` 发送表一致）：

```
[0] state·命令码   [1] runState·方向   [2] speed·速度
[3-4] endX · int16 大端   [5-6] endY · int16 大端   [7-8] 置 0
```

| t | 动作 |
|---|---|
| 0.0–1.0 | 标题字浮现 |
| 1.0–2.6 | 9 格依次落位（stagger .12），初始内容 `00` |
| 2.6–4.2 | 字段标注分组淡入（3 组） |
| 4.2–5.6 | byte0 **翻牌** `00→66`，旁挂大注释卡：「0x66 = 'f' — fullpathStartRoute · 全屋覆盖规划」+ 小字「命令码刻意对齐 ASCII：App 状态字码 = 下发命令」 |
| 5.6–7.0 | 字节带压缩成一枚小数据报（scale→.32 聚合为 pill「9 B」），沿弧线飞向右侧出现的小卡「紫派 · :5001」，重复 2 发（心跳感） |
| 7.0–9.0 | 紫派卡回弹出返程小包，旁注 mono：「↩ 每 500 ms 回传位姿 (x, y, θ)」+ 第二行「3 s 收不到指令 → 自动急停」 |
| 9.0–10.6 | 底部三枚 chip 快速淡入：`'m' 0x6d · 强制重建图`、`0x06 · 广播发现`、`'i' · 多机共享（主机 IP 藏进 byte 1·2·4·6）` |
| 10.6–12.0 | 尾句 body「大端，定长，零依赖。」+ hold |

---

#### S4 地图：栅格→图纸 — `s4-map.html` [16s | `--bg-map` | 4K60 + 末帧(给 T1)]

布局：左列文字（x=96，宽 560），右侧 860×860 地图画布（SVG，含双层：栅格层 / 图纸层）。
**数据用真地图 fixture**：`contracts/fixtures/defultMap.txt`（40×40，首行 `40 40`，体为密排 0/1）。
转换脚本（一次性，放 `tools/demo-film/scripts/make-map-data.mjs`）：

```js
import { readFileSync, writeFileSync } from 'node:fs';
const lines = readFileSync('../../../contracts/fixtures/defultMap.txt', 'utf8').trim().split(/\r?\n/);
const [h, w] = lines[0].trim().split(/\s+/).slice(-2).map(Number);
writeFileSync('../assets/data/map40.js',
  `window.MAP40=${JSON.stringify({ h, w, rows: lines.slice(1, 1 + h) })};`);
```

文案（左列，分三幕替换）：

- 幕一：h2「激光雷达把车间，变成 0 和 1。」/ caption「占据栅格 · 1 = 障碍 · 分辨率 5 cm」
- 幕二：h2「再把 0 和 1，变回图纸。」/ caption「Marching Squares + Chaikin 平滑 —— 与 App 同一套算法」
- 幕三：h2「点哪，去哪。」/ 右下 mono 遥测：「x 1.85 m · y 0.90 m · θ 36°」（数字随车点滚动）

| t | 动作 |
|---|---|
| 0.0–1.2 | 幕一文案浮现 |
| 1.2–3.2 | 栅格层进场：40×40 方块以 grid stagger 涌现（`stagger:{grid:[40,40],from:'center',amount:1.4}`），墙格 `#202124`、空格白，**刻意保留像素感/硬边** |
| 3.2–4.4 | 幕一→幕二文案交替（旧上移淡出、新浮现） |
| 4.4–6.4 | **擦除切换**：图纸层（白图纸 + 淡品牌绿网格 + 墙体等高线）从左向右 wipe 盖过栅格层；等高线本身同步"线生长"，软填充 `--wall-fill` 随描边渐显 |
| 6.4–8.0 | 幕二 caption 淡入；缓推运镜对准地图 |
| 8.0–9.0 | 幕三文案替换；目标 **pin 落下**（品牌红泪滴 + 白心 + 地面脉冲 2 次） |
| 9.0–10.2 | 路径"线生长"：起点→pin 的 BFS 路径（墨绿虚线 4-4，流动 dashoffset） |
| 10.2–14.5 | 机器人圆点（墨绿底白环 + 朝向楔形）沿路径 MotionPath 行进（`autoRotate`），遥测数字随行滚动 |
| 14.5–16.0 | 到点：pin 轻微下压回弹，路径虚线停止流动，hold |

实现要点：

- **等高线算法直接复用仓库实现**：`tools/verify/verify.mjs` 里有 App `model/mapContour.ets` 的 JS
  镜像（`marchingSquares / linkLoops / chaikin`）。把这几个纯几何函数拷出（剥掉断言）成
  `shared/contour.js`。40×40 不需要降采样池化，直接 MS → 连环 → chaikin×2 → SVG path。
  万一镜像函数不便剥离，fallback：`d3-contour`（vendor 本地化）取 0.5 等值线 + 自写 10 行 chaikin。
- 图纸视觉对齐 App `MapTheme`（方案A 暖纸）：底 `#edebe0`、图纸白、网格 `rgba(72,92,17,.05)`、墙描边 `#485c11`
  圆角接头 + 软填充 `rgba(72,92,17,.14)`、pin `#d9503f`。**这是"概念演示画的就是 App 真算法真配色"的卖点。**
- BFS 路径：4 邻接、起终点在空旷格中人工选定（渲染出来肉眼挑两个相距远的房间格，硬编码坐标）；
  路径折线过 chaikin 一次再喂 MotionPath，转角更顺。

---

#### S5 视觉与读数 — `s5-vision.html` [16s | `--bg` | 4K60 + 末帧]

布局三幕：

- **幕一（识别）**：左侧 980×640「相机帧」卡（暖灰相纸底 `#f1f0e9` 上画 SVG 压力表盘：表圈、刻度、
  指针；叠加识别层：信息蓝 bbox 圆角框 + score chip「0.97」+ 4 个关键点小圆及 mono 标签
  `center / pointer_tip / zero_mark / full_mark`）。右侧读数卡（ReadingPanel 风）：
  - 标题行「实时识别」+ 状态 pill「● 已连接」（成功绿）
  - 大数字 `.data`：「67.4 %」+ 副行「0.54 MPa · 压力表 01」
  - 两行小读数：「表 02 · 41.0 %」「表 03 · 88.2 %」（表 03 行 status pill 由「正常」绿翻成「告警」黄）
  - 底行 mono：「15 fps · 推理 38 ms」
- 顶部 WS 节拍条：胶囊交替滑入 `▮ JPEG 51 KB` / `{ frame_meta }`，mono 小字注：「一帧画面，一帧元数据，交替到达。」
- **幕二（PiP）**：整个相机帧卡 FLIP 收缩到左下角 280×158 小窗 → 再收成胶囊「● 实时识别」→ 弹回小窗；
  背景淡入 ControlPage 线稿（简化：浅色地图块 + 底部 sheet 圆角条 + 右下摇杆圆）。
- 标题（贯穿，左上）：幕一 h1「仪表自己报数。」→ 幕二替换 h2「随时看一眼，不打扰导航。」
  尾注 caption：「香橙派常驻推理，App 按需订阅。」

| t | 动作 |
|---|---|
| 0.0–1.0 | 标题浮现 |
| 1.0–2.2 | 相机帧卡落位，表盘线稿"线生长" |
| 2.2–3.4 | bbox 框线生长 + score chip 弹入 + 4 关键点依次点亮（stagger .15） |
| 3.4–5.4 | 指针从 0 摆到 67.4%（`power2.inOut`，1.4s，带 3° 微回摆一次）；右侧读数卡落位，大数字 `counterTo` 同步滚到 67.4 |
| 5.4–7.0 | WS 节拍条开始交替滑动（固定 6 拍）；frame_id mono 小字按 15Hz 跳动（用 counter 实现：`fmt=v=>'#'+(1200+Math.floor(v))`） |
| 7.0–8.4 | 表 03 行变警：行底闪 `--warn` 10% 底色一次、pill 翻黄、小数字滚到 88.2 |
| 8.4–9.4 | 标题替换为幕二；ControlPage 线稿淡入背景 |
| 9.4–11.6 | 相机帧卡 **FLIP 收缩**到左下小窗（width/height/x/y/圆角同 tween，1.1s `power3.inOut`） |
| 11.6–13.2 | 小窗→胶囊→小窗（各 .55s）；小注「WS 连接共享 · 引用计数，互不误断」淡入 |
| 13.2–16.0 | 尾注 caption 浮现 + hold |

实现要点：指针角度、计数、节拍全部 timeline tween（§4.2 铁律）；表盘/线稿全 SVG；
FLIP 不用插件，直接对固定起止几何做 fromTo。

---

#### S6 多机与互信 — `s6-fleet.html` [10s | `--bg` | 4K60]（精简版可砍）

标题 h1「配对一次，开机即互信。」
主体：左「鸿蒙平板」卡、右「巡检车 · 紫派」卡。中间时序三步（图标 + 短语，依次点亮）：

1. 「HDMI · 一次性配对」（线缆插入小动画：两段线段合拢）
2. 「分布式数据对象 · 自动同步」（平板侧发出 2 圈波纹扩散到车卡；车卡内小黑板矩形与 UDP 桥小字
   `blackboard ↔ localhost:UDP`，双向小点流动）
3. 「主机建好图，子机直接用」（车卡复制出第二张半透明车卡，地图缩略从主机飞到子机；mono 小注
   「cmd 'i' · 主机 IP 装进 4 个空字节」）

时间轴：0–1 标题；1–3 两卡落位；3–4.5 步1；4.5–6.5 步2；6.5–8.5 步3；8.5–10 hold。

---

#### S7 数据墙 — `s7-numbers.html` [8s | `--paper` 白底 | 4K60]

标题 caption（小，居中上方）：「过程，都有数。」
主体：2 行 × 4 列大数字（`.data` 96px 墨绿，单位/标签 24px 灰）：

| 9 B | :5001 | 500 ms | 3 s |
|---|---|---|---|
| 控制报文 | UDP 直连 | 位姿心跳 | 失联急停 |

| 15 fps | 5 cm | 1800² | 3 · 3 · 1 |
|---|---|---|---|
| 识别推理流 | 栅格分辨率 | 真机地图栅格 | 3 块板 · 3 种语言 · 1 套契约 |

时间轴：0–0.8 标题；0.8–4.2 八格按 grid stagger 落位 + 各自 `counterTo`（非数值格如 `:5001`
直接淡入）；4.2–8 hold + 缓推。
注：「推理 38 ms」若真机实测出稳定值，可替换 `3 s` 格（联调后回填，见 §3）。

---

#### S8 收尾 — `s8-ending.html` [8s | `--bg` | 4K60 + 首末帧]

构图回扣 S1：display「{NAME}」+ h2「让巡检自己跑起来。」
下方三行 caption 署名（占位）：「App · ___　　紫派 · ___　　香橙派 · ___」
底部小字：「Built on OpenHarmony 5.0 · Powered by Ascend」

时间轴：0–1.6 字标+口号浮现（同 S1 乐句但更快）；1.6–2.8 署名行淡入；2.8–3.6 底部小字淡入；
3.6–6.5 hold；6.5–8.0 整体缓慢淡出到纯底色（接黑场/片尾）。

---

### 4.7 资产与数据

- 字体：见 §2.2，下载放 `assets/fonts/`，文件名与 §4.4 `@font-face` 对齐（不一致就改 CSS 路径）。
- GSAP：`https://cdn.jsdelivr.net/npm/gsap@3/dist/gsap.min.js` 与 `.../MotionPathPlugin.min.js`
  下载存 `shared/vendor/`（录制全程离线、版本钉死）。
- 地图数据：跑一次 §S4 的 `make-map-data.mjs` 生成 `assets/data/map40.js`（该 js 很小，**可提交**）。
- 等高线：拷 `tools/verify/verify.mjs` 的镜像几何函数 → `shared/contour.js`（纯函数，**可提交**）。

### 4.8 验收清单（每场景完成后逐项打勾）

- [ ] **确定性**：同场景录两次，对首/中/末 3 帧 `Get-FileHash` 一致。
- [ ] **字体**：`document.fonts.check('700 84px HOS')` 为 true；截图肉眼确认标题非系统字体、非仿粗。
- [ ] **画面完整**：无滚动条、四角像素 = 背景色、无 1px 抖动（位移全用 transform）。
- [ ] **规格**：`ffprobe` 显示 60fps、3840×2160、yuv420p、时长 = duration ±1 帧。
- [ ] **文案**：与 §4.6 **逐字含标点**一致；色值只来自 film.css 变量。
- [ ] **节奏**：每段文案 settle 后 ≥1.2s 可读停留；末帧静止 hold ≥1.2s；首帧为纯背景或静止构图。
- [ ] **铁律扫描**：`grep -rn "Math.random\|Date.now\|setInterval\|setTimeout\|requestAnimationFrame\|transition:" scenes/ shared/film.css` 仅 film.js/record.mjs 中的白名单出现。
- [ ] **交付物**：`out/` 8 个 4K mp4；`stills/` 含 S1 首末、S2 末、S4 末、S5 末、S8 首末。

> 应急 fallback（仅当 Playwright 环境装不上）：Chrome 全屏打开 `scene.html?play`（显示器缩放 100%、
> 1920×1080），OBS 60fps 录屏。代价：可能掉帧、不可逐帧复现，仅救急。

### 4.9 给 Sonnet 的开工提示词（用户复制粘贴）

```
读 docs/demo-video-plan.md 的 §2 和 §4（其余章节不用读），在 tools/demo-film/ 按规范实施：
1. 先搭骨架：shared/film.css、shared/film.js（§4.2 逐字照抄）、record/record.mjs（§4.3 逐字照抄）、
   vendor 下载 gsap + MotionPathPlugin、.gitignore 追加 §4.1 的段落；
2. 实现 S1 与 S2 两个场景，跑通录制管线（node record.mjs + ffmpeg 合成），对照 §4.8 验收；
3. 验收过了再逐场景 S3→S8，每个场景：照 §4.6 的布局/逐字文案/时间轴表实现 → 录制 → 验收 → 删帧序列留 mp4；
4. 最后导出 §4.3 末尾列的 stills。
约束：不改 app-harmony/ contracts/ purplepi-control/ orangepi-vision/ 下任何文件；文案与配色不得
自创（只用 §4.6 文案与 film.css 变量）；动画铁律见 §4.2。字体文件若 assets/fonts/ 缺失，先提示我
下载链接并暂用 Noto Sans SC CDN 兜底继续。
```

---

## 5. Seedance 2 部分（AI 生成转场与氛围）

### 5.1 定位与清单

Seedance 只做 **HTML 做不了的"实拍质感"**：照片级氛围、图纸→实物的质变转场、实拍运镜增强。
**带 UI/文字的画面一律不交给 AI**（文字必花），那是 HTML 的领地。

| 编号 | 用途 | 模式 | 输入 | 时长 |
|---|---|---|---|---|
| **T1** | S4 等高线图纸 → 真实车间（接 R5 遥控段） | **首尾帧** | 首帧 = `stills/s4-map-last.png`；尾帧 = 实拍 R5 首帧截图 | 5s |
| **T2** | 开场后厂房氛围空镜 | 文生视频 | prompt | 5s |
| **T3** | 机器人 hero 运镜增强 | **图生视频** | 实拍机器人最佳静帧（R1 里挑） | 5s |
| **T4** | 结尾白场渐变 | — | **优先剪辑实现**（白场叠化），Seedance 备选 | 2s |

### 5.2 首尾帧衔接工作流（T1 关键）

1. HTML 侧：`node record.mjs ../scenes/s4-map.html --stills` 得末帧 4K PNG；
2. 实拍侧：从 R5 素材里截一帧静止首帧（构图：车在画面中相近位置，方便 AI 过渡）；
3. Seedance 选"首尾帧"模式上传两图 + prompt（下表 T1）；
4. 生成 ≥4 个候选，选过渡最干净的一条；
5. 剪辑时：T1 头尾各与 S4 末帧 / R5 首帧重叠 3–5 帧做 2 帧叠化，缝就看不见了。

### 5.3 Prompt（每条生成 ≥4 候选抽卡）

通用负面提示（每条都带）：
`文字、字幕、水印、logo、人脸特写、画面抖动、机械结构扭曲变形、夸张快速运动、过曝、霓虹色`

- **T1**：`浅米白图纸上的墨绿色建筑等高线图缓缓获得光影与立体质感，镜头缓慢推进，线稿逐渐显影为真实工厂车间的水泥地面、货架与管线，一台小型巡检机器人停在画面中，柔和漫射晨光，干净的工业产品广告质感，低饱和，米白与墨绿色调，运动平滑克制`
- **T2**：`清晨的现代化工厂车间内部，柔和天光从高侧窗洒落，干净的水泥地面，整齐的管线与压力仪表盘浅景深散景，镜头缓慢向右横移，极简工业美学，苹果产品广告质感，低饱和米白色调，无人`
- **T3**：`画面中的巡检机器人保持完全静止且结构不变，镜头围绕它缓慢环绕推近约30度，地面有柔和反光，背景为虚化的车间，光线干净柔和，高端产品广告运镜，浅色调`
  （⚠️ 必须用 i2v：拿实拍静帧喂，外观才一致；禁止 t2v 凭空生成机器人——形态会对不上实拍。）

### 5.4 注意事项

- 分辨率/时长档位以当前 Seedance 2 版本实际支持为准，**选最高分辨率档**；若产出非 60fps，
  剪辑里开"智能补帧/光流"到 60。
- 选片标准：无文字伪影、结构稳定不融化、**运动方向与下一镜衔接一致**（如 T2 向右横移 → 下一实拍
  镜头也右向运动更顺）。
- 生成画面色调若偏暖/偏暗，剪辑统一调色时向米白浅色靠（§7.5）。

---

## 6. 实拍部分

### 6.1 器材与统一设置

手机即可（够 1080p60）。**全部素材 1080p60**（统一帧率、随时可慢放）；锁定曝光与对焦
（点按住）；快门 ≥1/100 防工业灯 50Hz 频闪（专业模式设置，或开 60fps 自然规避）；横屏；
多拍 3 倍素材。增稳：手持稳定器最好，没有就双手贴肘 + 慢移，或手机放滑板/纸箱做"穷人滑轨"。

### 6.2 镜头清单 R1–R12

| 编号 | 内容 | 机位/要点 | 目标时长 |
|---|---|---|---|
| R1 | hero：机器人静止，环绕半圈 | **低机位贴地**（产品感的关键），慢、匀 | 10s×3 条 |
| R2 | 上电启动：指示灯亮、雷达起转 | 特写微距感，背景虚 | 5s×3 |
| R3 | 建图行驶：走廊直线段 | 侧面跟拍，与车同速 | 10s×3 |
| R4 | 避障：遇障碍绕行 | 固定机位拍全程 | 10s×2 |
| R5 | **同框**：前景手持平板推摇杆，背景车同步动 | 全片说服力最强的一镜；平板屏亮度拉满 | 10s×4 |
| R6 | 摇杆特写：手指推杆 | 俯拍平板，注意无反光 | 5s×2 |
| R7 | 平板录屏：发现→连接→「开始建图」→地图长出 | 录屏方法见 §6.3 | 完整流程 |
| R8 | 平板录屏：地图上点目标 pin → 路径 → 车到点 | 同上 | 完整流程 |
| R9 | 平板录屏：VisionPage 实时读数 + PiP 收放回导航 | 同上 | 完整流程 |
| R10 | 香橙派相机对真实压力表盘 | 拍"机器视角"：先拍表盘本体，再拍屏上叠加框对比 | 5s×3 |
| R11 | 配对/多机：HDMI 插上配对一次的动作（或双车同走） | 有啥拍啥，S6 概念段兜底 | 5s |
| R12 | 收尾空镜：车驶向走廊深处 / 三人围平板 | 给 T4 与片尾留素材 | 10s |

### 6.3 平板录屏方法

控制中心自带屏幕录制（录前：满电、清通知、亮度固定、关自动旋转）。成片只用 1080p 时间线，
录屏素材可在剪辑里 **放大 130–160% 推近关键区域**（按钮/读数），比全屏原样更有"演示感"。
关键操作前停顿 1s 再点（给观众反应时间，也方便剪辑卡点）。

### 6.4 拍摄日 checklist

场地预清扫（地面杂物会抢镜）→ 车满电、平板满电 → 先跑通一遍全流程再开拍 → 按 R7–R9 录屏 →
再拍 R1–R6/R10–R12 实拍 → 当场回看每条是否对焦/频闪 → 素材当天备份两份。

---

## 7. 剪辑与交付

- **工具**：剪映专业版（字幕快、有光流补帧）；想精调色用 DaVinci Resolve。二选一即可。
- **时间线**：1080p60。HTML 段用 4K 母版下放，需要强调处做 100%→115% 缓慢推近（与 §2.3 一致的"快进慢停"）。
- **组装**：照 §1 分镜表排列；HTML 段之间、HTML↔实拍之间**优先硬切在节拍上**；只有 T1/T4 用生成转场。少即是多：全片禁用剪辑软件自带花哨转场（叠化以上的都不用）。
- **音乐**：极简律动/电子钢琴，90–120 BPM（素材站搜 `minimal tech corporate / product launch`）。
  场景切换对齐乐句；S7 数据墙落在最后一段推进上；S8 用终止式收。
- **音效**：克制的 whoosh（场景切换）、UI tick（数字滚动/翻牌处）、低频 pop（卡落位）。比无声多 50% 质感，音量 -18dB 左右垫底。
- **字幕**：HTML 段文字已内置**不加字幕**；实拍段加小字幕（思源黑体 Medium、白字 60% 黑描边或品牌浅绿底条、底部安全区）。
- **调色**：实拍统一向"米白干净"靠：白平衡中性微暖、高光提一点、饱和 -10、对比 -5；与 HTML 段同屏不跳。
- **导出**：H.264 1080p60、码率 16–24 Mbps、yuv420p；另存一版 4K 母版归档。

---

## 8. 工序总表

| # | 谁 | 做什么 | 产出 |
|---|---|---|---|
| 1 | 用户 | 定 {NAME}、署名、（可选）改口号 → 回填本文 §0/§3 | 文案定稿 |
| 2 | Sonnet | §4.9 提示词开工：骨架 + S1/S2 + 管线验证 | film.css/js、record 管线、2 个 mp4 |
| 3 | Sonnet | S3→S8 逐场景实现+验收 | 8 个 4K mp4 + stills |
| 4 | 用户 | 实拍日（§6.4） | R1–R12 素材 |
| 5 | 用户 | Seedance：上传首尾帧/prompt、抽卡选片（§5） | T1–T3 片段 |
| 6 | 用户 | 剪辑（§7），按 §1 分镜组装 | 成片 1080p60 |
| 7 | 两人 | 互审一遍：数字/术语是否与 `contracts/` 一致、有无错别字 | 终版 |

预估工时：Sonnet HTML 1–2 个会话日；录帧半天（机器跑）；实拍半天；Seedance 半天；剪辑 1 天。

---

*数字与术语已核对来源：`contracts/udp-protocol.md`（9 字节布局、'f'=0x66、500ms 心跳、3s 急停、
IP 藏 byte 1/2/4/6、5cm 单位）、`contracts/vision-stream-api.md` 摘要（WS 双消息、15fps、关键点
name、`/api/summary`）、`docs/map-ui-redesign.md`（MS+Chaikin 管线、MapTheme 色值）、
`docs/map-pipeline.md`（真机 1800×1800）、`theme.ets`（全部色值）。改这些事实前先改对应契约/文档。*
