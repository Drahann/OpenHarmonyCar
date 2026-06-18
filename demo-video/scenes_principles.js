/* scenes_principles.js —— 每章 UI 前的"原理"场景。本轮(用户反馈)大改：
   不再清一色 mermaid 流程框图 + 单一渐入 + 底部字幕。改成"实物级可视化"(像 algo-trio 算法动画那样)：
     · pr-discover = 心电监护式连接(ECG 心跳 + 四态)；  · pr-map = 坐标变换流水线(取点穿三坐标系)；
     · pr-slam     = 实时 SLAM(激光扫描 + 等高线生长 + 位姿图关键帧)；· pr-vision = 真表盘读数(检测框→关键点→几何角度)。
   仅 pr-trust 保留时序图(握手用时序图最贴切，非"流程框")。文字用 FX.note 散布屏幕各处、不再堆底部。多样入场用 FX.enter。
   忠实《产品说明书》图6-1/6-2/4-1/3-1~3-3/5-3；setOrder 单一来源仍在 scenes7.js。
   ⚠ 本挂载 file-tool Edit 会截断——改本文件只用 Write/bash 整文件覆盖。 */
(function () {
  "use strict";
  const F = window.FILM, FX = window.FX, DK = window.DIAGKIT, MK = window.MAPKIT;
  const caption = F.caption, COL = { g: '#485c11', green: '#3f9352', amber: '#dfa32f', red: '#d9503f', blue: '#4678b8', ink: '#1d2016', sec: '#6f7461' };

  /* ════════════════════ 图 6-1 连接状态机 → 上"两边发包" + 下"状态机"同演 ════════════════════ */
  function buildDiscover(root) {
    root.classList.add('paper');
    const wrap = document.createElement('div'); wrap.className = 'cam'; root.appendChild(wrap);
    const L = 462, Rr = 1492, y1 = 392, y2 = 452;   // 命令包泳道 / 心跳包泳道
    let dots = '';
    for (let k = 0; k < 10; k++) dots += '<circle class="cmd-pkt" cx="' + L + '" cy="' + y1 + '" r="10" fill="' + COL.g + '" opacity="0"/>';
    for (let k = 0; k < 10; k++) dots += '<circle class="hb-pkt" cx="' + Rr + '" cy="' + y2 + '" r="9" fill="' + COL.blue + '" opacity="0"/>';
    const svg =
      '<svg viewBox="0 0 1920 1080" style="position:absolute;inset:0;width:100%;height:100%">' +
      '<line x1="' + L + '" y1="' + y1 + '" x2="' + Rr + '" y2="' + y1 + '" stroke="' + COL.g + '" stroke-width="1.5" stroke-dasharray="2 9" opacity=".4"/>' +
      '<line x1="' + L + '" y1="' + y2 + '" x2="' + Rr + '" y2="' + y2 + '" stroke="' + COL.blue + '" stroke-width="1.5" stroke-dasharray="2 9" opacity=".4"/>' +
      '<text x="977" y="376" text-anchor="middle" font-size="18" font-family="Roboto Mono,monospace" fill="#566518" font-weight="600">命令包 →</text>' +
      '<text x="977" y="486" text-anchor="middle" font-size="18" font-family="Roboto Mono,monospace" fill="#4678b8" font-weight="600">← 心跳包（位姿）</text>' +
      '<g class="dv-tab" opacity="0"><rect x="278" y="318" width="172" height="214" rx="18" fill="#26281f"/><rect x="291" y="331" width="146" height="188" rx="9" fill="#eef2dd"/></g>' +
      '<g class="dv-car" opacity="0"><image href="assets/car_node.png" x="1486" y="332" width="236" height="222"/></g>' +
      dots +
      '<g class="dv-redx" opacity="0"><circle cx="1690" cy="346" r="21" fill="#fff" stroke="' + COL.red + '" stroke-width="3.5"/><line x1="1679" y1="335" x2="1701" y2="357" stroke="' + COL.red + '" stroke-width="3.5"/><line x1="1701" y1="335" x2="1679" y2="357" stroke="' + COL.red + '" stroke-width="3.5"/></g>' +
      '</svg>';
    wrap.innerHTML = svg;
    const sm = DK.mountFlow(wrap, {
      nodes: [
        { k: 'n', x: 1170, y: 600, w: 340, h: 96, shape: 'cap', tone: 'app', t: '已连接', s: 'connected · <2s', sm: true },
        { k: 'd', x: 178, y: 690, w: 300, h: 96, shape: 'cap', tone: 'neutral', t: '已发现', s: 'discovered', sm: true },
        { k: 'c', x: 632, y: 690, w: 300, h: 96, shape: 'cap', tone: 'vision', t: '连接中', s: 'connecting', sm: true },
        { k: 'l', x: 1170, y: 808, w: 340, h: 96, shape: 'cap', tone: 'danger', t: '已丢失', s: 'lost · >2s', sm: true }
      ],
      edges: [
        { a: 'd', b: 'c', label: 'startHeartbeat()', tone: 'neutral' },
        { a: 'c', b: 'n', label: '收心跳', tone: 'app' },
        { a: 'n', b: 'l', label: '超 2s', tone: 'danger' },
        { a: 'l', b: 'n', fromSide: 'r', toSide: 'r', elbow: true, viaX: 1576, label: '恢复', tone: 'app', lx: 1576, ly: 752 },
        { a: 'c', b: 'd', fromSide: 'b', toSide: 'b', elbow: true, viaY: 912, dash: true, tone: 'neutral', label: 'stopHeartbeat() → 回已发现', lx: 545, ly: 912 }
      ]
    });
    const q = function (s) { return wrap.querySelector(s); };
    const qa = function (s) { return Array.prototype.slice.call(wrap.querySelectorAll(s)); };
    return { wrap: wrap, tab: q('.dv-tab'), car: q('.dv-car'), redx: q('.dv-redx'), cmd: qa('.cmd-pkt'), hb: qa('.hb-pkt'), sm: sm, geom: { L: L, Rr: Rr, y1: y1, y2: y2 } };
  }
  function animDiscover(tl, s) {
    const a = s.start, r = s.refs, g = r.geom; FX.scene(s.root);
    function trip(el, x0, x1, y, st) {
      tl.set(el, { attr: { cx: x0, cy: y }, opacity: 0 }, st);
      tl.to(el, { opacity: 1, duration: 0.14 }, st);
      tl.to(el, { attr: { cx: x1 }, duration: 0.82, ease: 'power1.inOut' }, st);
      tl.to(el, { opacity: 0, duration: 0.2 }, st + 0.66);
    }
    FX.note(tl, a + 0.3, 0, 200, 134, 'RobotTransport · 心跳保活', { style: 'kicker', enter: 'left' });
    FX.note(tl, a + 0.6, 0, 200, 176, '两边<b>发包</b>，派生四态', { style: 'title', enter: 'rise', w: 760 });
    FX.enter(tl, r.tab, a + 0.9, 'left', { dur: 0.7 });
    FX.enter(tl, r.car, a + 1.1, 'right', { dur: 0.7 });
    DK.revealFlow(tl, a + 1.2, r.sm, { step: 0.16, nodeDur: 0.5 });   // 下方状态机一次性铺开（与上方并演）
    let ci = 0, hi = 0;
    // ① 连接中：命令包出，无回
    let t = a + 2.8;
    FX.note(tl, t, 1.9, 960, 268, '<span class="fxn-tag" style="--ac:#dfa32f">● 连接中 connecting</span>', { style: 'body', anchor: 'c', enter: 'spring' });
    FX.note(tl, t + 0.15, 1.9, 960, 552, '平板每秒发<b>命令包</b>保活，<b>还没收到心跳</b>', { style: 'sub', anchor: 'c', enter: 'fall', accent: COL.amber, w: 760 });
    FX.pulse(tl, t + 0.2, r.sm.shapeEls['c'], 1.1);
    trip(r.cmd[ci++], g.L, g.Rr, g.y1, t + 0.2); trip(r.cmd[ci++], g.L, g.Rr, g.y1, t + 1.1);
    // ② 已连接：命令 + 心跳双向
    t = a + 4.8;
    FX.note(tl, t, 2.4, 960, 268, '<span class="fxn-tag" style="--ac:#3f9352">● 已连接 connected</span>', { style: 'body', anchor: 'c', enter: 'spring' });
    FX.note(tl, t + 0.15, 2.4, 960, 552, '车每 <b>600ms</b> 回一帧<b>心跳</b>（位姿）· 新鲜度 2s 内 = 健康', { style: 'sub', anchor: 'c', enter: 'fall', accent: COL.green, w: 820 });
    FX.pulse(tl, t + 0.2, r.sm.shapeEls['n'], 1.1);
    for (let i = 0; i < 3; i++) trip(r.cmd[ci++], g.L, g.Rr, g.y1, t + i * 1.0);
    for (let i = 0; i < 6; i++) trip(r.hb[hi++], g.Rr, g.L, g.y2, t + 0.3 + i * 0.6);
    // ③ 已丢失：心跳停，命令仍发，红叉
    t = a + 8.6;
    FX.note(tl, t, 2.4, 960, 268, '<span class="fxn-tag" style="--ac:#d9503f">● 已丢失 lost</span>', { style: 'body', anchor: 'c', enter: 'spring' });
    FX.note(tl, t + 0.15, 2.4, 960, 552, '心跳 <b>超 2s</b> 未回 · 提示丢失（命令包仍在重试）', { style: 'sub', anchor: 'c', enter: 'fall', accent: COL.red, w: 760 });
    FX.pulse(tl, t + 0.2, r.sm.shapeEls['l'], 1.1);
    tl.to(r.redx, { opacity: 1, duration: 0.3, ease: 'back.out(1.6)' }, t + 0.5);
    for (let i = 0; i < 3; i++) trip(r.cmd[ci++], g.L, g.Rr, g.y1, t + i * 1.0);
    // ④ 恢复
    t = a + 11.4;
    FX.note(tl, t, 2.2, 960, 268, '<span class="fxn-tag" style="--ac:#3f9352">● 心跳恢复 → 已连接</span>', { style: 'body', anchor: 'c', enter: 'spring' });
    FX.pulse(tl, t + 0.2, r.sm.shapeEls['n'], 1.1);
    tl.to(r.redx, { opacity: 0, duration: 0.3 }, t);
    for (let i = 0; i < 3; i++) trip(r.hb[hi++], g.Rr, g.L, g.y2, t + 0.2 + i * 0.6);
  }

  /* ════════════════════ 图 5-3 设备互信时序（保留时序图 · 握手适用）════════════════════ */
  function buildTrustPr(root) {
    root.classList.add('paper');
    const cam = document.createElement('div'); cam.className = 'cam'; root.appendChild(cam);
    const r = DK.mountSeq(cam, {
      fig: '图 5-3', title: '设备互信 · distributedDeviceManager + PIN', top: 252, bottom: 902,
      actors: [
        { k: 'T', x: 380, label: '平板', sub: '发起方', tone: 'app' },
        { k: 'M', x: 980, label: 'DeviceManager', sub: 'distributed', tone: 'neutral' },
        { k: 'C', x: 1560, label: '车端', sub: '目标设备', tone: 'pi' }
      ],
      msgs: [
        { from: 'T', to: 'M', y: 340, label: 'createDeviceManager(BUNDLE)', tone: 'app', g: 1 },
        { from: 'T', to: 'M', y: 388, label: 'startDiscovering {type:1}', tone: 'app', g: 1 },
        { from: 'M', to: 'T', y: 436, label: "on('discoverSuccess')", tone: 'neutral', dash: true, g: 1 },
        { from: 'T', to: 'M', y: 496, label: 'bindTarget(bindType:1 PIN)', tone: 'app', g: 2 },
        { from: 'M', to: 'C', y: 544, label: '发起安全认证', tone: 'neutral', g: 2 },
        { from: 'C', to: 'C', y: 590, self: true, label: '系统 PIN 弹窗（接 HDMI 确认）', tone: 'vision', g: 3 },
        { from: 'C', to: 'M', y: 660, label: '用户确认 PIN', tone: 'pi', dash: true, g: 3 },
        { from: 'M', to: 'T', y: 708, label: 'bindTarget 成功', tone: 'pi', dash: true, g: 4 },
        { from: 'T', to: 'C', y: 778, noteOnly: true, note: '互信建立（持久 · 跨重启）→ 同 bundle + 已互信，joinSession 同一 DDO 共享黑板', noteW: 620, g: 5 }
      ]
    });
    return { cam: cam, dk: r };
  }
  function animTrustPr(tl, s) {
    const a = s.start, r = s.refs; FX.scene(s.root);
    FX.note(tl, a + 0.3, 0, 96, 150, '软总线 · 设备互信', { style: 'kicker', enter: 'left' });
    DK.revealSeq(tl, a + 0.6, r.dk, { actorStep: 0.15, msgStep: 0.54 });
    FX.note(tl, a + 1.0, 4.0, 1760, 250, '发现 → PIN 认证<br>→ 入<b>可信环</b>', { style: 'h', anchor: 'r', enter: 'right', w: 360, accent: COL.g });
    FX.note(tl, a + 5.0, 3.2, 1760, 470, '配对一次，<b>跨重启持久</b><br>之后 agent 直接入会同步黑板', { style: 'sub', anchor: 'r', enter: 'right', w: 360, accent: COL.g });
  }

  /* ════════════════════ 图 6-2 三套坐标系 → 取点穿坐标系流水线 ════════════════════ */
  function buildMapPr(root) {
    root.classList.add('paper');
    const wrap = document.createElement('div'); wrap.className = 'cam'; root.appendChild(wrap);
    // 左：平板屏(像素)；右：真实世界图(5cm/格)；点从左穿到右，再反向渲染位姿
    const fp = MK.extractContoursPx(MK.makeGrid(64, 42, 7), 470, 300, 2);
    let cont = ''; fp.forEach(function (c) { let d = 'M' + (1180 + c.pts[0]).toFixed(0) + ' ' + (300 + c.pts[1]).toFixed(0); for (let i = 2; i < c.pts.length; i += 2) d += 'L' + (1180 + c.pts[i]).toFixed(0) + ' ' + (300 + c.pts[i + 1]).toFixed(0); if (c.closed) d += 'Z'; cont += '<path d="' + d + '"/>'; });
    const tg = MK.extractContoursPx(MK.makeGrid(64, 42, 7), 442, 292, 2);
    let tcont = ''; tg.forEach(function (c) { let d = 'M' + (214 + c.pts[0]).toFixed(0) + ' ' + (314 + c.pts[1]).toFixed(0); for (let i = 2; i < c.pts.length; i += 2) d += 'L' + (214 + c.pts[i]).toFixed(0) + ' ' + (314 + c.pts[i + 1]).toFixed(0); if (c.closed) d += 'Z'; tcont += '<path d="' + d + '"/>'; });
    const svg =
      '<svg viewBox="0 0 1920 1080" style="position:absolute;inset:0;width:100%;height:100%">' +
      // 平板屏（像素坐标系 ③）
      '<g class="mp-tab" opacity="0"><rect x="200" y="300" width="470" height="320" rx="20" fill="#26281f"/><rect x="214" y="314" width="442" height="292" rx="10" fill="#fbfaf4"/>' +
      '<g fill="none" stroke="' + COL.g + '" stroke-width="1.6" stroke-linejoin="round" opacity=".68">' + tcont + '</g>' +
      '<circle class="mp-tap" cx="520" cy="470" r="0" fill="none" stroke="' + COL.red + '" stroke-width="3"/></g>' +
      // 真实世界图（① 5cm/格）
      '<g class="mp-world" opacity="0"><rect x="1180" y="300" width="470" height="300" rx="10" fill="#fbfaf4" stroke="#e3e7d2" stroke-width="2"/>' +
      '<g stroke="rgba(72,92,17,.07)" stroke-width="1">' + (function () { let g = ''; for (let i = 1; i < 12; i++) g += '<line x1="' + (1180 + i * 470 / 12) + '" y1="300" x2="' + (1180 + i * 470 / 12) + '" y2="600"/>'; for (let i = 1; i < 8; i++) g += '<line x1="1180" y1="' + (300 + i * 300 / 8) + '" x2="1650" y2="' + (300 + i * 300 / 8) + '"/>'; return g; })() + '</g>' +
      '<g fill="none" stroke="' + COL.g + '" stroke-width="2.2" stroke-linejoin="round">' + cont + '</g>' +
      '<g class="mp-pin" transform="translate(1470,470)" opacity="0"><path d="M0 0 Q-10 -16 -10 -25 A10 10 0 0 1 10 -25 Q10 -16 0 0 Z" fill="' + COL.red + '"/><circle cy="-25" r="4" fill="#fff"/></g></g>' +
      // 飞行的点 + 连线
      '<line class="mp-arrow" x1="690" y1="455" x2="1160" y2="455" stroke="' + COL.g + '" stroke-width="2.5" stroke-dasharray="7 6" marker-end="" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1" opacity=".0"/>' +
      '<circle class="mp-dot" cx="520" cy="470" r="9" fill="' + COL.red + '" stroke="#fff" stroke-width="2" opacity="0"/>' +
      // 反向：位姿渲染
      '<line class="mp-arrow2" x1="1160" y1="660" x2="690" y2="660" stroke="' + COL.blue + '" stroke-width="2.5" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1" opacity="0"/>' +
      '<g class="mp-robot" opacity="0"><polygon points="0,-13 11,5 -11,5" fill="' + COL.blue + '"/></g>' +
      '</svg>';
    wrap.innerHTML = svg;
    const q = function (s) { return wrap.querySelector(s); };
    return { wrap: wrap, tab: q('.mp-tab'), tap: q('.mp-tap'), world: q('.mp-world'), pin: q('.mp-pin'), arrow: q('.mp-arrow'), dot: q('.mp-dot'), arrow2: q('.mp-arrow2'), robot: q('.mp-robot') };
  }
  function animMapPr(tl, s) {
    const a = s.start, r = s.refs; FX.scene(s.root);
    FX.note(tl, a + 0.3, 0, 200, 150, 'MapService · 三套坐标系', { style: 'kicker', enter: 'left' });
    FX.note(tl, a + 0.6, 0, 200, 188, '点哪走哪，<b>严格互逆</b>', { style: 'title', enter: 'rise', w: 760 });
    FX.enter(tl, r.tab, a + 1.0, 'left', { dur: 0.7 });
    FX.enter(tl, r.world, a + 1.3, 'right', { dur: 0.7 });
    FX.note(tl, a + 1.0, 9.5, 300, 640, '③ 屏幕像素', { style: 'chip', accent: COL.blue, enter: 'fall' });
    FX.note(tl, a + 1.3, 9.5, 1300, 640, '① 真实世界 · 5cm/格', { style: 'chip', accent: COL.g, enter: 'fall' });
    // 取点：finger tap
    let t = a + 2.2;
    FX.note(tl, t, 2.6, 430, 240, '手指点屏 → 像素坐标', { style: 'sub', enter: 'rise', accent: COL.blue });
    tl.fromTo(r.tap, { attr: { r: 4 }, opacity: 0.9 }, { attr: { r: 30 }, opacity: 0, duration: 0.7, ease: 'power2.out' }, t);
    tl.to(r.dot, { opacity: 1, duration: 0.25 }, t + 0.2);
    FX.note(tl, t + 0.3, 2.2, 520, 545, '(px 412, 286)', { style: 'mono', anchor: 'c', enter: 'pop' });
    // 飞过去：canvas2map → map2world
    t = a + 4.6;
    tl.to(r.arrow, { opacity: 0.9, duration: 0.2 }, t);
    FX.enter(tl, r.arrow, t, 'draw', { dur: 0.7 });
    FX.note(tl, t + 0.1, 2.6, 925, 408, 'canvas2map → map2world', { style: 'mono', anchor: 'c', enter: 'fall', accent: COL.g });
    tl.to(r.dot, { attr: { cx: 1470, cy: 470 }, duration: 1.0, ease: 'power2.inOut' }, t + 0.4);
    tl.to(r.dot, { opacity: 0, duration: 0.2 }, t + 1.4);
    tl.to(r.pin, { opacity: 1, duration: 0.3, ease: 'back.out(1.6)' }, t + 1.4);
    tl.fromTo(r.pin, { y: -10 }, { y: 0, duration: 0.4, ease: 'back.out(1.8)' }, t + 1.4);
    FX.note(tl, t + 1.5, 2.4, 1470, 545, '(x 5.1m, y 3.4m)', { style: 'mono', anchor: 'c', enter: 'pop', accent: COL.g });
    // 反向：位姿渲染
    t = a + 7.6;
    FX.note(tl, t, 3.0, 925, 700, '位姿渲染：world2map → map2canvas（必须严格互逆）', { style: 'mono', anchor: 'c', enter: 'rise', accent: COL.blue });
    tl.to(r.arrow2, { opacity: 0.9, duration: 0.2 }, t);
    FX.enter(tl, r.arrow2, t, 'draw', { dur: 0.7 });
    tl.set(r.robot, { x: 1180, y: 660, opacity: 0, transformOrigin: '0px 0px' }, t);
    tl.to(r.robot, { opacity: 1, duration: 0.25 }, t + 0.3);
    tl.to(r.robot, { x: 690, y: 660, duration: 1.1, ease: 'power2.inOut' }, t + 0.5);
    FX.note(tl, t + 1.8, 2.0, 430, 700, '机器人画回屏幕像素', { style: 'sub', enter: 'left', accent: COL.blue });
  }

  /* ════════════════════ 图 4-1 SLAM → 实时扫描建图 ════════════════════ */
  function buildSlam(root) {
    root.classList.add('paper');
    const wrap = document.createElement('div'); wrap.className = 'cam'; root.appendChild(wrap);
    const ox = 470, oy = 196, FW = 1000, FH = 640, seed = 7;
    const grid = MK.makeGrid(64, 42, seed), sx = FW / 64, sy = FH / 42;
    let fills = ''; for (let cy = 0; cy < 42; cy++) for (let cx = 0; cx < 64; cx++) if (grid.b[cy * 64 + cx] === 1) fills += '<rect x="' + (ox + cx * sx).toFixed(1) + '" y="' + (oy + cy * sy).toFixed(1) + '" width="' + (sx + 0.6).toFixed(1) + '" height="' + (sy + 0.6).toFixed(1) + '"/>';
    const cont = MK.extractContoursPx(grid, FW, FH, 2).map(function (c) { let d = 'M' + (ox + c.pts[0]).toFixed(1) + ' ' + (oy + c.pts[1]).toFixed(1); for (let i = 2; i < c.pts.length; i += 2) d += 'L' + (ox + c.pts[i]).toFixed(1) + ' ' + (oy + c.pts[i + 1]).toFixed(1); if (c.closed) d += 'Z'; return d; });
    const A = [770, 648], Bp = [1180, 648];   // 两个扫描位（中央块下方自由区）
    const lidar = function (cls, cx, cy) {
      const R = 172; let rays = ''; for (let k = 0; k < 14; k++) { const ang = -Math.PI + k / 13 * 2 * Math.PI, ex = cx + Math.cos(ang) * R, ey = cy + Math.sin(ang) * R; rays += '<line x1="' + cx + '" y1="' + cy + '" x2="' + ex.toFixed(0) + '" y2="' + ey.toFixed(0) + '" stroke="#7bbf4f" stroke-width="1.5" opacity=".5"/>'; }
      const wedge = '<path d="M' + cx + ' ' + cy + ' L' + (cx + R) + ' ' + (cy - 38) + ' A' + R + ' ' + R + ' 0 0 1 ' + (cx + R) + ' ' + (cy + 38) + ' Z" fill="#9ccb6a" opacity=".22"/>';
      return '<g class="' + cls + '" opacity="0"><g class="' + cls + '-rays">' + rays + '</g><g class="' + cls + '-wd">' + wedge + '</g></g>';
    };
    const svg =
      '<svg viewBox="0 0 1920 1080" style="position:absolute;inset:0;width:100%;height:100%">' +
      '<rect x="' + ox + '" y="' + oy + '" width="' + FW + '" height="' + FH + '" rx="6" fill="#fbfaf4" stroke="#e3e7d2" stroke-width="2"/>' +
      '<g class="sl-fills" fill="rgba(72,92,17,.13)" opacity="0">' + fills + '</g>' +
      '<g fill="none" stroke="#4a5d18" stroke-width="2.4" stroke-linejoin="round" stroke-linecap="round">' + cont.map(function (d) { return '<path class="sl-wp" d="' + d + '" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>'; }).join('') + '</g>' +
      // 位姿图：关键帧节点 + 边
      '<line class="sl-edge" x1="' + A[0] + '" y1="' + A[1] + '" x2="' + Bp[0] + '" y2="' + Bp[1] + '" stroke="' + COL.g + '" stroke-width="2" stroke-dasharray="5 5" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1" opacity=".0"/>' +
      lidar('sl-la', A[0], A[1]) + lidar('sl-lb', Bp[0], Bp[1]) +
      '<g class="sl-kfA" opacity="0"><circle cx="' + A[0] + '" cy="' + A[1] + '" r="7" fill="#fff" stroke="' + COL.g + '" stroke-width="3"/></g>' +
      '<g class="sl-kfB" opacity="0"><circle cx="' + Bp[0] + '" cy="' + Bp[1] + '" r="7" fill="#fff" stroke="' + COL.g + '" stroke-width="3"/></g>' +
      MK.robotSVG('sl-robot', COL.green, '1', 13) +
      '</svg>';
    wrap.innerHTML = svg;
    const q = function (s) { return wrap.querySelector(s); };
    const qa = function (s) { return Array.prototype.slice.call(wrap.querySelectorAll(s)); };
    return { wrap: wrap, fills: q('.sl-fills'), wp: qa('.sl-wp'), robot: q('.sl-robot'), la: q('.sl-la'), lb: q('.sl-lb'), laWd: q('.sl-la-wd'), lbWd: q('.sl-lb-wd'), kfA: q('.sl-kfA'), kfB: q('.sl-kfB'), edge: q('.sl-edge'), A: A, B: Bp };
  }
  function animSlam(tl, s) {
    const a = s.start, r = s.refs; FX.scene(s.root);
    FX.note(tl, a + 0.3, 0, 200, 150, 'Navi · 图优化扫描匹配 SLAM', { style: 'kicker', enter: 'left' });
    FX.note(tl, a + 0.6, 0, 200, 188, '边走边建，<b>边建边定位</b>', { style: 'title', enter: 'rise', w: 720 });
    tl.set(r.robot, { x: r.A[0], y: r.A[1], rotation: 0, transformOrigin: '0px 0px', opacity: 0 }, a);
    tl.to(r.robot, { opacity: 1, duration: 0.5, ease: 'power2.out' }, a + 1.0);
    // 扫描 A：激光旋转 + 似然场匹配 + 等高线近 A 揭示
    let t = a + 1.6;
    FX.note(tl, t, 3.0, 250, 470, '激光雷达 + 里程计<br><b>扫描匹配 → 反推位姿</b>（定位）', { style: 'sub', enter: 'left', w: 320, accent: COL.g });
    tl.to(r.la, { opacity: 1, duration: 0.3 }, t);
    tl.fromTo(r.laWd, { rotation: -120, svgOrigin: r.A[0] + ' ' + r.A[1] }, { rotation: 240, duration: 2.2, ease: 'power1.inOut' }, t + 0.1);
    tl.to(r.kfA, { opacity: 1, duration: 0.3, ease: 'back.out(1.6)' }, t + 0.6);
    r.wp.forEach(function (w, i) { if (i % 2 === 0) tl.fromTo(w, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: 1.6, ease: 'power1.out' }, t + 0.4 + (i / 2) * 0.18); });
    tl.to(r.fills, { opacity: 1, duration: 1.6 }, t + 0.6);
    tl.to(r.la, { opacity: 0, duration: 0.4 }, t + 2.6);
    // 机器人移动 A→B（位姿图加一条边）
    t = a + 4.8;
    FX.note(tl, t, 2.6, 975, 880, '位移 ≥ 0.35m / 转角 ≥ 30° → 新增<b>关键帧</b>（位姿图节点）', { style: 'mono', anchor: 'c', enter: 'rise', accent: COL.g });
    tl.to(r.edge, { opacity: 0.7, duration: 0.2 }, t);
    MK.driveRobot({ tl: tl, robot: r.robot, trail: null, pts: [r.A, r.B], t0: t + 0.2, segDur: 1.2, turnDur: 0.3 });
    tl.fromTo(r.edge, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: 1.2, ease: 'none' }, t + 0.5);
    // 扫描 B：再匹配 + 其余等高线揭示（建图累加）
    t = a + 7.4;
    FX.note(tl, t, 3.2, 1670, 470, '轮廓累加进概率地图<br><b>新观测 → 扩展地图</b>（建图）', { style: 'sub', anchor: 'r', enter: 'right', w: 320, accent: COL.g });
    tl.to(r.lb, { opacity: 1, duration: 0.3 }, t);
    tl.fromTo(r.lbWd, { rotation: -120, svgOrigin: r.B[0] + ' ' + r.B[1] }, { rotation: 240, duration: 2.2, ease: 'power1.inOut' }, t + 0.1);
    tl.to(r.kfB, { opacity: 1, duration: 0.3, ease: 'back.out(1.6)' }, t + 0.6);
    r.wp.forEach(function (w, i) { if (i % 2 === 1) tl.fromTo(w, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: 1.6, ease: 'power1.out' }, t + 0.4 + ((i - 1) / 2) * 0.18); });
    tl.to(r.lb, { opacity: 0, duration: 0.4 }, t + 2.6);
    // 收束
    t = a + 10.8;
    FX.note(tl, t, 1.9, 960, 880, '定位与建图逐帧交替、相互修正 · 不依赖任何 SLAM 库（OpenHarmony 自研）', { style: 'mono', anchor: 'c', enter: 'rise', accent: COL.g });
  }

  /* ════════════════════ 图 3-1~3-3 视觉两段式 → 真表盘读数 ════════════════════ */
  function buildVisionPr(root) {
    root.classList.add('paper');
    const wrap = document.createElement('div'); wrap.className = 'cam'; root.appendChild(wrap);
    // 真实指针压力表照片 gauge_real.png（透明底）。几何取自源图(1021×1002)实测：
    //   表盘圆心(488,478) · 指针 247°(读数≈0.40MPa) · 零刻度140° · 满刻度40°(扫角 260°)。
    // 放置：图像 x=386 y=292 宽541高531(scale=541/1021≈0.530)；表盘圆心落舞台(645,545)。
    const IMGX = 386, IMGY = 292, IMGW = 541, IMGH = 531;
    const ANG = function (deg, rr) { const a = deg * Math.PI / 180; return [645 + Math.cos(a) * rr, 545 + Math.sin(a) * rr]; };
    const C = [645, 545], Ptip = ANG(247, 183), Z = ANG(140, 197), Fp = ANG(40, 197);
    const arcR = 95, A0 = ANG(140, arcR), A1 = ANG(247, arcR);
    const bx = 395, by = 295, bw = 500;
    const kp = function (cls, p) { return '<g class="' + cls + '" opacity="0"><circle cx="' + p[0].toFixed(0) + '" cy="' + p[1].toFixed(0) + '" r="9" fill="' + COL.amber + '" stroke="#fff" stroke-width="2.5"/></g>'; };
    const svg =
      '<svg viewBox="0 0 1920 1080" style="position:absolute;inset:0;width:100%;height:100%">' +
      '<image class="vp-gauge" href="assets/gauge_real.png" x="' + IMGX + '" y="' + IMGY + '" width="' + IMGW + '" height="' + IMGH + '" opacity="0"/>' +
      // 检测框 + 置信度标签
      '<rect class="vp-box" x="' + bx + '" y="' + by + '" width="' + bw + '" height="' + bw + '" rx="14" fill="none" stroke="' + COL.blue + '" stroke-width="3.5" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1" opacity="0"/>' +
      '<text class="vp-boxlbl" x="' + bx + '" y="' + (by - 12) + '" font-size="22" font-family="Roboto Mono,monospace" fill="' + COL.blue + '" font-weight="700" opacity="0">gauge 0.96</text>' +
      // 关键点：圆心 / 指针尖 / 零刻度 / 满刻度
      kp('vp-kc', C) + kp('vp-kp', Ptip) + kp('vp-kz', Z) + kp('vp-kf', Fp) +
      // 几何：零/满射线 + 读数弧（圆心→指针 扫过角）
      '<g class="vp-geo"><line class="vp-rz" x1="' + C[0].toFixed(0) + '" y1="' + C[1].toFixed(0) + '" x2="' + Z[0].toFixed(0) + '" y2="' + Z[1].toFixed(0) + '" stroke="' + COL.g + '" stroke-width="2.5" stroke-dasharray="6 5" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>' +
      '<line class="vp-rf" x1="' + C[0].toFixed(0) + '" y1="' + C[1].toFixed(0) + '" x2="' + Fp[0].toFixed(0) + '" y2="' + Fp[1].toFixed(0) + '" stroke="' + COL.g + '" stroke-width="2.5" stroke-dasharray="6 5" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>' +
      '<path class="vp-arc" d="M' + A0[0].toFixed(0) + ' ' + A0[1].toFixed(0) + ' A' + arcR + ' ' + arcR + ' 0 0 1 ' + A1[0].toFixed(0) + ' ' + A1[1].toFixed(0) + '" fill="none" stroke="' + COL.amber + '" stroke-width="6" stroke-linecap="round" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/></g>' +
      '</svg>';
    wrap.innerHTML = svg;
    // 右侧补"端到端流水线"流程图（mermaid 回归），节点随表盘各步同步点亮
    const vflow = DK.mountFlow(wrap, {
      nodes: [
        { k: 'n1', x: 1150, y: 206, w: 300, h: 76, tone: 'vision', t: '摄像头帧', s: '香橙派 · BGR', sm: true },
        { k: 'n2', x: 1150, y: 312, w: 300, h: 76, tone: 'vision', t: '① YOLOv5s 检测框', s: '昇腾 OM', sm: true },
        { k: 'n3', x: 1150, y: 418, w: 300, h: 76, tone: 'app', t: '② 4 关键点', s: 'Simple Baselines 热力图', sm: true },
        { k: 'n4', x: 1150, y: 524, w: 300, h: 76, tone: 'app', t: '③ 几何换算 atan2', s: '占比 = ∠(Z→P)/∠(Z→F)', sm: true },
        { k: 'n5', x: 1150, y: 630, w: 300, h: 76, tone: 'info', t: '④ 读数% + 越限告警', s: 'WS 推 App · DeepSeek', sm: true }
      ],
      edges: [{ a: 'n1', b: 'n2' }, { a: 'n2', b: 'n3' }, { a: 'n3', b: 'n4' }, { a: 'n4', b: 'n5' }]
    });
    const q = function (s) { return wrap.querySelector(s); };
    return { wrap: wrap, vflow: vflow, gauge: q('.vp-gauge'), box: q('.vp-box'), boxlbl: q('.vp-boxlbl'), kc: q('.vp-kc'), kp: q('.vp-kp'), kz: q('.vp-kz'), kf: q('.vp-kf'), rz: q('.vp-rz'), rf: q('.vp-rf'), arc: q('.vp-arc') };
  }
  function animVisionPr(tl, s) {
    const a = s.start, r = s.refs; FX.scene(s.root);
    FX.note(tl, a + 0.3, 0, 120, 140, '香橙派 · 昇腾 NPU · 视觉', { style: 'kicker', enter: 'left' });
    FX.note(tl, a + 0.6, 0, 120, 182, '两段式<b>可解释</b>读数', { style: 'title', enter: 'rise', w: 760 });
    FX.enter(tl, r.gauge, a + 1.0, 'zoomBlur', { dur: 0.8, origin: '50% 50%' });
    // 右侧"端到端流水线"流程图：节点随表盘每一步同步点亮（mermaid 与实演同屏）
    const vf = r.vflow;
    function revN(k, at, ei) { FX.enter(tl, vf.shapeEls[k], at, 'spring', { dur: 0.5 }); FX.enter(tl, vf.textEls[k], at + 0.05, 'rise', { dur: 0.5, dist: 12 }); if (ei != null && vf.edgeEls[ei]) FX.enter(tl, vf.edgeEls[ei].path, at, 'draw', { dur: 0.5 }); }
    FX.note(tl, a + 0.9, 0, 1300, 158, '端到端流水线 · ~40ms', { style: 'kicker', anchor: 'c', enter: 'fall' });
    revN('n1', a + 1.3);
    // ① 检测：画检测框 + 点亮流程图 ①
    let t = a + 2.0;
    FX.enter(tl, r.box, t, 'draw', { dur: 0.9 });
    tl.to(r.boxlbl, { opacity: 1, duration: 0.4, ease: 'power2.out' }, t + 0.5);
    revN('n2', t + 0.2, 0); DK.pulse(tl, t + 0.8, vf.shapeEls['n2']);
    // ② 关键点
    t = a + 4.6;
    FX.enter(tl, r.kc, t + 0.0, 'spring', { dur: 0.4 });
    FX.enter(tl, r.kp, t + 0.18, 'spring', { dur: 0.4 });
    FX.enter(tl, r.kz, t + 0.36, 'spring', { dur: 0.4 });
    FX.enter(tl, r.kf, t + 0.54, 'spring', { dur: 0.4 });
    revN('n3', t + 0.4, 1); DK.pulse(tl, t + 1.0, vf.shapeEls['n3']);
    FX.note(tl, t + 0.6, 2.6, 130, 470, '圆心 · 指针尖<br>零刻度 · 满刻度', { style: 'sub', enter: 'left', w: 250, accent: COL.amber });
    // ③ 几何
    t = a + 7.8;
    FX.enter(tl, r.rz, t, 'draw', { dur: 0.6 });
    FX.enter(tl, r.rf, t + 0.3, 'draw', { dur: 0.6 });
    FX.enter(tl, r.arc, t + 0.9, 'draw', { dur: 0.9, ease: 'power2.out' });
    revN('n4', t + 0.5, 2); DK.pulse(tl, t + 1.1, vf.shapeEls['n4']);
    FX.note(tl, t + 1.4, 2.8, 130, 600, '<span class="fxn-num" style="--ac:#485c11">0.40<span class="u"> MPa</span></span><br><span class="fxn-mono">表1 · 40% · 正常</span>', { style: 'card', enter: 'pop', w: 230 });
    revN('n5', a + 9.7, 3); DK.pulse(tl, a + 10.2, vf.shapeEls['n5']);
    // 收束
    t = a + 11.4;
    FX.note(tl, t, 1.6, 960, 920, '深度学习只做"框"与"点"，读数交给纯几何 · 端到端 ~40ms · ~15FPS', { style: 'mono', anchor: 'c', enter: 'rise', accent: COL.g });
  }

  /* ════════════════════ 设备互信之后 → 软总线共享黑板（车入会 + 改数据自动广播）════════════════════ */
  F.addCSS(
    '.shz-node{position:absolute;background:var(--surface);border:1.5px solid var(--divider);border-radius:16px;box-shadow:var(--card-shadow);padding:15px 22px;text-align:center;opacity:0;}' +
    '.shz-node .nn{font-size:25px;font-weight:800;color:var(--text-title);}' +
    '.shz-node .nb{font-size:15px;color:var(--text-secondary);font-family:var(--font-mono);margin-top:5px;}' +
    '.shz-node.flash{border-color:var(--primary);box-shadow:0 0 0 5px var(--primary-soft),var(--card-shadow);}' +
    '.shz-bb{position:absolute;background:#fffdf4;border:2px solid var(--primary);border-radius:20px;box-shadow:0 26px 64px -22px rgba(40,46,14,.42);padding:20px 28px;opacity:0;}' +
    '.shz-bb .bt{font-size:24px;font-weight:800;color:var(--primary);display:flex;align-items:center;gap:11px;}' +
    '.shz-bb .bt .dot{width:11px;height:11px;border-radius:50%;background:var(--success);}' +
    '.shz-row{display:flex;justify-content:space-between;gap:20px;font-size:20px;margin-top:13px;padding:9px 14px;border-radius:10px;}' +
    '.shz-row .k{color:var(--text-secondary);font-family:var(--font-mono);}' +
    '.shz-row .v{color:var(--text-body);font-weight:700;}' +
    '.shz-row.hot{background:var(--primary-soft);}.shz-row.hot .v{color:var(--primary-pressed);}'
  );
  function setTextP(tl, at, el, txt, prev) { tl.to(el, { duration: 0.01, onComplete: function () { el.textContent = txt; }, onReverseComplete: function () { el.textContent = prev; } }, at); }
  function buildShare(root) {
    root.classList.add('paper');
    const wrap = document.createElement('div'); wrap.className = 'cam'; root.appendChild(wrap);
    const svg = '<svg viewBox="0 0 1920 1080" style="position:absolute;inset:0;width:100%;height:100%">' +
      '<path class="shz-l1" d="M960 350 L960 384" fill="none" stroke="#aeb977" stroke-width="2.5" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>' +
      '<path class="shz-l2" d="M462 740 L712 588" fill="none" stroke="#aeb977" stroke-width="2.5" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>' +
      '<path class="shz-l3" d="M1458 740 L1208 588" fill="none" stroke="#aeb977" stroke-width="2.5" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>' +
      (function () { let s = ''; for (let k = 0; k < 6; k++) s += '<circle class="shz-pk" cx="960" cy="520" r="9" fill="' + COL.g + '" opacity="0"/>'; return s; })() +
      '</svg>';
    wrap.innerHTML = svg;
    function node(x, y, w, nn, nb) { const d = document.createElement('div'); d.className = 'shz-node'; d.style.cssText = 'left:' + x + 'px;top:' + y + 'px;width:' + w + 'px;'; d.innerHTML = '<div class="nn">' + nn + '</div><div class="nb">' + nb + '</div>'; wrap.appendChild(d); return d; }
    const tab = node(760, 236, 400, '平板 · 总控', 'com.example.carapp');
    const c1 = node(180, 730, 300, '车1 agent', 'com.example.carapp');
    const c2 = node(1440, 730, 300, '车2 agent', 'com.example.carapp');
    const bb = document.createElement('div'); bb.className = 'shz-bb'; bb.style.cssText = 'left:660px;top:384px;width:600px;';
    bb.innerHTML = '<div class="bt"><span class="dot"></span>共享黑板 FleetMission · DDO</div>' +
      '<div class="shz-row"><span class="k">phase</span><span class="v">covering</span></div>' +
      '<div class="shz-row" data-r-assign><span class="k">assignments</span><span class="v" data-v-assign>车1→区域A · 车2→ —</span></div>' +
      '<div class="shz-row" data-r-robots><span class="k">robots</span><span class="v" data-v-robots>车1 38% · 车2 0%</span></div>';
    wrap.appendChild(bb);
    const q = function (s) { return wrap.querySelector(s); }, qa = function (s) { return Array.prototype.slice.call(wrap.querySelectorAll(s)); };
    return { wrap: wrap, tab: tab, c1: c1, c2: c2, bb: bb, l1: q('.shz-l1'), l2: q('.shz-l2'), l3: q('.shz-l3'), pk: qa('.shz-pk'), rAssign: q('[data-r-assign]'), rRobots: q('[data-r-robots]'), vAssign: q('[data-v-assign]'), vRobots: q('[data-v-robots]') };
  }
  function animShare(tl, s) {
    const a = s.start, r = s.refs; FX.scene(s.root);
    function pulse(el, x1, y1, st) { tl.set(el, { attr: { cx: 960, cy: 520 }, opacity: 0 }, st); tl.to(el, { opacity: 1, duration: 0.12 }, st); tl.to(el, { attr: { cx: x1, cy: y1 }, duration: 0.6, ease: 'power1.in' }, st); tl.to(el, { opacity: 0, duration: 0.16 }, st + 0.5); }
    function flash(el, st) { tl.to(el, { duration: 0.01, onComplete: function () { el.classList.add('flash'); }, onReverseComplete: function () { el.classList.remove('flash'); } }, st); tl.to(el, { duration: 0.01, onComplete: function () { el.classList.remove('flash'); }, onReverseComplete: function () { el.classList.add('flash'); } }, st + 0.7); }
    function hot(el, st, dur) { tl.to(el, { duration: 0.01, onComplete: function () { el.classList.add('hot'); }, onReverseComplete: function () { el.classList.remove('hot'); } }, st); tl.to(el, { duration: 0.01, onComplete: function () { el.classList.remove('hot'); }, onReverseComplete: function () { el.classList.add('hot'); } }, st + dur); }
    FX.note(tl, a + 0.3, 0, 200, 140, '软总线 · 共享黑板 FleetMission', { style: 'kicker', enter: 'left' });
    FX.note(tl, a + 0.6, 0, 200, 182, '互信之后：<b>入会 · 共享 · 各自响应</b>', { style: 'title', enter: 'rise', w: 820 });
    FX.enter(tl, r.bb, a + 1.0, 'zoomBlur', { dur: 0.8, origin: '50% 50%' });
    FX.enter(tl, r.tab, a + 1.4, 'fall', { dur: 0.6 }); FX.enter(tl, r.l1, a + 1.6, 'draw', { dur: 0.6 });
    FX.enter(tl, r.c1, a + 1.7, 'spring', { dur: 0.5 }); FX.enter(tl, r.l2, a + 1.9, 'draw', { dur: 0.6 });
    // ② 车2 入会
    let t = a + 2.9;
    FX.enter(tl, r.c2, t, 'right', { dur: 0.7, dist: 80 }); FX.enter(tl, r.l3, t + 0.5, 'draw', { dur: 0.6 });
    FX.note(tl, t + 0.3, 2.4, 1590, 880, '车2 <b>joinSession</b><br>入同一 DDO 会话', { style: 'sub', anchor: 'c', enter: 'fall', accent: COL.g, w: 320 });
    // ③ 平板改黑板 → 广播
    t = a + 5.2;
    FX.note(tl, t, 2.6, 960, 930, '③ 平板写 <b>assignments</b> → 自动广播 onChange', { style: 'h', anchor: 'c', enter: 'rise', accent: COL.g, w: 900 });
    hot(r.rAssign, t + 0.4, 2.2); setTextP(tl, t + 0.6, r.vAssign, '车1→区域A · 车2→区域B', '车1→区域A · 车2→ —');
    pulse(r.pk[0], 330, 785, t + 0.8); pulse(r.pk[1], 1590, 785, t + 0.8);
    flash(r.c1, t + 1.3); flash(r.c2, t + 1.3);
    // ④ 车2 回写进度 → 广播
    t = a + 7.4;
    FX.note(tl, t, 2.4, 960, 930, '④ 车2 回写 <b>robots 进度</b> → 平板/车1 onChange', { style: 'h', anchor: 'c', enter: 'rise', accent: COL.blue, w: 900 });
    hot(r.rRobots, t + 0.4, 2.2); setTextP(tl, t + 0.6, r.vRobots, '车1 38% · 车2 42%', '车1 38% · 车2 0%');
    pulse(r.pk[2], 960, 205, t + 0.8); pulse(r.pk[3], 330, 785, t + 0.8);
    flash(r.tab, t + 1.3); flash(r.c1, t + 1.3);
    // 收束
    t = a + 9.7;
    FX.note(tl, t, 1.7, 960, 985, '谁改 → 自动广播 → 各端 onChange · 共享态最终一致（取代脆弱的"远程拉起"）', { style: 'mono', anchor: 'c', enter: 'rise', accent: COL.g });
  }

  F.addScene({ id: 'pr-discover', dur: 14.5, build: buildDiscover, anim: animDiscover });
  F.addScene({ id: 'pr-trust', dur: 10.5, build: buildTrustPr, anim: animTrustPr });
  F.addScene({ id: 'pr-share', dur: 13, build: buildShare, anim: animShare });
  F.addScene({ id: 'pr-map', dur: 11.5, build: buildMapPr, anim: animMapPr });
  F.addScene({ id: 'pr-slam', dur: 13.5, build: buildSlam, anim: animSlam });   // 已移出 setOrder（并入 03-build）
  F.addScene({ id: 'pr-vision', dur: 14, build: buildVisionPr, anim: animVisionPr });
})();
