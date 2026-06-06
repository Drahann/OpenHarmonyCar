#!/usr/bin/env python3
"""PC 端"假紫派"——给鸿蒙 App 在无真车时联调用。

严格按 contracts/udp-protocol.md、udp-protocol-crosscheck.md、map-format.md、multi-robot-collab.md 实现：
  - UDP <bind>:5001：收 App 9 字节指令；收到命令 0(建连) 后每 500ms 回一帧 9 字节心跳，
    心跳 byte0=3 带坐标(x/y/r, int16 大端)。位姿随命令变化（导航/遥控/覆盖）。
  - 命令感知：0 建连 / 1 遥控 / 2 结束建图 / 3 目标点 / 4 取消 / 5 加载图(位姿归零) /
    102-104 全路径 / 105 子机拉主机图(主机IP在byte[1,2,4,6]) / 106 目标点 /
    107 覆盖矩形对角点1 / 108 对角点2+robot_id(开始覆盖)。
  - 发现：① 命令 0 走广播也能建连（兜底）；② 提案 0x06 "发现 ping"——回一帧身份(byte1=车号)
    但**不记为受控客户端、不武装 3s 急停**（contracts/integration-qa.md Q5，待 A 定）。
  - 超过 3s 未收到 App 指令则打印"急停"（验证保活逻辑）。
  - HTTP <bind>:8000 托管地图：URL /defultMap.txt（紫派 web 根=/data/test，URL 无前缀）；
    另托管 /roadFile.txt（cmd124 子机会一起拉）。可 --gen-map 生成大图以过 App 就绪阈值。

仅用 Python 标准库，跨平台。用法见 README / docs/testing.md：
    python mock_purplepi.py                          # 单车，取 contracts/fixtures/defultMap.txt
    python mock_purplepi.py --id 2 --gen-map 1800x1800   # 车号2 + 生成 ~3.3MB 大图(过就绪阈值)
    python mock_purplepi.py --bind 127.0.0.2         # 多车时绑不同 IP（见 mock_fleet / docs/testing.md §五）
"""

import argparse
import math
import os
import socket
import struct
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

try:
    sys.stdout.reconfigure(errors="replace")  # Windows GBK 控制台：非 GBK 字符替换为 ? 而非崩溃
except Exception:
    pass

# ── 协议常量（对齐 contracts/udp-protocol.md）──────────────────────────────
UDP_PORT_DEFAULT = 5001
HTTP_PORT_DEFAULT = 8000
PACKET_BYTES = 9
HEARTBEAT_INTERVAL = 0.5   # s（对账：紫派 udp.c usleep(500000)=500ms）
FAILSAFE_TIMEOUT = 3.0     # s，紫派侧 3s 未收指令急停
HEARTBEAT_STATE_HAS_POSE = 3
DISCOVERY_CMD = 6          # 0x06：发现 ping（提案，待 A 确认，见 integration-qa.md Q5）
STEP = 5                   # 每个心跳步进的栅格数（5 格=25cm/0.5s≈0.5m/s）
MAP_HTTP_PATH = "/defultMap.txt"  # 紫派 web 根=/data/test，URL 无前缀（✅A确认）

# 命令码 -> 名称（对齐 contracts/udp-protocol-crosscheck.md，已收口）
COMMANDS = {
    0: "beforeStart(建连/启动建图)",
    1: "pending(遥控运动)",
    2: "afterEnd(结束建图/存图)",
    3: "startRoute(设目标点)",
    4: "endRoute(取消导航)",
    5: "loadMap(加载地图/位姿归零0,0,0)",
    DISCOVERY_CMD: "discoveryPing(发现ping·提案)",
    102: "fullpathStartRoute('f')",
    103: "fullpathStartover('g')",
    104: "fullpathSelect('h')",
    105: "distributed('i'·子机拉主机图)",
    106: "distributedEnd('j'·目标点)",
    107: "distAreaCorner1('k'·覆盖矩形对角点1)",
    108: "distAreaCorner2('l'·对角点2+robot_id→覆盖)",
}
RUN_STATES = {0: "stop", 1: "go", 2: "left", 3: "right"}

_FIXTURE = os.path.normpath(
    os.path.join(os.path.dirname(__file__), "..", "..", "contracts", "fixtures", "defultMap.txt")
)


def gen_map_text(rows: int, cols: int) -> str:
    """生成 rows×cols 边框稠密栅格图。首行 4 值 `range resolution height width`（A 格式）。
    1800×1800 约 3.3MB，可过 App MAP_READY_MIN_BYTES(324e4)。"""
    lines = [f"0 0.05 {rows} {cols}"]
    border = "1" * cols
    middle = "1" + "0" * (cols - 2) + "1"
    for y in range(rows):
        lines.append(border if (y == 0 or y == rows - 1) else middle)
    return "\n".join(lines) + "\n"


