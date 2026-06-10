// 纯逻辑验证（可在 PC 上用 Node 直接跑）。
//
// 这里的函数是 app-harmony 里几个纯算法 .ets 的【逐行镜像】，用来在没有 DevEco/真机时
// 立即验证：① UDP 9 字节编解码（大端）② 坐标换算互逆 ③ 地图首行/包围盒解析
//   ④ 共享层副本同步：car-agent 复制的 model/constants/utils/RobotTransport 与 app-harmony 逐字节一致 + BUNDLE_NAME 一致。
// 镜像对象：
//   - model/protocol.ets   encodeSend / decodeReceive
//   - model/geometry.ets   canvasToMap / mapToCanvas
//   - service/MapService.ets parseMap（首行 + 包围盒 + 正方形化部分）
//   - model/mapContour.ets  marchingSquares / linkLoops / chaikin / extractContours（栅格→平滑矢量等高线）
// ⚠️ 改了上述 .ets 的算法，请同步改这里，保持镜像一致。
// ⚠️ 改了 app-harmony 的共享层文件（④ 列出的），务必同步到 car-agent —— 本脚本 ④ 会逐字节守卫。
//
// 用法： node tools/verify/verify.mjs

import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const FIXTURE = join(__dirname, '..', '..', 'contracts', 'fixtures', 'defultMap.txt');

// ── 镜像：model/protocol.ets ────────────────────────────────────────────────
function encodeSend(d) {
  const buffer = new ArrayBuffer(9);
  const bytes = new Uint8Array(buffer);
  bytes[0] = d.state;
  bytes[1] = d.runState ?? 0;
  bytes[2] = d.speed ?? 0;
  const view = new DataView(buffer);
  view.setInt16(3, d.endX ?? 0, false); // 大端
  view.setInt16(5, d.endY ?? 0, false);
  return buffer;
}
function decodeReceive(buffer) {
  const bytes = new Uint8Array(buffer);
  const view = new DataView(buffer);
  return { state: bytes[0], x: view.getInt16(3, false), y: view.getInt16(5, false), r: view.getInt16(7, false) };
}

// ── 镜像：model/geometry.ets ────────────────────────────────────────────────
function canvasToMap(c, t, half) {
  const halfTxt = t.txtAverSize / 2;
  return {
    x: Math.round((c.x + half) / t.gridSize + t.startX - halfTxt),
    y: -Math.round((c.y + half) / t.gridSize + halfTxt - t.endY),
  };
}
function mapToCanvas(m, t, half) {
  const halfTxt = t.txtAverSize / 2;
  return {
    x: Math.round((m.x - t.startX + halfTxt) * t.gridSize - half),
    y: Math.round((t.endY - halfTxt - m.y) * t.gridSize - half),
  };
}

