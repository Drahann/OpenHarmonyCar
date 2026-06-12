"""轻量视频流服务：仅摄像头采集 + WebSocket 推 JPEG，不加载模型。"""
import cv2
import asyncio
import time
import json
import uvicorn
from fastapi import FastAPI, WebSocket, WebSocketDisconnect

app = FastAPI(title="CamStream")

cap = None

def init_camera():
    global cap
    for dev in ["/dev/video0", "/dev/video1", "0", "1"]:
        try:
            c = cv2.VideoCapture(dev if "/" in str(dev) else int(dev))
            if c.isOpened():
                c.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
                c.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
                cap = c
                print(f"Camera opened: {dev}")
                return True
        except:
            continue
    print("No camera found!")
    return False

@app.get("/api/video/status")
def video_status():
    return {"running": cap is not None and cap.isOpened(), "fps": 0, "frame_count": 0}

@app.websocket("/ws/video")
async def ws_video(websocket: WebSocket):
    await websocket.accept()
    print("WebSocket client connected")
    frame_id = 0
    try:
        while True:
            if cap is None or not cap.isOpened():
                await asyncio.sleep(0.5)
                continue
            ret, frame = cap.read()
            if not ret:
                await asyncio.sleep(0.03)
                continue
            frame_id += 1
            ok, jpeg = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 50])
            if not ok:
                continue
            await websocket.send_bytes(jpeg.tobytes())
            meta = {
                "type": "frame_meta",
                "frame_id": frame_id,
                "timestamp": time.time(),
                "fps": 0,
                "inference_time_ms": 0,
                "yolo_time_ms": 0,
                "pose_time_ms": 0,
                "num_detections": 0,
                "detections": [],
                "gauge_angles": []
            }
            await websocket.send_text(json.dumps(meta))
            await asyncio.sleep(0.033)
    except WebSocketDisconnect:
        print("WebSocket client disconnected")

if __name__ == "__main__":
    init_camera()
    uvicorn.run(app, host="0.0.0.0", port=8000)