class RobotSim:
    """单车共享状态：当前客户端、最近收包时间、位姿、运动模式。"""

    def __init__(self, car_id: int, start_x: int = 100, start_y: int = 100):
        self.lock = threading.Lock()
        self.car_id = car_id
        self.client = None              # (ip, port)，cmd0 建连后才有
        self.last_rx = 0.0
        self.failsafe_announced = False
        # 位姿（栅格单位 = 5cm；与 App 整数坐标 1:1）
        self.x = float(start_x)
        self.y = float(start_y)
        self.r = 0.0
        # 运动：idle / manual / nav / coverage
        self.mode = "idle"
        self.tx = 0.0
        self.ty = 0.0
        self.manual_cmd = 0
        self.corner1 = None             # cmd107 暂存

    def on_command(self, addr):
        with self.lock:
            self.client = addr
            self.last_rx = time.monotonic()
            self.failsafe_announced = False

    def set_nav(self, tx, ty, mode="nav"):
        with self.lock:
            self.tx, self.ty, self.mode = float(tx), float(ty), mode

    def set_manual(self, cmd):
        with self.lock:
            self.manual_cmd, self.mode = cmd, "manual"

    def cancel(self):
        with self.lock:
            self.mode = "idle"

    def reset_pose(self):
        with self.lock:
            self.x, self.y, self.r, self.mode = 0.0, 0.0, 0.0, "idle"

    def snapshot(self):
        with self.lock:
            return self.car_id, int(round(self.x)), int(round(self.y)), int(round(self.r))

    def step_pose(self):
        """按 mode 推进位姿一帧，返回 (client, x, y, r)。"""
        with self.lock:
            if self.mode in ("nav", "coverage"):
                dx, dy = self.tx - self.x, self.ty - self.y
                dist = math.hypot(dx, dy)
                if dist <= STEP:
                    self.x, self.y, self.mode = self.tx, self.ty, "idle"
                else:
                    self.r = math.degrees(math.atan2(dy, dx))
                    self.x += STEP * dx / dist
                    self.y += STEP * dy / dist
            elif self.mode == "manual":
                if self.manual_cmd == 1:      # go
                    rad = math.radians(self.r)
                    self.x += STEP * math.cos(rad)
                    self.y += STEP * math.sin(rad)
                elif self.manual_cmd == 2:    # left
                    self.r += 15
                elif self.manual_cmd == 3:    # right
                    self.r -= 15
            # idle：保持不动
            self.r = ((self.r + 180) % 360) - 180   # 归一到 [-180,180]
            return self.client, int(round(self.x)), int(round(self.y)), int(round(self.r))

    def check_failsafe(self):
        with self.lock:
            if self.client is None:
                return False
            if (time.monotonic() - self.last_rx) > FAILSAFE_TIMEOUT and not self.failsafe_announced:
                self.failsafe_announced = True
                return True
            return False


def encode_frame(b0: int, b1: int, b2: int, x: int, y: int, r: int) -> bytes:
    return struct.pack(">BBBhhh", b0, b1, b2, x, y, r)


def handle_command(data: bytes, sim: RobotSim, sock: socket.socket, addr) -> None:
    """解析并处理一帧 App→紫派 9 字节包。"""
    raw = data[:PACKET_BYTES].ljust(PACKET_BYTES, b"\x00")
    state, b1, b2, end_x, end_y, _b78 = struct.unpack(">BBBhhh", raw)

    # ① 发现 ping：只回身份，不建连、不武装急停（提案 0x06）
    if state == DISCOVERY_CMD:
        cid, x, y, r = sim.snapshot()
        sock.sendto(encode_frame(DISCOVERY_CMD, cid, 0, x, y, r), addr)
        print(f"[UDP] discovery <- {addr[0]}:{addr[1]} -> 回身份 car_id={cid}")
        return

    # ② 其余命令：建连/续命（武装心跳与 3s 急停）
    sim.on_command(addr)
    name = COMMANDS.get(state, f"未知命令({state})")
    extra = ""
    if state == 1:
        sim.set_manual(b1)
        extra = f"  runState={RUN_STATES.get(b1, b1)} speed={b2}"
    elif state == 3:
        sim.set_nav(end_x, end_y, "nav")
        extra = f"  endX={end_x} endY={end_y}"
    elif state == 4:
        sim.cancel()
    elif state == 5:
        sim.reset_pose()
        extra = "  位姿归零(0,0,0)"
    elif state == 105:
        # 主机 IP 四段在 byte[1],[2],[4],[6]（endX 低字节=raw[4]、endY 低字节=raw[6]）
        ip = f"{raw[1]}.{raw[2]}.{raw[4]}.{raw[6]}"
        extra = f"  主机IP={ip}（子机将 wget 拉图）"
    elif state == 106:
        sim.set_nav(end_x, end_y, "nav")
        extra = f"  目标点 endX={end_x} endY={end_y}"
    elif state == 107:
        with sim.lock:
            sim.corner1 = (end_x, end_y)
        extra = f"  对角点1=({end_x},{end_y}) 暂存"
    elif state == 108:
        c1 = sim.corner1 or (0, 0)
        sim.set_nav(end_x, end_y, "coverage")   # 简化：朝对角点2移动模拟覆盖
        extra = f"  对角点2=({end_x},{end_y}) robot_id={b1}  矩形[{c1}→({end_x},{end_y})] 开始覆盖"
    print(f"[UDP] {addr[0]}:{addr[1]} -> cmd {state} {name}{extra}")


