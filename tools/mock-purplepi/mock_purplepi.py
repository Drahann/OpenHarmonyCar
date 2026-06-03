#!/usr/bin/env python3
"""PC 端"假紫派"——给鸿蒙 App 在无真车时联调用。

严格按 contracts/udp-protocol.md 与 contracts/map-format.md 实现：
  - UDP 0.0.0.0:5001：收 App 的 9 字节指令；收到命令 0(建连) 后每 1s 回一帧 9 字节心跳，
    心跳 byte0=3 带坐标(x/y/r, int16 大端)，做缓慢移动轨迹。
  - 命令 1(运动) 打印 runState/speed；命令 3(目标点) 打印 endX/endY；其余命令打印命令码。
  - 超过 3s 未收到 App 指令则打印"急停"（验证保活逻辑）。
  - HTTP :8000 托管示例地图，路径 /data/test/defultMap.txt。

仅用 Python 标准库，跨平台。用法：
    python mock_purplepi.py            # UDP:5001 + HTTP:8000，地图取 contracts/fixtures/defultMap.txt
    python mock_purplepi.py --udp-port 5001 --http-port 8000 --map /path/to/map.txt
App 内把目标 IP 设为本机 IP，地图 URL 指向 http://<本机IP>:8000/data/test/defultMap.txt
"""

import argparse
import math
import os
import socket
import struct
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# ── 协议常量（对齐 contracts/udp-protocol.md）──────────────────────────────
UDP_PORT_DEFAULT = 5001
HTTP_PORT_DEFAULT = 8000
PACKET_BYTES = 9
HEARTBEAT_INTERVAL = 0.5   # s（对账：紫派 udp.c usleep(500000)=500ms）
FAILSAFE_TIMEOUT = 3.0     # s，紫派侧 3s 未收指令急停
HEARTBEAT_STATE_HAS_POSE = 3
MAP_HTTP_PATH = "/defultMap.txt.txt"  # 紫派 http.server web 根=/data/test（对账更正）

# 命令码 -> 名称（对齐契约命令码表）
COMMANDS = {
    0: "beforeStart(建连/启动建图)",
    1: "pending(运动)",
    2: "afterEnd(结束建图)",
    3: "startRoute(设目标点)",
    4: "endRoute(取消导航)",
    5: "(5:加载地图/重置起点,待确认)",
    102: "fullpath_startRoute('f')",
    103: "fullpath_startover('g')",
    104: "fullpath_select('h')",
    105: "distributed('i')",
    106: "distributedEnd('j')",
    107: "PathStart('k',待确认)",
    108: "PathEnd('l',待确认)",
}
RUN_STATES = {0: "stop", 1: "go", 2: "left", 3: "right"}

_FIXTURE = os.path.normpath(
    os.path.join(os.path.dirname(__file__), "..", "..", "contracts", "fixtures", "defultMap.txt")
)


class RobotSim:
    """共享状态：当前客户端、最近收包时间、模拟位姿。"""

    def __init__(self):
        self.lock = threading.Lock()
        self.client = None              # (ip, port)
        self.last_rx = 0.0
        self.failsafe_announced = False
        # 模拟位姿（真实世界坐标）
        self.x = 100
        self.y = 100
        self.r = 0
        self.tick = 0

    def on_command(self, addr):
        with self.lock:
            self.client = addr
            self.last_rx = time.monotonic()
            self.failsafe_announced = False

    def step_pose(self):
        """缓慢移动：x 线性前进，y 正弦摆动，r 旋转。"""
        with self.lock:
            self.tick += 1
            self.x = 100 + (self.tick * 5) % 1000
            self.y = 100 + int(50 * math.sin(self.tick / 5.0))
            self.r = (self.r + 10) % 360
            return self.client, self.x, self.y, self.r

    def check_failsafe(self):
        with self.lock:
            if self.client is None:
                return False
            idle = time.monotonic() - self.last_rx
            if idle > FAILSAFE_TIMEOUT and not self.failsafe_announced:
                self.failsafe_announced = True
                return True
            return False


