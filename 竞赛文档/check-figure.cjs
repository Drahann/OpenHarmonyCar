// 批量检查 figures/html/*.html 的排版与学术图示规范。
// 用法：
//   node check-figure.cjs <name>
//   node check-figure.cjs all
//
// 失败条件：
// 1. 画布不是 588 / 882 两档；
// 2. 图中文字小于纸面 7.5 pt（588 档 10 px；882 档 15 px）；
// 3. 文字溢出、元素越界、卡片重叠或标签压住卡片；
// 4. 连线标签使用不透明/半透明白底，或卡片使用阴影；
// 5. 图 2-1 缺少车内双板闭环、多车配对或协同接口语义。

const { readdirSync } = require('fs');
const { join } = require('path');
const { pathToFileURL } = require('url');
const puppeteer = require('W:/paper/wangtiao/node_modules/puppeteer');

const htmlDir = join(__dirname, 'figures', 'html');
const requested = process.argv[2];
if (!requested) {
  console.error('用法: node check-figure.cjs <name|all>');
  process.exit(2);
}

const files = requested === 'all'
  ? readdirSync(htmlDir).filter((file) => file.endsWith('.html')).sort()
  : [`${requested}.html`];

const semanticContracts = {
  'fig-2-1-arch': [
    '机器人行进状态控制',
    '识别仪表',
    '减速',
    '恢复',
    '行进状态',
    '车辆 #1',
    '车辆 #2',
    '车辆 #N',
    '香橙派 #1',
    '香橙派 #2',
    '香橙派 #N',
    '紫派 #2',
    '紫派 #N',
    'COOP_AVOID',
    '车↔车 多播避障',
    'car-agent',
    '无界面 · 软总线',
  ],
};

function paperPointSize(canvasWidth, pixels) {
  return pixels * (canvasWidth === 588 ? 0.75 : 0.5);
}

function formatList(title, items) {
  if (!items.length) return [];
  return [`  ${title}:`, ...items.map((item) => `    - ${item}`)];
}

