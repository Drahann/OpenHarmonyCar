#!/usr/bin/env bash
# 把 film.css + engine.js + scenes*.js + 离线 gsap 打包成单文件自包含 film.html。
# 预览/导出无需加载任何外部文件。用法：bash build.sh
set -e
cd "$(dirname "$0")"
OUT=film.html
{
  cat <<'HTML'
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>OpenHarmony 工业巡检机器人 · 演示片</title>
<style>
HTML
  cat film.css
  cat <<'HTML'
</style>
</head>
<body>
<div id="viewport"><div id="stage">
  <div id="scenes"></div>
  <div class="ripple" id="ripple"></div>
  <div id="cursor"><div class="ring"></div><div class="dot"></div></div>
  <div id="caption"></div>
</div></div>
<div class="hint" id="hint">空格 播放/暂停 · ← → 拖动 · R 重播</div>
<div id="chrome"><button id="playBtn">▶</button><div id="tl"><div id="tlfill"></div></div><div id="tc">0.0 / 0.0s</div><div id="scl">—</div></div>
HTML
  echo '<script>'; cat vendor/gsap.min.js; echo '</script>'
  echo '<script>'; cat vendor/MotionPathPlugin.min.js; echo '</script>'
  echo '<script>'; cat engine.js; echo '</script>'
  for f in $(ls scenes*.js 2>/dev/null | sort); do
    echo "<script>"; cat "$f"; echo "</script>"
  done
  echo '<script>try{window.FILM.boot();}catch(e){console.error(e);var s=document.getElementById("scl");if(s)s.textContent="ERR:"+e.message;}</script>'
  cat <<'HTML'
</body>
</html>
HTML
} > "$OUT"
echo "built $OUT: $(wc -c < "$OUT") bytes | scenes: $(ls scenes*.js 2>/dev/null | sort | tr '\n' ' ')"
