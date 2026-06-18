/* scenes0_mapkit.js —— 演示片的"地图工具箱"（window.MAPKIT），与 App MapCanvas/mapContour 同源。
   忠实复刻真机渲染：
     · 墙体 = 栅格 → marching squares → linkLoops → chaikin（移植 tools/verify/verify.mjs）→ 平滑矢量等高线
       （不再画干净矩形房间），软填充墙 cell + 圆角描边；底为浅图纸 + 极淡比例网格。
     · 机器人 = 圆点 + 白环 + **朝向三角**（随运动方向转头），软投影。
     · 轨迹 = **彗尾拖尾**（软辉光底 + 渐亮头部），覆盖即"刷宽拖尾"扫满区域（通过轨迹判断进度）。
     · driveRobot：先转向后行驶（turn-in-place）+ 加减速 → 不再僵硬丝滑。
   该文件先于 scenes2/scenes8 加载（文件名排序 scenes.js < scenes0_mapkit.js < scenes2.js）。 */
window.MAPKIT = (function () {
  "use strict";
  const NS = 'http://www.w3.org/2000/svg';

  /* ── 移植自 tools/verify/verify.mjs：marching squares → linkLoops → chaikin ── */
  function marchingSquares(g) {
    const b = g.b, w = g.w, h = g.h, segs = [];
    for (let y = 0; y < h - 1; y++) for (let x = 0; x < w - 1; x++) {
      const tl = b[y * w + x], tr = b[y * w + x + 1], br = b[(y + 1) * w + x + 1], bl = b[(y + 1) * w + x];
      const idx = tl * 8 + tr * 4 + br * 2 + bl * 1;
      if (idx === 0 || idx === 15) continue;
      const tx = x + 0.5, ty = y, rx = x + 1, ry = y + 0.5, bx = x + 0.5, by = y + 1, lx = x, ly = y + 0.5;
      switch (idx) {
        case 1: segs.push(lx, ly, bx, by); break;
        case 2: segs.push(bx, by, rx, ry); break;
        case 3: segs.push(lx, ly, rx, ry); break;
        case 4: segs.push(tx, ty, rx, ry); break;
        case 5: segs.push(lx, ly, tx, ty); segs.push(bx, by, rx, ry); break;
        case 6: segs.push(tx, ty, bx, by); break;
        case 7: segs.push(lx, ly, tx, ty); break;
        case 8: segs.push(tx, ty, lx, ly); break;
        case 9: segs.push(tx, ty, bx, by); break;
        case 10: segs.push(tx, ty, rx, ry); segs.push(lx, ly, bx, by); break;
        case 11: segs.push(tx, ty, rx, ry); break;
        case 12: segs.push(lx, ly, rx, ry); break;
        case 13: segs.push(rx, ry, bx, by); break;
        case 14: segs.push(lx, ly, bx, by); break;
        default: break;
      }
    }
    return segs;
  }
  function ptKeyC(x, y) { return Math.round(x * 2) + '_' + Math.round(y * 2); }
  function linkLoops(segs) {
    const n = Math.floor(segs.length / 4), used = new Uint8Array(n), adj = new Map();
    for (let s = 0; s < n; s++) {
      const ka = ptKeyC(segs[s * 4], segs[s * 4 + 1]), kb = ptKeyC(segs[s * 4 + 2], segs[s * 4 + 3]);
      if (!adj.has(ka)) adj.set(ka, []); adj.get(ka).push(s);
      if (!adj.has(kb)) adj.set(kb, []); adj.get(kb).push(s);
    }
    const step = (cx, cy) => {
      const cand = adj.get(ptKeyC(cx, cy)); if (cand === undefined) return null;
      for (const t of cand) {
        if (used[t] === 1) continue; used[t] = 1;
        const px = segs[t * 4], py = segs[t * 4 + 1], qx = segs[t * 4 + 2], qy = segs[t * 4 + 3];
        if (ptKeyC(px, py) === ptKeyC(cx, cy)) return [qx, qy];
        return [px, py];
      }
      return null;
    };
    const out = [];
    for (let s = 0; s < n; s++) {
      if (used[s] === 1) continue; used[s] = 1;
      const ax = segs[s * 4], ay = segs[s * 4 + 1];
      const pts = [ax, ay, segs[s * 4 + 2], segs[s * 4 + 3]];
      let cx = pts[2], cy = pts[3], closed = false;
      while (true) { const nxt = step(cx, cy); if (nxt === null) break; cx = nxt[0]; cy = nxt[1]; pts.push(cx, cy); if (ptKeyC(cx, cy) === ptKeyC(ax, ay)) { closed = true; break; } }
      out.push({ pts, closed });
    }
    return out;
  }
  function chaikin(pts, closed, iters) {
    let cur = pts;
    for (let it = 0; it < iters; it++) {
      const m = Math.floor(cur.length / 2); if (m < 3) break;
      const next = []; if (!closed) next.push(cur[0], cur[1]);
      const segCount = closed ? m : m - 1;
      for (let i = 0; i < segCount; i++) {
        const i1 = (i + 1) % m, x0 = cur[i * 2], y0 = cur[i * 2 + 1], x1 = cur[i1 * 2], y1 = cur[i1 * 2 + 1];
        next.push(x0 * 0.75 + x1 * 0.25, y0 * 0.75 + y1 * 0.25);
        next.push(x0 * 0.25 + x1 * 0.75, y0 * 0.25 + y1 * 0.75);
      }
      if (!closed) next.push(cur[(m - 1) * 2], cur[(m - 1) * 2 + 1]);
      cur = next;
    }
    return cur;
  }

  /* ── 确定性 PRNG + "类 SLAM"有机栅格生成 ── */
  function mulberry32(a) { return function () { a |= 0; a = a + 0x6D2B79F5 | 0; let t = Math.imul(a ^ a >>> 15, 1 | a); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; }; }
  /** 生成 W×H 栅格（1=墙）：抖动厚墙边界（噪声）+ 散落障碍簇 + 中央结构块 → 经等高线渲染后呈有机楼面图。 */
  function makeGrid(W, H, seed) {
    const rnd = mulberry32(seed >>> 0), b = new Uint8Array(W * H);
    const set = (x, y, v) => { if (x >= 0 && x < W && y >= 0 && y < H) b[y * W + x] = v; };
    const mIn = Math.round(W * 0.08), band = Math.max(2, Math.round(W * 0.035));
    const noise = (x, y) => Math.sin(x * 0.45 + seed) * 1.4 + Math.cos(y * 0.52 + seed * 1.7) * 1.4 + (rnd() - 0.5) * 1.3;
    // 1) 抖动厚墙边界（自由区边界外 band 厚的墙带）
    for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
      const n = noise(x, y);
      const d = Math.min(x - (mIn + n), (W - 1 - mIn + n) - x, y - (mIn + n), (H - 1 - mIn - n) - y);
      if (d < 0 && d > -band) b[y * W + x] = 1;
    }
    const freeAt = (x, y) => {
      const n = noise(x, y);
      return Math.min(x - (mIn + n), (W - 1 - mIn + n) - x, y - (mIn + n), (H - 1 - mIn - n) - y) > band * 0.6;
    };
    // 2) 中央结构块（带圆角缺口，像桌台/隔断）
    const bx0 = Math.round(W * 0.30), by0 = Math.round(H * 0.30), bw = Math.round(W * 0.22), bh = Math.round(H * 0.26);
    for (let y = by0; y < by0 + bh; y++) for (let x = bx0; x < bx0 + bw; x++) if (freeAt(x, y)) set(x, y, 1);
    // 3) 散落障碍簇
    for (let k = 0; k < 13; k++) {
      const ox = Math.round(mIn + 3 + rnd() * (W - 2 * mIn - 6)), oy = Math.round(mIn + 3 + rnd() * (H - 2 * mIn - 6));
      if (!freeAt(ox, oy)) { continue; }
      const rr = 1 + Math.floor(rnd() * 2);
      for (let dy = -rr; dy <= rr; dy++) for (let dx = -rr; dx <= rr; dx++) if (dx * dx + dy * dy <= rr * rr + 1 && freeAt(ox + dx, oy + dy)) set(ox + dx, oy + dy, 1);
    }
    return { b, w: W, h: H, f: 1 };
  }

  /* ── 栅格 → SVG（浅图纸 + 网格 + 墙软填充 + 平滑等高线描边）。viewBox 0 0 PW PH。 ── */
  const TH = {
    bg: '#e9e7dc', floor: '#fbfaf4', grid: 'rgba(72,92,17,.06)', floorEdge: '#e3e7d2',
    wallFill: 'rgba(72,92,17,.13)', wallStroke: '#4a5d18',
    green: '#357a41', blue: '#4678b8', target: '#d9503f', region: '#485c11'
  };
  function mapSVG(grid, PW, PH, iters) {
    const sx = PW / grid.w, sy = PH / grid.h;
    const loops = linkLoops(marchingSquares(grid));
    let stroke = '';
    for (const c of loops) {
      const sm = chaikin(c.pts, c.closed, iters == null ? 2 : iters);
      if (sm.length < 6) continue;
      let d = 'M' + (sm[0] * sx).toFixed(1) + ' ' + (sm[1] * sy).toFixed(1);
      for (let i = 2; i < sm.length; i += 2) d += 'L' + (sm[i] * sx).toFixed(1) + ' ' + (sm[i + 1] * sy).toFixed(1);
      if (c.closed) d += 'Z';
      stroke += '<path d="' + d + '"/>';
    }
    let fill = '';
    for (let cy = 0; cy < grid.h; cy++) for (let cx = 0; cx < grid.w; cx++) {
      if (grid.b[cy * grid.w + cx] === 1) fill += '<rect x="' + (cx * sx).toFixed(1) + '" y="' + (cy * sy).toFixed(1) + '" width="' + (sx + 0.6).toFixed(1) + '" height="' + (sy + 0.6).toFixed(1) + '"/>';
    }
    let g = '';
    const gx = PW / 22, gy = PH / 22;
    for (let i = 1; i < 22; i++) { g += '<line x1="' + (i * gx).toFixed(1) + '" y1="0" x2="' + (i * gx).toFixed(1) + '" y2="' + PH + '"/>'; g += '<line x1="0" y1="' + (i * gy).toFixed(1) + '" x2="' + PW + '" y2="' + (i * gy).toFixed(1) + '"/>'; }
    return '<rect width="' + PW + '" height="' + PH + '" fill="' + TH.floor + '"/>' +
      '<g stroke="' + TH.grid + '" stroke-width="1">' + g + '</g>' +
      '<g class="mk-walls" fill="' + TH.wallFill + '">' + fill + '</g>' +
      '<g class="mk-wallstroke" fill="none" stroke="' + TH.wallStroke + '" stroke-width="2.4" stroke-linejoin="round" stroke-linecap="round">' + stroke + '</g>';
  }

  /* ── 机器人（圆点 + 白环 + 朝向三角；children 绕局部 0,0，便于 GSAP x/y/rotation）── */
  function robotSVG(cls, color, n, R) {
    R = R == null ? 9 : R; const tip = R + 9, base = R + 1, hb = 5;
    return '<g class="' + cls + '" fill="' + color + '">' +
      '<polygon class="mk-head" points="' + tip + ',0 ' + base + ',' + (-hb) + ' ' + base + ',' + hb + '"/>' +
      '<circle r="' + R + '"/><circle r="' + R + '" fill="none" stroke="#fff" stroke-width="2"/>' +
      '<text y="0.5" fill="#fff" font-size="' + (R + 2) + '" font-weight="700" text-anchor="middle" dominant-baseline="middle">' + n + '</text></g>';
  }

  function angTo(ax, ay, bx, by) { return Math.atan2(by - ay, bx - ax) * 180 / Math.PI; }
  function shortDelta(from, to) { let d = (to - from) % 360; if (d > 180) d -= 360; if (d < -180) d += 360; return d; }

  /**
   * 驱动机器人沿 pts 行走（更真实：每段先转向后行驶 + 头尾加减速），并生长拖尾。
   * opts: { tl, robot, trail(可空), pts:[[x,y]...], t0, segDur, turnDur, ease }
   * 返回结束时间。robot 用 x/y/rotation（绕局部 0,0 = 车心）。
   * 拖尾：trail 为 <path>（pathLength=1, dasharray 1, dashoffset 1）；按总时长把 dashoffset 1→0 同步揭示。
   */
  function driveRobot(opts) {
    const tl = opts.tl, robot = opts.robot, trail = opts.trail, pts = opts.pts;
    const segDur = opts.segDur == null ? 0.9 : opts.segDur, turnDur = opts.turnDur == null ? 0.28 : opts.turnDur;
    let t = opts.t0, heading = opts.heading == null ? angTo(pts[0][0], pts[0][1], pts[1][0], pts[1][1]) : opts.heading;
    tl.set(robot, { x: pts[0][0], y: pts[0][1], rotation: heading, transformOrigin: '0px 0px' }, t);
    const totalDur = (pts.length - 1) * (segDur + turnDur);
    if (trail) {
      tl.set(trail, { attr: { 'stroke-dashoffset': 1 } }, t);
      tl.to(trail, { attr: { 'stroke-dashoffset': 0 }, duration: totalDur, ease: 'none' }, t + turnDur);
    }
    for (let i = 1; i < pts.length; i++) {
      const want = angTo(pts[i - 1][0], pts[i - 1][1], pts[i][0], pts[i][1]);
      const nh = heading + shortDelta(heading, want);
      tl.to(robot, { rotation: nh, duration: turnDur, ease: 'power2.inOut' }, t);      // 先转向（原地）
      t += turnDur; heading = nh;
      tl.to(robot, { x: pts[i][0], y: pts[i][1], duration: segDur, ease: opts.ease || 'power1.inOut' }, t);  // 后行驶（加减速）
      t += segDur;
    }
    return t;
  }

  /** 轨迹 <path> 的 d（折线），pathLength=1 便于 dashoffset 揭示。 */
  function polyPath(pts) {
    let d = 'M' + pts[0][0].toFixed(1) + ' ' + pts[0][1].toFixed(1);
    for (let i = 1; i < pts.length; i++) d += 'L' + pts[i][0].toFixed(1) + ' ' + pts[i][1].toFixed(1);
    return d;
  }
  /** 牛耕折返路径点（在矩形 x0..x1,y0..y1 内，行距 step；用于覆盖刷扫）。 */
  function boustro(x0, y0, x1, y1, step) {
    const pts = []; let y = y0, left = true;
    while (y <= y1 + 0.1) {
      if (left) { pts.push([x0, y]); pts.push([x1, y]); } else { pts.push([x1, y]); pts.push([x0, y]); }
      y += step; left = !left;
    }
    return pts;
  }

  return {
    TH, makeGrid, mapSVG, robotSVG, driveRobot, polyPath, boustro, angTo,
    extractContoursPx: function (grid, PW, PH, iters) {
      const sx = PW / grid.w, sy = PH / grid.h, out = [];
      for (const c of linkLoops(marchingSquares(grid))) {
        const sm = chaikin(c.pts, c.closed, iters == null ? 2 : iters), m = [];
        for (let i = 0; i < sm.length; i += 2) m.push(sm[i] * sx, sm[i + 1] * sy);
        out.push({ pts: m, closed: c.closed });
      }
      return out;
    }
  };
})();
