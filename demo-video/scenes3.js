/* =====================================================================================
   场景库 act 5–6：全路径覆盖 + 多机协同。自包含（含 CSS + 控制页构建器）。
   勿用 file-tool Edit（会截断）。经 FILM.addScene 注册。
   ===================================================================================== */
(function () {
  "use strict";
  const F = window.FILM;
  const centerOf = F.centerOf, camTo = F.camTo, camReset = F.camReset,
    cursorShow = F.cursorShow, cursorHide = F.cursorHide, cursorTapAt = F.cursorTapAt,
    tapEl = F.tapEl, caption = F.caption, addCSS = F.addCSS, I = F.I, STAGE_W = F.STAGE_W;

  addCSS(
    '.op-sub{font-size:15px;color:var(--text-secondary);margin:6px 0 8px;}' +
    '.op-chips{display:flex;gap:12px;}' +
    '.chip2{flex:1;height:50px;border-radius:999px;border:1.5px solid var(--primary);display:flex;align-items:center;justify-content:center;font-size:19px;font-weight:600;color:var(--primary);background:var(--surface);}' +
    '.chip2.sel{background:var(--primary);color:#fff;}' +
    '.op-row2{display:flex;align-items:center;margin-top:12px;}' +
    '.op-area{font-size:16px;color:var(--text-secondary);margin-top:12px;}' +
    '.op-tg{display:flex;align-items:center;}' +
    '.op-tg .lab{flex:1;font-size:18px;color:var(--text-body);}' +
    '.toggle{width:64px;height:34px;border-radius:999px;background:var(--divider);position:relative;}' +
    '.toggle.on{background:var(--primary);}' +
    '.toggle-knob{position:absolute;top:3px;left:3px;width:28px;height:28px;border-radius:50%;background:#fff;box-shadow:0 1px 4px rgba(0,0,0,.3);}' +
    '.op-pullbtn{height:44px;border-radius:999px;background:var(--primary);color:#fff;font-size:16px;display:flex;align-items:center;justify-content:center;margin:8px 0;}' +
    '.prog{display:flex;align-items:center;gap:12px;margin-top:8px;}' +
    '.prog .pl{width:42px;font-size:18px;color:var(--text-body);}' +
    '.pbar{flex:1;height:10px;border-radius:5px;background:var(--divider);position:relative;overflow:hidden;}' +
    '.pfill{position:absolute;left:0;top:0;bottom:0;width:0;border-radius:5px;background:var(--primary);}' +
    '.prog .pv{width:150px;text-align:right;font-size:16px;color:var(--text-secondary);}' +
    '.toast{position:absolute;left:50%;bottom:210px;transform:translateX(-50%);background:rgba(29,32,22,.92);color:#fff;font-size:18px;padding:12px 22px;border-radius:999px;opacity:0;z-index:9;white-space:nowrap;}'
  );

  function sec(num, title, sub) {
    return {
      build: function (root) {
        root.classList.add('paper');
        const col = document.createElement('div'); col.className = 'center-col';
        col.innerHTML = '<div class="secnum">' + num + '</div><div class="sectitle">' + title + '</div><div class="secsub">' + sub + '</div>';
        root.appendChild(col); return { n: col.children[0], t: col.children[1], sb: col.children[2], col: col };
      },
      anim: function (tl, s) {
        const a = s.start, r = s.refs;
        F.rise(tl, r.n, a + 0.3, { y: 24, dur: 0.9 }); F.rise(tl, r.t, a + 0.6, { y: 24, dur: 0.9 }); F.rise(tl, r.sb, a + 0.95, { y: 18, dur: 0.8 });
        tl.fromTo(r.col, { scale: 1.0 }, { scale: 1.03, duration: s.dur, ease: 'none' }, a);
      }
    };
  }
  function setText(tl, at, el, txt, prev) { tl.to(el, { duration: 0.01, onComplete: function () { el.textContent = txt; }, onReverseComplete: function () { el.textContent = prev; } }, at); }

  function mapBase() {
    const fx = 285, fy = 41, fs = 700, pitch = fs / 22;
    let grid = '';
    for (let i = 0; i <= 22; i++) { const g = fx + i * pitch; grid += '<line x1="' + g + '" y1="' + fy + '" x2="' + g + '" y2="' + (fy + fs) + '"/>'; }
    for (let i = 0; i <= 22; i++) { const g = fy + i * pitch; grid += '<line x1="' + fx + '" y1="' + g + '" x2="' + (fx + fs) + '" y2="' + g + '"/>'; }
    return '<rect x="' + fx + '" y="' + fy + '" width="' + fs + '" height="' + fs + '" fill="#ffffff"/>' +
      '<g stroke="rgba(72,92,17,.06)" stroke-width="1">' + grid + '</g>' +
      '<rect x="' + fx + '" y="' + fy + '" width="' + fs + '" height="' + fs + '" fill="none" stroke="#e3e7d2" stroke-width="2"/>' +
      '<g fill="none" stroke="#485c11" stroke-width="5" stroke-linejoin="round" stroke-linecap="round">' +
      '<path d="M345 120 L345 662 L545 662 M700 662 L925 662 L925 120 L345 120"/><path d="M700 120 L700 360 M700 360 L925 360"/><path d="M345 470 L520 470"/></g>' +
      '<rect x="395" y="540" width="150" height="92" rx="6" fill="rgba(72,92,17,.14)" stroke="#485c11" stroke-width="4"/>';
  }
  function robotG(id, fill, n) {
    // 主体子元素不写自身 fill → 继承 <g> 的 fill；这样对 <g> 做 fill 动画即可整体重染机器人主体。
    return '<g class="' + id + '" fill="' + fill + '" opacity="1"><polygon points="0,-30 11,-12 -11,-12"/><circle r="16"/><circle r="16" fill="none" stroke="#fff" stroke-width="3"/><text y="1" fill="#fff" font-size="17" font-weight="700" text-anchor="middle" dominant-baseline="middle">' + n + '</text></g>';
  }
  function regionRect(id, x, y, w, h, color) {
    const c = 18;
    return '<g class="' + id + '" opacity="0"><rect x="' + x + '" y="' + y + '" width="' + w + '" height="' + h + '" fill="none" stroke="' + color + '" stroke-width="3"/>' +
      '<path d="M' + (x + c) + ' ' + y + ' L' + x + ' ' + y + ' L' + x + ' ' + (y + c) + '" stroke="' + color + '" stroke-width="4" fill="none" stroke-linecap="round"/>' +
      '<path d="M' + (x + w - c) + ' ' + y + ' L' + (x + w) + ' ' + y + ' L' + (x + w) + ' ' + (y + c) + '" stroke="' + color + '" stroke-width="4" fill="none" stroke-linecap="round"/>' +
      '<path d="M' + (x + c) + ' ' + (y + h) + ' L' + x + ' ' + (y + h) + ' L' + x + ' ' + (y + h - c) + '" stroke="' + color + '" stroke-width="4" fill="none" stroke-linecap="round"/>' +
      '<path d="M' + (x + w - c) + ' ' + (y + h) + ' L' + (x + w) + ' ' + (y + h) + ' L' + (x + w) + ' ' + (y + h - c) + '" stroke="' + color + '" stroke-width="4" fill="none" stroke-linecap="round"/></g>';
  }

  function buildCtl(root, mode) {
    const ip = '192.168.43.12', mlabel = mode === 'fullpath' ? '全路径覆盖' : '多机协同';
    root.classList.add('paper');
    const cam = document.createElement('div'); cam.className = 'cam'; root.appendChild(cam);
    const T = document.createElement('div'); T.className = 'tablet';
    const TW = 1300, TH = 812, TX = (STAGE_W - TW) / 2, TY = 128;
    T.style.cssText += 'left:' + TX + 'px;top:' + TY + 'px;width:' + TW + 'px;height:' + TH + 'px;';
    T.innerHTML = '<div class="cam-dot"></div>';
    const screen = document.createElement('div'); screen.className = 'screen';
    const am = document.createElement('div'); am.className = 'app-map';

    let extras = '', robots = '', sheet = '';
    if (mode === 'fullpath') {
      extras =
        '<rect class="cover" x="380" y="360" width="500" height="330" fill="#485c11" opacity="0"/>' +
        regionRect('region', 380, 360, 500, 330, '#485c11') +
        '<g class="verts"></g>';
      robots = robotG('robot', '#357a41', '1');
      sheet = '<div class="op-sub">① 选覆盖算法</div>' +
        '<div class="op-chips"><div class="chip2 sel" data-alg0>牛耕算法</div><div class="chip2" data-alg1>最小生成树</div></div>' +
        '<div class="op-hint" data-fphint style="margin-top:12px">② 点「选顶点」再点地图选 4 个顶点（左下→右下→右上→左上）</div>' +
        '<div class="op-pick" data-pick>选顶点</div>' +
        '<div class="op-row2"><div class="op-btn ghost" data-reset>重选顶点</div><div class="spacer"></div><div class="op-btn primary" data-stop>停止</div></div>' +
        '<div class="op-area" data-area style="opacity:0">④ 已覆盖 ≈ <span data-areaval>0.0</span> m²（按足迹估算）</div>';
    } else {
      extras =
        '<rect class="cov1" x="380" y="360" width="240" height="330" fill="#357a41" opacity="0"/>' +
        '<rect class="cov2" x="640" y="360" width="245" height="330" fill="#4678b8" opacity="0"/>' +
        regionRect('reg1', 380, 360, 240, 330, '#357a41') +
        regionRect('reg2', 640, 360, 245, 330, '#4678b8');
      robots = robotG('robot', '#357a41', '1') + robotG('robot2', '#4678b8', '2');
      sheet = '<div class="op-sub">① 下发方式</div>' +
        '<div class="op-tg"><div class="lab" data-tglab>平板直连双车（方案A·默认）</div><div class="toggle" data-toggle><div class="toggle-knob"></div></div></div>' +
        '<div class="op-sub">② 选车划分（给哪辆车划这块）</div>' +
        '<div class="op-chips"><div class="chip2 sel" data-car1>车1</div><div class="chip2" data-car2>车2</div></div>' +
        '<div class="op-pullbtn" data-pull style="opacity:0;height:0;margin:0;overflow:hidden">让车2拉主机图（105→5）</div>' +
        '<div class="op-hint" data-dhint style="margin-top:10px">③ 点「选点划区域」再点地图选两个对角点</div>' +
        '<div class="op-pick" data-pick>选点划区域</div>' +
        '<div class="op-sub">④ 各车覆盖进度</div>' +
        '<div class="prog"><span class="pl">车1</span><div class="pbar"><div class="pfill" data-p1></div></div><span class="pv mono" data-pv1>0% · 0.0m²</span></div>' +
        '<div class="prog"><span class="pl">车2</span><div class="pbar"><div class="pfill" style="background:#4678b8" data-p2></div></div><span class="pv mono" data-pv2>0% · 0.0m²</span></div>';
    }

    am.innerHTML =
      '<svg viewBox="0 0 1270 782" preserveAspectRatio="xMidYMid slice">' +
      '<defs><linearGradient id="mbg3" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#edebe0"/><stop offset="1" stop-color="#e2dfd0"/></linearGradient></defs>' +
      '<rect x="0" y="0" width="1270" height="782" fill="url(#mbg3)"/>' +
      '<g class="mapContent">' + mapBase() + extras + robots + '</g></svg>' +
      '<div class="ctl-top"><div class="ctl-back">' + I.back + '</div>' +
      '<div class="ctl-pill"><span class="dot"></span><span class="ip">' + ip + '</span><span class="md">' + mlabel + '</span><span class="caret">▼</span></div>' +
      '<div class="ctl-rebuild">重新建图</div><div class="ctl-icon ctl-video">' + I.video + '</div></div>' +
      '<div class="ctl-fabs"><div class="fab">＋</div><div class="fab">−</div><div class="fab">⊙</div></div>' +
      '<div class="ctl-pip"><div class="pip-cap" data-pip>' + I.video + '<span>仪表视频</span></div></div>' + '<div class="pip-card" data-pipcard><div class="ph"><span class="pe">实时仪表</span><span class="pbtn" data-pipmax>放大</span><span class="pclose">✕</span></div><div class="pip-vid"></div></div>' +
      '<div class="toast" data-toast></div>' +
      '<div class="sheet"><div class="handle"></div>' + sheet + '</div>';
    screen.appendChild(am); T.appendChild(screen); cam.appendChild(T);
    const q = function (s) { return am.querySelector(s); };
    return {
      cam: cam, T: T, am: am, mapContent: q('.mapContent'), robot: q('.robot'), robot2: q('.robot2'),
      sheet: q('.sheet'), pick: q('[data-pick]'), toast: q('[data-toast]'), pipcap: q('[data-pip]'), pipcard: q('[data-pipcard]'), pipmax: q('[data-pipmax]'),
      alg0: q('[data-alg0]'), alg1: q('[data-alg1]'), reset: q('[data-reset]'), stop: q('[data-stop]'),
      fphint: q('[data-fphint]'), area: q('[data-area]'), areaval: q('[data-areaval]'),
      region: q('.region'), cover: q('.cover'), verts: q('.verts'),
      toggle: q('[data-toggle]'), tglab: q('[data-tglab]'), car1: q('[data-car1]'), car2: q('[data-car2]'),
      pull: q('[data-pull]'), dhint: q('[data-dhint]'),
      reg1: q('.reg1'), reg2: q('.reg2'), cov1: q('.cov1'), cov2: q('.cov2'),
      p1: q('[data-p1]'), p2: q('[data-p2]'), pv1: q('[data-pv1]'), pv2: q('[data-pv2]')
    };
  }
  function svgPt(refs, sx, sy) { const c = centerOf(refs.am); return { x: c.x - 635 + sx, y: c.y - 391 + sy }; }
  function addVert(refs, n, sx, sy) {
    const ns = 'http://www.w3.org/2000/svg';
    const g = document.createElementNS(ns, 'g'); g.setAttribute('opacity', '0');
    g.innerHTML = '<circle cx="' + sx + '" cy="' + sy + '" r="15" fill="#485c11"/><text x="' + sx + '" y="' + (sy + 1) + '" fill="#fff" font-size="16" font-weight="700" text-anchor="middle" dominant-baseline="middle">' + n + '</text>';
    refs.verts.appendChild(g); return g;
  }

  /* ===================== 05 全路径覆盖 ===================== */
  function animCoverage(tl, s) {
    const a = s.start, r = s.refs;
    tl.set(r.robot, { x: 470, y: 300 }, a);
    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' }, { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    caption(tl, a + 0.5, 2.6, '<b>一寸不漏地扫过</b>', '全路径覆盖');
    cursorShow(tl, a + 1.6, centerOf(r.alg0).x, centerOf(r.alg0).y + 70);

    // ① 选算法
    let t = a + 2.6;
    camTo(tl, r.cam, t, centerOf(r.sheet).x, centerOf(r.alg0).y, 1.45, 1.0);
    caption(tl, t + 0.5, 2.6, '<b>两种覆盖算法</b>', '牛耕算法 · 最小生成树');
    tapEl(tl, t + 0.9, r.alg1, 0.5);
    tl.to(r.alg1, { backgroundColor: '#485c11', color: '#fff', duration: 0.3 }, t + 1.3);
    tl.to(r.alg0, { backgroundColor: '#ffffff', color: '#485c11', duration: 0.3 }, t + 1.3);
    tapEl(tl, t + 2.3, r.alg0, 0.5);
    tl.to(r.alg0, { backgroundColor: '#485c11', color: '#fff', duration: 0.3 }, t + 2.7);
    tl.to(r.alg1, { backgroundColor: '#ffffff', color: '#485c11', duration: 0.3 }, t + 2.7);

    // ② 选 4 顶点
    t = a + 6.2;
    tapEl(tl, t, r.pick, 0.5);
    tl.to(r.pick, { backgroundColor: '#485c11', color: '#fff', duration: 0.3 }, t + 0.5);
    setText(tl, t + 0.5, r.pick, '选点中…（点此取消）', '选顶点');
    camReset(tl, r.cam, t + 0.8, 0.9);
    caption(tl, t + 0.6, 3.4, '<b>四点框定作业区</b>', '左下 → 右下 → 右上 → 左上，满 4 点自动启动');
    const corners = [[380, 690], [880, 690], [880, 360], [380, 360]];
    corners.forEach(function (c, i) {
      const v = addVert(r, i + 1, c[0], c[1]);
      const p = svgPt(r, c[0], c[1]);
      const at = t + 1.4 + i * 1.0;
      cursorTapAt(tl, at, p.x, p.y, i === 0 ? 0.7 : 0.55);
      tl.fromTo(v, { opacity: 0, scale: 0.3 }, { opacity: 1, scale: 1, duration: 0.4, ease: 'back.out(2)', svgOrigin: c[0] + ' ' + c[1] }, at + 0.5);
    });
    // 满 4 点 → 区域出现
    tl.to(r.region, { opacity: 1, duration: 0.5 }, t + 5.6);
    tl.to(r.pick, { backgroundColor: '#ffffff', color: '#485c11', duration: 0.3 }, t + 5.6);
    setText(tl, t + 5.6, r.pick, '选顶点', '选点中…（点此取消）');

    // ③ 覆盖执行：牛耕清扫 + 绿色填充 + 面积 count-up
    t = a + 13.0;
    caption(tl, t + 0.3, 4.0, '<b>覆盖足迹 · 面积估算</b>', '机器人牛耕往复，刷宽 ≈ 40cm 实时累计');
    tl.set(r.robot, { x: 410, y: 400 }, t + 0.4);
    tl.to(r.cover, { opacity: 0.18, duration: 0.4 }, t + 0.5);
    tl.fromTo(r.cover, { scaleY: 0 }, { scaleY: 1, duration: 5.2, ease: 'none', svgOrigin: '630 690' }, t + 0.6);
    // 牛耕折返
    const zig = [[850, 400], [850, 470], [410, 470], [410, 540], [850, 540], [850, 610], [410, 610], [410, 660], [850, 660]];
    let zt = t + 0.6;
    zig.forEach(function (p, i) { tl.to(r.robot, { x: p[0], y: p[1], duration: 0.6, ease: 'power1.inOut' }, zt); zt += 0.58; });
    tl.to(r.area, { opacity: 1, duration: 0.4 }, t + 0.8);
    tl.to(r.areaval, { duration: 5.0, innerText: 12.4, snap: { innerText: 0.1 }, ease: 'none' }, t + 0.8);
    cursorHide(tl, t + 6.0);
  }

  /* ===================== 06 多机协同 ===================== */
  function animFleet(tl, s) {
    const a = s.start, r = s.refs;
    tl.set(r.robot, { x: 470, y: 300 }, a); tl.set(r.robot2, { x: 760, y: 300 }, a);
    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' }, { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    caption(tl, a + 0.5, 2.6, '<b>多车，一起干</b>', '鸿蒙软总线协同');
    cursorShow(tl, a + 1.6, centerOf(r.toggle).x, centerOf(r.toggle).y + 70);

    // ① 下发方式
    let t = a + 2.6;
    camTo(tl, r.cam, t, centerOf(r.sheet).x, centerOf(r.toggle).y, 1.4, 1.0);
    caption(tl, t + 0.5, 3.0, '<b>两种下发</b>', '平板直连双车 · 或经车载 agent / 软总线黑板');
    tapEl(tl, t + 0.9, r.toggle, 0.5);
    tl.to(r.toggle, { backgroundColor: '#485c11', duration: 0.25 }, t + 1.3);
    tl.to(r.toggle.querySelector('.toggle-knob'), { left: 33, duration: 0.25 }, t + 1.3);
    setText(tl, t + 1.4, r.tglab, '经车载 agent / 软总线（DDO·需互信）', '平板直连双车（方案A·默认）');
    tapEl(tl, t + 2.6, r.toggle, 0.5);
    tl.to(r.toggle, { backgroundColor: '#e7e5da', duration: 0.25 }, t + 3.0);
    tl.to(r.toggle.querySelector('.toggle-knob'), { left: 3, duration: 0.25 }, t + 3.0);
    setText(tl, t + 3.1, r.tglab, '平板直连双车（方案A·默认）', '经车载 agent / 软总线（DDO·需互信）');

    // ② 选车 + 拉图
    t = a + 6.6;
    tapEl(tl, t, r.car2, 0.5);
    tl.to(r.car2, { backgroundColor: '#485c11', color: '#fff', duration: 0.3 }, t + 0.4);
    tl.to(r.car1, { backgroundColor: '#ffffff', color: '#485c11', duration: 0.3 }, t + 0.4);
    tl.to(r.pull, { opacity: 1, height: 44, margin: 8, duration: 0.4 }, t + 0.5);
    caption(tl, t + 0.4, 2.8, '<b>从车拉取主机地图</b>', '让车2 拉主机图（cmd105 → cmd5），共享同一张图');
    tapEl(tl, t + 1.6, r.pull, 0.5);
    setText(tl, t + 2.2, r.toast, '车2：已发拉图(cmd105)，约 8s 后自动加载…', '');
    tl.fromTo(r.toast, { opacity: 0, y: 10 }, { opacity: 1, y: 0, duration: 0.4 }, t + 2.2);
    tl.to(r.toast, { opacity: 0, duration: 0.4 }, t + 4.4);
    tapEl(tl, t + 3.0, r.car1, 0.5);
    tl.to(r.car1, { backgroundColor: '#485c11', color: '#fff', duration: 0.3 }, t + 3.4);
    tl.to(r.car2, { backgroundColor: '#ffffff', color: '#485c11', duration: 0.3 }, t + 3.4);

    // ③ 划区域
    t = a + 11.4;
    tapEl(tl, t, r.pick, 0.5);
    tl.to(r.pick, { backgroundColor: '#485c11', color: '#fff', duration: 0.3 }, t + 0.4);
    caption(tl, t + 0.5, 3.6, '<b>对角矩形分区</b>', '各车一块，互不重叠');
    camReset(tl, r.cam, t + 0.7, 0.9);
    // 车1 区域：两对角点
    let p = svgPt(r, 380, 360); cursorTapAt(tl, t + 1.2, p.x, p.y, 0.6);
    p = svgPt(r, 620, 690); cursorTapAt(tl, t + 2.0, p.x, p.y, 0.5);
    tl.to(r.reg1, { opacity: 1, duration: 0.4 }, t + 2.6);
    // 切到车2 划第二块
    tapEl(tl, t + 3.2, r.car2, 0.5);
    p = svgPt(r, 640, 360); cursorTapAt(tl, t + 4.0, p.x, p.y, 0.5);
    p = svgPt(r, 885, 690); cursorTapAt(tl, t + 4.8, p.x, p.y, 0.5);
    tl.to(r.reg2, { opacity: 1, duration: 0.4 }, t + 5.4);

    // ④ 覆盖执行 + 进度
    t = a + 18.0;
    caption(tl, t + 0.3, 4.4, '<b>实时进度</b>', '双车分区清扫，各车进度条同步累计');
    // 车1(绿)清扫绿区、车2(蓝)清扫蓝区，各保持本色以便区分；原 r.robot 的 fill 动画既是空操作、又会与车2撞色，已移除。
    tl.set(r.robot, { x: 410, y: 400 }, t + 0.3); tl.set(r.robot2, { x: 670, y: 400 }, t + 0.3);
    tl.to(r.cov1, { opacity: 0.18, duration: 0.4 }, t + 0.4);
    tl.fromTo(r.cov1, { scaleY: 0 }, { scaleY: 1, duration: 4.6, ease: 'none', svgOrigin: '500 690' }, t + 0.5);
    tl.to(r.cov2, { opacity: 0.16, duration: 0.4 }, t + 0.4);
    tl.fromTo(r.cov2, { scaleY: 0 }, { scaleY: 1, duration: 4.6, ease: 'none', svgOrigin: '762 690' }, t + 0.5);
    // 车1 牛耕
    const z1 = [[590, 400], [590, 480], [410, 480], [410, 560], [590, 560], [590, 640], [410, 640]];
    let z1t = t + 0.5; z1.forEach(function (q) { tl.to(r.robot, { x: q[0], y: q[1], duration: 0.62, ease: 'power1.inOut' }, z1t); z1t += 0.6; });
    const z2 = [[855, 400], [855, 480], [670, 480], [670, 560], [855, 560], [855, 640], [670, 640]];
    let z2t = t + 0.5; z2.forEach(function (q) { tl.to(r.robot2, { x: q[0], y: q[1], duration: 0.62, ease: 'power1.inOut' }, z2t); z2t += 0.6; });
    // 进度条
    tl.to(r.p1, { width: '86%', duration: 4.4, ease: 'none' }, t + 0.6);
    tl.to(r.p2, { width: '79%', duration: 4.4, ease: 'none' }, t + 0.6);
    tl.to(r.pv1, { duration: 4.4, innerText: 86, snap: { innerText: 1 }, ease: 'none', onUpdate: function () { r.pv1.textContent = Math.round(this.targets()[0].innerText || 0) + '% · 6.3m²'; } }, t + 0.6);
    tl.to(r.pv2, { duration: 4.4, innerText: 79, snap: { innerText: 1 }, ease: 'none', onUpdate: function () { r.pv2.textContent = Math.round(this.targets()[0].innerText || 0) + '% · 5.8m²'; } }, t + 0.6);
    // 承接下一章：点左侧「仪表视频」→ 展开 →「放大」→ 转场进入仪表章节
    t = a + 24.2;
    cursorShow(tl, t, centerOf(r.pipcap).x, centerOf(r.pipcap).y + 60);
    tapEl(tl, t + 0.6, r.pipcap, 0.6);
    tl.to(r.pipcap, { opacity: 0, duration: 0.3 }, t + 1.2);
    tl.fromTo(r.pipcard, { opacity: 0, scale: 0.9, transformOrigin: '0 0' }, { opacity: 1, scale: 1, duration: 0.5, ease: 'back.out(1.4)' }, t + 1.3);
    caption(tl, t + 0.7, 3.0, '<b>仪表视频</b> · 点「放大」进入', '独立链路：边导航边瞥表 → 放大看全屏识别');
    tapEl(tl, t + 2.6, r.pipmax, 0.6);
    cursorHide(tl, t + 3.4);
  }

  F.addScene(Object.assign({ id: '05-sec', dur: 4.2 }, sec('05', '一寸不漏地扫过', '全路径覆盖')));
  F.addScene({ id: '05-cover', dur: 23, build: function (root) { return buildCtl(root, 'fullpath'); }, anim: animCoverage });
  F.addScene(Object.assign({ id: '06-sec', dur: 4.2 }, sec('06', '多车，一起干', '鸿蒙软总线协同')));
  F.addScene({ id: '06-fleet', dur: 28, build: function (root) { return buildCtl(root, 'distributed'); }, anim: animFleet });
})();
