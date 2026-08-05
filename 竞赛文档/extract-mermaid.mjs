import { readFileSync, writeFileSync, mkdirSync, existsSync } from 'fs';
import { join } from 'path';

const mdPath = 'W:\\paper\\wangtiao\\产品说明书v1.1.md';
const figDir = 'W:\\paper\\wangtiao\\figures';

if (!existsSync(figDir)) mkdirSync(figDir, { recursive: true });

const md = readFileSync(mdPath, 'utf-8');
const lines = md.split('\n');

// Map line numbers to figure IDs based on nearby text
const figureMap = [
  { line: 297, id: 'fig-2-1', caption: '系统总体架构图' },
  { line: 375, id: 'fig-2-2', caption: '四层通信总线数据流图' },
  { line: 417, id: 'fig-2-3', caption: '单车巡检任务端到端时序图' },
  { line: 623, id: 'fig-3-1', caption: 'YOLOv5s仪表检测流程图' },
  { line: 668, id: 'fig-3-2', caption: '关键点检测与热力图解码流程图' },
  { line: 819, id: 'fig-3-4', caption: '昇腾推理流水线工程架构图' },
  { line: 872, id: 'fig-3-5', caption: 'DeepSeek端侧推理流程图' },
  { line: 924, id: 'fig-3-6', caption: 'WebSocket视频流交互时序图' },
  { line: 973, id: 'fig-4-1', caption: '紫派内部运行链路图' },
  { line: 1197, id: 'fig-5-1', caption: '华为分布式软总线原理示意图' },
  { line: 1245, id: 'fig-5-2', caption: '共享黑板FleetMission模型图' },
  { line: 1288, id: 'fig-5-3', caption: '设备互信时序图' },
  { line: 1341, id: 'fig-5-4', caption: '车载agent Reconciler调和状态机' },
  { line: 1395, id: 'fig-5-5', caption: '双车协同避障时序图' },
  { line: 1468, id: 'fig-6-1', caption: 'App与车连接状态机' },
];

const manifest = [];
let extracted = 0;

for (const fig of figureMap) {
  // Find the mermaid block starting near this line
  let startLine = -1;
  for (let i = fig.line - 1; i < Math.min(fig.line + 10, lines.length); i++) {
    if (lines[i].trim() === '```mermaid') {
      startLine = i;
      break;
    }
  }
  if (startLine === -1) {
    console.log(`WARNING: Could not find mermaid block for ${fig.id} at line ${fig.line}`);
    continue;
  }

  // Find the closing ```
  let endLine = -1;
  for (let i = startLine + 1; i < lines.length; i++) {
    if (lines[i].trim() === '```') {
      endLine = i;
      break;
    }
  }
  if (endLine === -1) {
    console.log(`WARNING: Could not find end of mermaid block for ${fig.id}`);
    continue;
  }

  const content = lines.slice(startLine + 1, endLine).join('\n');
  const mmdFile = join(figDir, `${fig.id}.mmd`);
  writeFileSync(mmdFile, content, 'utf-8');
  console.log(`Extracted: ${fig.id} -> ${mmdFile} (${endLine - startLine - 1} lines)`);
  manifest.push(`${fig.id}|${fig.caption}`);
  extracted++;
}

writeFileSync(join(figDir, 'manifest.txt'), manifest.join('\n'), 'utf-8');
console.log(`\nTotal extracted: ${extracted}/${figureMap.length}`);
