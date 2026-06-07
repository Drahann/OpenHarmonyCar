#!/usr/bin/env python3
"""mock-app —— PC 端"假 App"命令驱动器（给紫派/mock-purplepi 联调用，也给成员A 用）。

无需真平板、无需鸿蒙 App UI 即可驱动 9 字节 UDP 协议：交互/脚本发命令、打印心跳坐标、验证
保活与 3s 失联急停、广播发现。严格按 contracts/udp-protocol.md、udp-protocol-crosscheck.md 实现，
字节布局与 app-harmony/model/protocol.ets、tools/mock-purplepi 完全一致。

帧格式（大端）：`>BBBhhh` = state(b0) b1 b2  endX/x(int16)  endY/y(int16)  b78/r(int16)，共 9 字节。
  发送（App→紫派）：b0=命令码，b1=runState/robotId/IP段，b2=speed/IP段，x=endX，y=endY，末 2 字节 0。
  接收（紫派→App 心跳）：b0=3 表示带坐标，x/y=位姿格(5cm)，r=朝向度[-180,180]。

仅用 Python 标准库，跨平台。用法：
    python mock_app.py --ip 127.0.0.1 --cmd connect          # 发一条建连，监听心跳 2s
    python mock_app.py --ip 127.0.0.1 --cmd "goto 120 80"    # 发目标点
    python mock_app.py --ip 127.0.0.1                        # 交互模式（逐条发，输 help 看命令）
    python mock_app.py --discover                            # 广播 0x06 发现，列出回应车
"""

import argparse
import socket
import struct
import sys
import threading
import time

try:
    sys.stdout.reconfigure(errors="replace")  # Windows GBK 控制台：非 GBK 字符替换为 ? 而非崩溃
except Exception:
    pass

# ── 协议常量（对齐 contracts/udp-protocol.md 与 mock-purplepi）──────────────
UDP_PORT_DEFAULT = 5001
PACKET_BYTES = 9
DEFAULT_IP = "172.168.11.99"        # 与 constants/protocol.ets DEFAULT_TARGET_IP 一致
KEEPALIVE_INTERVAL = 1.0            # App 至少每 1s 发一次（避免紫派 3s 急停）
DISCOVERY_CMD = 6                   # 0x06 发现 ping（提案，integration-qa.md Q5）
BROADCAST_ADDR = "255.255.255.255"
HEARTBEAT_STATE_HAS_POSE = 3
DEFAULT_SPEED = 20

# 命令码（与 model/protocol.ets RobotCommand 对齐）
CMD = {
    "connect": 0, "hello": 0,          # beforeStart 建连
    "pending": 1,                       # 遥控（一般用 go/left/right/stop）
    "endmap": 2,                        # afterEnd 结束建图
    "goto": 3,                          # startRoute 设目标点
    "cancel": 4,                        # endRoute 取消导航
    "loadmap": 5,                       # loadMap 加载地图(位姿归零)
    "fpstart": 102, "fpstop": 103, "fpselect": 104,
    "dist": 105, "distend": 106,
    "corner1": 107, "corner2": 108,
    "discover": DISCOVERY_CMD,
}
# 运动方向（byte1）
RUN_STATE = {"stop": 0, "go": 1, "left": 2, "right": 3}
RUN_STATE_NAME = {v: k for k, v in RUN_STATE.items()}


def encode_command(state: int, b1: int = 0, b2: int = 0, x: int = 0, y: int = 0) -> bytes:
    """编码 9 字节发送帧（末 2 字节恒 0）。"""
    return struct.pack(">BBBhhh", state & 0xFF, b1 & 0xFF, b2 & 0xFF, x, y, 0)


def decode_heartbeat(data: bytes):
    """解析 9 字节接收帧 → (state, b1, b2, x, y, r)。"""
    raw = data[:PACKET_BYTES].ljust(PACKET_BYTES, b"\x00")
    return struct.unpack(">BBBhhh", raw)


def ip_to_cmd105_fields(ip: str):
    """主机 IP → cmd105 的 (b1, b2, x, y)：IP 四段落在 byte[1,2,4,6]（与 mock-purplepi 解码一致）。"""
    parts = [int(p) for p in ip.split(".")]
    if len(parts) != 4:
        raise ValueError(f"非法 IP: {ip}")
    # raw[1]=b1, raw[2]=b2, raw[4]=x 低字节, raw[6]=y 低字节（大端 int16，高字节 0）
    return parts[0], parts[1], parts[2], parts[3]