// ── 镜像：service/MapService.ets parseMap（首行按位置 parts[2]/[3]；数据行归一化 空格(-1/0) 或 密排(1/0)）──
function parseRow(line) {
  const t = line.trim();
  if (t.length === 0) return new Uint8Array(0);
  if (t.indexOf(' ') >= 0) {              // 空格分隔（defultMap.txt）：-1=障碍
    const toks = t.split(/\s+/);
    const cells = new Uint8Array(toks.length);
    for (let i = 0; i < toks.length; i++) cells[i] = parseInt(toks[i], 10) < 0 ? 1 : 0;
    return cells;
  }
  const cells = new Uint8Array(t.length); // 密排（defultMap.txt.txt）：'1'=障碍
  for (let i = 0; i < t.length; i++) cells[i] = t[i] === '1' ? 1 : 0;
  return cells;
}
function parseMap(text, canvasW, canvasH) {
  const rawLines = text.split('\n');
  const grid = [];
  for (let y = 0; y < rawLines.length; y++) grid.push(y === 0 ? new Uint8Array(0) : parseRow(rawLines[y]));
  const headerParts = (rawLines[0] ?? '').trim().split(/\s+/);
  let height = parseInt(headerParts[2], 10);  // 位置：range resolution height width ...
  let width = parseInt(headerParts[3], 10);
  if (!(height > 0) || !(width > 0)) {        // 回退：首行非标准 → 按数据推断
    let maxCols = 0, dataRows = 0;
    for (let y = 1; y < grid.length; y++) if (grid[y].length > 0) { dataRows++; if (grid[y].length > maxCols) maxCols = grid[y].length; }
    height = dataRows; width = maxCols;
  }
  let yMin = Infinity, yMax = -Infinity, xMin = Infinity, xMax = -Infinity;
  for (let y = 1; y < grid.length; y++) {
    const row = grid[y], w = Math.min(width, row.length);
    let hasWall = false;
    for (let x = 0; x < w; x++) if (row[x] === 1) { hasWall = true; if (x < xMin) xMin = x; if (x > xMax) xMax = x; }
    if (hasWall) { if (!Number.isFinite(yMin)) { yMin = y; yMax = y; } else yMax = Math.max(yMax, y); }
  }
  if (!Number.isFinite(xMin) || !Number.isFinite(yMin)) { xMin = 0; xMax = Math.max(0, width - 1); yMin = 1; yMax = Math.max(1, height); }
  const drawHeight = yMax - yMin + 1, drawWidth = xMax - xMin + 1;
  const squareSize = Math.max(1, Math.max(drawWidth, drawHeight));
  const diff = (drawWidth - drawHeight) / 2;
  if (diff > 0) { yMin -= Math.trunc(diff); yMax += Math.ceil(diff); }
  else { xMin += Math.trunc(diff); xMax -= Math.floor(diff); }
  const gridWidth = canvasW / squareSize, gridHeight = canvasH / squareSize;
  return {
    rows: height, cols: width, startX: xMin, startY: yMin, endX: xMax, endY: yMax,
    squareSize, gridWidth, gridHeight, gridSize: (gridWidth + gridHeight) / 2, txtAverSize: (width + height) / 2,
  };
}

// ── 镜像：model/mapContour.ets（marching squares → linkLoops → chaikin → extractContours）──
function marchingSquares(g) {
  const b = g.b, w = g.w, h = g.h;
  const segs = [];
  for (let y = 0; y < h - 1; y++) {
    for (let x = 0; x < w - 1; x++) {
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
  }
  return segs;
}
function ptKeyC(x, y) { return `${Math.round(x * 2)}_${Math.round(y * 2)}`; }
function linkLoops(segs) {
  const n = Math.floor(segs.length / 4);
  const used = new Uint8Array(n);
  const adj = new Map();
  for (let s = 0; s < n; s++) {
    const ka = ptKeyC(segs[s * 4], segs[s * 4 + 1]), kb = ptKeyC(segs[s * 4 + 2], segs[s * 4 + 3]);
    if (!adj.has(ka)) adj.set(ka, []); adj.get(ka).push(s);
    if (!adj.has(kb)) adj.set(kb, []); adj.get(kb).push(s);
  }
  const step = (cx, cy) => {
    const cand = adj.get(ptKeyC(cx, cy));
    if (cand === undefined) return null;
    for (const t of cand) {
      if (used[t] === 1) continue;
      used[t] = 1;
      const px = segs[t * 4], py = segs[t * 4 + 1], qx = segs[t * 4 + 2], qy = segs[t * 4 + 3];
      if (ptKeyC(px, py) === ptKeyC(cx, cy)) return [qx, qy];
      return [px, py];
    }
    return null;
  };
  const out = [];
  for (let s = 0; s < n; s++) {
    if (used[s] === 1) continue;
    used[s] = 1;
    const ax = segs[s * 4], ay = segs[s * 4 + 1];
    const pts = [ax, ay, segs[s * 4 + 2], segs[s * 4 + 3]];
    let cx = pts[2], cy = pts[3], closed = false;
    while (true) {
      const nxt = step(cx, cy);
      if (nxt === null) break;
      cx = nxt[0]; cy = nxt[1]; pts.push(cx, cy);
      if (ptKeyC(cx, cy) === ptKeyC(ax, ay)) { closed = true; break; }
    }
    if (!closed) {
      const back = []; let bx2 = ax, by2 = ay;
      while (true) { const nxt = step(bx2, by2); if (nxt === null) break; bx2 = nxt[0]; by2 = nxt[1]; back.push(bx2, by2); }
      if (back.length > 0) {
        const head = [];
        for (let i = back.length - 2; i >= 0; i -= 2) head.push(back[i], back[i + 1]);
        for (let i = 0; i < pts.length; i++) head.push(pts[i]);
        out.push({ pts: head, closed: false }); continue;
      }
    }
    out.push({ pts, closed });
  }
  return out;
}
function chaikin(pts, closed, iters) {
  let cur = pts;
  for (let it = 0; it < iters; it++) {
    const m = Math.floor(cur.length / 2);
    if (m < 3) break;
    const next = [];
    if (!closed) next.push(cur[0], cur[1]);
    const segCount = closed ? m : m - 1;
    for (let i = 0; i < segCount; i++) {
      const i1 = (i + 1) % m;
      const x0 = cur[i * 2], y0 = cur[i * 2 + 1], x1 = cur[i1 * 2], y1 = cur[i1 * 2 + 1];
      next.push(x0 * 0.75 + x1 * 0.25, y0 * 0.75 + y1 * 0.25);
      next.push(x0 * 0.25 + x1 * 0.75, y0 * 0.25 + y1 * 0.75);
    }
    if (!closed) next.push(cur[(m - 1) * 2], cur[(m - 1) * 2 + 1]);
    cur = next;
  }
  return cur;
}
function extractContours(g, gridWidth, gridHeight, iters) {
  const loops = linkLoops(marchingSquares(g));
  const out = [];
  for (const c of loops) {
    const sm = chaikin(c.pts, c.closed, iters);
    const mapped = new Array(sm.length);
    const sx = g.f * gridWidth, sy = g.f * gridHeight;
    for (let i = 0; i < sm.length; i += 2) { mapped[i] = sm[i] * sx; mapped[i + 1] = sm[i + 1] * sy; }
    out.push({ pts: mapped, closed: c.closed });
  }
  return out;
}
function coarseFrom(rows) { // rows: array of '0'/'1' 字符串 → CoarseGrid（f=1）
  const h = rows.length, w = rows[0].length;
  const b = new Uint8Array(w * h);
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) b[y * w + x] = rows[y][x] === '1' ? 1 : 0;
  return { b, w, h, f: 1 };
}

