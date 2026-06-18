/* scenes_intro.js —— 开始页之后：项目背景叙述 + 项目架构图三连（图 2-1/2-2/2-3）。
   文案/数字/逻辑图忠实《产品说明书》§1（背景）、§2.2 图2-1、§2.4 图2-2、§2.5 图2-3。
   bg-context：三拍叙述(痛点→三能力→全栈国产)；arch-nodes：三节点+内部模块+四链路(数据包流动)；
   arch-bus：四层总线分层揭示；arch-flow：端到端时序(DIAGKIT.mountSeq)。
   ⚠ 本挂载 file-tool Edit 会截断——改本文件只用 Write/bash 整文件覆盖。经 FILM.addScene 注册。 */
(function () {
  "use strict";
  const F = window.FILM, DK = window.DIAGKIT, FX = window.FX;
  const rise = F.rise, caption = F.caption, addCSS = F.addCSS, STAGE_W = F.STAGE_W;

  addCSS(
    /* ── 背景叙述 ── */
    '.bg{position:absolute;inset:0;}' +
    '.bg-kick{position:absolute;left:50%;top:150px;transform:translateX(-50%);font-family:var(--font-mono);font-size:22px;letter-spacing:.34em;color:var(--text-secondary);opacity:0;white-space:nowrap;}' +
    '.bg-beat{position:absolute;left:0;right:0;top:262px;bottom:170px;display:flex;flex-direction:column;align-items:center;justify-content:center;opacity:0;}' +
    '.bg-h{font-size:70px;font-weight:800;color:var(--text-title);letter-spacing:-.03em;text-align:center;line-height:1.12;}' +
    '.bg-h b{color:var(--primary);}' +
    '.bg-chips{display:flex;gap:22px;margin-top:42px;}' +
    '.bg-chip{font-size:27px;font-weight:700;border-radius:999px;padding:13px 34px;}' +
    '.bg-chip.bad{color:var(--danger);background:#f6ddd7;}' +
    '.bg-stat{font-family:var(--font-mono);font-size:27px;color:var(--text-secondary);margin-top:40px;letter-spacing:.01em;}' +
    '.bg-stat b{color:var(--primary);font-size:38px;font-weight:700;}' +
    '.bg-cards{display:flex;gap:30px;margin-top:18px;}' +
    '.bg-card{width:392px;background:var(--surface);border-radius:22px;box-shadow:var(--card-shadow);border:1px solid var(--divider);padding:30px 32px;}' +
    '.bg-card .ci{width:58px;height:58px;border-radius:16px;background:var(--primary-soft);display:flex;align-items:center;justify-content:center;}' +
    '.bg-card .ci svg{width:32px;height:32px;}' +
    '.bg-card .ch{font-size:34px;font-weight:800;color:var(--text-title);margin-top:18px;letter-spacing:-.02em;}' +
    '.bg-card .cs{font-size:18px;color:var(--text-secondary);margin-top:11px;line-height:1.55;}' +
    '.bg-pills{display:flex;gap:20px;margin-top:46px;flex-wrap:wrap;justify-content:center;}' +
    '.bg-pill{font-size:27px;font-weight:700;color:var(--primary);background:var(--primary-soft);border-radius:999px;padding:13px 30px;}' +
    /* ── 架构节点图（图 2-1）── */
    '.ax{position:absolute;inset:0;}' +
    '.ax-svg{position:absolute;inset:0;width:100%;height:100%;overflow:visible;}' +
    '.ax-node{position:absolute;background:var(--surface);border-radius:22px;box-shadow:var(--card-shadow);border:1px solid var(--divider);padding:22px 26px;opacity:0;display:flex;flex-direction:column;}' +
    '.ax-node .axh{display:flex;align-items:center;gap:13px;}' +
    '.ax-node .axbadge{width:16px;height:16px;border-radius:5px;flex:none;}' +
    '.ax-node .axt{font-size:29px;font-weight:800;color:var(--text-title);letter-spacing:-.02em;}' +
    '.ax-node .axs{font-size:16px;color:var(--text-secondary);margin-top:7px;font-family:var(--font-mono);}' +
    '.ax-mods{display:flex;flex-wrap:wrap;gap:9px;margin-top:auto;padding-top:17px;}' +
    '.ax-mod{font-size:16px;color:var(--text-body);background:var(--surface-muted);border-radius:9px;padding:7px 13px;}' +
    '.ax-mod.key{background:var(--primary-soft);color:var(--primary-pressed);font-weight:600;}' +
    '.ax-label{position:absolute;transform:translate(-50%,-50%);background:var(--surface);border:1px solid var(--border);border-radius:999px;padding:7px 15px;font-size:17px;font-weight:700;color:var(--text-body);box-shadow:var(--soft-shadow);white-space:nowrap;opacity:0;}' +
    '.ax-label .lp{font-family:var(--font-mono);color:var(--text-secondary);font-weight:500;margin-left:7px;font-size:14px;}' +
    '.ax-label.ws{color:#2f5d96;border-color:#bcd0ea;}.ax-label.ws .lp{color:#6f93c4;}' +
    '.ax-pkt{position:absolute;width:14px;height:14px;border-radius:50%;background:var(--primary);box-shadow:0 1px 4px rgba(0,0,0,.22);opacity:0;margin:-7px 0 0 -7px;will-change:transform,opacity;}' +
    '.ax-pkt.ws{background:var(--info);}' +
    '.ax-foot{position:absolute;left:50%;bottom:96px;transform:translateX(-50%);font-size:18px;color:var(--text-secondary);background:#fbf6e4;border:1px dashed #d8c98a;border-radius:10px;padding:9px 18px;opacity:0;}' +
    '.ax-foot b{color:var(--primary);}' +
    /* ── 四层总线（图 2-2）── */
    '.bus{position:absolute;left:160px;right:160px;top:210px;}' +
    '.bus-row{display:flex;align-items:center;gap:24px;background:var(--surface);border-radius:18px;box-shadow:var(--soft-shadow);border:1px solid var(--divider);border-left:7px solid var(--primary);padding:20px 28px;margin-bottom:22px;opacity:0;}' +
    '.bus-no{font-family:var(--font-mono);font-size:46px;font-weight:700;color:var(--primary);width:64px;flex:none;text-align:center;}' +
    '.bus-mid{flex:1;}' +
    '.bus-t{font-size:27px;font-weight:800;color:var(--text-title);letter-spacing:-.01em;}' +
    '.bus-t .sc{font-size:17px;font-weight:500;color:var(--text-secondary);font-family:var(--font-mono);margin-left:12px;}' +
    '.bus-ch{display:flex;flex-wrap:wrap;gap:8px;margin-top:11px;}' +
    '.bus-tag{font-size:15.5px;color:var(--text-body);background:var(--surface-muted);border-radius:8px;padding:5px 11px;font-family:var(--font-mono);}' +
    '.bus-tag.k{background:var(--primary-soft);color:var(--primary-pressed);}'
  );

  /* ════════════════ 背景叙述（§1）════════════════ */
  const ICON = {
    walk: '<svg viewBox="0 0 24 24" fill="none" stroke="#485c11" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round"><circle cx="13" cy="4" r="1.6"/><path d="M11 21l1.5-6 2.5 2 1.5 4M12.5 9l3 1.5 2-2M12.5 9L9 11l-1 4"/></svg>',
    eye: '<svg viewBox="0 0 24 24" fill="none" stroke="#485c11" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round"><path d="M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7-10-7-10-7z"/><circle cx="12" cy="12" r="3"/></svg>',
    net: '<svg viewBox="0 0 24 24" fill="none" stroke="#485c11" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round"><circle cx="6" cy="6" r="2.4"/><circle cx="18" cy="6" r="2.4"/><circle cx="12" cy="18" r="2.4"/><path d="M7.6 7.8l3 8M16.4 7.8l-3 8M8 6h8"/></svg>'
  };
  function buildBg(root) {
    root.classList.add('paper');
    const wrap = document.createElement('div'); wrap.className = 'bg'; root.appendChild(wrap);
    const kick = document.createElement('div'); kick.className = 'bg-kick'; kick.textContent = '项目背景 · 工业巡检的刚需与困境';
    wrap.appendChild(kick);
    // 拍1：人工巡检三重困境
    const b1 = document.createElement('div'); b1.className = 'bg-beat';
    b1.innerHTML = '<div class="bg-h">人工巡检：高频 · 高危 · 高成本的<b>刚需</b></div>' +
      '<div class="bg-chips"><div class="bg-chip bad">效率低</div><div class="bg-chip bad">风险高</div><div class="bg-chip bad">数据孤岛</div></div>' +
      '<div class="bg-stat">全区域巡检：人工约 <b>300</b> min ，智能系统约 <b>120</b> min（效率差 2.5×）</div>';
    // 拍2：会走 · 会看 · 会协同
    const b2 = document.createElement('div'); b2.className = 'bg-beat';
    b2.innerHTML = '<div class="bg-h">让机器人代替人——<b>会走 · 会看 · 会协同</b></div>' +
      '<div class="bg-cards">' +
      '<div class="bg-card"><div class="ci">' + ICON.walk + '</div><div class="ch">会走</div><div class="cs">无人遥控、无预先地图，自主 SLAM 建图定位，规划不重不漏的全覆盖路径并实时避障。</div></div>' +
      '<div class="bg-card"><div class="ci">' + ICON.eye + '</div><div class="ch">会看</div><div class="cs">行进中实时识别工业仪表、定位表盘关键点，几何换算出指针读数，越限告警。</div></div>' +
      '<div class="bg-card"><div class="ci">' + ICON.net + '</div><div class="ch">会协同 · 会汇报</div><div class="cs">多车同坐标系分工协同，位姿/进度/读数实时汇聚到统一终端供管理者决策。</div></div>' +
      '</div>';
    // 拍3：全栈国产自主
    const b3 = document.createElement('div'); b3.className = 'bg-beat';
    b3.innerHTML = '<div class="bg-h">三类能力 ，<b>全栈国产自主</b>实现</div>' +
      '<div class="bg-pills"><div class="bg-pill">开源鸿蒙 OpenHarmony</div><div class="bg-pill">华为昇腾 NPU</div><div class="bg-pill">国产 ArkTS</div><div class="bg-pill">零 ROS 自研</div></div>';
    wrap.appendChild(b1); wrap.appendChild(b2); wrap.appendChild(b3);
    return { wrap: wrap, kick: kick, b1: b1, b2: b2, b3: b3 };
  }
  function animBg(tl, s) {
    const a = s.start, r = s.refs;
    tl.fromTo(r.kick, { opacity: 0, y: 12 }, { opacity: 1, y: 0, duration: 0.7, ease: 'power3.out' }, a + 0.3);
    // 拍1
    tl.fromTo(r.b1, { opacity: 0, y: 26, filter: 'blur(10px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: 0.85, ease: 'power4.out' }, a + 0.7);
    caption(tl, a + 0.9, 2.8, '<b>巡检：高频、高危、高成本的刚需</b>', '人工抄表效率低、风险高、数据成孤岛');
    tl.to(r.b1, { opacity: 0, y: -18, filter: 'blur(8px)', duration: 0.55, ease: 'power2.in' }, a + 3.9);
    // 拍2
    tl.fromTo(r.b2, { opacity: 0, y: 26, filter: 'blur(10px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: 0.85, ease: 'power4.out' }, a + 4.4);
    caption(tl, a + 4.6, 3.0, '<b>会走 · 会看 · 会协同——缺一不可</b>', '自主移动全覆盖 · 视觉读表分析 · 多机协同汇报');
    tl.to(r.b2, { opacity: 0, y: -18, filter: 'blur(8px)', duration: 0.55, ease: 'power2.in' }, a + 7.7);
    // 拍3
    tl.fromTo(r.b3, { opacity: 0, y: 26, filter: 'blur(10px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: 0.85, ease: 'power4.out' }, a + 8.2);
    caption(tl, a + 8.4, 3.0, '<b>从内核到算力，全链路国产自主</b>', '开源鸿蒙 · 昇腾 · ArkTS · 零 ROS——天然契合信创');
    tl.fromTo(r.wrap, { scale: 1.0 }, { scale: 1.02, duration: s.dur, ease: 'none' }, a);
  }

  /* ════════════════ 图 2-1 系统总体架构（三节点 + 内部模块 + 四链路）════════════════ */
  const AXN = [
    { k: 'app', x: 150, y: 392, w: 560, h: 330, badge: 'var(--primary)', t: '鸿蒙平板 · ArkTS', s: '总控 / 交互 / 可视化',
      mods: [['控制·地图·视觉页', 0], ['RobotTransport', 1], ['FleetMissionService', 1], ['MapService', 0], ['VisionService', 1]] },
    { k: 'pi', x: 1210, y: 150, w: 560, h: 300, badge: 'var(--primary-pressed)', t: '紫派 · OpenHarmony 5.0', s: 'RK3566 · 运动主控',
      mods: [['udp2lcm', 1], ['Navi · SLAM/A*/DWA/BCD', 1], ['serial 轮控', 0], ['lidar 雷达', 0], ['car-agent', 0]] },
    { k: 'oz', x: 1210, y: 660, w: 560, h: 280, badge: 'var(--warning)', t: '香橙派 · 昇腾 310B', s: 'NPU · 视觉智能',
      mods: [['FastAPI :8000', 1], ['video_camera · YOLO+关键点', 1], ['DeepSeek 端侧', 0], ['数据采集/存储', 0]] }
  ];
  // 链路：x0/y0(App 右缘) → x1/y1(目标左缘)；rev=反向(心跳回传)；ws=蓝色视觉链路
  const AXL = [
    { x0: 710, y0: 452, x1: 1210, y1: 232, name: 'UDP :5001 · 9 字节', port: '控制命令', lx: 905, ly: 322 },
    { x0: 1210, y0: 288, x1: 710, y1: 512, name: '500ms 心跳 · 位姿', port: '回传', rev: true, lx: 1010, ly: 392 },
    { x0: 710, y0: 572, x1: 1210, y1: 360, name: 'HTTP :8000 · 拉地图', port: 'ZMAP1', lx: 905, ly: 488 },
    { x0: 710, y0: 632, x1: 1210, y1: 770, name: 'WS :8000 · 视频 + 读数', port: 'JSON', ws: true, lx: 930, ly: 712 },
    { x0: 710, y0: 692, x1: 1210, y1: 416, name: '软总线 · DDO 黑板', port: 'FleetMission', lx: 928, ly: 600 }
  ];
  function buildArchNodes(root) {
    root.classList.add('paper');
    const cam = document.createElement('div'); cam.className = 'cam'; root.appendChild(cam);
    const wrap = document.createElement('div'); wrap.className = 'ax'; cam.appendChild(wrap);
    let svg = '<svg class="ax-svg" viewBox="0 0 ' + STAGE_W + ' ' + F.STAGE_H + '"><defs>' +
      '<marker id="ax-ar-g" viewBox="0 0 10 10" refX="8.5" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="#485c11"/></marker>' +
      '<marker id="ax-ar-b" viewBox="0 0 10 10" refX="8.5" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="#4678b8"/></marker></defs>';
    AXL.forEach(function (l, i) {
      const col = l.ws ? '#4678b8' : '#485c11', mk = l.ws ? 'ax-ar-b' : 'ax-ar-g';
      svg += '<line data-ln="' + i + '" x1="' + l.x0 + '" y1="' + l.y0 + '" x2="' + l.x1 + '" y2="' + l.y1 + '" stroke="' + col + '" stroke-width="2.6" stroke-linecap="round" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1" marker-end="url(#' + mk + ')"/>';
    });
    svg += '</svg>';
    wrap.innerHTML = svg;
    const nodeEls = {};
    AXN.forEach(function (n) {
      const d = document.createElement('div'); d.className = 'ax-node';
      d.style.cssText = 'left:' + n.x + 'px;top:' + n.y + 'px;width:' + n.w + 'px;height:' + n.h + 'px;';
      d.innerHTML = '<div class="axh"><span class="axbadge" style="background:' + n.badge + '"></span><span class="axt">' + n.t + '</span></div>' +
        '<div class="axs">' + n.s + '</div><div class="ax-mods">' + n.mods.map(function (m) { return '<span class="ax-mod' + (m[1] ? ' key' : '') + '">' + m[0] + '</span>'; }).join('') + '</div>';
      wrap.appendChild(d); nodeEls[n.k] = d;
    });
    const labels = [], pkts = [];
    AXL.forEach(function (l) {
      const lab = document.createElement('div'); lab.className = 'ax-label' + (l.ws ? ' ws' : '');
      lab.style.cssText = 'left:' + l.lx + 'px;top:' + l.ly + 'px;';
      lab.innerHTML = l.name + '<span class="lp">' + l.port + '</span>';
      wrap.appendChild(lab); labels.push(lab);
      const p = document.createElement('div'); p.className = 'ax-pkt' + (l.ws ? ' ws' : '');
      p.style.cssText = 'left:' + l.x0 + 'px;top:' + l.y0 + 'px;'; p._dx = l.x1 - l.x0; p._dy = l.y1 - l.y0;
      wrap.appendChild(p); pkts.push(p);
    });
    const foot = document.createElement('div'); foot.className = 'ax-foot';
    foot.innerHTML = '车 ↔ 车：<b>COOP_AVOID</b> 多播 LCM 协同避障（ttl=1，不经平板、不经软总线）';
    wrap.appendChild(foot);
    return { cam: cam, wrap: wrap, nodes: nodeEls, lines: Array.prototype.slice.call(wrap.querySelectorAll('line')), labels: labels, pkts: pkts, foot: foot };
  }
  function drawLink2(tl, at, line, label, pkt) {
    tl.fromTo(line, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: 0.8, ease: 'power2.inOut' }, at);
    tl.fromTo(label, { opacity: 0, scale: 0.82 }, { opacity: 1, scale: 1, duration: 0.5, ease: 'back.out(1.4)' }, at + 0.42);
    tl.fromTo(pkt, { opacity: 0, x: 0, y: 0 }, { opacity: 1, duration: 0.22 }, at + 0.55);
    tl.to(pkt, { x: pkt._dx, y: pkt._dy, duration: 1.0, ease: 'power1.inOut', repeat: 2, repeatDelay: 0.3 }, at + 0.55);
  }
  function animArchNodes(tl, s) {
    const a = s.start, r = s.refs;
    FX.enter(tl, r.nodes.app, a + 0.3, 'left', { dur: 0.9 });          // 多样入场：左滑 / 右滑 / 缩放模糊
    FX.enter(tl, r.nodes.pi, a + 0.7, 'right', { dur: 0.9 });
    FX.enter(tl, r.nodes.oz, a + 1.1, 'zoomBlur', { dur: 0.9, origin: '50% 50%' });
    caption(tl, a + 0.6, 2.8, '<b>三节点 · 各司其职</b>', '平板总控 · 紫派运动主控 · 香橙派视觉智能');
    // 五条链路连贯接通（不再等待，一气呵成）
    drawLink2(tl, a + 2.0, r.lines[0], r.labels[0], r.pkts[0]);
    drawLink2(tl, a + 2.4, r.lines[1], r.labels[1], r.pkts[1]);
    drawLink2(tl, a + 2.8, r.lines[2], r.labels[2], r.pkts[2]);
    drawLink2(tl, a + 3.2, r.lines[4], r.labels[4], r.pkts[4]);
    drawLink2(tl, a + 3.6, r.lines[3], r.labels[3], r.pkts[3]);   // 香橙派 WS：紧接紫派四线，一起连完
    caption(tl, a + 3.6, 3.0, '<b>控制/数据/协同 → 紫派 · 视觉 WS → 香橙派</b>', '导航链路与视觉链路物理解耦、互不打扰');
    tl.fromTo(r.foot, { opacity: 0, y: 10 }, { opacity: 1, y: 0, duration: 0.6, ease: 'power3.out' }, a + 4.9);
    tl.to(r.pkts, { opacity: 0, duration: 0.5 }, a + s.dur - 0.6);
    tl.fromTo(r.wrap, { scale: 1.0 }, { scale: 1.018, duration: s.dur, ease: 'none' }, a);
  }

  /* ════════════════ 图 2-2 四层通信总线 ════════════════ */
  const BUS = [
    { no: '①', t: '设备内总线 · LCM', sc: '紫派机内 · UDP 组播 ttl=0', tags: [['HOKUYO_LIDAR', 0], ['POSE', 0], ['PATH', 1], ['ROBOT_CONTROL', 0], ['CURRENTPOSE', 0]] },
    { no: '②', t: '控制总线 · UDP 5001', sc: 'App ↔ 车 · 9 字节大端', tags: [['命令 state/run/speed/endX/endY', 1], ['↕ 心跳 state/x/y/r + 协同态', 0]] },
    { no: '③', t: '数据总线 · HTTP / WebSocket', sc: '车 → App · 大块 / 流', tags: [['HTTP:8000 地图文件', 1], ['WS:8000 视频帧 + 读数 JSON', 1]] },
    { no: '④', t: '协同总线 · 软总线 + 多播 LCM', sc: '跨设备 / 车 ↔ 车', tags: [['软总线 DDO 共享黑板 FleetMission', 1], ['COOP_AVOID 多播 LCM ttl=1', 0]] }
  ];
  function buildBus(root) {
    root.classList.add('paper');
    const wrap = document.createElement('div'); wrap.className = 'bus'; root.appendChild(wrap);
    const rows = [];
    BUS.forEach(function (b) {
      const d = document.createElement('div'); d.className = 'bus-row';
      d.innerHTML = '<div class="bus-no">' + b.no + '</div><div class="bus-mid"><div class="bus-t">' + b.t + '<span class="sc">' + b.sc + '</span></div>' +
        '<div class="bus-ch">' + b.tags.map(function (t) { return '<span class="bus-tag' + (t[1] ? ' k' : '') + '">' + t[0] + '</span>'; }).join('') + '</div></div>';
      wrap.appendChild(d); rows.push(d);
    });
    return { wrap: wrap, rows: rows };
  }
  function animBus(tl, s) {
    const a = s.start, r = s.refs;
    caption(tl, a + 0.5, 2.6, '<b>四层总线 · 各司其职</b>', '从机内到跨设备 · 从小包到大块 · 从控制到协同');
    r.rows.forEach(function (d, i) {
      tl.fromTo(d, { opacity: 0, x: -28, filter: 'blur(8px)' }, { opacity: 1, x: 0, filter: 'blur(0px)', duration: 0.6, ease: 'power3.out' }, a + 0.7 + i * 0.48);
    });
    caption(tl, a + 3.3, 3.0, '<b>任何一比特数据，都能在四层里找到归属</b>', '①机内 LCM · ②UDP 控制 · ③HTTP/WS 数据 · ④软总线协同');
    tl.fromTo(r.wrap, { scale: 1.0 }, { scale: 1.012, duration: s.dur, ease: 'none' }, a);
  }

  /* ════════════════ 图 2-3 端到端时序（单车巡检）════════════════ */
  function buildArchFlow(root) {
    root.classList.add('paper');
    const cam = document.createElement('div'); cam.className = 'cam'; root.appendChild(cam);
    const r = DK.mountSeq(cam, {
      fig: '图 2-3', title: '单车巡检任务 · 端到端时序', top: 222, bottom: 928,
      actors: [
        { k: 'U', x: 300, label: '操作者', sub: 'operator', tone: 'neutral' },
        { k: 'T', x: 720, label: '鸿蒙平板', sub: 'tablet', tone: 'app' },
        { k: 'P', x: 1200, label: '紫派', sub: 'udp2lcm/Navi', tone: 'pi' },
        { k: 'O', x: 1640, label: '香橙派', sub: 'vision', tone: 'vision' }
      ],
      msgs: [
        { from: 'U', to: 'T', y: 318, label: '① 开始建图', tone: 'neutral', g: 1 },
        { from: 'T', to: 'P', y: 362, label: "UDP cmd 'm' 强制建图", tone: 'app', g: 1 },
        { from: 'P', to: 'P', y: 404, self: true, label: 'SLAM 扫描匹配建图', tone: 'pi', g: 2 },
        { from: 'P', to: 'T', y: 470, label: '500ms 心跳 · 实时位姿', tone: 'pi', dash: true, g: 2 },
        { from: 'U', to: 'T', y: 516, label: '③ 结束建图', tone: 'neutral', g: 3 },
        { from: 'T', to: 'P', y: 558, label: 'HTTP GET /zipedMap.txt', tone: 'app', g: 3 },
        { from: 'P', to: 'T', y: 600, label: '压缩地图 ZMAP1', tone: 'pi', dash: true, g: 3 },
        { from: 'U', to: 'T', y: 648, label: '④ 框选区域 · 开始覆盖', tone: 'neutral', g: 4 },
        { from: 'T', to: 'P', y: 690, label: 'cmd 107/108 覆盖矩形', tone: 'app', g: 4 },
        { from: 'P', to: 'P', y: 732, self: true, label: 'BCD 全覆盖 + A*/DWA', tone: 'pi', g: 4 },
        { from: 'T', to: 'O', y: 812, label: '⑤ WebSocket /ws/video（全程并行）', tone: 'vision', g: 5 },
        { from: 'O', to: 'T', y: 856, label: 'JPEG 帧 + 读数 JSON', tone: 'vision', dash: true, g: 5 }
      ]
    });
    return { cam: cam, dk: r };
  }
  function animArchFlow(tl, s) {
    const a = s.start, r = s.refs;
    DK.revealSeq(tl, a + 0.3, r.dk, { actorStep: 0.15, msgStep: 0.54 });
    caption(tl, a + 0.6, 2.6, '<b>一次完整的单车巡检</b>', '建图 → 取图 → 覆盖；视觉链路全程并行');
    caption(tl, a + 5.2, 3.0, '<b>运动链路与视觉链路：时间并行、物理解耦</b>', '只在平板 UI 上"会师"，由操作者统一感知');
    tl.fromTo(r.cam, { scale: 1.0 }, { scale: 1.012, duration: s.dur, ease: 'none' }, a);
  }

  F.addScene({ id: 'bg-context', dur: 13, build: buildBg, anim: animBg });
  F.addScene({ id: 'arch-nodes', dur: 9.5, build: buildArchNodes, anim: animArchNodes });
  F.addScene({ id: 'arch-bus', dur: 8, build: buildBus, anim: animBus });
  F.addScene({ id: 'arch-flow', dur: 10, build: buildArchFlow, anim: animArchFlow });
})();
