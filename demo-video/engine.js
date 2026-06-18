/* OpenHarmony 工业巡检机器人 · 演示片引擎（FILM core）。GSAP 主时间轴(paused, 可 seek 导出)。
   注意：本挂载 file-tool Edit 会截断——只用 Write/bash 整文件覆盖；新增场景另建 scenesN.js。 */
window.FILM = (function () {
  "use strict";
  const STAGE_W = 1920, STAGE_H = 1080, TRANS = 0.6, SPEED = 1; // SPEED：全片整体倍速（1=原速/作者意图节奏；2=时长减半）。用户要求回到 1×。
  let stage, scenesEl, captionEl, cursor, ripple, tl, TOTAL = 0;
  let _lastCap = null, _lastCapEnd = 0;   // 字幕单条不重叠：新字幕出现即清掉仍在显示的上一条
  let curCam = null, orderIds = null;
  const scenes = [];

  const I = {
    trust: '<svg viewBox="0 0 24 24" fill="none" stroke="#485c11" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><circle cx="9" cy="8" r="3"/><path d="M3.5 19a5.5 5.5 0 0 1 11 0"/><circle cx="17" cy="9" r="2.2"/><path d="M15 19a4.5 4.5 0 0 1 6.5-4"/></svg>',
    video: '<svg viewBox="0 0 24 24" fill="none" stroke="#485c11" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="6" width="13" height="12" rx="2.4"/><path d="M16 10l5-3v10l-5-3z"/></svg>',
    search: '<svg viewBox="0 0 24 24" fill="none" stroke="#485c11" stroke-width="2" stroke-linecap="round"><circle cx="11" cy="11" r="7"/><path d="M20 20l-3.2-3.2"/></svg>',
    fwd: '<svg viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M9 6l6 6-6 6"/></svg>',
    back: '<svg viewBox="0 0 24 24" fill="none" stroke="#485c11" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M15 5l-7 7 7 7"/></svg>'
  };

  function fit() { if (!stage) return; const s = Math.min(window.innerWidth / STAGE_W, window.innerHeight / STAGE_H); stage.style.transform = 'scale(' + s + ')'; }
  function addCSS(css) { const el = document.createElement('style'); el.textContent = css; document.head.appendChild(el); }

  function centerOf(el) {
    let x = 0, y = 0, n = el;
    while (n && n !== stage) {
      x += n.offsetLeft; y += n.offsetTop;
      if (n.style && !n.style.transform) {
        const cs = (window.getComputedStyle ? getComputedStyle(n).transform : '');
        if (cs && cs !== 'none' && window.DOMMatrixReadOnly) { try { const m = new DOMMatrixReadOnly(cs); x += m.e; y += m.f; } catch (e) {} }
      }
      n = n.offsetParent;
    }
    return { x: x + el.offsetWidth / 2, y: y + el.offsetHeight / 2 };
  }
  function camAncestor(el) { let n = el; while (n && n !== stage) { if (n.classList && n.classList.contains('cam')) return n; n = n.parentNode; } return null; }
  function camApply(cam, p) { if (!cam) return p; const s = cam._cs == null ? 1 : cam._cs, x = cam._cx == null ? 0 : cam._cx, y = cam._cy == null ? 0 : cam._cy; return { x: x + s * p.x, y: y + s * p.y }; }
  function screenOfEl(el) { return camApply(camAncestor(el), centerOf(el)); }
  function screenOfPt(p) { return camApply(curCam, p); }

  function rise(tl, el, pos, opt) { opt = opt || {}; const y = opt.y == null ? 28 : opt.y, d = opt.dur == null ? 0.9 : opt.dur; tl.fromTo(el, { opacity: 0, y: y, filter: 'blur(12px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: d, ease: 'power4.out' }, pos); }
  function fade(tl, el, pos, to, d) { tl.to(el, { opacity: to, duration: d == null ? 0.5 : d, ease: 'power2.out' }, pos); }
  function pop(tl, el, pos, opt) { opt = opt || {}; const d = opt.dur == null ? 0.7 : opt.dur; tl.fromTo(el, { opacity: 0, scale: opt.from == null ? 0.6 : opt.from }, { opacity: 1, scale: 1, duration: d, ease: 'back.out(1.5)' }, pos); }

  function camTo(tl, cam, pos, px, py, scale, d) {
    d = d == null ? 1.1 : d;
    const cx = STAGE_W / 2 - scale * px, cy = STAGE_H / 2 - scale * py;
    cam._cs = scale; cam._cx = cx; cam._cy = cy; curCam = cam;
    tl.to(cam, { scale: scale, x: cx, y: cy, transformOrigin: '0 0', duration: d, ease: 'power3.inOut' }, pos);
  }
  function camReset(tl, cam, pos, d) {
    cam._cs = 1; cam._cx = 0; cam._cy = 0; curCam = cam;
    tl.to(cam, { scale: 1, x: 0, y: 0, transformOrigin: '0 0', duration: d == null ? 1.0 : d, ease: 'power3.inOut' }, pos);
  }

  function cursorShow(tl, pos, x, y) { gsap.set(cursor, { x: x, y: y }); tl.to(cursor, { opacity: 1, duration: 0.4 }, pos); }
  function cursorHide(tl, pos) { tl.to(cursor, { opacity: 0, duration: 0.4 }, pos); }
  function cursorMove(tl, pos, x, y, d) { tl.to(cursor, { x: x, y: y, duration: d == null ? 0.6 : d, ease: 'power3.inOut' }, pos); }
  function rippleAt(tl, at, x, y) { tl.set(ripple, { x: x, y: y, scale: 0.4, opacity: 0.5 }, at); tl.to(ripple, { scale: 3.2, opacity: 0, duration: 0.55, ease: 'power2.out' }, at); }
  function press(tl, at) { tl.to(cursor, { scale: 0.82, duration: 0.09, ease: 'power2.in' }, at); tl.to(cursor, { scale: 1, duration: 0.22, ease: 'power2.out' }, at + 0.09); }
  function cursorTapAt(tl, pos, x, y, moveDur) { const sp = screenOfPt({ x: x, y: y }); cursorMove(tl, pos, sp.x, sp.y, moveDur); const at = pos + (moveDur == null ? 0.6 : moveDur); press(tl, at); rippleAt(tl, at, sp.x, sp.y); return at; }
  function tapEl(tl, pos, el, moveDur) {
    const c = centerOf(el), sp = camApply(camAncestor(el), c);
    cursorMove(tl, pos, sp.x, sp.y, moveDur);
    const at = pos + (moveDur == null ? 0.6 : moveDur);
    press(tl, at); rippleAt(tl, at, sp.x, sp.y);
    tl.fromTo(el, { scale: 1 }, { scale: 0.97, duration: 0.09, yoyo: true, repeat: 1, ease: 'power2.inOut', transformOrigin: '50% 50%' }, at);
    return c;
  }

  function caption(tl, pos, dur, mainHtml, subHtml) {
    // 单条不重叠：新字幕出现前，强制上一条在本条升起前淡出完（含其仍在进行的淡出尾巴，避免两条在屏上交叠）
    if (_lastCap && pos < _lastCapEnd + 0.55) tl.to(_lastCap, { opacity: 0, y: -10, filter: 'blur(6px)', duration: 0.3, ease: 'power2.in' }, Math.max(0, pos - 0.3));
    const d = document.createElement('div'); d.className = 'cap';
    d.innerHTML = '<div class="cap-main">' + mainHtml + '</div>' + (subHtml ? '<div class="cap-sub">' + subHtml + '</div>' : '');
    captionEl.appendChild(d);
    tl.fromTo(d, { opacity: 0, y: 16, filter: 'blur(8px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: 0.6, ease: 'power3.out' }, pos);
    tl.to(d, { opacity: 0, y: -10, filter: 'blur(6px)', duration: 0.5, ease: 'power2.in' }, pos + dur);
    _lastCap = d; _lastCapEnd = pos + dur;
  }

  /* 屏上标注：标签 + 引线指向某元素/舞台点（callout）；屏角信息板（infoPanel）。
     坐标走 screenOf*（含相机换算）；元素请在相机未缩放(或刚 reset)时标注，避免锚点漂移。 */
  function callout(tl, pos, dur, target, mainText, opts) {
    opts = opts || {};
    const an = (target && target.nodeType) ? screenOfEl(target) : screenOfPt(target);
    const dx = opts.dx == null ? 150 : opts.dx, dy = opts.dy == null ? -40 : opts.dy;
    const lx = an.x + dx, ly = an.y + dy, leftSide = dx < 0;
    const wrap = document.createElement('div'); wrap.className = 'callout';
    wrap.innerHTML =
      '<svg viewBox="0 0 1920 1080" preserveAspectRatio="none"><line class="co-line" x1="' + lx + '" y1="' + (ly + 22) + '" x2="' + an.x + '" y2="' + an.y + '"/><circle class="co-dot" cx="' + an.x + '" cy="' + an.y + '" r="5"/></svg>' +
      '<div class="co-label" style="left:' + lx + 'px;top:' + ly + 'px;' + (leftSide ? 'transform:translateX(-100%);' : '') + '">' + mainText + (opts.sub ? '<span class="co-sub">' + opts.sub + '</span>' : '') + '</div>';
    stage.appendChild(wrap);
    tl.fromTo(wrap, { opacity: 0 }, { opacity: 1, duration: 0.5, ease: 'power3.out' }, pos);
    tl.to(wrap, { opacity: 0, duration: 0.45, ease: 'power2.in' }, pos + dur);
    return wrap;
  }
  function infoPanel(tl, pos, dur, corner, titleHtml, linesHtml) {
    const p = document.createElement('div'); p.className = 'infopanel ' + (corner || 'tr');
    p.innerHTML = (titleHtml ? '<div class="ip-title">' + titleHtml + '</div>' : '') + (linesHtml || '');
    stage.appendChild(p);
    tl.fromTo(p, { opacity: 0, y: 14 }, { opacity: 1, y: 0, duration: 0.5, ease: 'power3.out' }, pos);
    tl.to(p, { opacity: 0, duration: 0.45, ease: 'power2.in' }, pos + dur);
    return p;
  }
  function injectAnnotCSS() {
    addCSS(
      '.callout{position:absolute;inset:0;pointer-events:none;z-index:46;opacity:0;}' +
      '.callout svg{position:absolute;inset:0;width:1920px;height:1080px;overflow:visible;}' +
      '.co-line{stroke:#485c11;stroke-width:2;stroke-dasharray:3 4;opacity:.8;}' +
      '.co-dot{fill:#485c11;}' +
      '.co-label{position:absolute;background:var(--surface);border:1.5px solid var(--primary);color:var(--text-title);font-size:22px;font-weight:700;padding:8px 15px;border-radius:11px;box-shadow:var(--soft-shadow);white-space:nowrap;}' +
      '.co-label .co-sub{display:block;font-size:16px;color:var(--text-secondary);font-weight:400;margin-top:2px;}' +
      '.infopanel{position:absolute;background:rgba(255,255,255,.94);border:1px solid var(--border);border-radius:14px;padding:16px 20px;box-shadow:var(--card-shadow);z-index:46;max-width:540px;opacity:0;}' +
      '.infopanel.tr{right:80px;top:96px;}.infopanel.tl{left:80px;top:96px;}.infopanel.bl{left:80px;bottom:172px;}.infopanel.br{right:80px;bottom:172px;}' +
      '.ip-title{font-size:24px;font-weight:700;color:var(--text-title);margin-bottom:8px;}' +
      '.ip-line{font-size:19px;color:var(--text-body);margin:5px 0;}' +
      '.ip-line b{color:var(--primary);}'
    );
  }

  function addScene(s) { scenes.push(s); }
  function setOrder(ids) { orderIds = ids; }

  function boot() {
    stage = document.getElementById('stage'); scenesEl = document.getElementById('scenes');
    captionEl = document.getElementById('caption'); cursor = document.getElementById('cursor'); ripple = document.getElementById('ripple');
    if (window.MotionPathPlugin) gsap.registerPlugin(MotionPathPlugin);
    injectAnnotCSS();
    fit(); window.addEventListener('resize', fit);
    if (window.ResizeObserver) { try { new ResizeObserver(function () { fit(); }).observe(document.documentElement); } catch (e) {} }
    setTimeout(fit, 50); setTimeout(fit, 250); setTimeout(fit, 800);
    // 显式重排 + 过滤（未列入 orderIds 的场景被丢弃 → 去章节卡）
    if (orderIds && orderIds.length) {
      const byId = {}; scenes.forEach(function (s) { byId[s.id] = s; });
      const ordered = []; orderIds.forEach(function (id) { if (byId[id]) ordered.push(byId[id]); });
      if (ordered.length) { scenes.length = 0; Array.prototype.push.apply(scenes, ordered); }
    }
    tl = gsap.timeline({ paused: true });
    let start = 0;
    scenes.forEach(function (s, i) {
      s.root = document.createElement('section'); s.root.className = 'scene'; s.root.dataset.id = s.id;
      scenesEl.appendChild(s.root); s.refs = s.build(s.root); s.start = start;
      start += s.dur - (i < scenes.length - 1 ? TRANS : 0);
    });
    TOTAL = start;
    scenes.forEach(function (s, i) {
      tl.fromTo(s.root, { opacity: 0 }, { opacity: 1, duration: TRANS, ease: 'power2.inOut' }, s.start);
      tl.fromTo(s.root, { scale: 1.035 }, { scale: 1.0, duration: TRANS * 1.6, ease: 'power3.out' }, s.start);
      if (i < scenes.length - 1) {
        tl.to(s.root, { opacity: 0, duration: TRANS, ease: 'power2.inOut' }, s.start + s.dur - TRANS);
        tl.to(s.root, { scale: 0.992, duration: TRANS, ease: 'power2.in' }, s.start + s.dur - TRANS);
      }
    });
    scenes.forEach(function (s) { curCam = null; if (s.anim) s.anim(tl, s); });
    tl.timeScale(SPEED);
    setupControls();
    console.log('[film] ' + scenes.map(function (s) { return s.id + ':' + s.dur + 's'; }).join(' | ') + '  total=' + TOTAL.toFixed(1) + 's');
  }

  function setupControls() {
    const playBtn = document.getElementById('playBtn'), tlEl = document.getElementById('tl'),
      tlfill = document.getElementById('tlfill'), tc = document.getElementById('tc'),
      scl = document.getElementById('scl'), chrome = document.getElementById('chrome'), hint = document.getElementById('hint');
    scenes.forEach(function (s) { const m = document.createElement('div'); m.className = 'tlmark'; m.style.left = (s.start / TOTAL * 100) + '%'; tlEl.appendChild(m); });
    function sceneAt(t) { let cur = scenes[0]; for (const s of scenes) { if (t >= s.start) cur = s; } return cur; }
    function ui() {
      const it = tl.time(); tlfill.style.width = (it / TOTAL * 100) + '%';
      tc.textContent = (it / SPEED).toFixed(1) + ' / ' + (TOTAL / SPEED).toFixed(1) + 's';
      const s = sceneAt(it); scl.textContent = s.id + '  (' + ((it - s.start) / SPEED).toFixed(1) + 's)';
      playBtn.textContent = tl.paused() ? '▶' : '❚❚';
    }
    gsap.ticker.add(ui);
    function toggle() { tl.paused() ? tl.play() : tl.pause(); }
    playBtn.onclick = toggle;
    tlEl.onclick = function (e) { const r = tlEl.getBoundingClientRect(); tl.pause(); tl.time((e.clientX - r.left) / r.width * TOTAL); ui(); };
    document.addEventListener('keydown', function (e) {
      if (e.code === 'Space') { e.preventDefault(); toggle(); }
      else if (e.code === 'ArrowRight') { tl.pause(); tl.time(Math.min(TOTAL, tl.time() + SPEED)); ui(); }
      else if (e.code === 'ArrowLeft') { tl.pause(); tl.time(Math.max(0, tl.time() - SPEED)); ui(); }
      else if (e.key === 'r' || e.key === 'R') { tl.pause(); tl.time(0); tl.play(); }
    });
    const q = new URLSearchParams(location.search);
    window.__duration = TOTAL / SPEED;
    window.__seek = function (t) { tl.pause(); tl.time(t * SPEED); ui(); };
    window.__tl = tl; window.__scenes = scenes;
    if (q.get('record') === '1') { chrome.classList.add('hidden'); if (hint) hint.style.display = 'none'; }
    if (q.has('scene')) { const id = q.get('scene'); const s = scenes.find(function (x) { return x.id === id || x.id.indexOf(id) >= 0; }); if (s) tl.time(s.start + 0.01); }
    if (q.get('play') === '1') tl.play();
    ui();
  }

  return {
    addScene: addScene, setOrder: setOrder, boot: boot, rise: rise, fade: fade, pop: pop, centerOf: centerOf,
    camTo: camTo, camReset: camReset, cursorShow: cursorShow, cursorHide: cursorHide,
    cursorMove: cursorMove, cursorTapAt: cursorTapAt, tapEl: tapEl, caption: caption,
    callout: callout, infoPanel: infoPanel, addCSS: addCSS,
    screenOfEl: screenOfEl, screenOfPt: screenOfPt, I: I, STAGE_W: STAGE_W, STAGE_H: STAGE_H
  };
})();
