/* =====================================================================================
   场景库 act 9：它是怎么做到的（原理）+ 收尾。自包含（CSS + 概念图，无平板）。
   ⚠️ 本文件名排序在末 → build.sh glob 后最后加载 → 保证「收尾字标」在全片最后。
   勿用 file-tool Edit（会截断）。经 FILM.addScene 注册。
   文案/色值/数字见 00-视频制作总纲.md §1(09)/§2.1/§8；架构链路与 9 字节字段已与
   contracts/udp-protocol.md 逐字段核对（byte0 命令码、5cm/格、:5001 等）。
   ===================================================================================== */
(function () {
  "use strict";
  const F = window.FILM;
  const rise = F.rise, fade = F.fade, pop = F.pop, centerOf = F.centerOf,
    camTo = F.camTo, camReset = F.camReset,
    cursorShow = F.cursorShow, cursorHide = F.cursorHide,
    caption = F.caption, addCSS = F.addCSS, STAGE_W = F.STAGE_W, STAGE_H = F.STAGE_H;

  addCSS(
    /* ── 09b 架构图 ── */
    '.arch{position:absolute;inset:0;}' +
    '.arch-svg{position:absolute;inset:0;width:100%;height:100%;overflow:visible;}' +
    '.arch-node{position:absolute;background:var(--surface);border-radius:20px;box-shadow:var(--card-shadow);padding:22px 28px;border:1px solid var(--divider);}' +
    '.arch-node .nh{display:flex;align-items:center;gap:13px;}' +
    '.arch-node .nbadge{width:16px;height:16px;border-radius:5px;flex:none;}' +
    '.arch-node .nt{font-size:31px;font-weight:800;color:var(--text-title);letter-spacing:-.02em;}' +
    '.arch-node .ns{font-size:19px;color:var(--text-secondary);margin-top:11px;font-family:var(--font-mono);}' +
    '.arch-node .nb{font-size:18px;color:var(--text-body);margin-top:13px;line-height:1.55;}' +
    '.arch-label{position:absolute;transform:translate(-50%,-50%);background:var(--surface);border:1px solid var(--border);border-radius:999px;padding:8px 17px;font-size:18px;font-weight:700;color:var(--text-body);box-shadow:var(--soft-shadow);white-space:nowrap;}' +
    '.arch-label .lp{font-family:var(--font-mono);color:var(--text-secondary);font-weight:500;margin-left:8px;font-size:15px;}' +
    '.arch-label.ws{color:var(--info);border-color:#bcd0ea;}' +
    '.arch-label.ws .lp{color:#6f93c4;}' +
    '.arch-pkt{position:absolute;width:13px;height:13px;border-radius:50%;background:var(--primary);box-shadow:0 1px 4px rgba(0,0,0,.22);opacity:0;will-change:transform,opacity;}' +
    '.arch-pkt.ws{background:var(--info);}' +
    /* ── 09c 9 字节协议显微镜 ── */
    '.proto{position:absolute;inset:0;}' +
    '.byte-row{position:absolute;display:flex;gap:11px;}' +
    '.byte{width:120px;height:150px;border-radius:14px;background:var(--surface);border:1.5px solid var(--border);box-shadow:var(--soft-shadow);display:flex;flex-direction:column;align-items:center;justify-content:center;will-change:transform,opacity;}' +
    '.byte .bf{font-size:16px;color:var(--text-secondary);}' +
    '.byte .bv{font-family:var(--font-mono);font-size:33px;font-weight:700;color:var(--text-body);margin-top:7px;}' +
    '.byte .bc{font-family:var(--font-mono);font-size:17px;color:var(--text-caption);margin-top:3px;height:20px;}' +
    '.byte .bi{font-family:var(--font-mono);font-size:14px;color:var(--text-caption);margin-top:9px;}' +
    '.byte.cmd{background:var(--primary);border-color:var(--primary);box-shadow:0 10px 26px -10px rgba(72,92,17,.5);}' +
    '.byte.cmd .bf{color:rgba(255,255,255,.82);}' +
    '.byte.cmd .bv{color:#fff;}' +
    '.byte.cmd .bc{color:rgba(255,255,255,.78);}' +
    '.byte.cmd .bi{color:rgba(255,255,255,.7);}' +
    '.byte.gX{background:var(--primary-soft);border-color:#cdd8a6;}' +
    '.byte.gY{background:#dbe7f3;border-color:#bcd0ea;}' +
    '.byte.gN{background:var(--surface-muted);border-color:var(--border);}' +
    '.byte.gN .bv{color:var(--text-caption);}' +
    '.proto-callout{position:absolute;transform:translateX(-50%);background:var(--text-title);color:#fff;border-radius:12px;padding:11px 20px;font-size:21px;font-weight:600;white-space:nowrap;box-shadow:var(--soft-shadow);}' +
    '.proto-callout b{color:var(--primary-soft);}' +
    '.proto-legend{position:absolute;transform:translateX(-50%);display:flex;gap:14px;align-items:center;}' +
    '.lg{background:var(--surface);border:1px solid var(--border);border-radius:999px;padding:8px 16px;font-size:17px;color:var(--text-body);box-shadow:var(--soft-shadow);}' +
    '.lg .k{font-family:var(--font-mono);color:var(--primary);font-weight:700;}' +
    '.lg.on{background:var(--primary);border-color:var(--primary);color:#fff;}' +
    '.lg.on .k{color:var(--primary-soft);}' +
    '.proto-port{position:absolute;transform:translateX(-50%);display:flex;flex-direction:column;align-items:center;}' +
    '.proto-port .pt{font-family:var(--font-mono);font-size:30px;font-weight:700;color:var(--primary);}' +
    '.proto-port .ps{font-size:17px;color:var(--text-secondary);margin-top:4px;}' +
    '.proto-pkt{position:absolute;width:16px;height:16px;border-radius:4px;background:var(--primary);opacity:0;will-change:transform,opacity;box-shadow:0 1px 4px rgba(0,0,0,.22);}' +
    /* ── 09d 数字墙 ── */
    '.nums{position:absolute;inset:0;}' +
    '.nums-grid{position:absolute;display:flex;flex-wrap:wrap;gap:40px;}' +
    '.num-cell{width:360px;height:200px;border-radius:20px;background:var(--surface);border:1px solid var(--divider);box-shadow:var(--soft-shadow);display:flex;flex-direction:column;align-items:flex-start;justify-content:center;padding:0 34px;will-change:transform,opacity;}' +
    '.num-v{font-family:var(--font-mono);font-size:58px;font-weight:700;color:var(--primary);letter-spacing:-.02em;line-height:1;}' +
    '.num-v .np,.num-v .nu{font-size:34px;color:var(--text-body);font-weight:600;}' +
    '.num-l{font-size:22px;color:var(--text-secondary);margin-top:18px;}' +
    /* ── 09e 收尾 ── */
    '.outro-credits{display:flex;gap:46px;font-family:var(--font-mono);font-size:24px;color:var(--text-secondary);opacity:0;margin-top:48px;}' +
    '.outro-credits .cv{color:var(--text-body);}' +
    '.outro-credits .blank{color:var(--text-caption);letter-spacing:.06em;}' +
    '.outro-rule{width:108px;height:2px;background:var(--border);opacity:0;margin-top:40px;}' +
    '.outro-foot{font-family:var(--font-mono);font-size:19px;letter-spacing:.16em;color:var(--text-caption);opacity:0;margin-top:26px;}'
  );

  /* ── 通用工具（每文件自包含；可 seek）─────────────────────────────────────────── */
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
        rise(tl, r.n, a + 0.3, { y: 24, dur: 0.9 }); rise(tl, r.t, a + 0.6, { y: 24, dur: 0.9 }); rise(tl, r.sb, a + 0.95, { y: 18, dur: 0.8 });
        tl.fromTo(r.col, { scale: 1.0 }, { scale: 1.03, duration: s.dur, ease: 'none' }, a);
      }
    };
  }
  /* 可 seek 的整数 count-up：onUpdate 续算，onReverseComplete 复位 from。 */
  function countUp(tl, at, dur, el, to, from) {
    from = from || 0; const o = { n: from };
    tl.to(o, { n: to, duration: dur, ease: 'power2.out',
      onUpdate: function () { el.textContent = String(Math.round(o.n)); },
      onReverseComplete: function () { el.textContent = String(from); } }, at);
  }

  /* ════════════════ 09b 架构图：三节点 · 四链路 ════════════════ */
  // App 右缘 x=570；紫派/香橙派 左缘 x=1320。链路端点与标签中点见下。
  const ARCH_NODES = [
    { k: 'app', cls: '', x: 190, y: 415, w: 380, h: 250, badge: 'var(--primary)',
      t: '平板 App', s: '鸿蒙 · 控制端', b: '发现设备 · 建图 · 导航<br>全覆盖 · 多机 · 仪表视频' },
    { k: 'pi', cls: '', x: 1320, y: 210, w: 430, h: 220, badge: 'var(--primary-pressed)',
      t: '紫派 · 导航主控', s: 'RK3566 · OpenHarmony 5.0', b: 'SLAM 建图 · A*/全覆盖<br>轮控 · 内部 LCM 信道' },
    { k: 'oz', cls: '', x: 1320, y: 645, w: 430, h: 215, badge: 'var(--warning)',
      t: '香橙派 · 视觉', s: '昇腾 NPU · INT8', b: 'YOLOv5s + 4 关键点<br>读表 · 告警 · 报告' }
  ];
  const ARCH_LINKS = [
    { x1: 570, y1: 472, x2: 1320, y2: 280, ws: false, name: 'UDP · 9 字节', port: ':5001 控制' },
    { x1: 570, y1: 535, x2: 1320, y2: 325, ws: false, name: 'HTTP 拉图', port: ':8000' },
    { x1: 570, y1: 598, x2: 1320, y2: 370, ws: false, name: '软总线 · 黑板', port: 'FleetMission' },
    { x1: 570, y1: 612, x2: 1320, y2: 700, ws: true, name: 'WS 视频', port: ':8000/ws/video' }
  ];
  function buildArch(root) {
    root.classList.add('paper');
    const cam = document.createElement('div'); cam.className = 'cam'; root.appendChild(cam);
    const wrap = document.createElement('div'); wrap.className = 'arch'; cam.appendChild(wrap);
    // SVG 连线层（用 stroke-dashoffset 揭示，可 seek）
    let svg = '<svg class="arch-svg" viewBox="0 0 ' + STAGE_W + ' ' + STAGE_H + '">';
    ARCH_LINKS.forEach(function (l, i) {
      const col = l.ws ? '#4678b8' : '#485c11';
      svg += '<line data-ln="' + i + '" x1="' + l.x1 + '" y1="' + l.y1 + '" x2="' + l.x2 + '" y2="' + l.y2 + '" stroke="' + col + '" stroke-width="3" stroke-linecap="round" stroke-dasharray="900" stroke-dashoffset="900"' + (l.ws ? ' stroke-opacity="0.95"' : '') + '/>';
    });
    svg += '</svg>';
    wrap.innerHTML = svg;
    const svgEl = wrap.querySelector('svg');
    // 节点卡
    const nodeEls = {};
    ARCH_NODES.forEach(function (n) {
      const d = document.createElement('div'); d.className = 'arch-node ' + n.cls;
      d.style.cssText = 'left:' + n.x + 'px;top:' + n.y + 'px;width:' + n.w + 'px;height:' + n.h + 'px;';
      d.innerHTML = '<div class="nh"><span class="nbadge" style="background:' + n.badge + '"></span><span class="nt">' + n.t + '</span></div>' +
        '<div class="ns">' + n.s + '</div><div class="nb">' + n.b + '</div>';
      wrap.appendChild(d); nodeEls[n.k] = d;
    });
    // 标签 + 数据包
    const labels = [], pkts = [];
    ARCH_LINKS.forEach(function (l) {
      const mx = (l.x1 + l.x2) / 2, my = (l.y1 + l.y2) / 2;
      const lab = document.createElement('div'); lab.className = 'arch-label' + (l.ws ? ' ws' : '');
      lab.style.cssText = 'left:' + mx + 'px;top:' + my + 'px;';
      lab.innerHTML = l.name + '<span class="lp">' + l.port + '</span>';
      wrap.appendChild(lab); labels.push(lab);
      const p = document.createElement('div'); p.className = 'arch-pkt' + (l.ws ? ' ws' : '');
      p.style.cssText = 'left:' + l.x1 + 'px;top:' + l.y1 + 'px;margin:-6.5px 0 0 -6.5px;';
      p._dx = l.x2 - l.x1; p._dy = l.y2 - l.y1;
      wrap.appendChild(p); pkts.push(p);
    });
    const lines = Array.prototype.slice.call(svgEl.querySelectorAll('line'));
    return { cam: cam, wrap: wrap, app: nodeEls.app, pi: nodeEls.pi, oz: nodeEls.oz, lines: lines, labels: labels, pkts: pkts };
  }
  function drawLink(tl, at, line, label, pkt) {
    tl.fromTo(line, { strokeDashoffset: 900 }, { strokeDashoffset: 0, duration: 0.85, ease: 'power2.inOut' }, at);
    tl.fromTo(label, { opacity: 0, scale: 0.8 }, { opacity: 1, scale: 1, duration: 0.5, ease: 'back.out(1.4)' }, at + 0.45);
    tl.fromTo(pkt, { opacity: 0, x: 0, y: 0 }, { opacity: 1, duration: 0.25 }, at + 0.6);
    tl.to(pkt, { x: pkt._dx, y: pkt._dy, duration: 1.05, ease: 'power1.inOut', repeat: 2, repeatDelay: 0.35 }, at + 0.6);
  }
  function animArch(tl, s) {
    const a = s.start, r = s.refs;
    rise(tl, r.app, a + 0.3, { y: 26, dur: 0.9 });
    rise(tl, r.pi, a + 0.7, { y: 26, dur: 0.9 });
    rise(tl, r.oz, a + 1.1, { y: 26, dur: 0.9 });
    caption(tl, a + 0.6, 3.0, '<b>三节点 · 各司其职</b>', 'App 控制端 · 紫派导航主控 · 香橙派视觉');
    drawLink(tl, a + 2.0, r.lines[0], r.labels[0], r.pkts[0]);
    drawLink(tl, a + 2.5, r.lines[1], r.labels[1], r.pkts[1]);
    drawLink(tl, a + 3.0, r.lines[2], r.labels[2], r.pkts[2]);
    drawLink(tl, a + 3.9, r.lines[3], r.labels[3], r.pkts[3]);
    caption(tl, a + 6.3, 3.2, '<b>导航链路与视觉链路解耦</b>', 'UDP / HTTP / 软总线走紫派 · WS 视频走香橙派 · 互不打扰');
    tl.to(r.pkts, { opacity: 0, duration: 0.5 }, a + s.dur - 0.7);
    tl.fromTo(r.wrap, { scale: 1.0 }, { scale: 1.022, duration: s.dur, ease: 'none' }, a);
  }

  /* ════════════════ 09c 9 字节协议显微镜 ════════════════ */
  // App→紫派发送帧（来源 contracts/udp-protocol.md「发送方向」）：示例＝建图指令 'm'。
  const BYTES = [
    { f: '命令码', v: '0x6d', c: "'m'", g: 'cmd' },
    { f: '方向', v: '0x01', c: 'go', g: '' },
    { f: '速度', v: '0x28', c: '40', g: '' },
    { f: 'endX', v: '0x00', c: '·', g: 'gX' },
    { f: 'endX', v: '0x00', c: '·', g: 'gX' },
    { f: 'endY', v: '0x00', c: '·', g: 'gY' },
    { f: 'endY', v: '0x00', c: '·', g: 'gY' },
    { f: '未用', v: '0x00', c: '·', g: 'gN' },
    { f: '未用', v: '0x00', c: '·', g: 'gN' }
  ];
  const LEGEND = [
    { k: "0x6d 'm'", t: '强制建图', on: true },
    { k: "0x66 'f'", t: '全路径', on: false },
    { k: '0x03', t: '目标点', on: false },
    { k: '0x06', t: '发现 ping', on: false },
    { k: "0x69 'i'", t: '拉主机图', on: false }
  ];
  const ROW_LEFT = 376, ROW_TOP = 404, CELL_W = 120, CELL_GAP = 11;
  const ROW_CX = ROW_LEFT + (9 * CELL_W + 8 * CELL_GAP) / 2;   // 960
  const B0_CX = ROW_LEFT + CELL_W / 2;                          // 436
  function buildProto(root) {
    root.classList.add('paper');
    const cam = document.createElement('div'); cam.className = 'cam'; root.appendChild(cam);
    const wrap = document.createElement('div'); wrap.className = 'proto'; cam.appendChild(wrap);
    // byte 行
    const row = document.createElement('div'); row.className = 'byte-row';
    row.style.cssText = 'left:' + ROW_LEFT + 'px;top:' + ROW_TOP + 'px;';
    BYTES.forEach(function (b, i) {
      const d = document.createElement('div'); d.className = 'byte' + (b.g ? ' ' + b.g : '');
      d.innerHTML = '<div class="bf">' + b.f + '</div><div class="bv">' + b.v + '</div><div class="bc">' + b.c + '</div><div class="bi">byte' + i + '</div>';
      row.appendChild(d);
    });
    wrap.appendChild(row);
    const cells = Array.prototype.slice.call(row.children);
    // byte0 上方说明
    const callout = document.createElement('div'); callout.className = 'proto-callout';
    callout.style.cssText = 'left:' + ROW_CX + 'px;top:312px;opacity:0;';
    callout.innerHTML = 'byte0 = App 状态 = 下发<b>命令码</b>（刻意对齐 ASCII）';
    wrap.appendChild(callout);
    // 命令码图例
    const legend = document.createElement('div'); legend.className = 'proto-legend';
    legend.style.cssText = 'left:' + ROW_CX + 'px;top:600px;opacity:0;';
    legend.innerHTML = LEGEND.map(function (g) {
      return '<div class="lg' + (g.on ? ' on' : '') + '"><span class="k">' + g.k + '</span> ' + g.t + '</div>';
    }).join('');
    wrap.appendChild(legend);
    // 目标端口 + 数据包
    const port = document.createElement('div'); port.className = 'proto-port';
    port.style.cssText = 'left:' + ROW_CX + 'px;top:678px;opacity:0;';
    port.innerHTML = '<div class="pt">紫派 :5001</div><div class="ps">大端整型 · 坐标 ÷20 = 米</div>';
    wrap.appendChild(port);
    const pkt = document.createElement('div'); pkt.className = 'proto-pkt';
    pkt.style.cssText = 'left:' + ROW_CX + 'px;top:558px;margin:-8px 0 0 -8px;opacity:0;';
    wrap.appendChild(pkt);
    return { cam: cam, wrap: wrap, cells: cells, b0: cells[0], callout: callout, legend: legend, port: port, pkt: pkt };
  }
  function animProto(tl, s) {
    const a = s.start, r = s.refs;
    r.cells.forEach(function (c, i) { pop(tl, c, a + 0.3 + i * 0.06, { from: 0.6, dur: 0.55 }); });
    caption(tl, a + 0.7, 3.0, '<b>9 字节，说清一条指令</b>', "byte0 既是状态也是命令码（如 'm'=0x6d 建图）");
    // 显微镜：聚焦 byte0
    camTo(tl, r.cam, a + 2.2, B0_CX, ROW_TOP + 75, 1.5, 1.0);
    tl.to(r.b0, { scale: 1.08, duration: 0.4, yoyo: true, repeat: 1, ease: 'power2.inOut', transformOrigin: '50% 50%' }, a + 3.0);
    tl.fromTo(r.callout, { opacity: 0, y: 12 }, { opacity: 1, y: 0, duration: 0.6, ease: 'power3.out' }, a + 2.8);
    camReset(tl, r.cam, a + 4.4, 1.0);
    // 图例
    rise(tl, r.legend, a + 5.0, { y: 16, dur: 0.7 });
    // 数据包飞向 :5001
    rise(tl, r.port, a + 5.6, { y: 16, dur: 0.7 });
    tl.fromTo(r.pkt, { opacity: 0, x: 0, y: 0 }, { opacity: 1, duration: 0.2 }, a + 5.8);
    tl.to(r.pkt, { y: 120, duration: 0.9, ease: 'power1.in', repeat: 1, repeatDelay: 0.5 }, a + 5.8);
    caption(tl, a + 6.0, 2.4, '<b>发往 紫派 · :5001</b>', '1 整数单位 = 5 cm（与地图格子 1:1）');
    tl.to(r.pkt, { opacity: 0, duration: 0.4 }, a + s.dur - 0.6);
    tl.fromTo(r.wrap, { scale: 1.0 }, { scale: 1.02, duration: s.dur, ease: 'none' }, a);
  }

  /* ════════════════ 09d 数字墙（§8 八数）════════════════ */
  const NUMS = [
    { pre: '~', n: 40, suf: ' ms', l: '端到端识别' },
    { pre: '~', n: 15, suf: ' FPS', l: '实时推理' },
    { full: '9 字节 · :5001', l: 'UDP 控制协议' },
    { pre: '~', n: 6, suf: '× 更小', l: '压缩地图 ZMAP1' },
    { full: 'RK3566 · NPU', l: '主控内置（INT8/INT16）' },
    { full: '1 格 = 5 cm', l: '栅格分辨率' },
    { pre: '', n: 4, suf: ' 关键点', l: '反算指针角度' },
    { pre: '', n: 3, suf: ' 节点 · 1 图', l: '软总线共享地图' }
  ];
  const GRID_W = 4 * 360 + 3 * 40;            // 1560
  const GRID_LEFT = (STAGE_W - GRID_W) / 2;   // 180
  function buildNums(root) {
    root.classList.add('paper');
    const cam = document.createElement('div'); cam.className = 'cam'; root.appendChild(cam);
    const wrap = document.createElement('div'); wrap.className = 'nums'; cam.appendChild(wrap);
    const grid = document.createElement('div'); grid.className = 'nums-grid';
    grid.style.cssText = 'left:' + GRID_LEFT + 'px;top:336px;width:' + GRID_W + 'px;';
    const counters = [];
    NUMS.forEach(function (m) {
      const cell = document.createElement('div'); cell.className = 'num-cell';
      let vhtml;
      if (m.full) { vhtml = m.full; }
      else { vhtml = (m.pre ? '<span class="np">' + m.pre + '</span>' : '') + '<span data-n>0</span>' + '<span class="nu">' + m.suf + '</span>'; }
      cell.innerHTML = '<div class="num-v">' + vhtml + '</div><div class="num-l">' + m.l + '</div>';
      grid.appendChild(cell);
      counters.push(m.full ? null : { el: cell.querySelector('[data-n]'), to: m.n });
    });
    wrap.appendChild(grid);
    const cells = Array.prototype.slice.call(grid.children);
    return { cam: cam, wrap: wrap, grid: grid, cells: cells, counters: counters };
  }
  function animNums(tl, s) {
    const a = s.start, r = s.refs;
    r.cells.forEach(function (c, i) { rise(tl, c, a + 0.3 + i * 0.11, { y: 24, dur: 0.8 }); });
    caption(tl, a + 0.6, 3.4, '<b>数字说话</b>', '每一项都与代码 / 契约逐字核对');
    r.counters.forEach(function (cu, i) { if (cu) countUp(tl, a + 1.2 + i * 0.12, 1.4, cu.el, cu.to, 0); });
    tl.fromTo(r.grid, { scale: 1.0 }, { scale: 1.022, duration: s.dur, ease: 'none' }, a);
  }

  /* ════════════════ 09e 收尾字标（回扣开场）════════════════ */
  const OUTRO_TITLE = 'OpenHarmony 工业巡检机器人';
  function buildOutro(root) {
    root.classList.add('paper');
    const col = document.createElement('div'); col.className = 'center-col';
    const kick = document.createElement('div'); kick.className = 'kicker'; kick.textContent = 'OpenHarmony · Ascend YOLO';
    const title = document.createElement('div'); title.className = 'bigtitle'; title.style.margin = '26px 0 30px';
    OUTRO_TITLE.split('').forEach(function (c) {
      const sp = document.createElement('span'); sp.className = 'ch'; sp.textContent = (c === ' ' ? ' ' : c); title.appendChild(sp);
    });
    const sub = document.createElement('div'); sub.className = 'subtitle'; sub.textContent = '让巡检自己跑起来。';
    const rule = document.createElement('div'); rule.className = 'outro-rule';
    const credits = document.createElement('div'); credits.className = 'outro-credits';
    credits.innerHTML = '<span>App · <span class="blank">＿＿＿</span></span><span>紫派 · <span class="blank">＿＿＿</span></span><span>香橙派 · <span class="blank">＿＿＿</span></span>';
    const foot = document.createElement('div'); foot.className = 'outro-foot'; foot.textContent = 'Built on OpenHarmony · Ascend YOLO';
    col.appendChild(kick); col.appendChild(title); col.appendChild(sub);
    col.appendChild(rule); col.appendChild(credits); col.appendChild(foot);
    root.appendChild(col);
    return { col: col, kick: kick, chars: Array.prototype.slice.call(title.querySelectorAll('.ch')), sub: sub, rule: rule, credits: credits, foot: foot };
  }
  function animOutro(tl, s) {
    const a = s.start, r = s.refs;
    rise(tl, r.kick, a + 0.4, { y: 14, dur: 0.9 });
    tl.fromTo(r.chars, { opacity: 0, y: 40, filter: 'blur(14px)' },
      { opacity: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out', stagger: 0.04 }, a + 0.7);
    rise(tl, r.sub, a + 2.0, { y: 18, dur: 0.9 });
    tl.to(r.rule, { opacity: 1, duration: 0.7, ease: 'power2.out' }, a + 2.9);
    tl.fromTo(r.credits, { opacity: 0, y: 16 }, { opacity: 1, y: 0, duration: 0.8, ease: 'power3.out' }, a + 3.2);
    tl.fromTo(r.foot, { opacity: 0, y: 12 }, { opacity: 1, y: 0, duration: 0.8, ease: 'power3.out' }, a + 3.8);
    tl.fromTo(r.col, { scale: 1.0 }, { scale: 1.028, duration: s.dur, ease: 'none' }, a);
  }

  /* ── 注册（顺序＝原理章节卡 → 架构 → 协议 → 数字 → 收尾）──────────────────────── */
  F.addScene(Object.assign({ id: '09-sec', dur: 4.2 }, sec('09', '它是怎么做到的', '三节点架构')));
  F.addScene({ id: '09-arch', dur: 11, build: buildArch, anim: animArch });
  F.addScene({ id: '09-proto', dur: 9, build: buildProto, anim: animProto });
  F.addScene({ id: '09-nums', dur: 9, build: buildNums, anim: animNums });
  F.addScene({ id: '09-outro', dur: 9.5, build: buildOutro, anim: animOutro });
})();