def udp_recv_loop(sock: socket.socket, sim: RobotSim):
    while True:
        try:
            data, addr = sock.recvfrom(64)
        except OSError as e:
            print(f"[UDP] recv error: {e}")
            continue
        handle_command(data, sim, sock, addr)


def heartbeat_loop(sock: socket.socket, sim: RobotSim):
    while True:
        time.sleep(HEARTBEAT_INTERVAL)
        if sim.check_failsafe():
            print(f"[UDP] [!] car{sim.car_id} 3s 未收到指令 - 急停（保活验证）")
        client, x, y, r = sim.step_pose()
        if client is not None:
            try:
                sock.sendto(encode_frame(HEARTBEAT_STATE_HAS_POSE, 0, 0, x, y, r), client)
                print(f"[UDP] hb car{sim.car_id} -> {client[0]}:{client[1]}  x={x} y={y} r={r} ({sim.mode})")
            except OSError as e:
                print(f"[UDP] send error: {e}")


def make_http_handler(map_text: str, road_text: str):
    map_body = map_text.encode("utf-8")
    road_body = road_text.encode("utf-8")

    class MapHandler(BaseHTTPRequestHandler):
        def _send(self, body: bytes):
            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):  # noqa: N802 (http.server 接口命名)
            if self.path.endswith("roadFile.txt"):
                self._send(road_body)
                print(f"[HTTP] GET {self.path} -> 200 roadFile ({len(road_body)}B)")
            elif self.path.endswith(".txt") or self.path == "/":  # 宽松匹配地图
                self._send(map_body)
                print(f"[HTTP] GET {self.path} -> 200 map ({len(map_body)}B)")
            else:
                self.send_error(404, "only map / roadFile are served here")

        def log_message(self, *args):
            pass  # 用我们自己的打印

    return MapHandler


def main():
    parser = argparse.ArgumentParser(description="mock-purplepi（假紫派，单车）")
    parser.add_argument("--id", type=int, default=1, help="车号（发现响应里回传）")
    parser.add_argument("--bind", default="0.0.0.0", help="绑定 IP（多车时各绑不同 IP，见 docs/testing.md §五）")
    parser.add_argument("--udp-port", type=int, default=UDP_PORT_DEFAULT)
    parser.add_argument("--http-port", type=int, default=HTTP_PORT_DEFAULT)
    parser.add_argument("--map", default=_FIXTURE, help="地图文件路径（默认 fixtures/defultMap.txt）")
    parser.add_argument("--gen-map", default=None, metavar="行x列",
                        help="生成边框图替代 --map，如 1800x1800（过 App 就绪阈值 324e4）")
    parser.add_argument("--start-x", type=int, default=100)
    parser.add_argument("--start-y", type=int, default=100)
    args = parser.parse_args()

    # 地图来源：--gen-map 优先
    if args.gen_map:
        rows, cols = (int(v) for v in args.gen_map.lower().split("x"))
        map_text = gen_map_text(rows, cols)
        map_src = f"生成 {rows}x{cols}（{len(map_text)}B）"
    else:
        with open(args.map, "r", encoding="utf-8") as f:
            map_text = f.read()
        map_src = f"{args.map}（{len(map_text)}B）"
    road_text = "# mock roadFile.txt (占位；真覆盖路径由紫派 Navi 产出)\n"

    sim = RobotSim(args.id, args.start_x, args.start_y)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.bind, args.udp_port))
    print(f"[UDP] car{args.id} listening on {args.bind}:{args.udp_port}（500ms 心跳，3s 急停）")

    threading.Thread(target=udp_recv_loop, args=(sock, sim), daemon=True).start()
    threading.Thread(target=heartbeat_loop, args=(sock, sim), daemon=True).start()

    handler = make_http_handler(map_text, road_text)
    httpd = ThreadingHTTPServer((args.bind, args.http_port), handler)
    print(f"[HTTP] serving 地图={map_src}")
    print(f"[HTTP] map  at http://{args.bind}:{args.http_port}{MAP_HTTP_PATH}")
    print(f"[HTTP] road at http://{args.bind}:{args.http_port}/roadFile.txt")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nbye")


if __name__ == "__main__":
    main()
