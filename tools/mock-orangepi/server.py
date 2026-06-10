#!/usr/bin/env python3
"""
mock-orangepi —— 香橙派视觉服务的本地假实现，供 App 端离线对接联调（无需真机/真香橙派）。

镜像成员B的真实栈（FastAPI + Uvicorn）与 contracts/vision-stream-api.md v1.0：
  - WS  /ws/video       每帧两条交替消息：① 二进制 JPEG（已叠加框/点/读数/FPS）② 文本 frame_meta JSON
  - REST /api/*          summary / video/status / history / data/stats / thresholds /
                         gauge/configs[/{type}] / data/report / data/reports[/{id}]

🔴 故意把 keypoints 按**部署模型的真实顺序** [pointer_tip, center, zero_mark, full_mark] 推送
   （与契约表 [center, pointer_tip, zero_mark, full_mark] 的 index 不一致），用来验证 App 端
   **按 name 映射、绝不按 index**（见 app-harmony/.../model/vision.ets、constants/vision.ets KP_*）。

用法：
  pip install -r requirements.txt
  python server.py                 # 监听 0.0.0.0:8000，App 连 ws://<本机IP>:8000/ws/video
  python server.py --port 8000 --host 0.0.0.0
  python server.py --selftest      # 不起服务，打印一帧 frame_meta 并校验契约字段后退出
  MOCK_ERROR=1 python server.py     # WS 连上即推 {"type":"error"} 再断开（测 App 错误/重连分支）
"""

import argparse
import asyncio
import io
import json
import math
import os
import sys
import time
from collections import deque

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("需要 Pillow 来生成样例帧：pip install -r requirements.txt", file=sys.stderr)
    raise

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import JSONResponse

app = FastAPI(title="mock-orangepi vision service")

# ── 内存状态（联调够用；真服务用数据库）─────────────────────────────────────
FRAME_W, FRAME_H = 640, 480
START_TS = time.time()
_frame_id = 0
_thresholds = {"low": 10.0, "high": 90.0}
_history = deque(maxlen=600)
_gauge_configs = {
    "pressure": {
        "unit": "MPa", "min_range": 0.0, "max_range": 1.6,
        "low_threshold": 0.0, "high_threshold": 1.4, "display_name": "压力表",
    }
}
_reports = [
    {"report_id": 1, "created_at": START_TS, "summary": "（示例）最近 24h 读数平稳，无越限。"},
]


def gauge_percent(t: float) -> float:
    """随时间在 0~100 间振荡的读数百分比（正弦），制造可见的指针运动。"""
    return 50.0 + 45.0 * math.sin(t * 0.6)


def keypoints_for(percent: float):
    """
    由读数百分比算 4 个关键点的像素坐标（表盘扫角 225°→-45°，顺时针 270° 量程）。
    返回顺序刻意为模型真实顺序 [pointer_tip, center, zero_mark, full_mark]（≠契约表 index）。
    """
    cx, cy, r = FRAME_W * 0.5, FRAME_H * 0.55, 120.0

    def pt(deg):
        rad = math.radians(deg)
        return cx + r * math.cos(rad), cy - r * math.sin(rad)

    zero_deg, full_deg = 225.0, -45.0
    ang = zero_deg + (full_deg - zero_deg) * (percent / 100.0)
    px, py = pt(ang)
    zx, zy = pt(zero_deg)
    fx, fy = pt(full_deg)
    return [
        {"name": "pointer_tip", "x": round(px, 1), "y": round(py, 1), "conf": 0.91},
        {"name": "center",      "x": round(cx, 1), "y": round(cy, 1), "conf": 0.95},
        {"name": "zero_mark",   "x": round(zx, 1), "y": round(zy, 1), "conf": 0.93},
        {"name": "full_mark",   "x": round(fx, 1), "y": round(fy, 1), "conf": 0.85},
    ]


