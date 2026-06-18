/* scenes_fx.js —— 动效与文字工具箱（window.FX）。
   解决：① 出现动画不再只有"渐入"——提供 slide/wipe(裁切揭示)/draw(描线)/spring/pop/char(逐字) 等多种入场；
        ② 文字不再永远困在屏幕底部——`note()` 可把说明放到屏幕任意 (x,y)，自带多种入场，作者自行错位避免重叠。
   依赖 window.FILM（引擎已先加载，GSAP 可用）。被 scenes_principles / scenes_intro 复用。
   ⚠ 本挂载 file-tool Edit 会截断——改本文件只用 Write/bash 整文件覆盖。 */
window.FX = (function () {
  "use strict";
  const F = window.FILM;
  let _stage = null, _curRoot = null;
  function stage() { return _stage || (_stage = document.getElementById('stage')); }
  function scene(root) { _curRoot = root; }   // 把后续 note 挂到当前场景根(随场景淡出，不泄漏到后续场景)

  F.addCSS(
    '.fxn-wrap{position:absolute;z-index:45;pointer-events:none;}' +
    '.fxn-wrap.a-c{transform:translate(-50%,-50%);}.fxn-wrap.a-t{transform:translate(-50%,0);}.fxn-wrap.a-b{transform:translate(-50%,-100%);}' +
    '.fxn-wrap.a-r{transform:translate(-100%,0);}.fxn-wrap.a-rb{transform:translate(-100%,-100%);}.fxn-wrap.a-l{transform:translate(0,0);}.fxn-wrap.a-lb{transform:translate(0,-100%);}' +
    '.fxn{opacity:0;will-change:transform,opacity,filter;}' +
    '.fxn-kicker{font-family:var(--font-mono);font-size:21px;font-weight:600;letter-spacing:.26em;color:var(--ac,var(--primary));text-transform:uppercase;}' +
    '.fxn-title{font-size:46px;font-weight:800;color:var(--text-title);letter-spacing:-.025em;line-height:1.12;}' +
    '.fxn-title b{color:var(--primary);}' +
    '.fxn-h{font-size:31px;font-weight:800;color:var(--ac,var(--text-title));letter-spacing:-.01em;line-height:1.3;}' +
    '.fxn-h b{color:var(--text-title);}' +
    '.fxn-body{font-size:24px;font-weight:600;color:var(--text-body);line-height:1.42;}.fxn-body b{color:var(--primary);}' +
    '.fxn-sub{font-size:23px;font-weight:600;color:var(--ac,var(--text-body));line-height:1.46;}.fxn-sub b{color:var(--text-title);font-weight:800;}' +
    '.fxn-mono{font-family:var(--font-mono);font-size:22px;font-weight:600;color:var(--ac,var(--text-body));letter-spacing:.01em;}.fxn-mono b{color:var(--text-title);font-weight:800;}' +
    '.fxn-tag{display:inline-block;font-size:19px;font-weight:700;color:#fff;background:var(--ac,var(--primary));border-radius:999px;padding:8px 19px;box-shadow:0 7px 18px -7px rgba(40,46,14,.55);}' +
    '.fxn-chip{display:inline-block;font-size:20px;font-weight:700;color:var(--ac,var(--primary));background:var(--surface);border:2px solid var(--ac,var(--primary));border-radius:999px;padding:8px 19px;box-shadow:var(--soft-shadow);}' +
    '.fxn-num{font-family:var(--font-mono);font-size:54px;font-weight:700;color:var(--ac,var(--primary));letter-spacing:-.02em;line-height:1;}' +
    '.fxn-num .u{font-size:26px;color:var(--text-secondary);font-weight:600;}' +
    '.fxn-card{background:rgba(255,255,255,.97);border:1.5px solid var(--ac,var(--border));border-radius:16px;padding:16px 20px;box-shadow:var(--card-shadow);}' +
    '.fxn-card .ct{font-size:23px;font-weight:800;color:var(--text-title);}.fxn-card .cs{font-size:18px;font-weight:600;color:var(--text-secondary);margin-top:6px;line-height:1.5;}.fxn-card .cs b{color:var(--primary);}'
  );

  /* ── 多样入场 ── el 可为 HTML/SVG。style: rise/fall/left/right/spring/pop/wipeUp/wipeDown/wipeRight/wipeLeft/draw/zoomBlur ── */
  function enter(tl, el, pos, style, o) {
    o = o || {}; const d = o.dur || 0.8, e = o.ease, dist = o.dist;
    switch (style) {
      case 'rise': tl.fromTo(el, { opacity: 0, y: dist == null ? 28 : dist, filter: 'blur(10px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: d, ease: e || 'power4.out' }, pos); break;
      case 'fall': tl.fromTo(el, { opacity: 0, y: -(dist == null ? 28 : dist), filter: 'blur(10px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: d, ease: e || 'power4.out' }, pos); break;
      case 'left': tl.fromTo(el, { opacity: 0, x: -(dist == null ? 46 : dist), filter: 'blur(8px)' }, { opacity: 1, x: 0, filter: 'blur(0px)', duration: d, ease: e || 'power3.out' }, pos); break;
      case 'right': tl.fromTo(el, { opacity: 0, x: (dist == null ? 46 : dist), filter: 'blur(8px)' }, { opacity: 1, x: 0, filter: 'blur(0px)', duration: d, ease: e || 'power3.out' }, pos); break;
      case 'spring': tl.fromTo(el, { opacity: 0, scale: o.from == null ? 0.55 : o.from, transformOrigin: o.origin || '50% 50%' }, { opacity: 1, scale: 1, duration: d, ease: e || 'back.out(1.7)' }, pos); break;
      case 'pop': tl.fromTo(el, { opacity: 0, scale: 1.14, filter: 'blur(9px)', transformOrigin: o.origin || '50% 50%' }, { opacity: 1, scale: 1, filter: 'blur(0px)', duration: d, ease: e || 'power3.out' }, pos); break;
      case 'zoomBlur': tl.fromTo(el, { opacity: 0, scale: o.from == null ? 0.82 : o.from, filter: 'blur(14px)', transformOrigin: o.origin || '50% 50%' }, { opacity: 1, scale: 1, filter: 'blur(0px)', duration: d, ease: e || 'power3.out' }, pos); break;
      case 'wipeUp': tl.set(el, { opacity: 1 }, pos); tl.fromTo(el, { clipPath: 'inset(100% 0% 0% 0%)' }, { clipPath: 'inset(0% 0% 0% 0%)', duration: d, ease: e || 'power3.inOut' }, pos); break;
      case 'wipeDown': tl.set(el, { opacity: 1 }, pos); tl.fromTo(el, { clipPath: 'inset(0% 0% 100% 0%)' }, { clipPath: 'inset(0% 0% 0% 0%)', duration: d, ease: e || 'power3.inOut' }, pos); break;
      case 'wipeRight': tl.set(el, { opacity: 1 }, pos); tl.fromTo(el, { clipPath: 'inset(0% 100% 0% 0%)' }, { clipPath: 'inset(0% 0% 0% 0%)', duration: d, ease: e || 'power3.inOut' }, pos); break;
      case 'wipeLeft': tl.set(el, { opacity: 1 }, pos); tl.fromTo(el, { clipPath: 'inset(0% 0% 0% 100%)' }, { clipPath: 'inset(0% 0% 0% 0%)', duration: d, ease: e || 'power3.inOut' }, pos); break;
      case 'draw': tl.set(el, { opacity: 1 }, pos); tl.fromTo(el, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: d, ease: e || 'power2.inOut' }, pos); break;
      default: tl.fromTo(el, { opacity: 0 }, { opacity: 1, duration: d }, pos);
    }
  }
  function enterEach(tl, els, pos, style, step, o) { Array.prototype.forEach.call(els, function (el, i) { enter(tl, el, pos + i * (step == null ? 0.08 : step), style, o); }); }
  function exit(tl, el, pos, style, d) {
    d = d || 0.45;
    switch (style) {
      case 'down': tl.to(el, { opacity: 0, y: 12, filter: 'blur(6px)', duration: d, ease: 'power2.in' }, pos); break;
      case 'scale': tl.to(el, { opacity: 0, scale: 0.92, filter: 'blur(6px)', duration: d, ease: 'power2.in' }, pos); break;
      case 'wipe': tl.to(el, { clipPath: 'inset(0% 0% 100% 0%)', duration: d, ease: 'power3.in' }, pos); break;
      default: tl.to(el, { opacity: 0, y: -10, filter: 'blur(6px)', duration: d, ease: 'power2.in' }, pos);
    }
  }
  /* 逐字入场（仅纯文本元素）。返回 span 数组。 */
  function chars(tl, el, pos, o) {
    o = o || {}; const txt = el.textContent; el.textContent = ''; const sps = [];
    txt.split('').forEach(function (c) { const sp = document.createElement('span'); sp.textContent = c === ' ' ? ' ' : c; sp.style.display = 'inline-block'; sp.style.willChange = 'transform,opacity,filter'; el.appendChild(sp); sps.push(sp); });
    tl.fromTo(sps, { opacity: 0, y: o.dist == null ? 20 : o.dist, filter: 'blur(8px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: o.dur || 0.6, ease: o.ease || 'power4.out', stagger: o.step == null ? 0.03 : o.step }, pos);
    return sps;
  }

  /* ── 任意位置文字说明 ── note(tl,pos,dur,x,y,html,{style,anchor,enter,exit,w,accent,inDur}) ── */
  function note(tl, pos, dur, x, y, html, o) {
    o = o || {};
    const wrap = document.createElement('div'); wrap.className = 'fxn-wrap' + (o.anchor ? ' a-' + o.anchor : '');
    wrap.style.cssText = 'left:' + x + 'px;top:' + y + 'px;' + (o.w ? 'width:' + o.w + 'px;' : '');
    const inner = document.createElement('div'); inner.className = 'fxn fxn-' + (o.style || 'body');
    if (o.accent) inner.style.setProperty('--ac', o.accent);
    inner.innerHTML = html; wrap.appendChild(inner); (_curRoot || stage()).appendChild(wrap);
    enter(tl, inner, pos, o.enter || 'rise', { dur: o.inDur || 0.6 });
    if (dur != null && dur > 0) exit(tl, inner, pos + dur, o.exit || 'up');   // dur<=0 → 常驻(随场景淡出)
    return { wrap: wrap, inner: inner };
  }
  /* 整数 count-up（可 seek）。 */
  function countUp(tl, at, dur, el, to, from, fmt) {
    from = from || 0; const o = { n: from };
    tl.to(o, { n: to, duration: dur, ease: 'power2.out', onUpdate: function () { el.textContent = fmt ? fmt(o.n) : String(Math.round(o.n)); }, onReverseComplete: function () { el.textContent = fmt ? fmt(from) : String(from); } }, at);
  }
  function pulse(tl, at, el, sc) { if (el) tl.to(el, { scale: sc || 1.08, duration: 0.34, yoyo: true, repeat: 1, ease: 'power2.inOut', transformOrigin: '50% 50%' }, at); }

  return { scene: scene, enter: enter, enterEach: enterEach, exit: exit, chars: chars, note: note, countUp: countUp, pulse: pulse };
})();
