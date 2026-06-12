// 车载 agent Reconciler 的 Node 镜像测试（纯逻辑，无需 DevEco/真机）。
// 镜像 car-agent/.../reconciler/Reconciler.ets 的决策与幂等逻辑，逐项断言。
//   运行：node tools/verify/verify-reconciler.mjs   （退出码 0 通过）
// ⚠️ 若改了 Reconciler.ets 的映射/状态机，请同步本文件。
//
// 方案B 时序（2026-06-12 对齐裁决，见 contracts/app-purplepi-alignment-audit.md 裁决记录）：
//   master(robotId=0) → [107,108]（不发 cmd5）；sub(robotId=1, masterIp 已知) → [105,5,107,108]。

// 命令码（与 model/protocol.RobotCommand 同源）
const CMD = { PULL_MAP: 105, LOAD_MAP: 5, START_ROUTE: 3, END_ROUTE: 4, CORNER1: 107, CORNER2: 108 };

class Reconciler {
  constructor() { this.lastSig = ''; this.mapPulled = false; this.loaded = false; this.active = false; }
  reconcile(v) {
    const sig = JSON.stringify({ p: v.phase, a: v.assignment, e: v.endPoint, m: v.masterIp });
    if (sig === this.lastSig) return [];
    this.lastSig = sig;
    const out = [];
    if (v.phase === 'covering' && v.assignment) {
      const isSub = v.assignment.robotId === 1;
      if (isSub) {
        if (v.masterIp === null || v.masterIp === undefined || v.masterIp.length === 0) return out; // 等 master 地址
        if (!this.mapPulled) { out.push({ cmd: CMD.PULL_MAP, masterIp: v.masterIp }); this.mapPulled = true; }
        if (!this.loaded) { out.push({ cmd: CMD.LOAD_MAP }); this.loaded = true; }
      }
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
  reset() { this.lastSig = ''; this.mapPulled = false; this.loaded = false; this.active = false; }
}

let passed = 0, failed = 0;
const codes = (cmds) => cmds.map((c) => c.cmd);
function check(name, cond, detail = '') {
  if (cond) { passed++; console.log(`  ✓ ${name}`); }
  else { failed++; console.log(`  ✗ ${name}  ${detail}`); }
}
const eq = (a, b) => JSON.stringify(a) === JSON.stringify(b);

console.log('车载 agent Reconciler（黑板 → 本机命令，幂等；方案B 时序）：');
const r = new Reconciler();

// ① 子车覆盖(robotId=1, masterIp 已知)首次 → [105,5,107,108]，105 带 masterIp、108 带 robotId
const sub = { phase: 'covering', carId: 2, endPoint: null, masterIp: '192.168.1.10',
  assignment: { corner1: { x: 10, y: 20 }, corner2: { x: 60, y: 80 }, robotId: 1 } };
const o1 = r.reconcile(sub);
check('① 子车覆盖首次 → [105,5,107,108]', eq(codes(o1), [CMD.PULL_MAP, CMD.LOAD_MAP, CMD.CORNER1, CMD.CORNER2]), JSON.stringify(codes(o1)));
check('① 105 带 masterIp', o1[0].masterIp === '192.168.1.10');
check('① 107 带对角点1坐标', o1[2].x === 10 && o1[2].y === 20);
check('① 108 带对角点2坐标 + robotId=1', o1[3].x === 60 && o1[3].y === 80 && o1[3].robotId === 1);

// ② 同一视图再来 → []（幂等，不重发）
check('② 切片未变 → [] 幂等', eq(r.reconcile(sub), []));

// ③ master 覆盖(robotId=0) → [107,108]，绝不发 cmd5/cmd105（不归零自身位姿）
const r2 = new Reconciler();
const master = { phase: 'covering', carId: 1, endPoint: null, masterIp: '192.168.1.10',
  assignment: { corner1: { x: 0, y: 0 }, corner2: { x: 30, y: 30 }, robotId: 0 } };
check('③ master 覆盖 → [107,108]（无 5/105）', eq(codes(r2.reconcile(master)), [CMD.CORNER1, CMD.CORNER2]), JSON.stringify(codes(r2.reconcile(master))));

// ④ 子车覆盖但 masterIp 未到 → []（等地址）；地址到达后 → [105,5,107,108]
const r3 = new Reconciler();
const subNoIp = { phase: 'covering', carId: 2, endPoint: null, masterIp: null,
  assignment: { corner1: { x: 1, y: 2 }, corner2: { x: 3, y: 4 }, robotId: 1 } };
check('④a masterIp 未到 → []（等地址）', eq(r3.reconcile(subNoIp), []));
const subIp = { phase: 'covering', carId: 2, endPoint: null, masterIp: '10.0.0.5',
  assignment: { corner1: { x: 1, y: 2 }, corner2: { x: 3, y: 4 }, robotId: 1 } };
check('④b 地址到达 → [105,5,107,108]', eq(codes(r3.reconcile(subIp)), [CMD.PULL_MAP, CMD.LOAD_MAP, CMD.CORNER1, CMD.CORNER2]));

// ⑤ 切到单点导航 → [3]
const r4 = new Reconciler();
const nav = { phase: 'covering', carId: 1, assignment: null, endPoint: { x: 123, y: -45 }, masterIp: null };
const o5 = r4.reconcile(nav);
check('⑤ 单点目标 → [3]', eq(codes(o5), [CMD.START_ROUTE]) && o5[0].x === 123 && o5[0].y === -45, JSON.stringify(o5));

// ⑥ 任务撤销（active→idle）→ [4]；idle 再来 → []
const idle = { phase: 'idle', carId: 1, assignment: null, endPoint: null, masterIp: null };
check('⑥ 撤销任务 → [4]', eq(codes(r4.reconcile(idle)), [CMD.END_ROUTE]));
check('⑥ idle→idle → []', eq(r4.reconcile(idle), []));

// ⑦ reset 后子车按新黑板全量重发（再含 105,5）
r.reset();
check('⑦ reset 后子车覆盖重新含 [105,5,...]', eq(codes(r.reconcile(sub)), [CMD.PULL_MAP, CMD.LOAD_MAP, CMD.CORNER1, CMD.CORNER2]));

console.log(`\n结果：${passed} 通过, ${failed} 失败`);
process.exit(failed === 0 ? 0 : 1);
