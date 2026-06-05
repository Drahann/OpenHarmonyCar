#!/usr/bin/env python3
"""mock_purplepi 的自检：启动 mock 子进程，验证 HTTP 拉图 + UDP 建连/心跳。

用法： python tools/mock-purplepi/smoke_test.py
退出码 0 表示通过。
"""

import os
import struct
import subprocess
import sys
import time
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
MOCK = os.path.join(HERE, "mock_purplepi.py")
UDP_PORT = 5001
HTTP_PORT = 8000

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
    proc = subprocess.Popen([sys.executable, MOCK],
                            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    try:
        time.sleep(1.5)  # 等服务起来

        # ① HTTP 拉图
        try:
            with urllib.request.urlopen(f"http://127.0.0.1:{HTTP_PORT}/defultMap.txt", timeout=3) as r:
                body = r.read().decode("utf-8", "replace")
            check("HTTP 200 + 首行行列数", r.status == 200 and body.splitlines()[0].strip().endswith("40"), body[:20])
            check("HTTP 地图含障碍行", "1111" in body)
        except Exception as e:  # noqa: BLE001
            check("HTTP 拉图", False, str(e))

        # ② UDP 建连 + 收心跳
        import socket
        cli = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        cli.settimeout(3.0)
        cli.bind(("0.0.0.0", 0))
        hello = struct.pack(">BBBhhh", 0, 0, 0, 0, 0, 0)  # 命令 0 建连
        cli.sendto(hello, ("127.0.0.1", UDP_PORT))
        got_pose = False
        try:
            for _ in range(3):
                data, _addr = cli.recvfrom(64)
                state, _r, _s, x, y, r = struct.unpack(">BBBhhh", data[:9])
                if state == 3:
                    got_pose = True
                    print(f"  ..  heartbeat state={state} x={x} y={y} r={r}")
                    break
        except socket.timeout:
            pass
        finally:
            cli.close()
        check("UDP 收到带坐标心跳(state=3)", got_pose)
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()

    print(f"\n结果：{passed} 通过, {failed} 失败")
    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
