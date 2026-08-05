const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const figDir = path.join(__dirname, 'figures');
const files = fs.readdirSync(figDir).filter(f => f.endsWith('.mmd'));

const puppeteerConfig = path.join(__dirname, 'puppeteer-config.json');

for (const file of files) {
  const input = path.join(figDir, file);
  const output = path.join(figDir, file.replace('.mmd', '.png'));
  
  console.log(`Rendering: ${file} -> ${file.replace('.mmd', '.png')}`);
  try {
    const cmd = `npx mmdc -i "${input}" -o "${output}" -w 1600 -b transparent -p "${puppeteerConfig}"`;
    execSync(cmd, { cwd: __dirname, timeout: 30000, stdio: 'pipe' });
    console.log(`  OK`);
  } catch (err) {
    console.log(`  FAILED: ${err.stderr ? err.stderr.toString().substring(0, 200) : err.message}`);
  }
}

console.log('\nDone.');
