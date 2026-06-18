// 导出 MP4：用系统 Edge 实时播放自包含 film.html 并录屏(playwright recordVideo)，再转码为 H.264 MP4。
// 实时录制能同时正确处理 currentTime 绑定的片尾实机视频 + autoplay 循环的仪表视频。
// 用法: node export_record.mjs   → 输出 film_preview.mp4
import { chromium } from 'playwright';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';
import { mkdirSync, rmSync, existsSync } from 'node:fs';
import { execFileSync } from 'node:child_process';

const REC = '_tmpframes/rec';
rmSync(REC, { recursive: true, force: true }); mkdirSync(REC, { recursive: true });

const browser = await chromium.launch({ channel: 'msedge', args: ['--autoplay-policy=no-user-gesture-required'] });
const context = await browser.newContext({
  viewport: { width: 1920, height: 1080 }, deviceScaleFactor: 1,
  recordVideo: { dir: REC, size: { width: 1920, height: 1080 } }
});
const page = await context.newPage();
page.on('console', (m) => { if (m.type() === 'error') console.log('  [page error]', m.text()); });
await page.goto(pathToFileURL(resolve('film.html')).href + '?record=1');
await page.waitForFunction('window.__duration && window.__tl && window.__scenes && window.__scenes.length', null, { timeout: 60000 }).catch(() => {});
await page.waitForTimeout(500);
const dur = await page.evaluate(() => window.__duration);
if (!dur) { console.error('film not ready'); await context.close(); await browser.close(); process.exit(1); }
console.log('duration', dur.toFixed(1), 's — recording in real time…');

// 预热视频解码：强制缓冲片尾实机视频——不动时间轴(t=0 时 demo-real 场景 opacity:0，不会露出画面)
await page.evaluate(() => window.__seek(0));
await page.evaluate(() => { const v = document.querySelector('video[src*="demo_real"]'); if (v) { v.preload = 'auto'; try { v.load(); } catch (e) {} } });
await page.waitForTimeout(2500);
await page.evaluate(() => window.__seek(0));
await page.waitForTimeout(400);

// 实时播放
const TAIL = 1.0;
await page.evaluate(() => { window.__tl.play(0); });
await page.waitForTimeout((dur + TAIL) * 1000);

const video = page.video();
await context.close();              // 关闭后 webm 落盘
await browser.close();
const webm = await video.path();
console.log('recorded webm:', webm);

// 录制含播放前的页面加载/预热段(时间轴停在 0=空白标题)。按 录制总长 − 时间轴 − 尾巴 算出前导秒数并裁掉。
const webmDur = parseFloat(execFileSync('ffprobe', ['-v', 'error', '-show_entries', 'format=duration',
  '-of', 'default=nokey=1:noprint_wrappers=1', webm]).toString().trim());
const lead = Math.max(0, webmDur - dur - TAIL);
console.log('webm', webmDur.toFixed(1), 's · lead-in', lead.toFixed(2), 's · timeline', dur.toFixed(1), 's');

// 转码 H.264 MP4（CFR 30），裁掉前导、保留整段时间轴
const out = 'film_preview.mp4';
execFileSync('ffmpeg', ['-y', '-i', webm, '-ss', lead.toFixed(2), '-t', dur.toFixed(2),
  '-c:v', 'libx264', '-crf', '20', '-preset', 'medium', '-pix_fmt', 'yuv420p', '-r', '30',
  '-movflags', '+faststart', out], { stdio: 'inherit' });
console.log('DONE ->', resolve(out), existsSync(out) ? '(ok)' : '(MISSING)');