def decode_command(data: bytes):
    """解析 App→紫派 9 字节包。返回 (state, run_state, speed, end_x, end_y)。"""
    if len(data) < PACKET_BYTES:
        data = data.ljust(PACKET_BYTES, b"\x00")
    state, run_state, speed, end_x, end_y, _ = struct.unpack(">BBBhhh", data[:PACKET_BYTES])
    return state, run_state, speed, end_x, end_y


def encode_heartbeat(x: int, y: int, r: int) -> bytes:
    """紫派→App 9 字节心跳：byte0=3，x/y/r 为 int16 大端。"""
    return struct.pack(">BBBhhh", HEARTBEAT_STATE_HAS_POSE, 0, 0, x, y, r)


def udp_recv_loop(sock: socket.socket, sim: RobotSim):
    while True:
        try:
            data, addr = sock.recvfrom(64)
        except OSError as e:
            print(f"[UDP] recv error: {e}")
            continue
        sim.on_command(addr)
        state, run_state, speed, end_x, end_y = decode_command(data)
        name = COMMANDS.get(state, f"未知命令({state})")
        extra = ""
        if state == 1:
            extra = f"  runState={RUN_STATES.get(run_state, run_state)} speed={speed}"
        elif state == 3:
            extra = f"  endX={end_x} endY={end_y}"
        elif state == 105:
            extra = f"  (IP高/低2字节复用 endX={end_x} endY={end_y})"
        print(f"[UDP] {addr[0]}:{addr[1]} -> cmd {state} {name}{extra}")


def heartbeat_loop(sock: socket.socket, sim: RobotSim):
    while True:
        time.sleep(HEARTBEAT_INTERVAL)
        if sim.check_failsafe():
            print("[UDP] ⚠️ 3s 未收到 App 指令 —— 急停（保活验证）")
        client, x, y, r = sim.step_pose()
        if client is not None:
            try:
                sock.sendto(encode_heartbeat(x, y, r), client)
                print(f"[UDP] heartbeat -> {client[0]}:{client[1]}  x={x} y={y} r={r}")
            except OSError as e:
                print(f"[UDP] send error: {e}")


def make_http_handler(map_path: str):
    class MapHandler(BaseHTTPRequestHandler):
        def do_GET(self):  # noqa: N802 (http.server 接口命名)
            if self.path.endswith(".txt") or self.path == "/":  # 宽松匹配，兼容 .txt / .txt.txt
                try:
                    with open(map_path, "rb") as f:
                        body = f.read()
                except OSError:
                    self.send_error(500, "map fixture not found")
                    return
                self.send_response(200)
                self.send_header("Content-Type", "text/plain; charset=utf-8")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
                print(f"[HTTP] GET {self.path} -> 200 ({len(body)} bytes)")
            else:
                self.send_error(404, "only the map is served here")

        def log_message(self, *args):
            pass  # 用我们自己的打印

    return MapHandler


def main():
    parser = argparse.ArgumentParser(description="mock-purplepi（假紫派）")
    parser.add_argument("--udp-port", type=int, default=UDP_PORT_DEFAULT)
    parser.add_argument("--http-port", type=int, default=HTTP_PORT_DEFAULT)
    parser.add_argument("--map", default=_FIXTURE, help="示例地图文件路径")
    args = parser.parse_args()

    sim = RobotSim()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", args.udp_port))
    print(f"[UDP] listening on 0.0.0.0:{args.udp_port}")

    threading.Thread(target=udp_recv_loop, args=(sock, sim), daemon=True).start()
    threading.Thread(target=heartbeat_loop, args=(sock, sim), daemon=True).start()

    handler = make_http_handler(args.map)
    httpd = ThreadingHTTPServer(("0.0.0.0", args.http_port), handler)
    print(f"[HTTP] serving {args.map}")
    print(f"[HTTP] map at http://<本机IP>:{args.http_port}{MAP_HTTP_PATH}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nbye")


if __name__ == "__main__":
    main()
