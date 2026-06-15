# PROGRESS · OpenHarmony 工业巡检机器人 · 演示视频

STATUS: IN_PROGRESS（用户手动迭代；看门狗已停用）
更新时间: 2026-06-14（承接改版：互信经首页「互信」按钮承接、仪表经多机末尾 PiP→放大 承接）

## 文件结构（拆分 + bash 打包；**改文件只用 bash heredoc 或 node 定点替换(带命中断言)，勿用 file-tool Edit/Write——会截断**）
- `film.css`；`engine.js`(setOrder+callout/infoPanel)；`scenes.js`(00标题/01首页/02控制台总览[弃用]) `scenes2`(建图/导航) `scenes3`(覆盖/多机+末尾PiP承接) `scenes4`(仪表·直接全屏) `scenes5`(互信/手填IP) `scenes6`(架构/协议/数字/收尾) `scenes7`(界面总览5屏 ui-* + home-enter 承接 + setOrder + 样式覆盖)
- `build.sh` glob 内联打包 → 自包含 `film.html`；自检 `node /tmp/t.mjs` `node /tmp/d.mjs`

## 现行顺序（19 场景 ≈ 2:06；整体加速 SPEED=2；0 报错/无章节卡/自包含）—— 章节按"操作承接"串起
00-title → 09-arch → 09-proto → ui-home → 01-home(末尾点**设备互信**) → ui-trust → 08-trust → **home-enter(回首页点进入控制)** → ui-control → 03-build → 04-nav → 05-cover → 06-fleet(末尾点**仪表视频→放大**) → ui-vision → 07-vision → ui-setip → 08-setip → 09-nums → 09-outro

## 本轮已完成（承接逻辑）
- [x] 仪表承接：07-vision 删掉自带 PiP 开场、直接全屏识别(14s)；改由 06-fleet 末尾「点仪表视频→放大」转场进入（给多机控制页加了可展开 PiP 卡）
- [x] 互信承接：互信章节从片尾移到 首页与控制之间；首页末尾点「设备互信」→ 互信章节 → home-enter 回首页点「进入控制」→ 控制章节
- [x] 全局加速：engine 主时间轴 timeScale=2 → 时长减半(4:11→2:06)，动画/停留同步加速，**零内容改动**；计时与导出钩子按真实时长换算（调 engine.js 顶部 SPEED 常量即可改速）
- [x] （前轮）UI 介绍并入各章节开头；首页减放大/删冗余点击；字幕间距；去重复 id

## 待确认 / 下一步
- [ ] 确认"IP界面"= 首页（已按此实现）；若指手填IP页请告知
- [ ] 手填IP(ui-setip/08-setip)目前在片尾(仪表之后)——是否挪到更靠前(连接相关)？待用户定
- [ ] callout 标签像素微调；三人署名(09-outro)；导出 mp4(需浏览器)