class MockApp:
    """假 App：持一个 UDP socket，收发 9 字节协议，可保活/广播发现。"""

    def __init__(self, ip: str, port: int = UDP_PORT_DEFAULT, bind: str = "0.0.0.0", quiet: bool = False):
        self.ip = ip
        self.port = port
        self.quiet = quiet
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.sock.bind((bind, 0))
        self.last_payload = None
        self._stop = threading.Event()
        self._keepalive = threading.Event()
        self._rx_cb = None  # 可选回调（smoke_test 用）

    # ── 收 ──────────────────────────────────────────────────────────────────
    def start_rx(self, cb=None):
        self._rx_cb = cb
        threading.Thread(target=self._rx_loop, daemon=True).start()

    def _rx_loop(self):
        while not self._stop.is_set():
            try:
                self.sock.settimeout(0.5)
                data, addr = self.sock.recvfrom(64)
            except socket.timeout:
                continue
            except OSError:
                break
            state, b1, b2, x, y, r = decode_heartbeat(data)
            if self._rx_cb is not None:
                self._rx_cb(state, b1, b2, x, y, r, addr)
            if not self.quiet:
                if state == HEARTBEAT_STATE_HAS_POSE:
                    print(f"[RX] hb <- {addr[0]}:{addr[1]}  x={x} y={y} r={r}")
                elif state == DISCOVERY_CMD:
                    print(f"[RX] discovery reply <- {addr[0]}  car_id={b1}")
                else:
                    print(f"[RX] frame <- {addr[0]}:{addr[1]}  state={state} x={x} y={y} r={r}")

    # ── 发 ──────────────────────────────────────────────────────────────────
    def send(self, payload: bytes, to=None, label: str = ""):
        target = to or (self.ip, self.port)
        self.sock.sendto(payload, target)
        self.last_payload = payload
        if not self.quiet:
            print(f"[TX] -> {target[0]}:{target[1]}  {label or payload.hex()}")

    def send_cmd(self, state: int, b1: int = 0, b2: int = 0, x: int = 0, y: int = 0, label: str = ""):
        self.send(encode_command(state, b1, b2, x, y), label=label or f"cmd {state}")

    # ── 保活（每 1s 重发最近一帧；关掉可验证 3s 急停）───────────────────────
    def keepalive(self, on: bool):
        if on and not self._keepalive.is_set():
            self._keepalive.set()
            threading.Thread(target=self._keepalive_loop, daemon=True).start()
        elif not on:
            self._keepalive.clear()

    def _keepalive_loop(self):
        while self._keepalive.is_set() and not self._stop.is_set():
            time.sleep(KEEPALIVE_INTERVAL)
            if self._keepalive.is_set() and self.last_payload is not None:
                self.sock.sendto(self.last_payload, (self.ip, self.port))

    # ── 广播发现（0x06 提案；A 确认前可用 cmd0 兜底）───────────────────────
    def discover(self, timeout: float = 3.0, ping: int = DISCOVERY_CMD, to: str = BROADCAST_ADDR):
        """广播(默认 255.255.255.255)发现；`to` 可指定具体主机/子网广播地址（联调/测试用单播更确定）。"""
        found = {}
        deadline = time.monotonic() + timeout
        self.send(encode_command(ping), to=(to, self.port), label=f"discover(cmd {ping}) -> {to}")
        self.sock.settimeout(0.5)
        while time.monotonic() < deadline:
            try:
                data, addr = self.sock.recvfrom(64)
            except socket.timeout:
                continue
            except OSError:
                break
            state, b1, _b2, x, y, r = decode_heartbeat(data)
            # 只有 0x06 身份回应的 car_id(b1) 可信；普通心跳(state=3)只记"存在"(car_id 未知=0)。
            # 注：广播回应可能来自车的真实网卡 IP（非回环），同一车经回环心跳与广播回应或现两条，属正常。
            if state == DISCOVERY_CMD:
                if found.get(addr[0]) != b1:
                    print(f"[DISCOVER] 发现车 {addr[0]}  car_id={b1}  (0x06 身份)")
                found[addr[0]] = b1
            elif addr[0] not in found:
                found[addr[0]] = 0
                print(f"[DISCOVER] 发现车 {addr[0]}  (心跳 presence, car_id 未知)")
        return found

    def close(self):
        self._stop.set()
        self._keepalive.clear()
        try:
            self.sock.close()
        except OSError:
            pass


# ── 命令行/交互解析 ────────────────────────────────────────────────────────
HELP = """可用命令：
  connect | hello           建连(cmd0)
  go [speed] | left | right | stop   遥控(cmd1)，默认 speed=20
  goto X Y                  设目标点(cmd3)
  cancel                    取消导航(cmd4)
  endmap                    结束建图(cmd2)
  loadmap                   加载地图/位姿归零(cmd5)
  fpstart | fpstop          全路径开始/停止(102/103)
  fpselect ROOM             全路径选房间(104, b1=ROOM)
  dist HOSTIP               子机拉主机图(105, 主机IP打包 byte[1,2,4,6])
  distend X Y               分布式目标点(106)
  corner1 X Y               覆盖矩形对角点1(107)
  corner2 X Y [ROBOTID]     对角点2+robot_id(108, b1=ROBOTID)
  raw STATE [B1 B2 X Y]     任意帧
  discover                  广播发现(0x06)
  keepalive on|off          每1s重发最近帧（off 可验证3s急停）
  help | quit"""


