/* scenes7.js —— 开场『界面总览』五屏(callout 同时出现) + 末尾 setOrder 总片序(单一来源)。
   本轮：ui-control 地图改 MAPKIT 有机等高线(seed 7，与 03-build/algo-trio 同源)；
   新片序在每段 UI 演示前插入"原理逻辑图"(pr-*) + 开场后插背景/架构(bg/arch-*)。
   ⚠ 本挂载 file-tool Edit 会截断——改本文件只用 Write/bash 整文件覆盖。复用各 scenes 已注入 CSS。 */
(function () {
  "use strict";
  const F = window.FILM, MK = window.MAPKIT;     // 注：scenes7 文件名排序在 scenes_fx 之前，FX 须在调用时(boot)取 window.FX，不能 load 期捕获
  const caption = F.caption, callout = F.callout, addCSS = F.addCSS, I = F.I, STAGE_W = F.STAGE_W;

  /* 全平板共用的"楼面图"区域(viewBox 0 0 1270 782 内)，与 03-build 同源 */
  const FLOOR = { ox: 240, oy: 110, FW: 800, FH: 525 };
  const NAVPATH = [[464, 353], [586, 353], [586, 479], [852, 479]];   // 由 algo-trio astar(seed7) 路径等比映射，已知落自由区
  const PIN = [852, 479];
  function floorParts(seed) {
    const grid = MK.makeGrid(64, 42, seed), ox = FLOOR.ox, oy = FLOOR.oy, FW = FLOOR.FW, FH = FLOOR.FH, sx = FW / 64, sy = FH / 42;
    let gl = '';
    for (let i = 1; i < 22; i++) { gl += '<line x1="' + (ox + i * FW / 22).toFixed(1) + '" y1="' + oy + '" x2="' + (ox + i * FW / 22).toFixed(1) + '" y2="' + (oy + FH) + '"/>'; gl += '<line x1="' + ox + '" y1="' + (oy + i * FH / 22).toFixed(1) + '" x2="' + (ox + FW) + '" y2="' + (oy + i * FH / 22).toFixed(1) + '"/>'; }
    const floor = '<rect x="' + ox + '" y="' + oy + '" width="' + FW + '" height="' + FH + '" fill="#fbfaf4"/><g stroke="rgba(72,92,17,.06)" stroke-width="1">' + gl + '</g><rect x="' + ox + '" y="' + oy + '" width="' + FW + '" height="' + FH + '" fill="none" stroke="#e3e7d2" stroke-width="2"/>';
    let fills = '';
    for (let cy = 0; cy < 42; cy++) for (let cx = 0; cx < 64; cx++) if (grid.b[cy * 64 + cx] === 1) fills += '<rect x="' + (ox + cx * sx).toFixed(1) + '" y="' + (oy + cy * sy).toFixed(1) + '" width="' + (sx + 0.6).toFixed(1) + '" height="' + (sy + 0.6).toFixed(1) + '"/>';
    const contours = MK.extractContoursPx(grid, FW, FH, 2).map(function (c) {
      let d = 'M' + (ox + c.pts[0]).toFixed(1) + ' ' + (oy + c.pts[1]).toFixed(1);
      for (let i = 2; i < c.pts.length; i += 2) d += 'L' + (ox + c.pts[i]).toFixed(1) + ' ' + (oy + c.pts[i + 1]).toFixed(1);
      if (c.closed) d += 'Z'; return d;
    });
    return { floor: floor, fills: fills, contours: contours };
  }

  addCSS(
    '.tablet{width:1209px!important;height:755px!important;left:355px!important;top:96px!important;}' +
    '#caption{bottom:70px;}' +
    '.cap{top:auto;bottom:0;left:0;width:100%;padding:0 180px;}' +
    '.cap .cap-main{font-size:30px;line-height:1.3;}' +
    '.callout .co-label{background:var(--primary);border:none;color:#fff;box-shadow:0 8px 22px -8px rgba(40,46,14,.5);}' +
    '.callout .co-label .co-sub{color:rgba(255,255,255,.86);}'
  );

  function makeTablet(cls, html) {
    const cam = document.createElement('div'); cam.className = 'cam';
    const T = document.createElement('div'); T.className = 'tablet';
    const TW = 1300, TH = 812, TX = (STAGE_W - TW) / 2, TY = 128;
    T.style.cssText += 'left:' + TX + 'px;top:' + TY + 'px;width:' + TW + 'px;height:' + TH + 'px;';
    T.innerHTML = '<div class="cam-dot"></div>';
    const screen = document.createElement('div'); screen.className = 'screen';
    const page = document.createElement('div'); page.className = cls; page.innerHTML = html;
    screen.appendChild(page); T.appendChild(screen); cam.appendChild(T);
    return { cam: cam, T: T, page: page };
  }
  function enterIn(tl, T, a, style) {           // 平板入场多样化（不再清一色渐入）
    window.FX.enter(tl, T, a + 0.2, style || 'rise', { dur: 1.0, dist: 26, from: 0.9, origin: '50% 56%' });
  }
  function calloutsAll(tl, atIn, hold, items) {
    items.forEach(function (it, i) {
      const pos = atIn + i * 0.16, dur = (atIn + hold) - pos;
      callout(tl, pos, dur, it[0], it[1], { sub: it[2], dx: it[3].dx, dy: it[3].dy });
    });
  }

  /* 1) 首页 HomePage */
  function buildUIHome(root) {
    root.classList.add('paper');
    function card(ip, detail, kind) {
      const dot = kind === 'enter' ? 'var(--success)' : 'var(--text-caption)';
      const act = kind === 'enter'
        ? '<div class="act"><div class="variant v-enter" style="position:relative;opacity:1">进入控制 ' + I.fwd + '</div></div>'
        : '<div class="act"><div class="variant v-connect" style="position:relative;opacity:1">连接</div></div>';
      return '<div class="card"><div class="sdot" style="background:' + dot + '"></div><div class="col" style="flex:1"><div class="cname">巡检车</div><div class="cip mono">' + ip + '</div><div class="cdetail">' + detail + '</div></div>' + act + '</div>';
    }
    const html =
      '<div class="row"><div class="col"><div class="app-title" data-title>巡检控制</div><div class="app-sub">发现 2 台车辆</div></div><div class="spacer"></div>' +
      '<div class="iconbtn" data-trust>' + I.trust + '</div><div class="iconbtn" data-video>' + I.video + '</div><div class="btn-primary" data-discover>发现设备</div></div>' +
      '<div class="chips" data-chips><div class="chip sel">单机导航</div><div class="chip">全路径覆盖</div><div class="chip">多机协同</div></div>' +
      '<div class="cards" data-cards>' + card('192.168.43.12', '已连接 · 心跳正常 · 刚刚', 'enter') + card('192.168.43.27', '已发现 · 未连接', 'connect') + '</div>';
    const m = makeTablet('app', html); root.appendChild(m.cam);
    const q = function (s) { return m.page.querySelector(s); };
    return { cam: m.cam, T: m.T, title: q('[data-title]'), trust: q('[data-trust]'), video: q('[data-video]'), discover: q('[data-discover]'), chips: q('[data-chips]'), card1: m.page.querySelector('.card') };
  }
  function animUIHome(tl, s) {
    const a = s.start, r = s.refs; enterIn(tl, r.T, a, 'zoomBlur');
    caption(tl, a + 0.5, 5.5, '<b>界面总览 · 首页「巡检控制」</b>', '发现车辆 → 选作业模式 → 进入控制');
    calloutsAll(tl, a + 1.4, 4.4, [
      [r.title, '标题 / 状态副行', '巡检控制 · 发现 N 台车辆', { dx: -320, dy: -70 }],
      [r.trust, '设备互信', '多机前一次性配对', { dx: -30, dy: -150 }],
      [r.discover, '发现设备', '局域网广播自动发现', { dx: 70, dy: -150 }],
      [r.chips, '三种作业模式', '单机 / 全覆盖 / 多机', { dx: -350, dy: 30 }],
      [r.card1, '发现的车 · 卡片', '四态点 + IP + 连接/进入', { dx: 360, dy: 8 }]
    ]);
  }

  /* 2) 控制页 ControlPage —— 地图改 MAPKIT 有机等高线(seed 7) */
  function mapSVG() {
    const fp = floorParts(7), G = MK.TH.green;
    const rb = '<g transform="translate(' + NAVPATH[0][0] + ',' + NAVPATH[0][1] + ') rotate(8)">' + MK.robotSVG('o-robot', G, '1', 16) + '</g>';
    return '<svg viewBox="0 0 1270 782" preserveAspectRatio="xMidYMid slice"><defs><linearGradient id="mbgo" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#e9e7dc"/><stop offset="1" stop-color="#e1ded0"/></linearGradient></defs>' +
      '<rect width="1270" height="782" fill="url(#mbgo)"/>' + fp.floor +
      '<g class="mk-walls" fill="' + MK.TH.wallFill + '">' + fp.fills + '</g>' +
      '<g fill="none" stroke="' + MK.TH.wallStroke + '" stroke-width="2.6" stroke-linejoin="round" stroke-linecap="round">' + fp.contours.map(function (d) { return '<path d="' + d + '"/>'; }).join('') + '</g>' +
      '<path class="o-trail" d="' + MK.polyPath(NAVPATH) + '" fill="none" stroke="' + G + '" stroke-width="4" stroke-linecap="round" stroke-linejoin="round" opacity=".4"/>' +
      '<g class="o-pin" transform="translate(' + PIN[0] + ',' + PIN[1] + ')"><path d="M0 0 Q-9 -14 -9 -23 A9 9 0 0 1 9 -23 Q9 -14 0 0 Z" fill="' + MK.TH.target + '"/><circle cy="-23" r="3.6" fill="#fff"/></g>' + rb + '</svg>';
  }
  function buildUIControl(root) {
    root.classList.add('paper');
    const html = mapSVG() +
      '<div class="ctl-top"><div class="ctl-back">' + I.back + '</div><div class="ctl-pill"><span class="dot"></span><span class="ip">192.168.43.12</span><span class="md">单机导航</span><span class="caret">▼</span></div><div class="ctl-rebuild">重新建图</div><div class="ctl-icon ctl-video">' + I.video + '</div></div>' +
      '<div class="ctl-fabs"><div class="fab" data-fab>＋</div><div class="fab">−</div><div class="fab">⊙</div></div>' +
      '<div class="ctl-pip"><div class="pip-cap" data-pip>' + I.video + '<span>仪表视频</span></div></div>' +
      '<div class="sheet" data-sheet><div class="handle"></div><div class="op-hint">① 点「选目标点」再点地图选点 → ② 点「开始导航」</div><div class="op-pick">选目标点</div><div class="op-row"><div class="op-target"><div class="lbl">目标点</div><div class="val">(412, 286)</div></div><div class="op-btn ghost">取消</div><div class="op-btn primary">开始导航</div></div></div>';
    const m = makeTablet('app-map', html); root.appendChild(m.cam);
    const q = function (s) { return m.page.querySelector(s); };
    return { cam: m.cam, T: m.T, pill: q('.ctl-pill'), fab: q('[data-fab]'), pip: q('[data-pip]'), sheet: q('[data-sheet]'), robot: q('.o-robot'), pin: q('.o-pin') };
  }
  function animUIControl(tl, s) {
    const a = s.start, r = s.refs; enterIn(tl, r.T, a, 'rise');
    caption(tl, a + 0.5, 6.0, '<b>控制页 ControlPage</b>', '全屏地图打底，操作以浮层叠加（Google Maps 范式）');
    calloutsAll(tl, a + 1.4, 5.0, [
      [r.pill, '顶栏状态胶囊', '连接点 · IP · 模式', { dx: 80, dy: -130 }],
      [r.fab, '缩放 FAB', '＋ / − / ⊙ 回中', { dx: -240, dy: -20 }],
      [r.sheet, '底部操作卡', '随模式变 · 默认收起一行', { dx: 330, dy: -130 }],
      [r.pip, '仪表 PiP', '独立链路瞥表', { dx: 210, dy: 0 }]
    ]);
    caption(tl, a + 4.8, 2.4, '<b>地图：SLAM 有机等高线楼面图</b>', '机器人位姿 + 目标 pin（与建图/算法章同一张图）');
  }

  /* 3) 仪表识别页 VisionPage */
  function buildUIVision(root) {
    root.classList.add('paper');
    const gauge = '<svg viewBox="0 0 1232 392" style="position:absolute;inset:0;width:100%;height:100%"><circle cx="616" cy="200" r="120" fill="#fff" stroke="#dcd9cb" stroke-width="3"/><line x1="616" y1="200" x2="690" y2="126" stroke="#d9503f" stroke-width="5" stroke-linecap="round"/><circle cx="616" cy="200" r="8" fill="#2b2f23"/><rect x="468" y="54" width="296" height="296" rx="10" fill="none" stroke="#4678b8" stroke-width="3"/><text x="474" y="46" font-size="16" font-family="monospace" fill="#4678b8">gauge 0.94</text><circle cx="690" cy="126" r="6" fill="#dfa32f" stroke="#fff" stroke-width="2"/><circle cx="616" cy="200" r="6" fill="#dfa32f" stroke="#fff" stroke-width="2"/></svg>';
    const html =
      '<div class="v-top"><div class="ctl-back" style="box-shadow:none;background:transparent">' + I.back + '</div><div style="margin-left:10px"><div class="v-title">仪表识别</div><div class="v-host">192.168.43.66:8000</div></div><div style="flex:1"></div><div class="v-conn"><span class="dot"></span>已连接</div></div>' +
      '<div class="v-video" data-video>' + gauge + '<div class="v-pct">65.0 <small>%</small></div></div>' +
      '<div class="v-stats" data-stats><div class="v-stat"><div class="sv">15 <small>fps</small></div><div class="sl">帧率</div></div><div class="v-stat"><div class="sv">38 <small>ms</small></div><div class="sl">端到端推理</div></div><div class="v-stat"><div class="sv">2</div><div class="sl">检测数</div></div></div>' +
      '<div class="v-readings" data-readings><div class="rd"><div class="rl">表 1</div><div class="rv">0.62 MPa</div><div class="rs">仪表 1 · 正常</div></div><div class="rd alarm"><div class="rl">表 2</div><div class="rv">0.86 MPa</div><div class="rs">仪表 2 · 超上限告警</div></div></div>' +
      '<div class="v-report" data-report><div class="v-rtop"><div class="v-rtitle">分析报告</div><div class="v-rbtn">生成报告</div></div><div class="v-rnote">基于最近 24h 读数生成趋势 / 异常分析（DeepSeek）。</div></div>';
    const m = makeTablet('app-v', html); root.appendChild(m.cam);
    const q = function (s) { return m.page.querySelector(s); };
    return { cam: m.cam, T: m.T, top: m.page.querySelector('.v-title'), video: q('[data-video]'), stats: q('[data-stats]'), readings: q('[data-readings]'), report: q('[data-report]') };
  }
  function animUIVision(tl, s) {
    const a = s.start, r = s.refs; enterIn(tl, r.T, a, 'fall');
    caption(tl, a + 0.5, 5.3, '<b>仪表识别页 VisionPage</b>', '独立 WS 连香橙派，实时帧 + 读数（与导航解耦）');
    calloutsAll(tl, a + 1.4, 4.3, [
      [r.top, '顶栏', '标题 · host:port · 连接点', { dx: 300, dy: -2 }],
      [r.video, '视频区', '实时帧 + 检测框 + 4 关键点', { dx: -330, dy: -30 }],
      [r.readings, '读数面板', 'fps / 推理ms / 检测数 + 各表读数·告警', { dx: 330, dy: -8 }],
      [r.report, '分析报告', '生成报告 → DeepSeek 趋势/异常', { dx: 330, dy: 40 }]
    ]);
  }

  /* 4) 设备互信页 DeviceTrustPage */
  function buildUITrust(root) {
    root.classList.add('paper');
    const html =
      '<div class="tr-top"><span class="tr-back">〈</span><span class="tr-title" data-title>设备互信</span><span style="flex:1"></span><span class="tr-research" data-research>重新搜索</span></div>' +
      '<div class="tr-note" data-note><div class="nt">配对步骤</div><div class="nb">① 平板与车同一 WiFi/热点　② 车接显示器+鼠标　③ 点「配对」→ 车屏弹系统 PIN → 按平板提示输入。配对一次即持久互信。</div></div>' +
      '<div class="tr-sec"><span class="st">已信任设备</span><span class="sc">（0）</span></div>' +
      '<div class="tr-slot" data-tslot><div class="tr-hint">尚无已信任设备。在下方发现列表里选中车辆配对。</div></div>' +
      '<div class="tr-sec"><span class="st">发现的新设备</span><span class="sc">（1）</span></div>' +
      '<div class="tr-slot" data-dslot><div class="tr-row" data-drow><span class="dot" style="background:var(--text-caption)"></span><div><div class="dn">巡检车</div><div class="di">a1c93f…7f4e</div></div><span class="sp"></span><span class="tr-pill pair" data-pair>配对</span><span class="tr-pill ghost">解绑重置</span></div></div>';
    const m = makeTablet('app-trust', html); root.appendChild(m.cam);
    const q = function (s) { return m.page.querySelector(s); };
    return { cam: m.cam, T: m.T, title: q('[data-title]'), note: q('[data-note]'), tslot: q('[data-tslot]'), pair: q('[data-pair]') };
  }
  function animUITrust(tl, s) {
    const a = s.start, r = s.refs; enterIn(tl, r.T, a, 'zoomBlur');
    caption(tl, a + 0.5, 5.3, '<b>设备互信页 DeviceTrustPage</b>', '多机前一次性软总线配对，配对一次跨重启持久');
    calloutsAll(tl, a + 1.4, 4.3, [
      [r.title, '顶栏', '设备互信 · 重新搜索', { dx: 300, dy: -2 }],
      [r.note, '配对步骤', '同网 → 接屏 → 配对输 PIN', { dx: 340, dy: -8 }],
      [r.tslot, '已信任设备', '配对成功的车在此（持久）', { dx: -330, dy: 0 }],
      [r.pair, '发现的车 · 配对', '点「配对」→ 车屏弹 PIN', { dx: 300, dy: 8 }]
    ]);
  }

  /* 5) 手填 IP 页 SetIPPage */
  function buildUISetIP(root) {
    root.classList.add('paper');
    const html =
      '<div class="tr-top"><div class="ctl-back" style="box-shadow:none;background:transparent">' + I.back + '</div><span style="flex:1"></span></div>' +
      '<div class="si-title" data-title>手动连接</div>' +
      '<div class="si-sub" data-sub>发现不可用时（客户端隔离 / 固定 IP / 调试）手填车辆 IP。优先用首页“发现设备”。</div>' +
      '<div class="si-field" data-field><div class="si-label">车辆 IP</div><div class="si-input" data-input><span class="val">192.168.43.12</span></div></div>' +
      '<div class="si-save" data-save style="background:var(--primary);color:#fff">保存并连接</div>';
    const m = makeTablet('app-setip', html); root.appendChild(m.cam);
    const q = function (s) { return m.page.querySelector(s); };
    return { cam: m.cam, T: m.T, title: q('[data-title]'), input: q('[data-input]'), save: q('[data-save]') };
  }
  function animUISetIP(tl, s) {
    const a = s.start, r = s.refs; enterIn(tl, r.T, a, 'rise');
    caption(tl, a + 0.5, 7.5, '<b>手填 IP 页 SetIPPage</b>', '自动发现不可用时的兜底：手输 IP，实时校验后连接');
    calloutsAll(tl, a + 1.4, 6.5, [
      [r.title, '标题 / 说明', '手动连接 · 何时用兜底', { dx: 360, dy: -2 }],
      [r.input, 'IP 输入框', '边输边校验 isValidIp', { dx: 340, dy: -8 }],
      [r.save, '保存并连接', '校验通过才启用 → 控制页', { dx: 340, dy: -8 }]
    ]);
  }

  F.addScene({ id: 'ui-home', dur: 7.0, build: buildUIHome, anim: animUIHome });
  F.addScene({ id: 'ui-control', dur: 7.6, build: buildUIControl, anim: animUIControl });
  F.addScene({ id: 'ui-vision', dur: 7.0, build: buildUIVision, anim: animUIVision });
  F.addScene({ id: 'ui-trust', dur: 7.0, build: buildUITrust, anim: animUITrust });
  F.addScene({ id: 'ui-setip', dur: 10, build: buildUISetIP, anim: animUISetIP });
  function animHomeEnter(tl, s) {
    const a = s.start, r = s.refs; enterIn(tl, r.T, a, 'fall');
    caption(tl, a + 0.5, 3.2, '<b>互信完成 · 回到首页</b>', '点车卡「进入控制」进入控制台');
    var eb = r.card1.querySelector('.v-enter');
    F.cursorShow(tl, a + 1.4, F.centerOf(eb).x - 120, F.centerOf(eb).y + 80);
    F.tapEl(tl, a + 2.4, eb, 0.7);
    F.cursorHide(tl, a + 4.0);
  }
  F.addScene({ id: 'home-enter', dur: 5.5, build: buildUIHome, anim: animHomeEnter });

  /* ===== 总片序(单一来源) =====
     开始页 → 背景叙述 → 架构三图 → [每段 UI 演示前插该部分原理逻辑图] →
     各 pr-* 见 scenes_principles.js / bg/arch-* 见 scenes_intro.js。 */
  F.setOrder([
    '00-title',
    'bg-context',
    'hw-overview', 'hw-explode',
    'arch-nodes', 'arch-bus', 'arch-flow',
    'pr-discover', 'ui-home', '01-home',
    'pr-trust', 'ui-trust', '08-trust', 'pr-share', 'home-enter',
    'pr-map', 'ui-control',
    'pr-slam', '03-build',
    'algo-trio',
    'pr-vision', '07-vision',
    'demo-real'
  ]);
})();
