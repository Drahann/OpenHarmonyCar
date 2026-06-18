/* 场景库 act 3–4：建图(SLAM) + 单机导航(A*)。自包含。
   本轮：地图改 MAPKIT 有机等高线(seed 7，与 ui-control/algo-trio 同源——诉求"以后章地图为准")；
   建图"墙生长" = 等高线 path 逐条 dashoffset 描绘 + 软填充淡入；机器人沿已知自由区路径探索。
   ⚠ 本挂载 file-tool Edit 会截断——改本文件只用 Write/bash 整文件覆盖。 */
(function () {
  "use strict";
  const F = window.FILM, MK = window.MAPKIT;
  const centerOf = F.centerOf, camTo = F.camTo, camReset = F.camReset,
    cursorShow = F.cursorShow, cursorHide = F.cursorHide, cursorTapAt = F.cursorTapAt,
    tapEl = F.tapEl, caption = F.caption, addCSS = F.addCSS, I = F.I;

  /* 楼面图区域(与 scenes7 ui-control 完全一致) + 已知落自由区的路径 */
  const FLOOR = { ox: 240, oy: 110, FW: 800, FH: 525 };
  const NAVPATH = [[464, 353], [586, 353], [586, 479], [852, 479]], PIN = [852, 479];
  const EXPLORE = [[464, 353], [586, 353], [586, 479], [852, 479], [586, 479], [586, 353], [464, 353]];
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
    '.bc-title{font-size:25px;font-weight:700;color:var(--text-body);margin-bottom:6px;}' +
    '.bc-desc{font-size:18px;color:var(--text-secondary);margin-bottom:16px;line-height:1.4;}' +
    '.bc-btn{height:54px;border-radius:999px;background:var(--primary);color:#fff;font-size:20px;font-weight:600;display:flex;align-items:center;justify-content:center;}' +
    '.bc-building{display:flex;align-items:center;gap:10px;font-size:19px;color:var(--text-body);margin-bottom:16px;}' +
    '.bc-building .minispin{width:22px;height:22px;border-radius:50%;border:3px solid var(--surface-muted);border-top-color:var(--primary);animation:spin 1s linear infinite;flex:none;}' +
    '.stack{position:relative;min-height:180px;}.stack>div{position:absolute;left:0;right:0;top:0;}' +
    '.joystick{position:absolute;left:70px;bottom:150px;width:156px;height:156px;border-radius:50%;background:rgba(255,255,255,.82);box-shadow:var(--soft-shadow);z-index:6;}' +
    '.joystick::after{content:"";position:absolute;inset:18px;border-radius:50%;border:1.5px dashed var(--border);}' +
    '.joy-knob{position:absolute;left:50%;top:50%;width:66px;height:66px;margin:-33px 0 0 -33px;border-radius:50%;background:var(--primary);box-shadow:0 4px 12px rgba(0,0,0,.28);}' +
    '.speed{position:absolute;left:58px;bottom:330px;width:200px;background:var(--surface);border-radius:16px;box-shadow:var(--soft-shadow);padding:14px 16px;z-index:6;}' +
    '.speed .sl{font-size:16px;color:var(--text-secondary);margin-bottom:9px;}' +
    '.speed-track{height:8px;border-radius:4px;background:var(--divider);position:relative;}' +
    '.speed-fill{position:absolute;left:0;top:0;bottom:0;width:20%;border-radius:4px;background:var(--primary);}' +
    '.speed-knob{position:absolute;top:50%;left:20%;width:22px;height:22px;margin:-11px 0 0 -11px;border-radius:50%;background:var(--primary);box-shadow:0 2px 6px rgba(0,0,0,.3);}' +
    '.map-hint{position:absolute;left:50%;top:40%;transform:translate(-50%,-50%);font-size:24px;color:var(--text-secondary);font-weight:600;opacity:0;z-index:4;display:flex;align-items:center;gap:10px;}' +
    '.map-hint .minispin{width:22px;height:22px;border-radius:50%;border:3px solid var(--surface-muted);border-top-color:var(--primary);animation:spin 1s linear infinite;}' +
    '.load-ov{position:absolute;inset:0;background:rgba(0,0,0,.42);display:flex;align-items:center;justify-content:center;z-index:8;opacity:0;}' +
    '.load-card{background:var(--surface);border-radius:18px;padding:30px 40px;display:flex;flex-direction:column;align-items:center;gap:14px;}' +
    '.load-card .bigspin{width:52px;height:52px;border-radius:50%;border:5px solid var(--surface-muted);border-top-color:var(--primary);animation:spin 1s linear infinite;}' +
    '.load-card .lt{font-size:24px;font-weight:700;color:var(--text-body);}.load-card .ls{font-size:17px;color:var(--text-secondary);}' +
    '.toast{position:absolute;left:50%;bottom:150px;transform:translateX(-50%);background:rgba(29,32,22,.92);color:#fff;font-size:18px;padding:12px 22px;border-radius:999px;opacity:0;z-index:9;white-space:nowrap;}'
  );

  function setText(tl, at, el, txt, prev) { tl.to(el, { duration: 0.01, onComplete: function () { el.textContent = txt; }, onReverseComplete: function () { el.textContent = prev; } }, at); }

  /* 控制页（建图/导航通用）。ready: true=已有图(导航) / false=空图(建图)。地图=MAPKIT 有机等高线。 */
  function buildControl(root, opts) {
    opts = opts || {}; const ip = opts.ip || '192.168.43.12', ready = !!opts.mapReady;
    root.classList.add('paper');
    const cam = document.createElement('div'); cam.className = 'cam'; root.appendChild(cam);
    const T = document.createElement('div'); T.className = 'tablet';
    const TW = 1300, TH = 812, TX = (1920 - TW) / 2, TY = 128;
    T.style.cssText += 'left:' + TX + 'px;top:' + TY + 'px;width:' + TW + 'px;height:' + TH + 'px;';
    T.innerHTML = '<div class="cam-dot"></div>';
    const screen = document.createElement('div'); screen.className = 'screen';
    const am = document.createElement('div'); am.className = 'app-map';
    const fp = floorParts(7), G = MK.TH.green, wd = ready ? '' : ' stroke-dasharray="1" stroke-dashoffset="1"';
    const svg =
      '<svg viewBox="0 0 1270 782" preserveAspectRatio="xMidYMid slice"><defs><linearGradient id="mbg2" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#e9e7dc"/><stop offset="1" stop-color="#e1ded0"/></linearGradient></defs>' +
      '<rect x="0" y="0" width="1270" height="782" fill="url(#mbg2)"/>' +
      '<g class="mapContent"><g class="floorG" style="opacity:' + (ready ? 1 : 0) + '">' + fp.floor +
      '<g class="mk-fills" fill="' + MK.TH.wallFill + '" style="opacity:' + (ready ? 1 : 0) + '">' + fp.fills + '</g>' +
      '<g fill="none" stroke="' + MK.TH.wallStroke + '" stroke-width="2.6" stroke-linejoin="round" stroke-linecap="round">' +
      fp.contours.map(function (d) { return '<path class="wp" d="' + d + '" pathLength="1"' + wd + '/>'; }).join('') + '</g></g>' +
      '<path class="trail" d="' + MK.polyPath(NAVPATH) + '" fill="none" stroke="' + G + '" stroke-width="4" stroke-linecap="round" stroke-linejoin="round" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>' +
      '<g class="pinG" opacity="0" transform="translate(' + PIN[0] + ',' + PIN[1] + ')"><path d="M0 0 Q-9 -14 -9 -23 A9 9 0 0 1 9 -23 Q9 -14 0 0 Z" fill="' + MK.TH.target + '"/><circle cy="-23" r="3.6" fill="#fff"/></g>' +
      MK.robotSVG('robot', G, '1', 16) + '</g></svg>';
    const opInit = ready ? 1 : 0, nomapInit = ready ? 0 : 1;
    am.innerHTML = svg +
      '<div class="map-hint" data-hint><span class="minispin"></span>建图中…</div>' +
      '<div class="ctl-top"><div class="ctl-back">' + I.back + '</div><div class="ctl-pill"><span class="dot"></span><span class="ip">' + ip + '</span><span class="md">单机导航</span><span class="caret">▼</span></div><div class="ctl-rebuild">重新建图</div><div class="ctl-icon ctl-video">' + I.video + '</div></div>' +
      '<div class="ctl-fabs"><div class="fab">＋</div><div class="fab">−</div><div class="fab">⊙</div></div>' +
      '<div class="ctl-pip"><div class="pip-cap">' + I.video + '<span>仪表视频</span></div></div>' +
      '<div class="speed" data-speed style="opacity:0"><div class="sl">速度 <span data-spv>20</span>%</div><div class="speed-track"><div class="speed-fill" data-spfill></div><div class="speed-knob" data-spknob></div></div></div>' +
      '<div class="joystick" data-joy style="opacity:0"><div class="joy-knob" data-knob></div></div>' +
      '<div class="joy-toggle" data-joytoggle style="opacity:0">收起摇杆</div>' +
      '<div class="toast" data-toast></div>' +
      '<div class="sheet" data-sheet><div class="handle"></div><div class="stack">' +
      '<div data-nomap style="opacity:' + nomapInit + '"><div class="bc-title">尚无地图</div><div class="bc-desc">还没有地图，需先建图。点「开始建图」，再用摇杆驱动小车走一圈环境。</div><div class="bc-btn" data-startbuild>开始建图</div></div>' +
      '<div data-building style="opacity:0"><div class="bc-building"><span class="minispin"></span>建图中… 用摇杆驱动小车探索，走完点「结束建图」</div><div class="bc-btn" data-finishbuild>结束建图</div></div>' +
      '<div data-op style="opacity:' + opInit + '"><div class="op-hint">① 点「选目标点」再点地图选点 → ② 点「开始导航」</div><div class="op-pick" data-pick>选目标点</div><div class="op-row"><div class="op-target"><div class="lbl">目标点</div><div class="val" data-tgt>点地图选点</div></div><div class="op-btn ghost" data-cancel>取消</div><div class="op-btn primary" data-go>开始导航</div></div></div>' +
      '</div></div>' +
      '<div class="load-ov" data-load><div class="load-card"><div class="bigspin"></div><div class="lt">正在载入地图…</div><div class="ls">结束建图后正在拉取存好的地图</div></div></div>';
    screen.appendChild(am); T.appendChild(screen); cam.appendChild(T);
    const q = function (s) { return am.querySelector(s); };
    const qa = function (s) { return Array.prototype.slice.call(am.querySelectorAll(s)); };
    return {
      cam: cam, T: T, am: am, mapContent: q('.mapContent'), floorG: q('.floorG'), fills: q('.mk-fills'), walls: qa('.wp'),
      robot: q('.robot'), pin: q('.pinG'), trail: q('.trail'), hint: q('[data-hint]'), sheet: q('[data-sheet]'),
      nomap: q('[data-nomap]'), building: q('[data-building]'), op: q('[data-op]'),
      startBuild: q('[data-startbuild]'), finishBuild: q('[data-finishbuild]'),
      pick: q('[data-pick]'), tgt: q('[data-tgt]'), go: q('[data-go]'), cancel: q('[data-cancel]'),
      load: q('[data-load]'), joy: q('[data-joy]'), knob: q('[data-knob]'), joyToggle: q('[data-joytoggle]'),
      speed: q('[data-speed]'), spv: q('[data-spv]'), spfill: q('[data-spfill]'), spknob: q('[data-spknob]'), toast: q('[data-toast]')
    };
  }
  function svgPt(refs, sx, sy) { const c = centerOf(refs.am); return { x: c.x - 635 + sx, y: c.y - 391 + sy }; }

  /* ===================== 03 建图（省略控车过程：开始建图 → 建图中转圈 → 结束建图直接出图；
     建图细节见上一章 pr-slam，本章只演 App 触发与取图，省时且不与前章重复） ===================== */
  function animMapping(tl, s) {
    const a = s.start, r = s.refs;
    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' }, { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    caption(tl, a + 0.5, 2.6, '<b>先建一张图</b>', '点开始建图 → 建图中 → 结束建图直接出图');
    tl.set(r.robot, { opacity: 0 }, a);
    cursorShow(tl, a + 1.4, svgPt(r, 980, 700).x, svgPt(r, 980, 700).y);
    // ① 点开始建图（轻放大）→ 收起控制栏 → 屏幕"建图中…"转圈（不演控车，过程见上一章）
    let t = a + 2.2;
    camTo(tl, r.cam, t, centerOf(r.startBuild).x, centerOf(r.startBuild).y, 1.32, 0.9);
    tapEl(tl, t + 0.8, r.startBuild, 0.6);
    tl.to(r.nomap, { opacity: 0, duration: 0.3 }, t + 1.4);
    tl.to(r.building, { opacity: 1, duration: 0.35 }, t + 1.6);
    camReset(tl, r.cam, t + 2.2, 0.9);
    tl.to(r.sheet, { y: 300, duration: 0.6, ease: 'power2.inOut' }, t + 2.6);
    tl.to(r.hint, { opacity: 1, duration: 0.5 }, t + 2.8);
    caption(tl, t + 2.8, 3.0, '<b>建图中…</b>', '收起控制栏，SLAM 自主建图（建图细节见上一章）');
    // ② 点结束建图 → 控制栏滑回 → 载入遮罩 → 地图一次铺满
    t = a + 8.0;
    tl.to(r.sheet, { y: 0, duration: 0.5, ease: 'power2.out' }, t);
    tl.to(r.hint, { opacity: 0, duration: 0.3 }, t + 0.3);
    tapEl(tl, t + 1.0, r.finishBuild, 0.6);
    tl.to(r.load, { opacity: 1, duration: 0.4 }, t + 1.7);
    caption(tl, t + 1.2, 3.0, '<b>结束建图 · 存图落盘</b>', '自动拉压缩图 ZMAP1（1bit/格，~6× 更小）');
    tl.to(r.load, { opacity: 0, duration: 0.5 }, t + 3.4);
    // 出图：楼面 + 等高线 + 软填充随遮罩散去一次铺满（"直接出图"，不演逐段生长）
    r.walls.forEach(function (w) { tl.set(w, { attr: { 'stroke-dashoffset': 0 } }, t + 3.4); });
    tl.to(r.floorG, { opacity: 1, duration: 0.6, ease: 'power2.out' }, t + 3.4);
    tl.to(r.fills, { opacity: 1, duration: 0.6 }, t + 3.4);
    tl.set(r.robot, { x: NAVPATH[0][0], y: NAVPATH[0][1], rotation: 8, transformOrigin: '0px 0px' }, t + 3.9);
    tl.fromTo(r.robot, { opacity: 0 }, { opacity: 1, duration: 0.4 }, t + 3.9);
    tl.to(r.building, { opacity: 0, duration: 0.3 }, t + 3.8);
    tl.to(r.op, { opacity: 1, duration: 0.4 }, t + 4.0);
    caption(tl, t + 3.9, 2.4, '<b>地图就绪</b>', '压缩图载入完成，可选点导航 / 覆盖 / 多机');
    cursorHide(tl, t + 4.5);
  }

  /* ===================== 04 单机导航（代码保留，未列入 setOrder） ===================== */
  function animNav(tl, s) {
    const a = s.start, r = s.refs;
    tl.set(r.robot, { x: NAVPATH[0][0], y: NAVPATH[0][1], rotation: 8, transformOrigin: '0px 0px' }, a);
    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' }, { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    caption(tl, a + 0.5, 2.6, '<b>点一下，它自己去</b>', 'A* 单机导航');
    cursorShow(tl, a + 1.5, centerOf(r.pick).x, centerOf(r.pick).y + 70);
    let t = a + 2.4;
    tapEl(tl, t, r.pick, 0.6);
    tl.to(r.pick, { backgroundColor: '#485c11', color: '#ffffff', duration: 0.3 }, t + 0.6);
    setText(tl, t + 0.6, r.pick, '选点中…（点此取消）', '选目标点');
    caption(tl, t + 0.3, 2.6, '点「选目标点」武装', '再点地图任意处选点（避免误触）');
    t = a + 5.4;
    const p = svgPt(r, PIN[0], PIN[1]);
    cursorTapAt(tl, t, p.x, p.y, 0.8);
    tl.fromTo(r.pin, { opacity: 0, scale: 0.3 }, { opacity: 1, scale: 1, duration: 0.5, ease: 'back.out(1.8)' }, t + 0.85);
    setText(tl, t + 0.9, r.tgt, '(412, 286)', '点地图选点');
    tl.to(r.pick, { backgroundColor: '#ffffff', color: '#485c11', duration: 0.3 }, t + 0.9);
    setText(tl, t + 0.9, r.pick, '选目标点', '选点中…（点此取消）');
    caption(tl, t + 0.6, 2.6, '<b>栅格选点 · A* 规划</b>', '红色泪滴 pin 落点，坐标等宽显示');
    t = a + 9.0;
    tapEl(tl, t, r.go, 0.6);
    setText(tl, t + 0.6, r.go, '重新导航', '开始导航');
    tl.to(r.sheet, { y: 300, duration: 0.6, ease: 'power2.inOut' }, t + 0.8);
    caption(tl, t + 0.7, 3.6, '<b>自主导航中 · 收起底栏</b>', '沿规划路径行驶，接近终点自动判定到达');
    MK.driveRobot({ tl: tl, robot: r.robot, trail: r.trail, pts: NAVPATH, t0: t + 1.2, segDur: 1.1, turnDur: 0.3 });
    tl.fromTo(r.toast, { opacity: 0, y: 10 }, { opacity: 1, y: 0, duration: 0.4 }, t + 5.2);
    setText(tl, t + 5.2, r.toast, '到达终点', '');
    tl.to(r.toast, { opacity: 0, duration: 0.4 }, t + 7.0);
    caption(tl, t + 5.2, 2.4, '<b>到达终点</b>', '「取消」停止 · 再点地图即重选');
    cursorHide(tl, t + 7.2);
  }

  F.addScene({ id: '03-build', dur: 13, build: function (root) { return buildControl(root, { mapReady: false }); }, anim: animMapping });
  F.addScene({ id: '04-nav', dur: 17, build: function (root) { return buildControl(root, { mapReady: true }); }, anim: animNav });
})();