def make_meta(frame_id: int, percent: float, fps: float) -> dict:
    """构造一帧 frame_meta（字段名逐字对齐 contracts/vision-stream-api.md §2）。"""
    kps = keypoints_for(percent)
    return {
        "type": "frame_meta",
        "frame_id": frame_id,
        "timestamp": round(time.time(), 3),
        "fps": round(fps, 1),
        "inference_time_ms": 29.5,
        "yolo_time_ms": 21.0,
        "pose_time_ms": 8.5,
        "num_detections": 1,
        "detections": [{
            "bbox": [int(FRAME_W * 0.25), int(FRAME_H * 0.18),
                     int(FRAME_W * 0.5), int(FRAME_H * 0.7)],
            "score": 0.997,
            "keypoints": kps,
        }],
        "gauge_angles": [round(percent, 1)],
    }


def render_frame(meta: dict) -> bytes:
    """画一帧"已叠加推理结果"的 JPEG（框 + 关键点 + 读数 + FPS），质量 50。"""
    img = Image.new("RGB", (FRAME_W, FRAME_H), (24, 26, 30))
    d = ImageDraw.Draw(img)
    det = meta["detections"][0]
    x, y, w, h = det["bbox"]
    d.rectangle([x, y, x + w, y + h], outline=(72, 200, 120), width=3)
    # 表盘圆
    kp = {p["name"]: p for p in det["keypoints"]}
    cx, cy = kp["center"]["x"], kp["center"]["y"]
    d.ellipse([cx - 120, cy - 120, cx + 120, cy + 120], outline=(120, 130, 140), width=2)
    # 指针（center → pointer_tip）
    d.line([cx, cy, kp["pointer_tip"]["x"], kp["pointer_tip"]["y"]], fill=(234, 67, 53), width=4)
    # 关键点
    colors = {"center": (255, 255, 255), "pointer_tip": (234, 67, 53),
              "zero_mark": (26, 115, 232), "full_mark": (251, 188, 4)}
    for p in det["keypoints"]:
        c = colors.get(p["name"], (255, 255, 0))
        d.ellipse([p["x"] - 5, p["y"] - 5, p["x"] + 5, p["y"] + 5], fill=c)
    d.text((12, 10), f"FPS {meta['fps']:.1f}  reading {meta['gauge_angles'][0]:.0f}%", fill=(255, 255, 255))
    buf = io.BytesIO()
    img.save(buf, format="JPEG", quality=50)
    return buf.getvalue()


# ── WebSocket：视频流 ────────────────────────────────────────────────────────
@app.websocket("/ws/video")
async def ws_video(ws: WebSocket):
    await ws.accept()
    # 测试错误分支：连上即推 error 再断开
    if os.environ.get("MOCK_ERROR") == "1":
        await ws.send_text(json.dumps({"type": "error", "message": "摄像头初始化失败"}))
        await ws.close()
        return

    global _frame_id
    last = time.time()
    fps = 15.0
    try:
        while True:
            now = time.time()
            dt = now - last
            last = now
            if dt > 0:
                fps = 0.8 * fps + 0.2 * (1.0 / dt)
            _frame_id += 1
            percent = gauge_percent(now - START_TS)
            meta = make_meta(_frame_id, percent, fps)
            # 历史记录（供 /api/history、/api/summary 复用）
            _record_history(meta)
            # 交替推送：① 二进制 JPEG ② 文本 JSON
            await ws.send_bytes(render_frame(meta))
            await ws.send_text(json.dumps(meta))
            await asyncio.sleep(1.0 / 15.0)
    except WebSocketDisconnect:
        return


def _status_for(percent: float) -> str:
    if percent < _thresholds["low"] or percent > _thresholds["high"]:
        return "ALARM"
    return "NORMAL"


def _record_history(meta: dict):
    percent = meta["gauge_angles"][0] if meta["gauge_angles"] else float("nan")
    _history.append({
        "timestamp": meta["timestamp"],
        "frame_id": meta["frame_id"],
        "readings": [{"value": percent, "status": _status_for(percent)}],
    })