// ── 迷你断言框架 ────────────────────────────────────────────────────────────
let passed = 0, failed = 0;
function check(name, cond, detail = '') {
  if (cond) { passed++; console.log(`  ✓ ${name}`); }
  else { failed++; console.log(`  ✗ ${name}  ${detail}`); }
}
function bytesOf(buf) { return Array.from(new Uint8Array(buf)); }
function eqArr(a, b) { return a.length === b.length && a.every((v, i) => v === b[i]); }

// ── ① 协议编解码 ────────────────────────────────────────────────────────────
console.log('UDP 9 字节协议（大端）：');
{
  const buf = encodeSend({ state: 3, runState: 1, speed: 35, endX: 258, endY: -1 });
  const got = bytesOf(buf);
  const want = [3, 1, 35, 1, 2, 255, 255, 0, 0]; // 258=0x0102, -1=0xFFFF, byte7/8=0
  check('encode 固定向量（大端 + 负数）', eqArr(got, want), `got=${got} want=${want}`);
  check('encode 长度恒为 9', got.length === 9);
}
{
  const frame = new Uint8Array([3, 0, 0, 0x00, 0x64, 0xFF, 0x9C, 0x01, 0x68]).buffer;
  const r = decodeReceive(frame);
  check('decode 心跳 state=3', r.state === 3, JSON.stringify(r));
  check('decode x=100', r.x === 100, `x=${r.x}`);
  check('decode y=-100（负数大端）', r.y === -100, `y=${r.y}`);
  check('decode r=360', r.r === 360, `r=${r.r}`);
}
{
  let ok = true;
  for (const s of [{ state: 0, endX: 0, endY: 0 }, { state: 3, endX: 1234, endY: -5678 }, { state: 105, endX: 32767, endY: -32768 }]) {
    const d = decodeReceive(encodeSend(s));
    if (!(d.state === s.state && d.x === s.endX && d.y === s.endY && d.r === 0)) { ok = false; break; }
  }
  check('round-trip decode(encode(x)) 还原 state/endX/endY', ok);
}

