/* scenes_diagkit.js —— 演示片"逻辑图工具箱"（window.DIAGKIT）。
   把《产品说明书》里的 Mermaid 流程图/时序图/状态机/坐标关系图，渲染成发布会质感的可逐帧 seek 动画。
   渲染：SVG 画形状(圆角矩形/判定菱形/胶囊)+ 箭头(marker) + 泳道；HTML 叠加清晰文字。
   揭示：确定性"逻辑流"——节点按作者数组顺序错峰浮现(位移+模糊→清晰)，每条边随其目标节点画出(实线 dashoffset 描绘 / 虚线整体淡入)。
   与色板同源(film.css :root)；类名独立(dk-*)不与其它场景冲突。
   依赖：window.FILM(引擎已先加载)。被 scenes_intro / scenes_principles 复用。
   ⚠ 本挂载 file-tool Edit 会截断——改本文件只用 Write/bash 整文件覆盖。 */
window.DIAGKIT = (function () {
  "use strict";
  const F = window.FILM, W = F.STAGE_W, H = F.STAGE_H;

  /* tone → {fill 浅底, stroke 描边/箭头, ink 文字, mark 箭头色名} */
  const TONE = {
    app: { fill: '#eef2dd', stroke: '#485c11', ink: '#2b2f23', mark: 'green' },
    pi: { fill: '#e6edcb', stroke: '#3a4a0e', ink: '#2b2f23', mark: 'pidark' },
    vision: { fill: '#f7ead0', stroke: '#b6831f', ink: '#4a3c19', mark: 'warn' },
    info: { fill: '#dde8f4', stroke: '#4678b8', ink: '#244468', mark: 'blue' },
    neutral: { fill: '#ffffff', stroke: '#cfccbd', ink: '#2b2f23', mark: 'gray' },
    soft: { fill: '#e4ebc8', stroke: '#aeb977', ink: '#3a4012', mark: 'green' },
    danger: { fill: '#f6ddd7', stroke: '#d9503f', ink: '#7a2c22', mark: 'red' },
    ink: { fill: '#23271a', stroke: '#23271a', ink: '#ffffff', mark: 'ink' }
  };
  const MARK = { green: '#485c11', pidark: '#3a4a0e', blue: '#4678b8', gray: '#9a9888', warn: '#b6831f', red: '#d9503f', ink: '#23271a' };

  F.addCSS(
    '.dk{position:absolute;inset:0;}' +
    '.dk-svg{position:absolute;inset:0;width:100%;height:100%;overflow:visible;}' +
    '.dk-fig{position:absolute;left:96px;top:74px;display:flex;align-items:baseline;gap:14px;opacity:0;}' +
    '.dk-fig .fn{font-family:var(--font-mono);font-size:21px;font-weight:700;letter-spacing:.04em;color:var(--primary);}' +
    '.dk-fig .ft{font-size:30px;font-weight:800;letter-spacing:-.02em;color:var(--text-title);}' +
    '.dk-text{position:absolute;transform:translate(-50%,-50%);text-align:center;pointer-events:none;line-height:1.24;}' +
    '.dk-text .tt{font-size:20px;font-weight:700;color:var(--text-title);letter-spacing:-.01em;}' +
    '.dk-text .ts{font-size:15px;color:var(--text-secondary);margin-top:3px;font-family:var(--font-mono);}' +
    '.dk-text.ink .tt{color:#fff;}.dk-text.ink .ts{color:rgba(255,255,255,.82);}' +
    '.dk-text.sm .tt{font-size:16px;}.dk-text.sm .ts{font-size:13px;}' +
    '.dk-elabel{position:absolute;transform:translate(-50%,-50%);background:var(--surface);border:1px solid var(--border);border-radius:999px;padding:5px 13px;font-size:15px;font-weight:600;color:var(--text-body);white-space:nowrap;box-shadow:var(--soft-shadow);}' +
    '.dk-elabel .ep{font-family:var(--font-mono);color:var(--text-secondary);font-weight:500;}' +
    '.dk-elabel.on-green{color:#42540f;border-color:#bcc88c;}.dk-elabel.on-blue{color:#2f5d96;border-color:#bcd0ea;}.dk-elabel.on-warn{color:#8a6217;border-color:#e3c98c;}' +
    '.dk-note{position:absolute;transform:translate(-50%,-50%);background:#fbf6e4;border:1px dashed #d8c98a;border-radius:10px;padding:8px 14px;font-size:15px;color:#6a5a1f;max-width:360px;text-align:center;box-shadow:var(--soft-shadow);}' +
    '.dk-actor{position:absolute;transform:translate(-50%,-50%);background:var(--surface);border-radius:13px;box-shadow:var(--soft-shadow);padding:11px 18px;text-align:center;border:1px solid var(--divider);}' +
    '.dk-actor .an{font-size:20px;font-weight:700;color:var(--text-title);}' +
    '.dk-actor .as{font-size:13px;color:var(--text-secondary);margin-top:2px;font-family:var(--font-mono);}'
  );

  function el(html) { const d = document.createElement('div'); d.innerHTML = html; return d.firstElementChild; }
  function anchor(n, side) {
    const cx = n.x + n.w / 2, cy = n.y + n.h / 2;
    switch (side) {
      case 't': return [cx, n.y]; case 'b': return [cx, n.y + n.h];
      case 'l': return [n.x, cy]; case 'r': return [n.x + n.w, cy];
      case 'tl': return [n.x + n.w * 0.28, n.y]; case 'tr': return [n.x + n.w * 0.72, n.y];
      case 'bl': return [n.x + n.w * 0.28, n.y + n.h]; case 'br': return [n.x + n.w * 0.72, n.y + n.h];
      case 'lt': return [n.x, n.y + n.h * 0.3]; case 'lb': return [n.x, n.y + n.h * 0.7];
      case 'rt': return [n.x + n.w, n.y + n.h * 0.3]; case 'rb': return [n.x + n.w, n.y + n.h * 0.7];
      default: return [cx, cy];
    }
  }
  function autoSides(a, b) {
    const acx = a.x + a.w / 2, acy = a.y + a.h / 2, bcx = b.x + b.w / 2, bcy = b.y + b.h / 2;
    const dx = bcx - acx, dy = bcy - acy;
    if (Math.abs(dy) >= Math.abs(dx)) return dy >= 0 ? ['b', 't'] : ['t', 'b'];
    return dx >= 0 ? ['r', 'l'] : ['l', 'r'];
  }
  function edgePath(p0, fs, p1, ts, elbow, via) {
    const [x0, y0] = p0, [x1, y1] = p1;
    if (!elbow || (via == null && (Math.abs(x0 - x1) < 6 || Math.abs(y0 - y1) < 6))) return 'M' + x0 + ' ' + y0 + ' L' + x1 + ' ' + y1;
    if (fs === 'b' || fs === 't' || fs === 'bl' || fs === 'br' || fs === 'tl' || fs === 'tr') {
      const my = via == null ? (y0 + y1) / 2 : via;              // 竖直出 → 折(走廊 y) → 竖直入
      return 'M' + x0 + ' ' + y0 + ' L' + x0 + ' ' + my + ' L' + x1 + ' ' + my + ' L' + x1 + ' ' + y1;
    }
    const mx = via == null ? (x0 + x1) / 2 : via;                // 水平出 → 折(走廊 x) → 水平入
    return 'M' + x0 + ' ' + y0 + ' L' + mx + ' ' + y0 + ' L' + mx + ' ' + y1 + ' L' + x1 + ' ' + y1;
  }
  function shapeSVG(n) {
    const tn = TONE[n.tone || 'neutral'], cx = n.x + n.w / 2, cy = n.y + n.h / 2;
    const sw = n.tone === 'ink' ? 0 : 2;
    if (n.shape === 'diamond') {
      const hw = n.w / 2, hh = n.h / 2;
      const pts = cx + ',' + (cy - hh) + ' ' + (cx + hw) + ',' + cy + ' ' + cx + ',' + (cy + hh) + ' ' + (cx - hw) + ',' + cy;
      return '<polygon class="dk-shape" data-k="' + n.k + '" points="' + pts + '" fill="' + tn.fill + '" stroke="' + tn.stroke + '" stroke-width="2" stroke-linejoin="round" opacity="0"/>';
    }
    const r = n.shape === 'cap' ? n.h / 2 : (n.shape === 'round' ? 22 : 13);
    return '<rect class="dk-shape" data-k="' + n.k + '" x="' + n.x + '" y="' + n.y + '" width="' + n.w + '" height="' + n.h + '" rx="' + r + '" fill="' + tn.fill + '" stroke="' + tn.stroke + '" stroke-width="' + sw + '" opacity="0"/>';
  }

  /* ── 流程图/状态机 ── spec:{vb,title,fig,nodes:[{k,x,y,w,h,t,s,shape,tone,sm}],edges:[{a,b,label,sub,tone,dash,fromSide,toSide,elbow}]} */
  function mountFlow(parent, spec) {
    const vb = spec.vb || [W, H];
    const wrap = el('<div class="dk"></div>'); parent.appendChild(wrap);
    let marks = '';
    Object.keys(MARK).forEach(function (k) {
      marks += '<marker id="dk-ar-' + k + '" viewBox="0 0 10 10" refX="8.5" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="' + MARK[k] + '"/></marker>';
    });
    const nmap = {}; spec.nodes.forEach(function (n) { nmap[n.k] = n; });
    let edgeSVG = '';
    (spec.edges || []).forEach(function (e, i) {
      const a = nmap[e.a], b = nmap[e.b]; if (!a || !b) return;
      const auto = autoSides(a, b);
      const p0 = anchor(a, e.fromSide || auto[0]), p1 = anchor(b, e.toSide || auto[1]);
      const tn = TONE[e.tone || 'neutral'], d = edgePath(p0, e.fromSide || auto[0], p1, e.toSide || auto[1], e.elbow, e.viaX != null ? e.viaX : e.viaY);
      const dashAttr = e.dash ? ' stroke-dasharray="7 6"' : ' pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"';
      edgeSVG += '<g class="dk-edge" data-i="' + i + '" opacity="' + (e.dash ? 0 : 1) + '">' +
        '<path d="' + d + '" fill="none" stroke="' + tn.stroke + '" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round"' + dashAttr + ' marker-end="url(#dk-ar-' + tn.mark + ')"/></g>';
    });
    let shapeSVGs = ''; spec.nodes.forEach(function (n) { shapeSVGs += shapeSVG(n); });
    wrap.innerHTML = '<svg class="dk-svg" viewBox="0 0 ' + vb[0] + ' ' + vb[1] + '"><defs>' + marks + '</defs>' +
      '<g class="dk-edges">' + edgeSVG + '</g><g class="dk-shapes">' + shapeSVGs + '</g></svg>';
    // 图题
    let fig = null;
    if (spec.fig || spec.title) {
      fig = el('<div class="dk-fig"><span class="fn">' + (spec.fig || '') + '</span><span class="ft">' + (spec.title || '') + '</span></div>');
      wrap.appendChild(fig);
    }
    // 节点文字（HTML 叠加）
    const textEls = {};
    spec.nodes.forEach(function (n) {
      const t = el('<div class="dk-text' + (n.tone === 'ink' ? ' ink' : '') + (n.sm ? ' sm' : '') + '" style="left:' + (n.x + n.w / 2) + 'px;top:' + (n.y + n.h / 2) + 'px;width:' + (n.w - 18) + 'px;opacity:0;">' +
        '<div class="tt">' + n.t + '</div>' + (n.s ? '<div class="ts">' + n.s + '</div>' : '') + '</div>');
      wrap.appendChild(t); textEls[n.k] = t;
    });
    // 边标签
    const edgeEls = [];
    const svg = wrap.querySelector('svg');
    const groups = Array.prototype.slice.call(svg.querySelectorAll('.dk-edge'));
    (spec.edges || []).forEach(function (e, i) {
      const a = nmap[e.a], b = nmap[e.b]; if (!a || !b) { edgeEls.push(null); return; }
      let lab = null;
      if (e.label) {
        const auto = autoSides(a, b);
        const p0 = anchor(a, e.fromSide || auto[0]), p1 = anchor(b, e.toSide || auto[1]);
        const mx = e.lx != null ? e.lx : (p0[0] + p1[0]) / 2, my = e.ly != null ? e.ly : (p0[1] + p1[1]) / 2;
        const on = e.tone === 'info' ? ' on-blue' : (e.tone === 'vision' ? ' on-warn' : ' on-green');
        lab = el('<div class="dk-elabel' + on + '" style="left:' + mx + 'px;top:' + my + 'px;opacity:0;">' + e.label + (e.sub ? ' <span class="ep">' + e.sub + '</span>' : '') + '</div>');
        wrap.appendChild(lab);
      }
      edgeEls.push({ group: groups[i], path: groups[i] ? groups[i].querySelector('path') : null, label: lab, dash: !!e.dash });
    });
    const shapeEls = {};
    Array.prototype.slice.call(svg.querySelectorAll('.dk-shape')).forEach(function (sh) { shapeEls[sh.dataset.k] = sh; });
    // 注记
    const notes = [];
    (spec.notes || []).forEach(function (nt) {
      const d = el('<div class="dk-note" style="left:' + nt.x + 'px;top:' + nt.y + 'px;opacity:0;' + (nt.w ? 'width:' + nt.w + 'px;' : '') + '">' + nt.t + '</div>');
      wrap.appendChild(d); notes.push(d);
    });
    return { wrap: wrap, svg: svg, fig: fig, nodes: spec.nodes, nmap: nmap, shapeEls: shapeEls, textEls: textEls, edgeEls: edgeEls, edges: spec.edges || [], notes: notes };
  }

  /* 揭示：节点按数组顺序错峰；每条边随其(后到的)端点画出。返回结束时间。 */
  function revealFlow(tl, at, r, opt) {
    opt = opt || {};
    const step = opt.step == null ? 0.4 : opt.step, ndur = opt.nodeDur == null ? 0.62 : opt.nodeDur, edur = opt.edgeDur == null ? 0.6 : opt.edgeDur;
    if (r.fig) tl.fromTo(r.fig, { opacity: 0, y: 12 }, { opacity: 1, y: 0, duration: 0.6, ease: 'power3.out' }, at);
    const t0 = at + (r.fig ? 0.35 : 0), ntime = {};
    r.nodes.forEach(function (n, i) {
      const nt = t0 + i * step; ntime[n.k] = nt;
      tl.fromTo([r.shapeEls[n.k], r.textEls[n.k]], { opacity: 0, y: 22, filter: 'blur(9px)' },
        { opacity: 1, y: 0, filter: 'blur(0px)', duration: ndur, ease: 'power3.out' }, nt);
    });
    r.edges.forEach(function (e, i) {
      const ee = r.edgeEls[i]; if (!ee) return;
      const ta = ntime[e.a] == null ? t0 : ntime[e.a], tb = ntime[e.b] == null ? t0 : ntime[e.b];
      const et = Math.max(ta, tb) - 0.04;
      if (ee.dash) tl.fromTo(ee.group, { opacity: 0 }, { opacity: 1, duration: edur, ease: 'power2.out' }, et);
      else if (ee.path) tl.fromTo(ee.path, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: edur, ease: 'power2.inOut' }, et);
      if (ee.label) tl.fromTo(ee.label, { opacity: 0, scale: 0.82 }, { opacity: 1, scale: 1, duration: 0.42, ease: 'back.out(1.4)' }, et + edur * 0.5);
    });
    let end = t0;
    r.nodes.forEach(function (n, i) { end = Math.max(end, t0 + i * step + ndur); });
    if (r.notes.length) r.notes.forEach(function (d, i) { tl.fromTo(d, { opacity: 0, y: 10 }, { opacity: 1, y: 0, duration: 0.5, ease: 'power3.out' }, end - 0.2 + i * 0.25); });
    return end;
  }
  function pulse(tl, at, el, sc) {
    if (!el) return;
    tl.to(el, { scale: sc || 1.07, duration: 0.34, yoyo: true, repeat: 1, ease: 'power2.inOut', transformOrigin: '50% 50%' }, at);
  }

  /* ── 时序图 ── spec:{vb,fig,title,top,bottom,actors:[{k,x,label,sub,tone}],msgs:[{from,to,y,label,dash,self,note,noteW,ret}]} */
  function mountSeq(parent, spec) {
    const vb = spec.vb || [W, H], top = spec.top || 250, bottom = spec.bottom || 940;
    const wrap = el('<div class="dk"></div>'); parent.appendChild(wrap);
    let marks = '';
    Object.keys(MARK).forEach(function (k) {
      marks += '<marker id="dk-ar-' + k + '" viewBox="0 0 10 10" refX="8.5" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="' + MARK[k] + '"/></marker>';
    });
    const amap = {}; spec.actors.forEach(function (a) { amap[a.k] = a; });
    let life = '';
    spec.actors.forEach(function (a) {
      life += '<line class="dk-life" data-k="' + a.k + '" x1="' + a.x + '" y1="' + (top + 34) + '" x2="' + a.x + '" y2="' + bottom + '" stroke="#cfccbd" stroke-width="2" stroke-dasharray="4 7" opacity="0"/>';
    });
    let msgSVG = '';
    (spec.msgs || []).forEach(function (m, i) {
      const fa = amap[m.from], ta = amap[m.to]; if (!fa || !ta) return;
      const tn = TONE[m.tone || 'neutral'];
      if (m.noteOnly) { msgSVG += '<g class="dk-msg" data-i="' + i + '" opacity="0"></g>'; return; }
      if (m.self) {
        const x = fa.x, y = m.y, wd = 64, hh = 30;
        msgSVG += '<g class="dk-msg" data-i="' + i + '" opacity="0"><path d="M' + x + ' ' + y + ' h' + wd + ' v' + hh + ' h-' + (wd - 2) + '" fill="none" stroke="' + tn.stroke + '" stroke-width="2.2" stroke-linecap="round"' + (m.dash ? ' stroke-dasharray="6 5"' : '') + ' marker-end="url(#dk-ar-' + tn.mark + ')"/></g>';
      } else {
        msgSVG += '<g class="dk-msg" data-i="' + i + '" opacity="0"><line x1="' + fa.x + '" y1="' + m.y + '" x2="' + ta.x + '" y2="' + m.y + '" stroke="' + tn.stroke + '" stroke-width="2.4" stroke-linecap="round"' + (m.dash ? ' stroke-dasharray="6 5"' : '') + ' marker-end="url(#dk-ar-' + tn.mark + ')"/></g>';
      }
    });
    wrap.innerHTML = '<svg class="dk-svg" viewBox="0 0 ' + vb[0] + ' ' + vb[1] + '"><defs>' + marks + '</defs>' +
      '<g class="dk-lifes">' + life + '</g><g class="dk-msgs">' + msgSVG + '</g></svg>';
    let fig = null;
    if (spec.fig || spec.title) { fig = el('<div class="dk-fig"><span class="fn">' + (spec.fig || '') + '</span><span class="ft">' + (spec.title || '') + '</span></div>'); wrap.appendChild(fig); }
    const actorEls = {};
    spec.actors.forEach(function (a) {
      const t = el('<div class="dk-actor" style="left:' + a.x + 'px;top:' + top + 'px;opacity:0;"><div class="an">' + a.label + '</div>' + (a.sub ? '<div class="as">' + a.sub + '</div>' : '') + '</div>');
      wrap.appendChild(t); actorEls[a.k] = t;
    });
    const svg = wrap.querySelector('svg');
    const lifeEls = Array.prototype.slice.call(svg.querySelectorAll('.dk-life'));
    const msgGroups = Array.prototype.slice.call(svg.querySelectorAll('.dk-msg'));
    const msgEls = [];
    (spec.msgs || []).forEach(function (m, i) {
      const fa = amap[m.from], ta = amap[m.to]; if (!fa || !ta) { msgEls.push(null); return; }
      let lab = null;
      if (m.label) {
        const mx = m.self ? fa.x + 78 : (fa.x + ta.x) / 2, anchorL = m.self ? 'left' : 'center';
        lab = el('<div class="dk-elabel ' + (m.tone === 'info' ? 'on-blue' : m.tone === 'vision' ? 'on-warn' : 'on-green') + '" style="left:' + mx + 'px;top:' + (m.y - 19) + 'px;opacity:0;' + (m.self ? 'transform:translate(0,-50%);' : '') + '">' + m.label + '</div>');
        wrap.appendChild(lab);
      }
      let note = null;
      if (m.note) {
        note = el('<div class="dk-note" style="left:' + ((fa.x + ta.x) / 2) + 'px;top:' + m.y + 'px;opacity:0;' + (m.noteW ? 'width:' + m.noteW + 'px;' : '') + '">' + m.note + '</div>');
        wrap.appendChild(note);
      }
      msgEls.push({ group: msgGroups[i], label: lab, note: note });
    });
    return { wrap: wrap, svg: svg, fig: fig, actors: spec.actors, actorEls: actorEls, lifeEls: lifeEls, msgs: spec.msgs || [], msgEls: msgEls };
  }
  function revealSeq(tl, at, r, opt) {
    opt = opt || {};
    const astep = opt.actorStep == null ? 0.16 : opt.actorStep, mstep = opt.msgStep == null ? 0.62 : opt.msgStep;
    if (r.fig) tl.fromTo(r.fig, { opacity: 0, y: 12 }, { opacity: 1, y: 0, duration: 0.6, ease: 'power3.out' }, at);
    let t = at + (r.fig ? 0.3 : 0);
    r.actors.forEach(function (a, i) {
      tl.fromTo(r.actorEls[a.k], { opacity: 0, y: 18, filter: 'blur(8px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: 0.55, ease: 'power3.out' }, t + i * astep);
    });
    r.lifeEls.forEach(function (l) { tl.fromTo(l, { opacity: 0 }, { opacity: 1, duration: 0.5 }, t + 0.2); });
    // 消息按 g 分批：同 g 一批一次性出（多箭头并发，节奏更快、更省时）
    const batches = []; let cur = [], curG = '\0';
    r.msgs.forEach(function (m, i) { const g = m.g == null ? 'u' + i : 'g' + m.g; if (g !== curG) { if (cur.length) batches.push(cur); cur = []; curG = g; } cur.push(i); });
    if (cur.length) batches.push(cur);
    let mt = t + r.actors.length * astep + 0.3;
    batches.forEach(function (batch) {
      batch.forEach(function (i, j) {
        const me = r.msgEls[i]; if (!me) return; const off = j * 0.07;   // 同批内极小错峰，避免完全机械
        tl.fromTo(me.group, { opacity: 0 }, { opacity: 1, duration: 0.42, ease: 'power2.out' }, mt + off);
        if (me.label) tl.fromTo(me.label, { opacity: 0, y: 6 }, { opacity: 1, y: 0, duration: 0.34, ease: 'power3.out' }, mt + off + 0.1);
        if (me.note) tl.fromTo(me.note, { opacity: 0, scale: 0.92 }, { opacity: 1, scale: 1, duration: 0.4, ease: 'back.out(1.3)' }, mt + off + 0.15);
      });
      mt += mstep;
    });
    return mt;
  }

  return { TONE: TONE, mountFlow: mountFlow, revealFlow: revealFlow, mountSeq: mountSeq, revealSeq: revealSeq, pulse: pulse };
})();
