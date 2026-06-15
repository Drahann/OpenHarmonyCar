/* 场景库 act 3–4：建图(SLAM) + 单机导航(A*)。自包含。
   本轮精修：id→class(去重复，自检可信) + 唯一渐变 id；减放大；执行时收起底栏露地图；精简。
   勿用 file-tool Edit/Write（会截断）——只用 bash heredoc 覆盖。 */
(function () {
  "use strict";
  const F = window.FILM;
  const centerOf = F.centerOf, camTo = F.camTo, camReset = F.camReset,
    cursorShow = F.cursorShow, cursorHide = F.cursorHide, cursorTapAt = F.cursorTapAt,
    tapEl = F.tapEl, caption = F.caption, addCSS = F.addCSS, I = F.I;

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
    '.map-hint{position:absolute;left:50%;top:44%;transform:translate(-50%,-50%);font-size:24px;color:var(--text-secondary);font-weight:600;opacity:0;z-index:4;display:flex;align-items:center;gap:10px;}' +
    '.map-hint .minispin{width:22px;height:22px;border-radius:50%;border:3px solid var(--surface-muted);border-top-color:var(--primary);animation:spin 1s linear infinite;}' +
    '.load-ov{position:absolute;inset:0;background:rgba(0,0,0,.42);display:flex;align-items:center;justify-content:center;z-index:8;opacity:0;}' +
    '.load-card{background:var(--surface);border-radius:18px;padding:30px 40px;display:flex;flex-direction:column;align-items:center;gap:14px;}' +
    '.load-card .bigspin{width:52px;height:52px;border-radius:50%;border:5px solid var(--surface-muted);border-top-color:var(--primary);animation:spin 1s linear infinite;}' +
    '.load-card .lt{font-size:24px;font-weight:700;color:var(--text-body);}.load-card .ls{font-size:17px;color:var(--text-secondary);}' +
    '.toast{position:absolute;left:50%;bottom:150px;transform:translateX(-50%);background:rgba(29,32,22,.92);color:#fff;font-size:18px;padding:12px 22px;border-radius:999px;opacity:0;z-index:9;white-space:nowrap;}'
  );

  function setText(tl, at, el, txt, prev) { tl.to(el, { duration: 0.01, onComplete: function () { el.textContent = txt; }, onReverseComplete: function () { el.textContent = prev; } }, at); }

  /* 控制页（建图/导航通用）。ready: true=已有图(导航) / false=空图(建图)。id 全改 class（去重复，自检可信）。 */
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
    const fx = 285, fy = 41, fs = 700, pitch = fs / 22; let grid = '';
    for (let i = 0; i <= 22; i++) { const g = fx + i * pitch; grid += '<line x1="' + g + '" y1="' + fy + '" x2="' + g + '" y2="' + (fy + fs) + '"/>'; }
    for (let i = 0; i <= 22; i++) { const g = fy + i * pitch; grid += '<line x1="' + fx + '" y1="' + g + '" x2="' + (fx + fs) + '" y2="' + g + '"/>'; }
    const wd = ready ? '' : ' stroke-dasharray="1" stroke-dashoffset="1"';
    const svg =
      '<svg viewBox="0 0 1270 782" preserveAspectRatio="xMidYMid slice"><defs><linearGradient id="mbg2" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#edebe0"/><stop offset="1" stop-color="#e2dfd0"/></linearGradient></defs>' +
      '<rect x="0" y="0" width="1270" height="782" fill="url(#mbg2)"/>' +
      '<g class="mapContent"><g class="floorG" style="opacity:' + (ready ? 1 : 0) + '">' +
      '<rect x="' + fx + '" y="' + fy + '" width="' + fs + '" height="' + fs + '" fill="#ffffff"/>' +
      '<g stroke="rgba(72,92,17,.06)" stroke-width="1">' + grid + '</g>' +
      '<rect x="' + fx + '" y="' + fy + '" width="' + fs + '" height="' + fs + '" fill="none" stroke="#e3e7d2" stroke-width="2"/>' +
      '<rect x="395" y="540" width="150" height="92" rx="6" fill="rgba(72,92,17,.14)" stroke="#485c11" stroke-width="4" pathLength="1"' + wd + ' class="wp"/></g>' +
      '<g fill="none" stroke="#485c11" stroke-width="5" stroke-linejoin="round" stroke-linecap="round">' +
      '<path class="wp" pathLength="1"' + wd + ' d="M345 120 L345 662 L545 662 M700 662 L925 662 L925 120 L345 120"/>' +
      '<path class="wp" pathLength="1"' + wd + ' d="M700 120 L700 360 M700 360 L925 360"/>' +
      '<path class="wp" pathLength="1"' + wd + ' d="M345 470 L520 470"/></g>' +
      '<path class="trail" d="M470 300 L560 300 L560 560 L820 560" fill="none" stroke="#357a41" stroke-width="4" stroke-linecap="round" stroke-linejoin="round" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>' +
      '<g class="pinG" opacity="0"><path d="M0 0 Q-9 -14 -9 -23 A9 9 0 0 1 9 -23 Q9 -14 0 0 Z" fill="#d9503f"/><circle cx="0" cy="-23" r="3.6" fill="#fff"/></g>' +
      '<g class="robot" opacity="' + (ready ? 1 : 0) + '"><polygon points="0,-30 11,-12 -11,-12" fill="#357a41"/><circle r="16" fill="#357a41"/><circle r="16" fill="none" stroke="#fff" stroke-width="3"/><text y="1" fill="#fff" font-size="17" font-weight="700" text-anchor="middle" dominant-baseline="middle">1</text></g></g></svg>';
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
      cam: cam, T: T, am: am, mapContent: q('.mapContent'), floorG: q('.floorG'), walls: qa('.wp'),
      robot: q('.robot'), pin: q('.pinG'), trail: q('.trail'), hint: q('[data-hint]'), sheet: q('[data-sheet]'),
      nomap: q('[data-nomap]'), building: q('[data-building]'), op: q('[data-op]'),
      startBuild: q('[data-startbuild]'), finishBuild: q('[data-finishbuild]'),
      pick: q('[data-pick]'), tgt: q('[data-tgt]'), go: q('[data-go]'), cancel: q('[data-cancel]'),
      load: q('[data-load]'), joy: q('[data-joy]'), knob: q('[data-knob]'), joyToggle: q('[data-joytoggle]'),
      speed: q('[data-speed]'), spv: q('[data-spv]'), spfill: q('[data-spfill]'), spknob: q('[data-spknob]'), toast: q('[data-toast]')
    };
  }
  function svgPt(refs, sx, sy) { const c = centerOf(refs.am); return { x: c.x - 635 + sx, y: c.y - 391 + sy }; }

  /* ===================== 03 建图（1 次轻放大；执行时收起底栏） ===================== */
  function animMapping(tl, s) {
    const a = s.start, r = s.refs;
    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' }, { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    caption(tl, a + 0.5, 2.6, '<b>先建一张图</b>', '开始建图 → 摇杆探索一圈 → 结束建图');
    cursorShow(tl, a + 1.4, svgPt(r, 760, 760).x, svgPt(r, 760, 760).y);
    // ① 开始建图（唯一一次轻放大）
    let t = a + 2.4;
    camTo(tl, r.cam, t, centerOf(r.startBuild).x, centerOf(r.startBuild).y, 1.32, 0.9);
    tapEl(tl, t + 0.8, r.startBuild, 0.6);
    tl.to(r.nomap, { opacity: 0, duration: 0.3 }, t + 1.4);
    tl.to(r.building, { opacity: 1, duration: 0.35 }, t + 1.6);
    tl.to([r.hint, r.floorG], { opacity: 1, duration: 0.5 }, t + 1.6);
    tl.fromTo(r.robot, { opacity: 0 }, { opacity: 1, duration: 0.4 }, t + 1.8); tl.set(r.robot, { x: 470, y: 300 }, t + 1.8);
    tl.to([r.joy, r.speed, r.joyToggle], { opacity: 1, duration: 0.4 }, t + 1.9);
    camReset(tl, r.cam, t + 2.6, 0.9);
    // 收起底栏：底部卡滑下，地图全可见
    tl.to(r.sheet, { y: 300, duration: 0.6, ease: 'power2.inOut' }, t + 3.2);
    caption(tl, t + 3.2, 2.4, '<b>收起底栏 · 地图全可见</b>', '实拍时底部卡收起，不挡地图');
    // ② 探索：速度滑块 + 摇杆 + 机器人 + 墙体生长（无放大）
    t = a + 8.2;
    caption(tl, t + 0.2, 4.4, '<b>摇杆驱动 · 实时建图</b>', '上=前进 · 左/右=转向 · 松手即停');
    tl.to(r.spfill, { width: '55%', duration: 1.0, ease: 'power2.inOut' }, t + 0.4);
    tl.to(r.spknob, { left: '55%', duration: 1.0, ease: 'power2.inOut' }, t + 0.4);
    tl.to(r.spv, { duration: 1.0, snap: { innerText: 1 }, innerText: 55, ease: 'power2.inOut' }, t + 0.4);
    tl.to(r.knob, { y: -34, duration: 0.5, ease: 'power2.out' }, t + 0.5);
    tl.to(r.knob, { y: 0, duration: 0.4, ease: 'power3.out' }, t + 3.0);
    tl.to(r.knob, { x: 30, duration: 0.5, ease: 'power2.out' }, t + 3.4);
    tl.to(r.knob, { x: 0, duration: 0.4, ease: 'power3.out' }, t + 5.0);
    tl.to(r.robot, { x: 600, y: 200, duration: 1.6, ease: 'power1.inOut' }, t + 0.6);
    tl.to(r.robot, { x: 820, y: 240, duration: 1.4, ease: 'power1.inOut' }, t + 2.2);
    tl.to(r.robot, { x: 820, y: 520, duration: 1.6, ease: 'power1.inOut' }, t + 3.6);
    tl.to(r.robot, { x: 470, y: 470, duration: 1.8, ease: 'power1.inOut' }, t + 5.2);
    tl.to(r.robot, { x: 470, y: 300, duration: 1.2, ease: 'power1.inOut' }, t + 7.0);
    r.walls.forEach(function (w, i) { tl.to(w, { attr: { 'stroke-dashoffset': 0 }, duration: 1.6, ease: 'power1.out' }, t + 1.0 + i * 1.4); });
    // ③ 结束建图：底栏滑回 → 点结束（无放大）→ 载入 → 成图
    t = a + 16.5;
    tl.to([r.joy, r.speed, r.joyToggle, r.hint], { opacity: 0, duration: 0.3 }, t);
    tl.to(r.sheet, { y: 0, duration: 0.5, ease: 'power2.out' }, t + 0.2);
    tapEl(tl, t + 1.0, r.finishBuild, 0.6);
    tl.to(r.load, { opacity: 1, duration: 0.4 }, t + 1.7);
    caption(tl, t + 1.2, 3.0, '<b>结束建图 · 存图落盘</b>', '自动拉压缩图 ZMAP1（1bit/格，~6× 更小）');
    tl.to(r.load, { opacity: 0, duration: 0.5 }, t + 3.3);
    tl.to(r.building, { opacity: 0, duration: 0.3 }, t + 3.5);
    tl.to(r.op, { opacity: 1, duration: 0.4 }, t + 3.7);
    cursorHide(tl, t + 4.1);
  }

  /* ===================== 04 单机导航（0 放大；导航时收起底栏） ===================== */
  function animNav(tl, s) {
    const a = s.start, r = s.refs;
    tl.set(r.robot, { x: 470, y: 300 }, a);
    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' }, { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    caption(tl, a + 0.5, 2.6, '<b>点一下，它自己去</b>', 'A* 单机导航');
    cursorShow(tl, a + 1.5, centerOf(r.pick).x, centerOf(r.pick).y + 70);
    // ① 选目标点（武装，无放大）
    let t = a + 2.4;
    tapEl(tl, t, r.pick, 0.6);
    tl.to(r.pick, { backgroundColor: '#485c11', color: '#ffffff', duration: 0.3 }, t + 0.6);
    setText(tl, t + 0.6, r.pick, '选点中…（点此取消）', '选目标点');
    caption(tl, t + 0.3, 2.6, '点「选目标点」武装', '再点地图任意处选点（避免误触）');
    // ② 点地图落 pin
    t = a + 5.4;
    const p = svgPt(r, 820, 560);
    cursorTapAt(tl, t, p.x, p.y, 0.8);
    tl.set(r.pin, { x: 820, y: 560 }, t + 0.85);
    tl.fromTo(r.pin, { opacity: 0, scale: 0.3 }, { opacity: 1, scale: 1, duration: 0.5, ease: 'back.out(1.8)' }, t + 0.85);
    setText(tl, t + 0.9, r.tgt, '(412, 286)', '点地图选点');
    tl.to(r.pick, { backgroundColor: '#ffffff', color: '#485c11', duration: 0.3 }, t + 0.9);
    setText(tl, t + 0.9, r.pick, '选目标点', '选点中…（点此取消）');
    caption(tl, t + 0.6, 2.6, '<b>栅格选点 · A* 规划</b>', '红色泪滴 pin 落点，坐标等宽显示');
    // ③ 开始导航 → 收起底栏 → 自主行驶
    t = a + 9.0;
    tapEl(tl, t, r.go, 0.6);
    setText(tl, t + 0.6, r.go, '重新导航', '开始导航');
    tl.to(r.sheet, { y: 300, duration: 0.6, ease: 'power2.inOut' }, t + 0.8);
    caption(tl, t + 0.7, 3.6, '<b>自主导航中 · 收起底栏</b>', '沿规划路径行驶，接近终点自动判定到达');
    tl.to(r.trail, { attr: { 'stroke-dashoffset': 0 }, duration: 3.6, ease: 'none' }, t + 1.2);
    tl.to(r.robot, { x: 560, y: 300, duration: 1.0, ease: 'power1.inOut' }, t + 1.2);
    tl.to(r.robot, { x: 560, y: 560, duration: 1.6, ease: 'power1.inOut' }, t + 2.2);
    tl.to(r.robot, { x: 820, y: 560, duration: 1.4, ease: 'power1.inOut' }, t + 3.8);
    tl.fromTo(r.toast, { opacity: 0, y: 10 }, { opacity: 1, y: 0, duration: 0.4 }, t + 5.2);
    setText(tl, t + 5.2, r.toast, '到达终点', '');
    tl.to(r.toast, { opacity: 0, duration: 0.4 }, t + 7.0);
    caption(tl, t + 5.2, 2.4, '<b>到达终点</b>', '「取消」停止 · 再点地图即重选');
    cursorHide(tl, t + 7.2);
  }

  F.addScene({ id: '03-build', dur: 21, build: function (root) { return buildControl(root, { mapReady: false }); }, anim: animMapping });
  F.addScene({ id: '04-nav', dur: 17, build: function (root) { return buildControl(root, { mapReady: true }); }, anim: animNav });
})();
