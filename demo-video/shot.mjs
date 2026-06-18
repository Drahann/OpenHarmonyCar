// 截图工具：用系统 Edge 打开自包含 film.html，按"场景内偏移秒"seek 后截图（1920×1080）。
// 用法: node shot.mjs <sceneId> <off1> [off2 ...]
//   例: node shot.mjs 01-home 7        → _tmpframes/01-home@7.png
//       node shot.mjs 08-trust 2 5 9
import { chromium } from 'playwright';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';

const args = process.argv.slice(2);
const sceneId = args[0];
const offs = args.slice(1).map(Number);
if (!sceneId || !offs.length) { console.error('用法: node shot.mjs <sceneId> <off...>'); process.exit(1); }

const browser = await chromium.launch({ channel: 'msedge', args: ['--autoplay-policy=no-user-gesture-required'] });
const page = await browser.newPage({ viewport: { width: 1920, height: 1080 }, deviceScaleFactor: 1 });
page.on('console', (m) => { if (m.type() === 'error') console.log('  [page error]', m.text()); });
await page.goto(pathToFileURL(resolve('film.html')).href + '?record=1');
await page.waitForFunction('window.__ready === true || (window.__scenes && window.__scenes.length)', null, { timeout: 30000 }).catch(() => {});
await page.waitForTimeout(400);

const info = await page.evaluate((id) => {
  const s = (window.__scenes || []).find((x) => x.id === id);
  return s ? { start: s.start, dur: s.dur } : null;
}, sceneId);
if (!info) { console.error('scene not found:', sceneId); await browser.close(); process.exit(1); }
const SPEED = await page.evaluate(() => (window.__duration && window.__tl) ? (window.__tl.duration() / window.__duration) : 1);

// 视频需要解码：先把时间轴打到该场景中部让视频开始加载，再等一下
await page.evaluate((t) => window.__seek(t), info.start / SPEED + Math.min(info.dur * 0.5, 3));
await page.waitForTimeout(1200);

for (const off of offs) {
  const real = info.start / SPEED + off;
  await page.evaluate((t) => window.__seek(t), real);
  await page.evaluate(() => new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r))));
  await page.waitForTimeout(350);
  const out = `_tmpframes/${sceneId}@${off}.png`;
  await page.screenshot({ path: out });
  console.log('shot', out, '(scene', sceneId, 'start', info.start.toFixed(1) + 's, off', off + 's)');
}
await browser.close();
