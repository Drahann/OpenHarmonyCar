// 逐帧确定性导出 60fps MP4：用系统 Edge 把时间轴 __seek 到每一帧、手动步进所有视频(片尾实机视频按场景内时间、
// 仪表循环视频按 t%时长)，截图后用 ffmpeg 以 60fps 合成。比实时录屏(playwright 只有 ~25fps)更丝滑、零丢帧、可复现。
// 用法: node export_frames.mjs   → film_preview.mp4
import { chromium } from 'playwright';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';
import { mkdirSync, rmSync, existsSync } from 'node:fs';
import { execFileSync } from 'node:child_process';

const FPS = 60;
const DIR = '_tmpframes/seq';
rmSync(DIR, { recursive: true, force: true }); mkdirSync(DIR, { recursive: true });

const browser = await chromium.launch({ channel: 'msedge', args: ['--autoplay-policy=no-user-gesture-required'] });
const page = await browser.newPage({ viewport: { width: 1920, height: 1080 }, deviceScaleFactor: 1 });
page.on('console', (m) => { if (m.type() === 'error') console.log('  [page error]', m.text()); });
await page.goto(pathToFileURL(resolve('film.html')).href + '?record=1');
await page.waitForFunction('window.__duration && window.__seek && window.__scenes && window.__scenes.length', null, { timeout: 60000 }).catch(() => {});
await page.waitForTimeout(500);
const dur = await page.evaluate(() => window.__duration);
const SPEED = await page.evaluate(() => window.__tl.duration() / window.__duration);
const scenes = await page.evaluate(() => window.__scenes.map(s => ({ id: s.id, start: s.start, dur: s.dur })));
if (!dur) { console.error('film not ready'); await browser.close(); process.exit(1); }
const demo = scenes.find(s => s.id === 'demo-real');
const demoStart = demo ? demo.start / SPEED : 1e9;

// 含 <video> 的场景时间段（这些帧给视频多一点解码时间）
const vidRanges = await page.evaluate(() => {
  return window.__scenes.filter(s => s.root && s.root.querySelector && s.root.querySelector('video'))
    .map(s => [s.start, s.start + s.dur]);
});
const inVid = (tl) => vidRanges.some(([a, b]) => tl >= a - 0.05 && tl <= b + 0.05);

// 暂停并接管所有视频
await page.evaluate(() => { document.querySelectorAll('video').forEach(v => { try { v.pause(); v.muted = true; } catch (e) {} }); });
// 预热片尾实机视频解码
await page.evaluate(() => { const v = document.querySelector('video[src*="demo_real"]'); if (v) { v.preload = 'auto'; try { v.load(); } catch (e) {} } });
await page.waitForTimeout(2500);

const total = Math.round(dur * FPS);
const START_AT = Math.max(0, parseFloat(process.env.START_AT || '0'));   // 可选：从某秒开始导出（验证用），默认 0=全片
console.log('duration', dur.toFixed(1), 's · frames', total, '@', FPS, 'fps' + (START_AT ? ` · start@${START_AT}s` : ''));
const t0 = Date.now();
for (let i = Math.round(START_AT * FPS); i < total; i++) {
  const t = i / FPS;                       // 真实秒（已除以 SPEED）
  await page.evaluate((tt) => window.__seek(tt), t);
  // 手动步进视频：片尾实机视频按场景内时间，仪表循环视频按 t%时长。
  // 必须等每个视频的 seeked 事件——逐帧 seek 时若前帧解码未完成就被下帧新值打断，
  // 截图会落在解码器还没离开的旧帧上，成片表现为"卡住几秒后突然跳跃"。
  await page.evaluate(({ t, ds }) => {
    const vids = Array.from(document.querySelectorAll('video'));
    return Promise.all(vids.map(v => new Promise((resolve) => {
      const src = v.currentSrc || v.src;
      if (!v.duration || !isFinite(v.duration)) return resolve();
      let target;
      if (/demo_real/.test(src)) { const lt = t - ds; if (lt < 0) return resolve(); target = Math.min(lt, v.duration - 0.05); }
      else target = t % v.duration;
      if (Math.abs((v.currentTime || 0) - target) < 1e-3) return resolve();
      let done = false;
      const finish = () => { if (done) return; done = true; v.removeEventListener('seeked', finish); resolve(); };
      v.addEventListener('seeked', finish);
      try { v.currentTime = target; } catch (e) { finish(); }
      setTimeout(finish, 2000);   // 超时兜底，避免某帧卡死整个导出
    })));
  }, { t, ds: demoStart });
  await page.evaluate(() => new Promise(r => requestAnimationFrame(() => requestAnimationFrame(r))));
  await page.screenshot({ path: `${DIR}/f${String(i).padStart(6, '0')}.jpg`, type: 'jpeg', quality: 90, clip: { x: 0, y: 0, width: 1920, height: 1080 } });
  if (i % 600 === 0) { const el = (Date.now() - t0) / 1000; console.log(`  ${i}/${total}  t=${t.toFixed(1)}s  elapsed ${el.toFixed(0)}s`); }
}
await browser.close();
console.log('frames done in', ((Date.now() - t0) / 1000).toFixed(0), 's — encoding…');

const out = 'film_preview.mp4';
const startNo = Math.round(START_AT * FPS);
execFileSync('ffmpeg', ['-y', '-framerate', String(FPS), '-start_number', String(startNo), '-i', `${DIR}/f%06d.jpg`,
  '-c:v', 'libx264', '-crf', '18', '-preset', 'medium', '-pix_fmt', 'yuv420p', '-r', String(FPS),
  '-movflags', '+faststart', out], { stdio: 'inherit' });
console.log('DONE ->', resolve(out), existsSync(out) ? '(ok)' : '(MISSING)');
