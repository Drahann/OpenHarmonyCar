const fs = require('fs');
const path = require('path');

const md = fs.readFileSync(path.join(__dirname, '产品说明书v1.1.md'), 'utf8');
const lines = md.split('\n');

const figures = [
  { startLine: 297, endLine: 339, name: 'fig-2-1-system-arch', figNum: '图 2-1', caption: '系统总体架构图', section: '2.2' },
  { startLine: 375, endLine: 397, name: 'fig-2-2-comm-bus', figNum: '图 2-2', caption: '四层通信总线数据流图', section: '2.4' },
  { startLine: 417, endLine: 447, name: 'fig-2-3-sequence', figNum: '图 2-3', caption: '单车巡检任务端到端时序图', section: '2.5' },
  { startLine: 623, endLine: 633, name: 'fig-3-1-yolo-flow', figNum: '图 3-1', caption: 'YOLOv5s 仪表检测流程图', section: '3.3.2' },
  { startLine: 668, endLine: 676, name: 'fig-3-2-keypoint-flow', figNum: '图 3-2', caption: '关键点检测与热力图解码流程图', section: '3.4.2' },
  { startLine: 819, endLine: 840, name: 'fig-3-4-ascend-pipeline', figNum: '图 3-4', caption: '昇腾推理流水线工程架构图', section: '3.7.2' },
  { startLine: 872, endLine: 887, name: 'fig-3-5-deepseek-jit', figNum: '图 3-5', caption: 'DeepSeek 端侧推理（JIT + StaticCache + 流式）流程图', section: '3.8.3' },
  { startLine: 924, endLine: 937, name: 'fig-3-6-ws-video', figNum: '图 3-6', caption: 'WebSocket 视频流交互时序图', section: '3.9.2' },
  { startLine: 973, endLine: 984, name: 'fig-4-1-purplepi-lcm', figNum: '图 4-1', caption: '紫派内部运行链路', section: '4.1' },
  { startLine: 1197, endLine: 1219, name: 'fig-5-1-softbus', figNum: '图 5-1', caption: '华为分布式软总线原理示意图', section: '5.2.1' },
  { startLine: 1245, endLine: 1261, name: 'fig-5-2-fleetmission', figNum: '图 5-2', caption: '共享黑板 FleetMission 模型图', section: '5.3.1' },
  { startLine: 1288, endLine: 1306, name: 'fig-5-3-pin-trust', figNum: '图 5-3', caption: '设备互信（distributedDeviceManager + PIN）时序图', section: '5.4.1' },
  { startLine: 1341, endLine: 1358, name: 'fig-5-4-reconciler', figNum: '图 5-4', caption: '车载 agent Reconciler 调和状态机', section: '5.5.2' },
  { startLine: 1395, endLine: 1412, name: 'fig-5-5-coop-avoid', figNum: '图 5-5', caption: '双车协同避障（COOP_AVOID）时序图', section: '5.6.2' },
  { startLine: 1468, endLine: 1478, name: 'fig-6-1-conn-state', figNum: '图 6-1', caption: 'App 与车连接状态机', section: '6.2.2' },
];

const figDir = path.join(__dirname, 'figures');

let manifest = '';
manifest += 'Mermaid Diagram Manifest\n';
manifest += '='.repeat(60) + '\n\n';

for (const fig of figures) {
  // Lines are 1-indexed in the file, array is 0-indexed
  const startIdx = fig.startLine; // skip the ```mermaid line itself
  const endIdx = fig.endLine - 1; // skip the closing ``` line
  const mermaidCode = lines.slice(startIdx, endIdx).join('\n');
  
  const filePath = path.join(figDir, fig.name + '.mmd');
  fs.writeFileSync(filePath, mermaidCode, 'utf8');
  
  manifest += `File: ${fig.name}.mmd\n`;
  manifest += `  Figure: ${fig.figNum}\n`;
  manifest += `  Caption: ${fig.caption}\n`;
  manifest += `  Section: ${fig.section}\n`;
  manifest += `  Source lines: ${fig.startLine + 1}-${fig.endLine - 1}\n`;
  manifest += '\n';
  
  console.log(`Wrote: ${fig.name}.mmd (${endIdx - startIdx} lines)`);
}

fs.writeFileSync(path.join(figDir, 'manifest.txt'), manifest, 'utf8');
console.log(`\nWrote manifest.txt`);
console.log(`Total: ${figures.length} diagrams extracted`);
