// 纯逻辑验证（可在 PC 上用 Node 直接跑）。
//
// 这里的函数是 app-harmony 里几个纯算法 .ets 的【逐行镜像】，用来在没有 DevEco/真机时
// 立即验证：① UDP 9 字节编解码（大端）② 坐标换算互逆 ③ 地图首行/包围盒解析
//   ④ 共享层副本同步：car-agent 复制的 model/constants/utils/RobotTransport 与 app-harmony 逐字节一致 + BUNDLE_NAME 一致。
// 镜像对象：
//   - model/protocol.ets   encodeSend / decodeReceive
//   - model/geometry.ets   canvasToMap / mapToCanvas
//   - service/MapService.ets parseMap（首行 + 包围盒 + 正方形化部分）
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

// ── 镜像：service/MapService.ets parseMap ───────────────────────────────────
function parseMap(text, canvasW, canvasH) {
  const lines = text.split('\n');
  const headerNums = lines[0].trim().split(/\s+/).map((t) => parseInt(t, 10)).filter((n) => !Number.isNaN(n));
  const height = headerNums[headerNums.length - 2];
  const width = headerNums[headerNums.length - 1];
  let yMin = Infinity, yMax = -Infinity, xMin = Infinity, xMax = -Infinity;
  for (let y = 1; y < lines.length; y++) {
    const d = lines[y];
    if (d.includes('1')) {
      if (!Number.isFinite(yMin)) yMin = y; else yMax = Math.max(yMax, y);
      for (let x = 0; x < width; x++) {
        if (d[x] === '1') { xMin = Math.min(xMin, x); xMax = Math.max(xMax, x); }
      }
    }
  }
  const drawHeight = yMax - yMin + 1, drawWidth = xMax - xMin + 1;
  const squareSize = Math.max(drawWidth, drawHeight);
  const diff = (drawWidth - drawHeight) / 2;
  if (diff > 0) { yMin -= Math.trunc(diff); yMax += Math.ceil(diff); }
  else { xMin += Math.trunc(diff); xMax -= Math.floor(diff); }
  const gridWidth = canvasW / squareSize, gridHeight = canvasH / squareSize;
  return {
    rows: height, cols: width, startX: xMin, startY: yMin, endX: xMax, endY: yMax,
    squareSize, gridWidth, gridHeight, gridSize: (gridWidth + gridHeight) / 2, txtAverSize: (width + height) / 2,
  };
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

console.log(`\n结果：${passed} 通过, ${failed} 失败`);
process.exit(failed === 0 ? 0 : 1);
