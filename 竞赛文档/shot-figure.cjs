// 将 figures/html/<name>.html 截图到 figures/preview/<name>.png（2x 清晰度，用于肉眼检查排版）
// 用法：node shot-figure.cjs <文件名，不含扩展名>
const { mkdirSync } = require('fs');
const { join } = require('path');
const { pathToFileURL } = require('url');
const puppeteer = require('W:/paper/wangtiao/node_modules/puppeteer');

const name = process.argv[2];
if (!name) { console.log('用法: node shot-figure.cjs <name>'); process.exit(1); }

const outDir = join(__dirname, 'figures', 'preview');
mkdirSync(outDir, { recursive: true });

(async () => {
  const browser = await puppeteer.launch({
    headless: true,
    executablePath: 'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  });
  const page = await browser.newPage();
  await page.goto(pathToFileURL(join(__dirname, 'figures', 'html', `${name}.html`)).href, { waitUntil: 'networkidle0' });
  await page.evaluate(() => document.fonts.ready);
  const size = await page.evaluate(() => {
    const el = document.getElementById('fig');
    return { w: el.offsetWidth, h: el.offsetHeight };
  });
  await page.setViewport({ width: size.w, height: size.h, deviceScaleFactor: 2 });
  const el = await page.$('#fig');
  const out = join(outDir, `${name}.png`);
  await el.screenshot({ path: out });
  console.log(`OK: ${out} (${size.w}x${size.h} @2x)`);
  await browser.close();
})().catch(e => { console.error(e.message); process.exit(1); });
