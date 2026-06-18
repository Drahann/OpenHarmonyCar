#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
推理结果图片服务器
- 浏览所有推理结果图片
- 支持实时刷新（WebSocket 自动检测新图片）
- 局域网内任意设备可访问

启动: python3 image_server.py [--port 8080] [--results-dir ./results]
访问: http://192.168.1.5:8080
"""

import os
import sys
import time
import json
import argparse
import logging
import asyncio
from typing import List, Dict

import uvicorn
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse, HTMLResponse, JSONResponse
from fastapi.middleware.cors import CORSMiddleware

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# 配置
# ---------------------------------------------------------------------------
IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}
DEFAULT_RESULTS_ROOT = os.path.expanduser("~/simple_baselines/deployment_bundle")

# ---------------------------------------------------------------------------
# FastAPI App
# ---------------------------------------------------------------------------
app = FastAPI(title="仪表推理结果图片服务器")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# 全局变量：由 main() 设置
RESULTS_ROOT = DEFAULT_RESULTS_ROOT


def scan_images(root: str) -> List[Dict]:
    """扫描所有子目录中的图片，按修改时间倒序返回"""
    images = []
    for dirpath, dirnames, filenames in os.walk(root):
        for fn in filenames:
            ext = os.path.splitext(fn)[1].lower()
            if ext not in IMAGE_EXTS:
                continue
            full = os.path.join(dirpath, fn)
            rel = os.path.relpath(full, root)
            stat = os.stat(full)
            images.append({
                "name": fn,
                "path": rel.replace("\\", "/"),
                "folder": os.path.relpath(dirpath, root).replace("\\", "/"),
                "size": stat.st_size,
                "mtime": stat.st_mtime,
                "mtime_str": time.strftime("%Y-%m-%d %H:%M:%S",
                                           time.localtime(stat.st_mtime)),
            })
    images.sort(key=lambda x: x["mtime"], reverse=True)
    return images


# ---------------------------------------------------------------------------
# API 路由
# ---------------------------------------------------------------------------
@app.get("/api/images")
def list_images(folder: str = None):
    """列出所有推理结果图片"""
    imgs = scan_images(RESULTS_ROOT)
    if folder:
        imgs = [i for i in imgs if i["folder"] == folder]
    folders = sorted(set(i["folder"] for i in imgs))
    return {"total": len(imgs), "folders": folders, "images": imgs}


@app.get("/api/folders")
def list_folders():
    """列出所有结果文件夹"""
    imgs = scan_images(RESULTS_ROOT)
    folder_map: Dict[str, int] = {}
    for i in imgs:
        f = i["folder"]
        folder_map[f] = folder_map.get(f, 0) + 1
    return {
        "folders": [{"name": k, "count": v} for k, v in sorted(folder_map.items())]
    }


@app.get("/img/{path:path}")
def serve_image(path: str):
    """提供图片文件"""
    full = os.path.normpath(os.path.join(RESULTS_ROOT, path))
    # 安全检查：防止路径遍历
    if not full.startswith(os.path.normpath(RESULTS_ROOT)):
        return JSONResponse({"error": "forbidden"}, status_code=403)
    if not os.path.isfile(full):
        return JSONResponse({"error": "not found"}, status_code=404)
    return FileResponse(full)


# ---------------------------------------------------------------------------
# WebSocket: 实时推送新图片通知
# ---------------------------------------------------------------------------
ws_clients: list = []


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    ws_clients.append(websocket)
    logger.info("WebSocket client connected, total: %d", len(ws_clients))
    try:
        while True:
            imgs = scan_images(RESULTS_ROOT)
            latest = imgs[:5] if imgs else []
            await websocket.send_json({
                "type": "update",
                "latest": latest,
                "total": len(imgs),
            })
            await asyncio.sleep(2)
    except WebSocketDisconnect:
        ws_clients.remove(websocket)
        logger.info("WebSocket client disconnected, total: %d", len(ws_clients))
    except Exception:
        if websocket in ws_clients:
            ws_clients.remove(websocket)


# ---------------------------------------------------------------------------
# 前端 HTML（内嵌，无需额外文件）
# ---------------------------------------------------------------------------
FRONTEND_HTML = """<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>仪表推理结果</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body { font-family: -apple-system, "Microsoft YaHei", sans-serif;
         background: #0f172a; color: #e2e8f0; }

  .header {
    background: linear-gradient(135deg, #1e293b, #334155);
    padding: 20px 30px;
    display: flex; justify-content: space-between; align-items: center;
    border-bottom: 2px solid #3b82f6;
  }
  .header h1 { font-size: 22px; color: #60a5fa; }
  .header .info { font-size: 14px; color: #94a3b8; }
  .header .dot {
    display: inline-block; width: 10px; height: 10px;
    border-radius: 50%; background: #22c55e; margin-right: 6px;
  }

  .toolbar {
    padding: 15px 30px; background: #1e293b;
    display: flex; gap: 10px; flex-wrap: wrap; align-items: center;
  }
  .toolbar select, .toolbar button {
    padding: 8px 16px; border-radius: 6px; border: 1px solid #475569;
    background: #334155; color: #e2e8f0; font-size: 14px; cursor: pointer;
  }
  .toolbar button:hover { background: #3b82f6; border-color: #3b82f6; }
  .toolbar .auto-refresh {
    display: flex; align-items: center; gap: 6px;
    margin-left: auto; font-size: 14px;
  }

  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
    gap: 16px; padding: 20px 30px;
  }

  .card {
    background: #1e293b; border-radius: 10px; overflow: hidden;
    border: 1px solid #334155;
    transition: transform 0.2s, box-shadow 0.2s;
  }
  .card:hover {
    transform: translateY(-3px);
    box-shadow: 0 8px 25px rgba(59,130,246,0.2);
  }
  .card img {
    width: 100%; aspect-ratio: 4/3; object-fit: contain;
    background: #0f172a; cursor: pointer;
  }
  .card .meta {
    padding: 12px 16px;
    display: flex; justify-content: space-between;
    font-size: 13px; color: #94a3b8;
  }
  .card .meta .name { color: #e2e8f0; font-weight: 500; }

  /* 灯箱 */
  .lightbox {
    display: none; position: fixed; inset: 0;
    background: rgba(0,0,0,0.92); z-index: 1000;
    justify-content: center; align-items: center; cursor: zoom-out;
  }
  .lightbox.active { display: flex; }
  .lightbox img {
    max-width: 95vw; max-height: 95vh;
    object-fit: contain; border-radius: 8px;
  }

  .empty {
    text-align: center; padding: 80px 20px;
    color: #64748b; font-size: 18px;
  }

  @media (max-width: 600px) {
    .grid { grid-template-columns: 1fr; padding: 10px; }
    .header { padding: 15px; }
    .toolbar { padding: 10px; }
  }
</style>
</head>
<body>

<div class="header">
  <h1>&#128205; 仪表推理结果查看器</h1>
  <div class="info">
    <span class="dot" id="wsStatus"></span>
    <span id="totalCount">加载中...</span>
  </div>
</div>

<div class="toolbar">
  <select id="folderSelect"><option value="">全部文件夹</option></select>
  <button onclick="loadImages()">&#128260; 刷新</button>
  <div class="auto-refresh">
    <input type="checkbox" id="autoRefresh" checked>
    <label for="autoRefresh">自动刷新</label>
  </div>
</div>

<div class="grid" id="imageGrid"></div>
<div class="empty" id="emptyMsg" style="display:none">暂无推理结果图片</div>

<div class="lightbox" id="lightbox" onclick="this.classList.remove('active')">
  <img id="lightboxImg">
</div>

<script>
const grid      = document.getElementById('imageGrid');
const folderSel = document.getElementById('folderSelect');
const wsStatus  = document.getElementById('wsStatus');
let   prevTotal = -1;

async function loadFolders() {
  const res  = await fetch('/api/folders');
  const data = await res.json();
  const all  = data.folders.reduce((s, f) => s + f.count, 0);
  folderSel.innerHTML = '<option value="">\\u5168\\u90E8\\u6587\\u4EF6\\u5939 (' + all + ')</option>';
  data.folders.forEach(f => {
    folderSel.innerHTML += '<option value="' + f.name + '">' + f.name + ' (' + f.count + ')</option>';
  });
}

async function loadImages() {
  const folder = folderSel.value;
  const url = folder ? '/api/images?folder=' + encodeURIComponent(folder) : '/api/images';
  const res  = await fetch(url);
  const data = await res.json();
  prevTotal = data.total;
  document.getElementById('totalCount').textContent = '\\u5171 ' + data.total + ' \\u5F20\\u56FE\\u7247';

  if (data.images.length === 0) {
    grid.innerHTML = '';
    document.getElementById('emptyMsg').style.display = '';
    return;
  }
  document.getElementById('emptyMsg').style.display = 'none';

  grid.innerHTML = data.images.map(function(img) {
    return '<div class="card">'
      + '<img src="/img/' + img.path + '" loading="lazy"'
      + ' onclick="openLB(\\'/img/' + img.path + '\\')" alt="' + img.name + '">'
      + '<div class="meta"><span class="name">' + img.name + '</span>'
      + '<span>' + img.folder + ' &middot; ' + img.mtime_str + '</span></div></div>';
  }).join('');
}

function openLB(src) {
  document.getElementById('lightboxImg').src = src;
  document.getElementById('lightbox').classList.add('active');
}

function connectWS() {
  var proto = location.protocol === 'https:' ? 'wss' : 'ws';
  var ws = new WebSocket(proto + '://' + location.host + '/ws');
  ws.onopen  = function() { wsStatus.style.background = '#22c55e'; };
  ws.onclose = function() { wsStatus.style.background = '#ef4444'; setTimeout(connectWS, 3000); };
  ws.onmessage = function(e) {
    var msg = JSON.parse(e.data);
    if (msg.type === 'update' && document.getElementById('autoRefresh').checked) {
      document.getElementById('totalCount').textContent = '\\u5171 ' + msg.total + ' \\u5F20\\u56FE\\u7247';
      if (msg.total !== prevTotal) { loadFolders(); loadImages(); }
    }
  };
}

folderSel.addEventListener('change', loadImages);
loadFolders();
loadImages();
connectWS();
</script>
</body>
</html>
"""


@app.get("/")
def index():
    return HTMLResponse(FRONTEND_HTML)


# ---------------------------------------------------------------------------
# 主入口
# ---------------------------------------------------------------------------
def main():
    global RESULTS_ROOT

    parser = argparse.ArgumentParser(description="推理结果图片服务器")
    parser.add_argument("--port", type=int, default=8080,
                        help="监听端口 (默认 8080)")
    parser.add_argument("--host", type=str, default="0.0.0.0",
                        help="监听地址 (默认 0.0.0.0)")
    parser.add_argument("--results-dir", type=str, default=DEFAULT_RESULTS_ROOT,
                        help="推理结果根目录")
    args = parser.parse_args()

    RESULTS_ROOT = os.path.abspath(args.results_dir)

    if not os.path.isdir(RESULTS_ROOT):
        logger.error("结果目录不存在: %s", RESULTS_ROOT)
        sys.exit(1)

    imgs = scan_images(RESULTS_ROOT)
    logger.info("结果目录: %s", RESULTS_ROOT)
    logger.info("已发现 %d 张图片", len(imgs))
    logger.info("服务启动: http://%s:%d", args.host, args.port)
    logger.info("局域网访问: http://192.168.1.5:%d", args.port)

    uvicorn.run(app, host=args.host, port=args.port, log_level="info")


if __name__ == "__main__":
    main()
