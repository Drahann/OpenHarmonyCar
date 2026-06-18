/* =====================================================================================
   场景库（act 0–2）。每个场景经 FILM.addScene 注册；顺序见 00-视频制作总纲.md §1。
   新增/后续 act 可另建 scenes-actN.js，在 film.html 里 boot 前加载即可。
   ===================================================================================== */
(function () {
  "use strict";
  const F = window.FILM;
  const rise = F.rise, fade = F.fade, pop = F.pop, centerOf = F.centerOf,
    camTo = F.camTo, camReset = F.camReset,
    cursorShow = F.cursorShow, cursorHide = F.cursorHide, cursorMove = F.cursorMove,
    cursorTapAt = F.cursorTapAt, tapEl = F.tapEl, caption = F.caption, I = F.I, STAGE_W = F.STAGE_W;

  /* 手填 IP 兜底小窗（发现设备页旁的"顺带演示"）样式 + 打字机 */
  F.addCSS(
    '.ipside{position:absolute;right:24px;top:332px;width:270px;background:var(--surface);border-radius:20px;box-shadow:var(--card-shadow);padding:22px 22px 24px;z-index:30;opacity:0;}' +
    '.ipside .ips-badge{display:inline-block;font-size:14px;font-weight:600;color:var(--primary);background:var(--primary-soft);border-radius:999px;padding:4px 12px;}' +
    '.ipside .ips-title{font-size:25px;font-weight:700;color:var(--text-title);margin-top:12px;}' +
    '.ipside .ips-sub{font-size:14px;color:var(--text-secondary);margin-top:6px;line-height:1.5;}' +
    '.ipside .ips-label{font-size:14px;color:var(--text-secondary);margin-top:18px;}' +
    '.ipside .ips-input{height:52px;border-radius:12px;background:var(--page-bg);border:1.5px solid var(--border);display:flex;align-items:center;padding:0 16px;margin-top:8px;}' +
    '.ipside .ips-val{font-family:var(--font-mono);font-size:20px;color:var(--text-body);white-space:pre;}' +
    '.ipside .ips-caret{width:2px;height:23px;background:var(--primary);margin-left:1px;}' +
    '.ipside .ips-save{height:50px;border-radius:999px;background:var(--surface-muted);color:var(--text-caption);font-size:18px;font-weight:600;display:flex;align-items:center;justify-content:center;margin-top:16px;}'
  );
  function typeIP(tl, at, dur, el, full) { const o = { n: 0 }; tl.to(o, { n: full.length, duration: dur, ease: 'none', onUpdate: function () { el.textContent = full.substring(0, Math.round(o.n)); }, onReverseComplete: function () { el.textContent = ''; } }, at); }

  /* ===================== 00 标题 ===================== */
  function buildTitle(root) {
    root.classList.add('paper');
    const col = document.createElement('div'); col.className = 'center-col';
    const title = document.createElement('div'); title.className = 'bigtitle';
    title.style.cssText = 'font-size:66px;line-height:1.28;letter-spacing:-.01em;max-width:1560px;margin:0 auto;';
    '基于 OpenHarmony 分布式软总线与昇腾边缘智能的自主巡检与工业仪表读数分析机器人'.split('').forEach(function (c) {
      const s = document.createElement('span'); s.className = 'ch'; s.textContent = (c === ' ' ? ' ' : c); title.appendChild(s);
    });
    col.appendChild(title); root.appendChild(col);
    return { chars: Array.prototype.slice.call(title.querySelectorAll('.ch')), col: col };
  }
  function animTitle(tl, s) {
    const a = s.start, r = s.refs;
    tl.fromTo(r.chars, { opacity: 0, y: 36, filter: 'blur(12px)' },
      { opacity: 1, y: 0, filter: 'blur(0px)', duration: 0.9, ease: 'power4.out', stagger: 0.026 }, a + 0.5);
    tl.fromTo(r.col, { scale: 1.0 }, { scale: 1.03, duration: s.dur, ease: 'none' }, a);
  }

  /* ===================== 章节卡（参数化）===================== */
  function makeSection(num, title, sub) {
    return {
      build: function (root) {
        root.classList.add('paper');
        const col = document.createElement('div'); col.className = 'center-col';
        const n = document.createElement('div'); n.className = 'secnum'; n.textContent = num;
        const t = document.createElement('div'); t.className = 'sectitle'; t.textContent = title;
        const sb = document.createElement('div'); sb.className = 'secsub'; sb.textContent = sub;
        col.appendChild(n); col.appendChild(t); col.appendChild(sb); root.appendChild(col);
        return { n: n, t: t, sb: sb, col: col };
      },
      anim: function (tl, s) {
        const a = s.start, r = s.refs;
        rise(tl, r.n, a + 0.3, { y: 24, dur: 0.9 });
        rise(tl, r.t, a + 0.6, { y: 24, dur: 0.9 });
        rise(tl, r.sb, a + 0.95, { y: 18, dur: 0.8 });
        tl.fromTo(r.col, { scale: 1.0 }, { scale: 1.03, duration: s.dur, ease: 'none' }, a);
      }
    };
  }

  /* ===================== 01 首页 HomePage ===================== */
  function buildHome(root) {
    root.classList.add('paper');
    const cam = document.createElement('div'); cam.className = 'cam'; root.appendChild(cam);
    const T = document.createElement('div'); T.className = 'tablet';
    const TW = 1300, TH = 812, TX = (STAGE_W - TW) / 2, TY = 128;
    T.style.cssText += 'left:' + TX + 'px;top:' + TY + 'px;width:' + TW + 'px;height:' + TH + 'px;';
    T.innerHTML = '<div class="cam-dot"></div>';
    const screen = document.createElement('div'); screen.className = 'screen';
    const app = document.createElement('div'); app.className = 'app';
    app.innerHTML =
      '<div class="row">' +
        '<div class="col"><div class="app-title">巡检控制</div><div class="app-sub" data-sub>正在搜索同网络车辆…</div></div>' +
        '<div class="spacer"></div>' +
        '<div class="iconbtn" data-trust>' + I.trust + '</div>' +
        '<div class="iconbtn" data-video>' + I.video + '</div>' +
        '<div class="btn-primary" data-discover>发现设备</div>' +
      '</div>' +
      '<div class="chips" data-chips>' +
        '<div class="chip sel" data-chip="0">单机导航</div>' +
        '<div class="chip" data-chip="1">全路径覆盖</div>' +
        '<div class="chip" data-chip="2">多机协同</div>' +
      '</div>' +
      '<div class="empty" data-empty>' +
        '<div class="ring"><div class="inner"></div><div class="spin"></div></div>' +
        '<div class="et">正在搜索车辆</div><div class="es">确保平板与车在同一热点 / 局域网</div>' +
      '</div>' +
      '<div class="cards" data-cards></div>';
    screen.appendChild(app); T.appendChild(screen); cam.appendChild(T);

    const cards = app.querySelector('[data-cards]');
    function mkCard(ip, detail) {
      const c = document.createElement('div'); c.className = 'card';
      c.innerHTML =
        '<div class="sdot"></div>' +
        '<div class="col" style="flex:1"><div class="cname">巡检车</div>' +
        '<div class="cip mono">' + ip + '</div><div class="cdetail" data-detail>' + detail + '</div></div>' +
        '<div class="act">' +
          '<div class="variant v-connect" data-v="connect">连接</div>' +
          '<div class="variant v-connecting" data-v="connecting" style="opacity:0"><span class="minispin"></span>连接中</div>' +
          '<div class="variant v-enter" data-v="enter" style="opacity:0">进入控制 ' + I.fwd + '</div>' +
        '</div>';
      return c;
    }
    const c1 = mkCard('192.168.43.12', '已发现 · 未连接');
    const c2 = mkCard('192.168.43.27', '已发现 · 未连接');
    cards.appendChild(c1); cards.appendChild(c2);

    // 顺带演示：手填 IP 兜底小窗（挂在 root 上 → 不随相机缩放移动，固定在平板右侧）
    const ipside = document.createElement('div'); ipside.className = 'ipside';
    ipside.innerHTML = '<div class="ips-badge">兜底</div><div class="ips-title">手动连接</div><div class="ips-sub">发现不可用时（客户端隔离 / 固定 IP / 调试）手填车辆 IP。</div><div class="ips-label">车辆 IP</div><div class="ips-input"><span class="ips-val" data-ipval></span><span class="ips-caret"></span></div><div class="ips-save" data-ipsave>保存并连接</div>';
    root.appendChild(ipside);

    return {
      cam: cam, T: T,
      ipside: ipside, ipval: ipside.querySelector('[data-ipval]'), ipsave: ipside.querySelector('[data-ipsave]'),
      sub: app.querySelector('[data-sub]'),
      discover: app.querySelector('[data-discover]'),
      trust: app.querySelector('[data-trust]'),
      video: app.querySelector('[data-video]'),
      chips: Array.prototype.slice.call(app.querySelectorAll('[data-chip]')),
      empty: app.querySelector('[data-empty]'),
      cards: [c1, c2],
      c1: {
        root: c1, dot: c1.querySelector('.sdot'), detail: c1.querySelector('[data-detail]'),
        connect: c1.querySelector('[data-v=connect]'), connecting: c1.querySelector('[data-v=connecting]'), enter: c1.querySelector('[data-v=enter]')
      }
    };
  }
  function animHome(tl, s) {
    const a = s.start, r = s.refs;
    const pDiscover = centerOf(r.discover), pTrust = centerOf(r.trust), pVideo = centerOf(r.video);
    const pCard = centerOf(r.c1.root), pChips = centerOf(r.chips[1]), pEnter = centerOf(r.c1.enter);

    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' },
      { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    caption(tl, a + 0.5, 2.6, '<b>发现你的巡检车</b>', '同一热点，一键找到它');
    cursorShow(tl, a + 1.4, pDiscover.x - 160, pDiscover.y + 90);

    // ① 发现设备
    let t = a + 2.4;
    camTo(tl, r.cam, t, pDiscover.x, pDiscover.y, 1.7, 1.0);
    tapEl(tl, t + 0.7, r.discover, 0.6);
    caption(tl, t + 0.6, 2.4, '点击 <b>发现设备</b>', '向局域网广播 0x06，按回包源 IP 收集在线设备');
    camReset(tl, r.cam, t + 2.6, 1.0);

    // ② 卡片出现
    t = a + 6.0;
    fade(tl, r.empty, t, 0, 0.5);
    r.cards.forEach(function (c, i) {
      tl.fromTo(c, { opacity: 0, y: 30, filter: 'blur(8px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: 0.8, ease: 'power4.out' }, t + 0.3 + i * 0.18);
    });
    tl.to(r.sub, { duration: 0.01, onComplete: function () { r.sub.textContent = '发现 2 台车辆'; }, onReverseComplete: function () { r.sub.textContent = '正在搜索同网络车辆…'; } }, t + 0.4);
    caption(tl, t + 0.6, 2.8, '<b>局域网广播发现</b>', '免手填 IP · 列表整体重建，绝不累积重复');
    // 旁开「手动连接」小窗：顺带演示手填 IP 兜底（与发现并列，发现可用时不需要它）
    tl.fromTo(r.ipside, { opacity: 0, x: 28, filter: 'blur(8px)' }, { opacity: 1, x: 0, filter: 'blur(0px)', duration: 0.6, ease: 'power3.out' }, t + 0.2);
    typeIP(tl, t + 0.9, 1.0, r.ipval, '192.168.43.12');
    tl.to(r.ipsave, { backgroundColor: '#485c11', color: '#ffffff', duration: 0.4 }, t + 2.0);
    tl.to(r.ipside, { opacity: 0, x: 22, filter: 'blur(6px)', duration: 0.45, ease: 'power2.in' }, t + 2.9);

    // ③ 连接状态机 ⚪→🟡→🟢
    t = a + 9.6;
    camTo(tl, r.cam, t, pCard.x, pCard.y, 1.85, 1.0);
    tapEl(tl, t + 0.8, r.c1.connect, 0.6);
    caption(tl, t + 0.7, 3.4, '<b>四态连接</b> ⚪已发现 → 🟡连接中 → 🟢已连接 → 🔴丢失', '点「连接」起中性保活，收到心跳即在线（每 600ms 刷新）');
    tl.to(r.c1.dot, { backgroundColor: '#dfa32f', duration: 0.4 }, t + 1.5);
    tl.to(r.c1.connect, { opacity: 0, duration: 0.3 }, t + 1.5);
    tl.to(r.c1.connecting, { opacity: 1, duration: 0.3 }, t + 1.6);
    tl.to(r.c1.detail, { duration: 0.01, onComplete: function () { r.c1.detail.textContent = '连接中…（等待心跳）'; }, onReverseComplete: function () { r.c1.detail.textContent = '已发现 · 未连接'; } }, t + 1.6);
    tl.to(r.c1.dot, { backgroundColor: '#3f9352', duration: 0.4 }, t + 3.0);
    tl.to(r.c1.connecting, { opacity: 0, duration: 0.3 }, t + 3.0);
    tl.to(r.c1.enter, { opacity: 1, duration: 0.35 }, t + 3.1);
    tl.to(r.c1.detail, { duration: 0.01, onComplete: function () { r.c1.detail.textContent = '已连接 · 心跳正常 · 刚刚'; }, onReverseComplete: function () { r.c1.detail.textContent = '连接中…（等待心跳）'; } }, t + 3.1);
    camReset(tl, r.cam, t + 4.2, 1.0);

    // ④ 模式 chip 自动轮播高亮（不模拟点击，更快——避免拖沓）
    t = a + 15.2;
    caption(tl, t + 0.4, 2.4, '<b>三种作业模式</b>', '单机导航 · 全路径覆盖 · 多机协同');
    function selChip(at, idx) {
      r.chips.forEach(function (ch, i) {
        tl.to(ch, { backgroundColor: i === idx ? '#485c11' : '#ffffff', color: i === idx ? '#ffffff' : '#485c11', duration: 0.3, ease: 'power2.out' }, at);
      });
    }
    selChip(t + 0.5, 1); selChip(t + 1.05, 2); selChip(t + 1.6, 0);

    // ⑥ 点「设备互信」→ 进入互信章节（多机协同前一次性配对）
    t = a + 17.6;
    camTo(tl, r.cam, t, pTrust.x, pTrust.y, 1.7, 1.0);
    tapEl(tl, t + 0.9, r.trust, 0.6);
    caption(tl, t + 0.8, 2.4, '点 <b>设备互信</b>', '多机协同前，先一次性配对');
    camReset(tl, r.cam, t + 2.8, 1.0);
    cursorHide(tl, t + 3.2);
  }

  /* ===================== 控制页：SVG 地图（复刻 MapCanvas.ets）===================== */
  function mapSVG() {
    const fx = 285, fy = 41, fs = 700, pitch = fs / 22;
    let grid = '';
    for (let i = 0; i <= 22; i++) { const g = fx + i * pitch; grid += '<line x1="' + g + '" y1="' + fy + '" x2="' + g + '" y2="' + (fy + fs) + '"/>'; }
    for (let i = 0; i <= 22; i++) { const g = fy + i * pitch; grid += '<line x1="' + fx + '" y1="' + g + '" x2="' + (fx + fs) + '" y2="' + g + '"/>'; }
    return '<svg viewBox="0 0 1270 782" preserveAspectRatio="xMidYMid slice">' +
      '<defs><linearGradient id="mbg" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#edebe0"/><stop offset="1" stop-color="#e2dfd0"/></linearGradient></defs>' +
      '<rect x="0" y="0" width="1270" height="782" fill="url(#mbg)"/>' +
      '<g id="mapContent">' +
        '<rect x="' + fx + '" y="' + fy + '" width="' + fs + '" height="' + fs + '" fill="#ffffff"/>' +
        '<g stroke="rgba(72,92,17,.06)" stroke-width="1">' + grid + '</g>' +
        '<rect x="' + fx + '" y="' + fy + '" width="' + fs + '" height="' + fs + '" fill="none" stroke="#e3e7d2" stroke-width="2"/>' +
        '<g fill="none" stroke="#485c11" stroke-width="5" stroke-linejoin="round" stroke-linecap="round">' +
          '<path d="M345 120 L345 662 L545 662 M700 662 L925 662 L925 120 L345 120"/>' +
          '<path d="M700 120 L700 360 M700 360 L925 360"/>' +
          '<path d="M345 470 L520 470"/>' +
        '</g>' +
        '<rect x="395" y="540" width="150" height="92" rx="6" fill="rgba(72,92,17,.14)" stroke="#485c11" stroke-width="4"/>' +
        '<path id="trail" d="M470 300" fill="none" stroke="#357a41" stroke-width="4" stroke-linecap="round" stroke-linejoin="round" opacity="0"/>' +
        '<g id="pin" opacity="0"><path d="M0 0 Q-9 -14 -9 -23 A9 9 0 0 1 9 -23 Q9 -14 0 0 Z" fill="#d9503f"/><circle cx="0" cy="-23" r="3.6" fill="#fff"/></g>' +
        '<g id="robot" transform="translate(470,300)">' +
          '<polygon points="0,-30 11,-12 -11,-12" fill="#357a41"/>' +
          '<circle cx="0" cy="0" r="16" fill="#357a41"/><circle cx="0" cy="0" r="16" fill="none" stroke="#fff" stroke-width="3"/>' +
          '<text x="0" y="1" fill="#fff" font-size="17" font-weight="700" text-anchor="middle" dominant-baseline="middle">1</text>' +
        '</g>' +
      '</g></svg>';
  }
  function opCardHTML(mode) {
    if (mode === 'astar') return '<div class="handle"></div>' +
      '<div class="op-hint">① 点「选目标点」再点地图选点 → ② 点「开始导航」</div>' +
      '<div class="op-pick" data-pick>选目标点</div>' +
      '<div class="op-row"><div class="op-target"><div class="lbl">目标点</div><div class="val" data-tgt>点地图选点</div></div>' +
      '<div class="op-btn ghost" data-cancel>取消</div><div class="op-btn primary" data-go>开始导航</div></div>';
    return '<div class="handle"></div><div class="op-hint">操作卡</div>';
  }
  function buildControl(root, opts) {
    opts = opts || {}; const mode = opts.mode || 'astar', ip = opts.ip || '192.168.43.12';
    const mlabel = { astar: '单机导航', fullpath: '全路径覆盖', distributed: '多机协同' }[mode];
    root.classList.add('paper');
    const cam = document.createElement('div'); cam.className = 'cam'; root.appendChild(cam);
    const T = document.createElement('div'); T.className = 'tablet';
    const TW = 1300, TH = 812, TX = (STAGE_W - TW) / 2, TY = 128;
    T.style.cssText += 'left:' + TX + 'px;top:' + TY + 'px;width:' + TW + 'px;height:' + TH + 'px;';
    T.innerHTML = '<div class="cam-dot"></div>';
    const screen = document.createElement('div'); screen.className = 'screen';
    const am = document.createElement('div'); am.className = 'app-map';
    am.innerHTML = mapSVG() +
      '<div class="ctl-top">' +
        '<div class="ctl-back">' + I.back + '</div>' +
        '<div class="ctl-pill"><span class="dot"></span><span class="ip">' + ip + '</span><span class="md">' + mlabel + '</span><span class="caret">▼</span></div>' +
        '<div class="ctl-rebuild">重新建图</div>' +
        '<div class="ctl-icon ctl-video">' + I.video + '</div>' +
      '</div>' +
      '<div class="ctl-fabs"><div class="fab" data-fab="in">＋</div><div class="fab" data-fab="out">−</div><div class="fab" data-fab="reset">⊙</div></div>' +
      '<div class="ctl-pip"><div class="pip-cap">' + I.video + '<span>仪表视频</span></div></div>' +
      '<div class="sheet">' + opCardHTML(mode) + '</div>';
    screen.appendChild(am); T.appendChild(screen); cam.appendChild(T);
    const q = function (sel) { return am.querySelector(sel); };
    return {
      cam: cam, T: T, mapContent: q('#mapContent'), robot: q('#robot'), pin: q('#pin'), trail: q('#trail'),
      back: q('.ctl-back'), pill: q('.ctl-pill'), rebuild: q('.ctl-rebuild'), video: q('.ctl-video'),
      fabIn: q('[data-fab=in]'), fabOut: q('[data-fab=out]'), fabReset: q('[data-fab=reset]'),
      pip: q('.ctl-pip'), sheet: q('.sheet'),
      pick: q('[data-pick]'), tgt: q('[data-tgt]'), go: q('[data-go]'), cancel: q('[data-cancel]')
    };
  }

  /* ===================== 02 控制台总览 + 缩放 FAB ===================== */
  function animControlOverview(tl, s) {
    const a = s.start, r = s.refs;
    const pPill = centerOf(r.pill), pFab = centerOf(r.fabIn), pSheet = centerOf(r.sheet), pPip = centerOf(r.pip);
    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' },
      { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    caption(tl, a + 0.5, 2.6, '<b>一张地图，掌控全局</b>', 'Google Maps 范式：全屏地图打底，浮层操控');
    cursorShow(tl, a + 1.8, pFab.x, pFab.y + 130);

    let t = a + 3.0;
    camTo(tl, r.cam, t, pPill.x, pPill.y, 1.7, 1.0);
    caption(tl, t + 0.6, 2.6, '顶部状态胶囊', '连接点 · IP · 模式 · 点开设备面板');
    camReset(tl, r.cam, t + 3.0, 1.0);

    t = a + 7.0;
    camTo(tl, r.cam, t, pFab.x, pFab.y, 1.7, 1.0);
    caption(tl, t + 0.5, 4.4, '<b>＋ / − / ⊙ 回中</b>', '看清细节，或纵览全局');
    tapEl(tl, t + 1.0, r.fabIn, 0.5); tl.to(r.mapContent, { scale: 1.5, svgOrigin: '635 391', duration: 0.8, ease: 'power2.inOut' }, t + 1.5);
    tapEl(tl, t + 2.4, r.fabIn, 0.4); tl.to(r.mapContent, { scale: 2.0, svgOrigin: '635 391', duration: 0.7, ease: 'power2.inOut' }, t + 2.7);
    tapEl(tl, t + 3.7, r.fabOut, 0.4); tl.to(r.mapContent, { scale: 1.4, svgOrigin: '635 391', duration: 0.7, ease: 'power2.inOut' }, t + 4.0);
    tapEl(tl, t + 5.0, r.fabReset, 0.4); tl.to(r.mapContent, { scale: 1.0, svgOrigin: '635 391', duration: 0.8, ease: 'power2.inOut' }, t + 5.3);
    camReset(tl, r.cam, t + 6.3, 1.0);

    t = a + 14.2;
    camTo(tl, r.cam, t, pSheet.x, pSheet.y, 1.45, 1.0);
    caption(tl, t + 0.5, 2.6, '底部操作卡', '随模式变体：导航 / 覆盖 / 协同');
    camReset(tl, r.cam, t + 3.0, 1.0);

    t = a + 17.8;
    camTo(tl, r.cam, t, pPip.x, pPip.y, 1.8, 1.0);
    caption(tl, t + 0.5, 2.4, '仪表识别 PiP', '边导航边瞥仪表，独立链路不打扰导航');
    camReset(tl, r.cam, t + 2.8, 1.0);
    cursorHide(tl, t + 3.0);
  }

  /* ===================== 注册 ===================== */
  F.addScene({ id: '00-title', dur: 4.5, build: buildTitle, anim: animTitle });
  F.addScene(Object.assign({ id: '01-sec', dur: 4.2 }, makeSection('01', '发现你的巡检车', '同一热点，一键找到它')));
  F.addScene({ id: '01-home', dur: 21, build: buildHome, anim: animHome });
  F.addScene(Object.assign({ id: '02-sec', dur: 4.2 }, makeSection('02', '一张地图，掌控全局', 'Google Maps 范式的控制台')));
  F.addScene({ id: '02-ctrl', dur: 23, build: function (root) { return buildControl(root, { mode: 'astar' }); }, anim: animControlOverview });
})();
