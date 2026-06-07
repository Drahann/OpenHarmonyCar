#!/usr/bin/env python3
"""mock_app 的自检：白盒验证编解码 + 黑盒驱动 mock-purplepi 跑通协议联调。

启动 mock_purplepi 子进程，用 mock_app 发命令并核对心跳/位姿/发现回应。
这正是 mock-app ↔ mock-purplepi 协议联调的最小闭环（无需真 App UI、无需真车）。

用法： python tools/mock-app/smoke_test.py        退出码 0 表示通过。
"""

import os
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from mock_app import (  # noqa: E402  白盒导入
    MockApp, encode_command, decode_heartbeat, ip_to_cmd105_fields,
)

MOCK_PURPLEPI = os.path.join(HERE, "..", "mock-purplepi", "mock_purplepi.py")

try:
    sys.stdout.reconfigure(errors="replace")  # Windows GBK 控制台兜底
except Exception:
    pass

UDP_PORT = 5001
passed = 0
failed = 0


def check(name, cond, detail=""):
    global passed, failed
    if cond:
        passed += 1
        print(f"  ok  {name}")
    else:
        failed += 1
        print(f"  FAIL {name}  {detail}")


def main():
    # ── ⓪ 白盒：编解码（对齐 contracts/udp-protocol.md 字节布局）──────────────
    pkt = encode_command(3, x=100, y=-100)
    check("encode 长度恒为 9", len(pkt) == 9, str(len(pkt)))
    st, b1, b2, x, y, r = struct.unpack(">BBBhhh", pkt)
    check("encode cmd3 大端 + 负数还原", st == 3 and x == 100 and y == -100, f"{st},{x},{y}")
    check("encode 末 2 字节恒 0", r == 0, str(r))
    s2, _b1, _b2, x2, y2, r2 = decode_heartbeat(encode_command(1, b1=1, b2=20, x=7, y=-3))
    check("decode 往返 state/x/y", s2 == 1 and x2 == 7 and y2 == -3, f"{s2},{x2},{y2}")
    check("ip_to_cmd105_fields 拆 4 段", ip_to_cmd105_fields("172.168.11.99") == (172, 168, 11, 99))

    # ── 黑盒：起 mock-purplepi，跑协议联调 ─────────────────────────────────────
    proc = subprocess.Popen([sys.executable, MOCK_PURPLEPI],
                            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    app = None
    try:
        time.sleep(1.5)  # 等 mock 起来
        frames = []
        app = MockApp("127.0.0.1", UDP_PORT, quiet=True)
        app.start_rx(cb=lambda *a: frames.append(a))  # (state,b1,b2,x,y,r,addr)

        # ① connect → 收到带坐标心跳(state=3)
        app.send_cmd(0, label="connect")
        time.sleep(1.2)
        poses = [f for f in frames if f[0] == 3]
        check("① connect 后收到带坐标心跳(state=3)", len(poses) > 0, f"{len(frames)} frames")

        # ② goto 远端目标 → 位姿向目标推进
        start = poses[-1] if poses else (3, 0, 0, 100, 100, 0)
        app.send_cmd(3, x=300, y=300, label="goto 300 300")
        time.sleep(2.0)
        latest = [f for f in frames if f[0] == 3][-1]
        moved = latest[3] > start[3] and latest[4] > start[4]
        check("② goto 后位姿向目标推进(x,y 增大)", moved, f"start={start[3]},{start[4]} now={latest[3]},{latest[4]}")

        # ③ 发现 ping(0x06) → 回身份 car_id=1。单播到 127.0.0.1 求确定（真机用默认广播；
        #    255.255.255.255 在本机跨虚拟网卡不一定回环到 mock，属网络环境问题非 mock-app 逻辑）。
        found = app.discover(timeout=2.0, to="127.0.0.1")
        check("③ 发现 ping 回身份(有 car_id=1)", 1 in found.values(), str(found))
    finally:
        if app is not None:
            app.close()
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()

    print(f"\n结果：{passed} 通过, {failed} 失败")
    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