// ── ② 坐标换算互逆 ──────────────────────────────────────────────────────────
console.log('坐标换算（map -> canvas -> map 互逆）：');
{
  const parsed = parseMap(readFileSync(FIXTURE, 'utf8'), 1000, 1000);
  const t = parsed; // parsed 含 startX/endY/gridSize/txtAverSize，即 MapTransform 字段
  const half = 500;
  let maxErr = 0;
  for (const p of [{ x: 10, y: 10 }, { x: 20, y: 5 }, { x: 35, y: 30 }, { x: 1, y: 39 }]) {
    const back = canvasToMap(mapToCanvas(p, t, half), t, half);
    maxErr = Math.max(maxErr, Math.abs(back.x - p.x), Math.abs(back.y - p.y));
  }
  check('map->canvas->map 误差 ≤ 1', maxErr <= 1, `maxErr=${maxErr}`);
}

// ── ③ 地图解析 ──────────────────────────────────────────────────────────────
console.log('地图解析（fixtures/defultMap.txt, 画布 1000x1000）：');
{
  const m = parseMap(readFileSync(FIXTURE, 'utf8'), 1000, 1000);
  check('rows=40', m.rows === 40, `rows=${m.rows}`);
  check('cols=40', m.cols === 40, `cols=${m.cols}`);
  check('包围盒 startX=0', m.startX === 0, `startX=${m.startX}`);
  check('包围盒 startY=1', m.startY === 1, `startY=${m.startY}`);
  check('包围盒 endX=39', m.endX === 39, `endX=${m.endX}`);
  check('包围盒 endY=40', m.endY === 40, `endY=${m.endY}`);
  check('squareSize=40', m.squareSize === 40, `squareSize=${m.squareSize}`);
  check('gridSize=25', m.gridSize === 25, `gridSize=${m.gridSize}`);
  check('txtAverSize=40', m.txtAverSize === 40, `txtAverSize=${m.txtAverSize}`);
}

// ── ③b 真机地图格式（7 值首行 + 空格分隔 -1/0）：防回归"取末两个=负偏移 / 把 -1 当密排字符"──
console.log('真机地图格式（首行 range resolution height width metersPerPixel x0 y0 + 空格分隔 -1/0）：');
{
  const realMap = [
    '0.05 0.05 4 5 0.05 -45 -44',  // range res height=4 width=5 mpp x0=-45 y0=-44
    '-1 -1 -1 -1 -1',
    '-1 0 0 0 -1',
    '-1 0 0 0 -1',
    '-1 -1 -1 -1 -1',
  ].join('\n');
  const rm = parseMap(realMap, 1000, 1000);
  check('首行按位置取 rows=4（parts[2]，不是取末两个的 x0=-45）', rm.rows === 4, `rows=${rm.rows}`);
  check('首行按位置取 cols=5（parts[3]，不是 y0=-44）', rm.cols === 5, `cols=${rm.cols}`);
  check('空格分隔 -1 识别为障碍：xMin=0', rm.startX === 0, `startX=${rm.startX}`);
  check('空格分隔 -1 识别为障碍：xMax=4', rm.endX === 4, `endX=${rm.endX}`);
  check('未把负偏移当行列（rows/cols 均 >0）', rm.rows > 0 && rm.cols > 0, `rows=${rm.rows} cols=${rm.cols}`);
}

