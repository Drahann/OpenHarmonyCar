/* 场景库 act 7：仪表识别 VisionPage。本章直接在全屏识别页演示（进入由上一章「多机」末尾点 PiP→放大 承接，
   开头由 ui-vision 介绍）。自包含。保留 .pip-card 等 CSS 供 scenes3 多机章节末尾的 PiP 卡使用。
   勿用 file-tool Edit/Write（会截断）——只用 bash heredoc 覆盖。 */
(function () {
  "use strict";
  const F = window.FILM;
  const centerOf = F.centerOf, cursorShow = F.cursorShow, cursorHide = F.cursorHide, tapEl = F.tapEl,
    caption = F.caption, addCSS = F.addCSS, I = F.I, STAGE_W = F.STAGE_W;

  addCSS(
    '.app-v{position:absolute;inset:0;background:var(--page-bg);padding:26px 34px;display:flex;flex-direction:column;}' +
    '.v-top{display:flex;align-items:center;}' +
    '.v-title{font-size:30px;font-weight:700;color:var(--text-title);}' +
    '.v-host{font-size:16px;color:var(--text-secondary);font-family:var(--font-mono);margin-top:2px;}' +
    '.v-conn{display:flex;align-items:center;gap:8px;font-size:18px;color:var(--text-secondary);}' +
    '.v-conn .dot{width:11px;height:11px;border-radius:50%;background:var(--success);}' +
    '.v-video{margin-top:16px;height:392px;border-radius:16px;background:radial-gradient(120% 120% at 50% 40%,#2a2d22,#16180f);position:relative;overflow:hidden;}' +
    '.v-pct{position:absolute;left:24px;top:20px;font-family:var(--font-mono);font-size:30px;font-weight:700;color:#fff;}' +
    '.v-pct small{font-size:16px;color:rgba(255,255,255,.7);font-weight:400;}' +
    '.v-stats{display:flex;gap:30px;margin-top:16px;}' +
    '.v-stat .sv{font-size:30px;font-weight:700;color:var(--text-title);font-family:var(--font-mono);}' +
    '.v-stat .sv small{font-size:15px;color:var(--text-secondary);font-weight:400;}' +
    '.v-stat .sl{font-size:14px;color:var(--text-secondary);}' +
    '.v-readings{display:flex;gap:14px;margin-top:14px;}' +
    '.rd{flex:1;background:var(--surface);border-radius:14px;box-shadow:var(--soft-shadow);padding:14px 18px;}' +
    '.rd .rl{font-size:16px;color:var(--text-secondary);}' +
    '.rd .rv{font-size:30px;font-weight:700;font-family:var(--font-mono);color:var(--text-body);margin-top:2px;}' +
    '.rd.alarm{outline:2px solid var(--danger);}.rd.alarm .rv{color:var(--danger);}' +
    '.rd .rs{font-size:13px;color:var(--text-caption);margin-top:2px;}' +
    '.v-report{margin-top:14px;background:var(--surface);border-radius:14px;box-shadow:var(--soft-shadow);padding:14px 18px;}' +
    '.v-rtop{display:flex;align-items:center;}.v-rtitle{font-size:22px;font-weight:700;color:var(--text-title);flex:1;}' +
    '.v-rbtn{height:46px;border-radius:999px;background:var(--primary);color:#fff;font-size:18px;font-weight:600;display:flex;align-items:center;justify-content:center;padding:0 20px;}' +
    '.v-rnote{font-size:15px;color:var(--text-secondary);margin-top:8px;}.v-rnote.busy{color:var(--warning);}' +
    '.v-rtext{font-size:15px;color:var(--text-body);margin-top:10px;background:var(--surface-muted);border-radius:10px;padding:12px;line-height:1.45;opacity:0;}' +
    /* 以下 PiP 卡样式供 scenes3「多机」章节末尾承接使用 */
    '.pip-card{position:absolute;left:40px;top:96px;width:236px;background:var(--surface);border-radius:16px;box-shadow:var(--card-shadow);padding:12px;z-index:7;opacity:0;}' +
    '.pip-card .ph{display:flex;align-items:center;font-size:16px;color:var(--text-body);margin-bottom:8px;}.pip-card .ph .pe{flex:1;}' +
    '.pip-card .pbtn{font-size:15px;color:var(--primary);border:1.5px solid var(--primary);border-radius:999px;padding:3px 12px;}' +
    '.pip-card .pclose{width:30px;height:30px;border-radius:50%;background:var(--surface-muted);display:flex;align-items:center;justify-content:center;margin-left:8px;color:var(--text-secondary);font-size:16px;}' +
    '.pip-vid{height:120px;border-radius:10px;background:radial-gradient(120% 120% at 50% 40%,#2a2d22,#16180f);}'
  );

  function setText(tl, at, el, txt, prev) { tl.to(el, { duration: 0.01, onComplete: function () { el.textContent = txt; }, onReverseComplete: function () { el.textContent = prev; } }, at); }
  function setClass(tl, at, el, cls, add) { tl.to(el, { duration: 0.01, onComplete: function () { el.classList[add ? 'add' : 'remove'](cls); }, onReverseComplete: function () { el.classList[add ? 'remove' : 'add'](cls); } }, at); }

  function gaugeSVG() {
    const cx = 616, cy = 196, R = 132; let ticks = '';
    for (let i = 0; i <= 10; i++) { const ang = (-210 + i * 24) * Math.PI / 180; const x1 = cx + Math.cos(ang) * (R - 4), y1 = cy + Math.sin(ang) * (R - 4), x2 = cx + Math.cos(ang) * (R - 20), y2 = cy + Math.sin(ang) * (R - 20); ticks += '<line x1="' + x1.toFixed(1) + '" y1="' + y1.toFixed(1) + '" x2="' + x2.toFixed(1) + '" y2="' + y2.toFixed(1) + '" stroke="#2b2f23" stroke-width="2.5"/>'; }
    const nAng = (-210 + 6.5 * 24) * Math.PI / 180; const nx = cx + Math.cos(nAng) * (R - 26), ny = cy + Math.sin(nAng) * (R - 26);
    const minAng = -210 * Math.PI / 180, maxAng = 30 * Math.PI / 180;
    const minx = cx + Math.cos(minAng) * (R - 12), miny = cy + Math.sin(minAng) * (R - 12), maxx = cx + Math.cos(maxAng) * (R - 12), maxy = cy + Math.sin(maxAng) * (R - 12);
    return '<svg viewBox="0 0 1232 392" preserveAspectRatio="xMidYMid meet" style="position:absolute;inset:0;width:100%;height:100%">' +
      '<circle cx="' + cx + '" cy="' + cy + '" r="' + (R + 14) + '" fill="#f3f2ea"/><circle cx="' + cx + '" cy="' + cy + '" r="' + R + '" fill="#fff" stroke="#dcd9cb" stroke-width="3"/>' + ticks +
      '<text x="' + cx + '" y="' + (cy + 64) + '" text-anchor="middle" font-size="20" font-family="monospace" fill="#6f7461">MPa</text>' +
      '<line x1="' + cx + '" y1="' + cy + '" x2="' + nx.toFixed(1) + '" y2="' + ny.toFixed(1) + '" stroke="#d9503f" stroke-width="5" stroke-linecap="round"/><circle cx="' + cx + '" cy="' + cy + '" r="9" fill="#2b2f23"/>' +
      '<rect class="detbox" x="' + (cx - R - 22) + '" y="' + (cy - R - 22) + '" width="' + (2 * R + 44) + '" height="' + (2 * R + 44) + '" rx="10" fill="none" stroke="#4678b8" stroke-width="3" opacity="0"/>' +
      '<text class="detlabel" x="' + (cx - R - 22) + '" y="' + (cy - R - 30) + '" font-size="16" font-family="monospace" fill="#4678b8" opacity="0">gauge 0.94</text>' +
      '<g class="kps">' + kp(cx, cy) + kp(nx, ny) + kp(minx, miny) + kp(maxx, maxy) + '</g></svg>';
  }
  function kp(x, y) { return '<g class="kpg" opacity="0"><circle cx="' + x.toFixed(1) + '" cy="' + y.toFixed(1) + '" r="6" fill="#dfa32f" stroke="#fff" stroke-width="2"/></g>'; }

  function buildVision(root) {
    root.classList.add('paper');
    const cam = document.createElement('div'); cam.className = 'cam'; root.appendChild(cam);
    const T = document.createElement('div'); T.className = 'tablet';
    const TW = 1300, TH = 812, TX = (STAGE_W - TW) / 2, TY = 128;
    T.style.cssText += 'left:' + TX + 'px;top:' + TY + 'px;width:' + TW + 'px;height:' + TH + 'px;';
    T.innerHTML = '<div class="cam-dot"></div>';
    const screen = document.createElement('div'); screen.className = 'screen';
    const vis = document.createElement('div'); vis.className = 'app-v';
    vis.innerHTML =
      '<div class="v-top"><div class="ctl-back" style="box-shadow:none;background:transparent">' + I.back + '</div><div style="margin-left:10px"><div class="v-title">仪表识别</div><div class="v-host">192.168.43.66:8000</div></div><div style="flex:1"></div><div class="v-conn"><span class="dot"></span>已连接</div></div>' +
      '<div class="v-video"><div class="v-pct">65.0 <small>%</small></div>' + gaugeSVG() + '</div>' +
      '<div class="v-stats"><div class="v-stat"><div class="sv"><span data-fps>0</span> <small>fps</small></div><div class="sl">帧率</div></div><div class="v-stat"><div class="sv"><span data-inf>0</span> <small>ms</small></div><div class="sl">端到端推理</div></div><div class="v-stat"><div class="sv"><span data-det>0</span></div><div class="sl">检测数</div></div></div>' +
      '<div class="v-readings"><div class="rd"><div class="rl">表 1</div><div class="rv">0.62 MPa</div><div class="rs">仪表 1 · 正常</div></div><div class="rd alarm" data-alarm style="outline-color:transparent"><div class="rl">表 2</div><div class="rv">0.86 MPa</div><div class="rs">仪表 2 · 超上限告警</div></div></div>' +
      '<div class="v-report"><div class="v-rtop"><div class="v-rtitle">分析报告</div><div class="v-rbtn" data-rbtn>生成报告</div></div><div class="v-rnote" data-rnote>基于最近 24h 读数生成趋势 / 异常分析（调用香橙派 DeepSeek）。</div><div class="v-rtext" data-rtext>表2 压力 0.86MPa 超上限 0.80，近 6h 持续上行，建议巡检阀门 V-12；其余仪表读数平稳，处正常区间。</div></div>';
    screen.appendChild(vis); T.appendChild(screen); cam.appendChild(T);
    const q = function (s) { return vis.querySelector(s); };
    return { cam: cam, T: T, detbox: q('.detbox'), detlabel: q('.detlabel'), kps: Array.prototype.slice.call(vis.querySelectorAll('.kpg')), fps: q('[data-fps]'), inf: q('[data-inf]'), det: q('[data-det]'), alarm: q('[data-alarm]'), readings: q('.v-readings'), rbtn: q('[data-rbtn]'), rnote: q('[data-rnote]'), rtext: q('[data-rtext]') };
  }

  function animVision(tl, s) {
    const a = s.start, r = s.refs;
    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' }, { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    cursorHide(tl, a + 0.2);
    caption(tl, a + 0.6, 3.2, '<b>YOLOv5s ＋ 关键点 · 昇腾 NPU</b>', '端到端 ~40ms · ~15FPS（上一章点 PiP「放大」进入）');
    // 检测框 + 4 关键点 + 统计 count-up
    let t = a + 1.8;
    tl.to(r.detbox, { opacity: 1, duration: 0.5 }, t);
    tl.to(r.detlabel, { opacity: 1, duration: 0.5 }, t + 0.2);
    r.kps.forEach(function (k, i) { tl.fromTo(k, { opacity: 0 }, { opacity: 1, duration: 0.3, ease: 'power2.out' }, t + 0.6 + i * 0.25); });
    caption(tl, t + 0.4, 3.0, '<b>4 关键点</b>', '表盘中心 · 指针尖 · 量程 min / max → 反算指针角度');
    tl.to(r.fps, { duration: 1.2, innerText: 15, snap: { innerText: 1 }, ease: 'power2.out' }, t + 0.6);
    tl.to(r.inf, { duration: 1.2, innerText: 38, snap: { innerText: 1 }, ease: 'power2.out' }, t + 0.6);
    tl.to(r.det, { duration: 1.2, innerText: 2, snap: { innerText: 1 }, ease: 'power2.out' }, t + 0.6);
    // 读数 + 告警
    t = a + 5.6;
    caption(tl, t + 0.3, 3.0, '<b>实时读数 ＋ 告警</b>', '表2 超上限，卡片描红');
    tl.fromTo(r.alarm, { outlineColor: 'rgba(217,80,63,0)' }, { outlineColor: 'rgba(217,80,63,1)', duration: 0.5, repeat: 1, yoyo: true }, t + 0.5);
    tl.to(r.alarm, { outlineColor: 'rgba(217,80,63,1)', duration: 0.3 }, t + 1.7);
    // 生成报告
    t = a + 9.2;
    cursorShow(tl, t - 0.6, centerOf(r.rbtn).x, centerOf(r.rbtn).y + 60);
    tapEl(tl, t, r.rbtn, 0.6);
    setText(tl, t + 0.6, r.rbtn, '生成中…', '生成报告');
    tl.to(r.rbtn, { backgroundColor: '#9b9d8d', duration: 0.3 }, t + 0.6);
    setClass(tl, t + 0.6, r.rnote, 'busy', true);
    setText(tl, t + 0.6, r.rnote, '生成中：DeepSeek 占用 NPU，香橙派暂停摄像头，视频与读数短时停顿，完成后自动恢复。', '基于最近 24h 读数生成趋势 / 异常分析（调用香橙派 DeepSeek）。');
    caption(tl, t + 0.5, 3.4, '<b>DeepSeek 趋势 / 异常分析</b>', '生成期间香橙派暂停摄像头释放 NPU');
    tl.to(r.rtext, { opacity: 1, duration: 0.5 }, t + 2.8);
    setClass(tl, t + 2.8, r.rnote, 'busy', false);
    setText(tl, t + 2.8, r.rnote, '基于最近 24h 读数生成趋势 / 异常分析（调用香橙派 DeepSeek）。', '生成中：DeepSeek 占用 NPU，香橙派暂停摄像头，视频与读数短时停顿，完成后自动恢复。');
    setText(tl, t + 2.8, r.rbtn, '生成报告', '生成中…');
    tl.to(r.rbtn, { backgroundColor: '#485c11', duration: 0.3 }, t + 2.8);
    cursorHide(tl, t + 3.6);
  }

  F.addScene({ id: '07-vision', dur: 14, build: buildVision, anim: animVision });
})();