(async () => {
  const browser = await puppeteer.launch({
    headless: true,
    executablePath: 'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  });
  const reports = [];

  for (const file of files) {
    const name = file.replace(/\.html$/, '');
    const page = await browser.newPage();
    await page.goto(pathToFileURL(join(htmlDir, file)).href, { waitUntil: 'networkidle0' });
    await page.evaluate(() => document.fonts.ready);

    const report = await page.evaluate((requiredText) => {
      const fig = document.getElementById('fig');
      if (!fig) return { fatal: '缺少 #fig 根元素' };

      const fr = fig.getBoundingClientRect();
      const selectors = [
        '.card', '.lab', '.vlab', '.grp', '.head', '.partag', '.looptag',
        '.note', '.lt', '.phase', '.state', '.legend', 'svg text',
      ];
      const elements = [...fig.querySelectorAll(selectors.join(','))];
      const visible = elements.filter((element) => {
        const style = getComputedStyle(element);
        const rect = element.getBoundingClientRect();
        return style.display !== 'none' && style.visibility !== 'hidden'
          && Number(style.opacity) > 0 && rect.width > 0 && rect.height > 0;
      });

      const info = visible.map((element) => {
        const rect = element.getBoundingClientRect();
        const style = getComputedStyle(element);
        const className = typeof element.className === 'string'
          ? element.className.split(/\s+/)[0]
          : element.tagName.toLowerCase();
        return {
          className,
          text: (element.innerText || element.textContent || '').replace(/\s+/g, ' ').trim().slice(0, 48),
          x: rect.left - fr.left,
          y: rect.top - fr.top,
          width: rect.width,
          height: rect.height,
          scrollWidth: element.scrollWidth,
          scrollHeight: element.scrollHeight,
          clientWidth: element.clientWidth,
          clientHeight: element.clientHeight,
          fontSize: Number.parseFloat(style.fontSize) || null,
          backgroundColor: style.backgroundColor,
          boxShadow: style.boxShadow,
        };
      });

      const overflow = info.filter((item) =>
        ['card', 'head', 'note', 'lt', 'state'].includes(item.className)
        && (item.scrollWidth > item.clientWidth + 2 || item.scrollHeight > item.clientHeight + 2));
      const outside = info.filter((item) =>
        item.x < -1 || item.y < -1
        || item.x + item.width > fr.width + 1
        || item.y + item.height > fr.height + 1);

      const cards = info.filter((item) => item.className === 'card');
      const labels = info.filter((item) => ['lab', 'vlab'].includes(item.className));
      const overlap = (a, b) => {
        const width = Math.min(a.x + a.width, b.x + b.width) - Math.max(a.x, b.x);
        const height = Math.min(a.y + a.height, b.y + b.height) - Math.max(a.y, b.y);
        return width > 4 && height > 4 ? `${Math.round(width)}x${Math.round(height)}` : null;
      };

      const cardOverlap = [];
      for (let i = 0; i < cards.length; i += 1) {
        for (let j = i + 1; j < cards.length; j += 1) {
          const amount = overlap(cards[i], cards[j]);
          if (amount) cardOverlap.push(`[${cards[i].text}] × [${cards[j].text}] ${amount}`);
        }
      }

      const labelCardOverlap = [];
      for (const label of labels) {
        for (const card of cards) {
          const amount = overlap(label, card);
          if (amount) labelCardOverlap.push(`[${label.text}] × [${card.text}] ${amount}`);
        }
      }

      const labelBackground = labels
        .filter((item) => !/^rgba?\([^)]*,\s*0(?:\.0+)?\)$/.test(item.backgroundColor)
          && item.backgroundColor !== 'rgba(0, 0, 0, 0)')
        .map((item) => `[${item.text}] ${item.backgroundColor}`);
      const shadows = info
        .filter((item) => ['card', 'grp', 'head'].includes(item.className) && item.boxShadow !== 'none')
        .map((item) => `[${item.text}] ${item.boxShadow}`);
      const missingText = requiredText.filter((token) => !fig.innerText.includes(token));

      return {
        canvas: { width: fr.width, height: fr.height },
        overflow,
        outside,
        cardOverlap,
        labelCardOverlap,
        labelBackground,
        shadows,
        missingText,
        fonts: info.filter((item) => item.fontSize && item.text)
          .map((item) => ({ text: item.text, className: item.className, fontSize: item.fontSize })),
      };
    }, semanticContracts[name] || []);

    await page.close();
    reports.push({ name, ...report });
  }

  await browser.close();

  let failureCount = 0;
  for (const report of reports) {
    if (report.fatal) {
      failureCount += 1;
      console.log(`FAIL ${report.name}: ${report.fatal}`);
      continue;
    }

    const errors = [];
    const { width, height } = report.canvas;
    if (![588, 882].includes(Math.round(width))) {
      errors.push(`画布宽度 ${width}px 不属于 588 / 882 两档`);
    }
    const smallFonts = report.fonts
      .filter((item) => paperPointSize(Math.round(width), item.fontSize) < 7.5 - 0.01)
      .map((item) => `[${item.text}] ${item.fontSize}px / ${paperPointSize(Math.round(width), item.fontSize).toFixed(2)}pt`);

    errors.push(...formatList('文字溢出', report.overflow.map((item) =>
      `[${item.text}] ${item.clientWidth}x${item.clientHeight} < ${item.scrollWidth}x${item.scrollHeight}`)));
    errors.push(...formatList('元素越界', report.outside.map((item) =>
      `[${item.text}] @(${Math.round(item.x)},${Math.round(item.y)}) ${Math.round(item.width)}x${Math.round(item.height)}`)));
    errors.push(...formatList('卡片重叠', report.cardOverlap));
    errors.push(...formatList('标签压卡片', report.labelCardOverlap));
    errors.push(...formatList('标签仍有白底', report.labelBackground));
    errors.push(...formatList('卡片仍有阴影', report.shadows));
    errors.push(...formatList('纸面字号低于 7.5pt', smallFonts));
    errors.push(...formatList('缺少语义', report.missingText));

    if (errors.length) {
      failureCount += 1;
      console.log(`FAIL ${report.name} (${width}x${height})`);
      errors.forEach((error) => console.log(error));
    } else {
      const minPoint = Math.min(...report.fonts.map((item) => paperPointSize(Math.round(width), item.fontSize)));
      console.log(`PASS ${report.name} (${width}x${height}, 最小字号 ${minPoint.toFixed(2)}pt)`);
    }
  }

  console.log(`\n汇总: ${reports.length - failureCount}/${reports.length} 通过`);
  process.exit(failureCount ? 1 : 0);
})().catch((error) => {
  console.error(error);
  process.exit(1);
});
