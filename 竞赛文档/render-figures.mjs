// 将 figures/html/*.html 渲染为矢量 PDF 到 figures/pdf/
// 复用项目内 mermaid-cli 自带的 puppeteer（无需新装依赖）
// 用法：node render-figures.mjs [某个文件名，不含扩展名]
import { readdirSync, mkdirSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath, pathToFileURL } from 'url';
import puppeteer from 'puppeteer';

const root = dirname(fileURLToPath(import.meta.url));
const htmlDir = join(root, 'figures', 'html');
const outDir = join(root, 'figures', 'pdf');
mkdirSync(outDir, { recursive: true });

const only = process.argv[2];
let files = readdirSync(htmlDir).filter(f => f.endsWith('.html'));
if (only) files = files.filter(f => f === `${only}.html`);
if (files.length === 0) {
  console.log('没有找到待渲染的 HTML 文件');
  process.exit(1);
}

const browser = await puppeteer.launch({ headless: true });
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
