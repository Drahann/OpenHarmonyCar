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
sys.path.insert(0, HERE)
from mock_purplepi import gen_map_text  # noqa: E402  白盒：直接验证地图生成

try:
    sys.stdout.reconfigure(errors="replace")  # Windows GBK 控制台：非 GBK 字符替换为 ? 而非崩溃
except Exception:
    pass

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
    # ⓪ 白盒：地图生成（首行 4-token 格式 + 1800² 过 App 就绪阈值 324e4）
    big = gen_map_text(1800, 1800)
    check("gen-map 首行 4 值(range resolution height width)", len(big.splitlines()[0].split()) == 4,
          big.splitlines()[0])
    check("gen-map 1800x1800 过就绪阈值(>324e4B)", len(big) > 3_240_000, f"{len(big)}B")

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

        # ③ 发现 ping（提案 0x06）：回身份 byte1=车号(默认1)
        disc = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        disc.settimeout(3.0)
        disc.bind(("0.0.0.0", 0))
        disc.sendto(struct.pack(">BBBhhh", 6, 0, 0, 0, 0, 0), ("127.0.0.1", UDP_PORT))
        got_disc = False
        try:
            d, _a = disc.recvfrom(64)
            ds, did, _s, _x, _y, _r = struct.unpack(">BBBhhh", d[:9])
            got_disc = (ds == 6 and did == 1)
            print(f"  ..  discovery reply state={ds} car_id={did}")
        except socket.timeout:
            pass
        finally:
            disc.close()
        check("UDP 发现 ping 回身份(0x06, car_id=1)", got_disc)
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