def _latest_percent() -> float:
    if _history:
        return _history[-1]["readings"][0]["value"]
    return gauge_percent(time.time() - START_TS)


# ── REST ─────────────────────────────────────────────────────────────────────
@app.get("/api/summary")
def summary():
    percent = _latest_percent()
    status = _status_for(percent)
    return {
        "fps": 15.2,
        "latency_ms": 29.5,
        "frame_id": _frame_id,
        "over_limit": status == "ALARM",
        "readings": [{
            "value": round(percent, 1), "unit": "%", "label": "仪表读数",
            "status": status, "gauge_id": 1,
        }],
    }


@app.get("/api/video/status")
def video_status():
    return {"running": True, "fps": 15.2, "frame_count": _frame_id}


@app.get("/api/history")
def history():
    return {"history": list(_history)}


@app.get("/api/data/stats")
def data_stats():
    vals = [h["readings"][0]["value"] for h in _history] or [0.0]
    return {"count": len(vals), "min": min(vals), "max": max(vals), "avg": sum(vals) / len(vals)}


@app.post("/api/thresholds")
async def set_thresholds(req: dict):
    _thresholds["low"] = float(req.get("low", _thresholds["low"]))
    _thresholds["high"] = float(req.get("high", _thresholds["high"]))
    return {"ok": True, "thresholds": _thresholds}


@app.get("/api/gauge/configs")
def gauge_configs():
    return _gauge_configs


@app.get("/api/gauge/configs/{gauge_type}")
def gauge_config(gauge_type: str):
    if gauge_type in _gauge_configs:
        return _gauge_configs[gauge_type]
    return JSONResponse({"detail": "not found"}, status_code=404)


@app.post("/api/gauge/configs/{gauge_type}")
async def save_gauge_config(gauge_type: str, req: dict):
    _gauge_configs[gauge_type] = req
    return {"ok": True, "gauge_type": gauge_type}


@app.delete("/api/gauge/configs/{gauge_type}")
def delete_gauge_config(gauge_type: str):
    _gauge_configs.pop(gauge_type, None)
    return {"ok": True}


@app.post("/api/data/report")
async def data_report(req: dict = None):
    # 真服务此处会暂停摄像头释放 NPU；mock 直接返回示例报告
    rid = len(_reports) + 1
    rep = {"report_id": rid, "created_at": time.time(),
           "summary": f"（示例报告 #{rid}）最近 {(req or {}).get('hours', 24)}h 读数分析：平稳。"}
    _reports.append(rep)
    return rep


@app.get("/api/data/reports")
def data_reports(limit: int = 10, offset: int = 0):
    return {"reports": _reports[offset:offset + limit], "total": len(_reports)}


@app.get("/api/data/reports/{report_id}")
def data_report_detail(report_id: int):
    for r in _reports:
        if r["report_id"] == report_id:
            return r
    return JSONResponse({"detail": "not found"}, status_code=404)


def selftest():
    """离线校验：构造一帧 meta，断言契约必需字段齐全、keypoints 含全部 4 个 name。"""
    meta = make_meta(1, 80.5, 15.2)
    required = {"type", "frame_id", "timestamp", "fps", "inference_time_ms",
                "yolo_time_ms", "pose_time_ms", "num_detections", "detections", "gauge_angles"}
    assert required <= set(meta), f"缺字段: {required - set(meta)}"
    names = {kp["name"] for kp in meta["detections"][0]["keypoints"]}
    assert names == {"center", "pointer_tip", "zero_mark", "full_mark"}, names
    assert meta["type"] == "frame_meta"
    print(json.dumps(meta, ensure_ascii=False, indent=2))
    print("selftest OK：字段齐全、4 个关键点 name 完整（注意推送顺序故意≠契约 index）")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if args.selftest:
        selftest()
    else:
        import uvicorn
        uvicorn.run(app, host=args.host, port=args.port)
