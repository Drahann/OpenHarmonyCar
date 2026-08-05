// 将 figures/html/*.html 渲染为矢量 PDF 到 figures/pdf/
// 复用 W:\paper\wangtiao 下已安装的 puppeteer（mermaid-cli 的依赖）
// 用法：node render-figures.cjs [某个文件名，不含扩展名]
const { readdirSync, mkdirSync } = require('fs');
const { join } = require('path');
const { pathToFileURL } = require('url');
const puppeteer = require('W:/paper/wangtiao/node_modules/puppeteer');

const htmlDir = join(__dirname, 'figures', 'html');
const outDir = join(__dirname, 'figures', 'pdf');
mkdirSync(outDir, { recursive: true });

const only = process.argv[2];
let files = readdirSync(htmlDir).filter(f => f.endsWith('.html'));
if (only) files = files.filter(f => f === `${only}.html`);
if (files.length === 0) {
  console.log('没有找到待渲染的 HTML 文件');
  process.exit(1);
}

(async () => {
  // puppeteer 缓存的 Chrome 不完整，改用系统 Edge（Chromium 内核，支持矢量 PDF 打印）
  const browser = await puppeteer.launch({
    headless: true,
    executablePath: 'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  });
  for (const f of files) {
    const page = await browser.newPage();
    await page.goto(pathToFileURL(join(htmlDir, f)).href, { waitUntil: 'networkidle0' });
    await page.evaluate(() => document.fonts.ready);
    const size = await page.evaluate(() => {
      const el = document.getElementById('fig');
      return { w: el.offsetWidth, h: el.offsetHeight };
    });
    await page.setViewport({ width: size.w, height: size.h });
    const out = join(outDir, f.replace(/\.html$/, '.pdf'));
    await page.pdf({
      path: out,
      width: `${size.w}px`,
      height: `${size.h}px`,
      printBackground: true,
      margin: { top: 0, right: 0, bottom: 0, left: 0 },
      pageRanges: '1',
    });
    console.log(`OK: ${f} -> figures/pdf/${f.replace(/\.html$/, '.pdf')} (${size.w}x${size.h})`);
    await page.close();
  }
  await browser.close();
})().catch(e => { console.error(e.message); process.exit(1); });