// ── ④ 共享层副本同步（car-agent 自带共享层 vs app-harmony 权威；§6.1 "改一处改两处" 守卫）──
// car-agent 精确复制了 app-harmony 的 UI 无关层（HAR 化前的权宜）。app-harmony 改了这些文件却
// 忘了同步 car-agent，会导致两端协议/常量漂移 → 真机集成时静默故障。此处逐字节守卫。
console.log('共享层副本同步（car-agent vs app-harmony，逐字节）：');
{
  const ROOT = join(__dirname, '..', '..');
  const appEts = (f) => join(ROOT, 'app-harmony', 'entry', 'src', 'main', 'ets', f);
  const agEts = (f) => join(ROOT, 'car-agent', 'entry', 'src', 'main', 'ets', f);
  const SHARED = [
    'model/protocol.ets', 'model/mission.ets', 'model/geometry.ets',
    'constants/protocol.ets', 'constants/debug.ets', 'utils/log.ets', 'service/RobotTransport.ets',
  ];
  for (const f of SHARED) {
    let same = false, detail = '';
    try { same = readFileSync(appEts(f), 'utf8') === readFileSync(agEts(f), 'utf8'); }
    catch (e) { detail = String(e?.message ?? e); }
    check(`copy 同步 ${f}`, same, detail || '内容不一致 → 把 app-harmony 的改动同步到 car-agent');
  }
  // FleetMissionService 是 agent 的"无界面适配版"，刻意不同 → 不逐字节比，只查关键常量 BUNDLE_NAME 一致
  // （distributedDataObject 跨设备同步要求两端同 bundleName，见 docs/distributed-trust.md）。
  const bundleRe = /const BUNDLE_NAME[^']*'([^']+)'/;
  const appBundle = (readFileSync(appEts('service/FleetMissionService.ets'), 'utf8').match(bundleRe) || [])[1];
  const agBundle = (readFileSync(agEts('service/FleetMissionService.ets'), 'utf8').match(bundleRe) || [])[1];
  check('FleetMissionService BUNDLE_NAME 两端一致（DDO 同 bundle 前提）',
    appBundle !== undefined && appBundle === agBundle, `app=${appBundle} agent=${agBundle}`);
}

// ── ⑤ 墙体平滑等高线（marching squares → linkLoops → chaikin → extractContours）──
// 防回归"把栅格当方块画"：等高线管线把占据栅格转成平滑矢量边界（见 docs/map-ui-redesign.md）。
console.log('墙体平滑矢量等高线（mapContour：栅格 → 平滑边界）：');
{
  // 2×2 墙块居中于 4×4 空网格 → 一条闭环包住墙块
  const g = coarseFrom(['0000', '0110', '0110', '0000']);
  const segs = marchingSquares(g);
  check('marchingSquares 产出边界线段（2×2 墙块）', segs.length >= 16, `segs=${segs.length / 4}`);
  const loops = linkLoops(segs);
  const closed = loops.filter((c) => c.closed);
  check('linkLoops 连成闭环', closed.length >= 1, `loops=${loops.length} closed=${closed.length}`);
  // 闭环顶点都贴住墙块边界（中点在 [0.4,2.6]，即 hug 居中 2×2 块）
  let hug = true;
  for (const c of closed) for (let i = 0; i < c.pts.length; i += 2) {
    if (c.pts[i] < 0.4 || c.pts[i] > 2.6 || c.pts[i + 1] < 0.4 || c.pts[i + 1] > 2.6) hug = false;
  }
  check('闭环顶点贴住墙块边界（不是整图方块）', hug);
  // chaikin 平滑应增密顶点（折线 → 顺滑曲线）
  const before = closed[0].pts.length / 2;
  const after = chaikin(closed[0].pts, true, 2).length / 2;
  check('chaikin 平滑增密顶点', after > before, `before=${before} after=${after}`);
  // extractContours：映射回 base px（f=1, 每格 10px）→ 坐标有限且按格尺寸放大
  const cs = extractContours(g, 10, 10, 2);
  let finite = cs.length > 0, maxXY = 0;
  for (const c of cs) for (const v of c.pts) { if (!Number.isFinite(v)) finite = false; if (v > maxXY) maxXY = v; }
  check('extractContours 坐标有限', finite);
  check('extractContours 映射到 base px（×格尺寸 10）', maxXY > 10 && maxXY < 30, `maxXY=${maxXY}`);
}
{
  // 墙环（5×5 边框、中心 3×3 空）→ 含洞场景：可提取且不崩、坐标有限
  const g = coarseFrom(['11111', '10001', '10001', '10001', '11111']);
  const cs = extractContours(g, 4, 4, 1);
  check('墙环（含洞）可提取等高线且不崩', cs.length >= 1, `loops=${cs.length}`);
  let finite = true;
  for (const c of cs) for (const v of c.pts) if (!Number.isFinite(v)) finite = false;
  check('墙环等高线坐标有限', finite);
}

console.log(`\n结果：${passed} 通过, ${failed} 失败`);
process.exit(failed === 0 ? 0 : 1);
