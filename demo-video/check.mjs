// 烟雾测试：用 jsdom 执行自包含 film.html 的全部内联脚本，
// 捕获运行期异常并回读最终场景顺序/时长。仅校验"能 boot、无抛错、顺序对"。
// 用法: node check.mjs
import { readFileSync } from 'node:fs';
import { JSDOM, VirtualConsole } from 'jsdom';

const html = readFileSync(new URL('./film.html', import.meta.url), 'utf8');
const errors = [];
const vc = new VirtualConsole();
vc.on('jsdomError', (e) => errors.push('jsdomError: ' + (e.detail?.message || e.message)));
let filmLog = '';
vc.on('error', (...a) => errors.push('console.error: ' + a.join(' ')));
vc.on('log', (...a) => { const s = a.join(' '); if (s.startsWith('[film]')) filmLog = s; });

const dom = new JSDOM(html, {
  runScripts: 'dangerously',
  pretendToBeVisual: true,            // 提供 requestAnimationFrame
  virtualConsole: vc,
});

await new Promise((r) => setTimeout(r, 800));   // 等内联脚本/GSAP 初始化

const w = dom.window;
const scenes = (w.__scenes || []).map((s) => ({ id: s.id, dur: s.dur, start: +s.start?.toFixed?.(1) }));
console.log('SCENES (' + scenes.length + '):');
scenes.forEach((s) => console.log('  ' + s.id.padEnd(12) + ' dur=' + s.dur + 's  start=' + s.start));
console.log('TOTAL __duration (real sec, /SPEED) =', w.__duration?.toFixed?.(1));
console.log('film log:', filmLog || '(none)');
console.log('ERRORS (' + errors.length + '):');
errors.forEach((e) => console.log('  ' + e));
process.exit(errors.length ? 1 : 0);