def dispatch(app: MockApp, tokens):
    """执行一条命令（tokens=已分词）。返回 False 表示退出交互。"""
    if not tokens:
        return True
    op = tokens[0].lower()
    args = tokens[1:]
    try:
        if op in ("quit", "exit", "q"):
            return False
        if op == "help":
            print(HELP)
        elif op in ("connect", "hello"):
            app.send_cmd(CMD["connect"], label="cmd 0 connect")
        elif op in RUN_STATE:
            speed = int(args[0]) if args else DEFAULT_SPEED
            app.send_cmd(CMD["pending"], b1=RUN_STATE[op], b2=speed,
                         label=f"cmd 1 pending runState={op} speed={speed}")
        elif op == "goto":
            x, y = int(args[0]), int(args[1])
            app.send_cmd(CMD["goto"], x=x, y=y, label=f"cmd 3 goto endX={x} endY={y}")
        elif op == "cancel":
            app.send_cmd(CMD["cancel"], label="cmd 4 cancel")
        elif op == "endmap":
            app.send_cmd(CMD["endmap"], label="cmd 2 endmap")
        elif op == "loadmap":
            app.send_cmd(CMD["loadmap"], label="cmd 5 loadmap")
        elif op == "fpstart":
            app.send_cmd(CMD["fpstart"], label="cmd 102 fpstart")
        elif op == "fpstop":
            app.send_cmd(CMD["fpstop"], label="cmd 103 fpstop")
        elif op == "fpselect":
            room = int(args[0])
            app.send_cmd(CMD["fpselect"], b1=room, label=f"cmd 104 fpselect room={room}")
        elif op == "dist":
            b1, b2, x, y = ip_to_cmd105_fields(args[0])
            app.send_cmd(CMD["dist"], b1=b1, b2=b2, x=x, y=y, label=f"cmd 105 dist hostIP={args[0]}")
        elif op == "distend":
            x, y = int(args[0]), int(args[1])
            app.send_cmd(CMD["distend"], x=x, y=y, label=f"cmd 106 distend endX={x} endY={y}")
        elif op == "corner1":
            x, y = int(args[0]), int(args[1])
            app.send_cmd(CMD["corner1"], x=x, y=y, label=f"cmd 107 corner1=({x},{y})")
        elif op == "corner2":
            x, y = int(args[0]), int(args[1])
            rid = int(args[2]) if len(args) > 2 else 0
            app.send_cmd(CMD["corner2"], b1=rid, x=x, y=y, label=f"cmd 108 corner2=({x},{y}) robotId={rid}")
        elif op == "raw":
            vals = [int(a) for a in args]
            state = vals[0]
            b1 = vals[1] if len(vals) > 1 else 0
            b2 = vals[2] if len(vals) > 2 else 0
            x = vals[3] if len(vals) > 3 else 0
            y = vals[4] if len(vals) > 4 else 0
            app.send_cmd(state, b1=b1, b2=b2, x=x, y=y, label=f"cmd {state} raw")
        elif op == "discover":
            app.discover()
        elif op == "keepalive":
            on = args and args[0].lower() in ("on", "1", "true")
            app.keepalive(bool(on))
            print(f"[keepalive] {'on' if on else 'off'}")
        else:
            print(f"未知命令: {op}（输 help）")
    except (IndexError, ValueError) as e:
        print(f"参数错误: {e}（输 help）")
    return True


def run_interactive(app: MockApp):
    print(f"mock-app 交互模式 → {app.ip}:{app.port}（输 help 看命令，quit 退出）")
    app.start_rx()
    while True:
        try:
            line = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            break
        if not dispatch(app, line.split()):
            break


def main():
    parser = argparse.ArgumentParser(description="mock-app（假 App 命令驱动器）")
    parser.add_argument("--ip", default=DEFAULT_IP, help=f"目标车 IP（默认 {DEFAULT_IP}）")
    parser.add_argument("--port", type=int, default=UDP_PORT_DEFAULT)
    parser.add_argument("--bind", default="0.0.0.0", help="本地绑定 IP")
    parser.add_argument("--cmd", default=None, help='单条命令（如 "goto 120 80"）；省略=交互模式')
    parser.add_argument("--watch", type=float, default=2.0, help="--cmd 后监听心跳秒数")
    parser.add_argument("--keepalive", action="store_true", help="--cmd 后保持每1s重发（验证保活）")
    parser.add_argument("--discover", action="store_true", help="广播发现后退出")
    args = parser.parse_args()

    app = MockApp(args.ip, args.port, args.bind)
    try:
        if args.discover:
            app.discover()
        elif args.cmd:
            app.start_rx()
            dispatch(app, args.cmd.split())
            if args.keepalive:
                app.keepalive(True)
            time.sleep(args.watch)
        else:
            run_interactive(app)
    finally:
        app.close()


if __name__ == "__main__":
    main()
