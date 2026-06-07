// 车载 agent Reconciler 的 Node 镜像测试（纯逻辑，无需 DevEco/真机）。
// 镜像 car-agent/.../reconciler/Reconciler.ets 的决策与幂等逻辑，逐项断言。
//   运行：node tools/verify/verify-reconciler.mjs   （退出码 0 通过）
// ⚠️ 若改了 Reconciler.ets 的映射/状态机，请同步本文件。

// 命令码（与 model/protocol.RobotCommand 同源）
const CMD = { LOAD_MAP: 5, START_ROUTE: 3, END_ROUTE: 4, CORNER1: 107, CORNER2: 108 };

class Reconciler {
  constructor() { this.lastSig = ''; this.loaded = false; this.active = false; }
  reconcile(v) {
    const sig = JSON.stringify({ p: v.phase, a: v.assignment, e: v.endPoint });
    if (sig === this.lastSig) return [];
    this.lastSig = sig;
    const out = [];
    if (v.phase === 'covering' && v.assignment) {
      if (!this.loaded) { out.push({ cmd: CMD.LOAD_MAP }); this.loaded = true; }
      out.push({ cmd: CMD.CORNER1, x: v.assignment.corner1.x, y: v.assignment.corner1.y });
      out.push({ cmd: CMD.CORNER2, x: v.assignment.corner2.x, y: v.assignment.corner2.y, robotId: v.assignment.robotId });
      this.active = true;
      return out;
    }
    if (v.endPoint) {
      out.push({ cmd: CMD.START_ROUTE, x: v.endPoint.x, y: v.endPoint.y });
      this.active = true;
      return out;
    }
    if (this.active) { out.push({ cmd: CMD.END_ROUTE }); this.active = false; }
    return out;
  }
  reset() { this.lastSig = ''; this.loaded = false; this.active = false; }
}

let passed = 0, failed = 0;
const codes = (cmds) => cmds.map((c) => c.cmd);
function check(name, cond, detail = '') {
  if (cond) { passed++; console.log(`  ✓ ${name}`); }
  else { failed++; console.log(`  ✗ ${name}  ${detail}`); }
}
const eq = (a, b) => JSON.stringify(a) === JSON.stringify(b);

console.log('车载 agent Reconciler（黑板 → 本机命令，幂等）：');
const r = new Reconciler();

// ① 覆盖 + 分配（首次未归零）→ [5,107,108]，108 带 robotId
const cov = { phase: 'covering', carId: 1, endPoint: null,
  assignment: { corner1: { x: 10, y: 20 }, corner2: { x: 60, y: 80 }, robotId: 1 } };
const o1 = r.reconcile(cov);
check('① 覆盖首次 → [5,107,108]', eq(codes(o1), [CMD.LOAD_MAP, CMD.CORNER1, CMD.CORNER2]), JSON.stringify(codes(o1)));
check('① 107 带对角点1坐标', o1[1].x === 10 && o1[1].y === 20);
check('① 108 带对角点2坐标 + robotId', o1[2].x === 60 && o1[2].y === 80 && o1[2].robotId === 1);

// ② 同一视图再来 → []（幂等，不重发）
check('② 切片未变 → [] 幂等', eq(r.reconcile(cov), []));

// ③ 新分配（已归零）→ [107,108]，不再 cmd5
const cov2 = { phase: 'covering', carId: 1, endPoint: null,
  assignment: { corner1: { x: 0, y: 0 }, corner2: { x: 30, y: 30 }, robotId: 0 } };
const o3 = r.reconcile(cov2);
check('③ 新分配（已归零）→ [107,108]', eq(codes(o3), [CMD.CORNER1, CMD.CORNER2]), JSON.stringify(codes(o3)));

// ④ 切到单点导航 → [3]
const nav = { phase: 'covering', carId: 1, assignment: null, endPoint: { x: 123, y: -45 } };
const o4 = r.reconcile(nav);
check('④ 单点目标 → [3]', eq(codes(o4), [CMD.START_ROUTE]) && o4[0].x === 123 && o4[0].y === -45, JSON.stringify(o4));

// ⑤ 任务撤销（active→idle）→ [4]
const idle = { phase: 'idle', carId: 1, assignment: null, endPoint: null };
check('⑤ 撤销任务 → [4]', eq(codes(r.reconcile(idle)), [CMD.END_ROUTE]));

// ⑥ idle 再来 → []（已非 active，不重复取消）
check('⑥ idle→idle → []', eq(r.reconcile(idle), []));

// ⑦ reset 后按新黑板全量重发（覆盖再发 5）
r.reset();
check('⑦ reset 后覆盖重新含 cmd5', eq(codes(r.reconcile(cov)), [CMD.LOAD_MAP, CMD.CORNER1, CMD.CORNER2]));

console.log(`\n结果：${passed} 通过, ${failed} 失败`);
process.exit(failed === 0 ? 0 : 1);
