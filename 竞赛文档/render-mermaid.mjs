import { readdirSync } from 'fs';
import { execSync } from 'child_process';
import { join } from 'path';

const figDir = 'W:\\paper\\wangtiao\\figures';
const mmdConfig = join(figDir, 'mermaid-config.json');
const puppeteerConfig = 'W:\\paper\\wangtiao\\puppeteer-config.json';
const files = readdirSync(figDir).filter(f => f.endsWith('.mmd'));

console.log(`Rendering ${files.length} diagrams with larger fonts...`);

for (const f of files) {
  const input = join(figDir, f);
  const output = join(figDir, f.replace('.mmd', '.png'));
  const cmd = `mmdc -i "${input}" -o "${output}" -w 2000 -b white -c "${mmdConfig}" -p "${puppeteerConfig}" --scale 2`;
  try {
    execSync(cmd, { timeout: 60000, stdio: 'pipe' });
    console.log(`OK: ${f} -> ${f.replace('.mmd', '.png')}`);
  } catch (e) {
    console.log(`FAIL: ${f} - ${e.message.substring(0, 200)}`);
  }
}

console.log('Done.');
