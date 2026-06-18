#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""香橙派热点动态 IP 发现响应。

香橙派改连 WiFi 热点后是 DHCP 动态 IP（会随租约变），不能在 App 里写死。
本脚本让香橙派像车一样"可被 App 广播发现"：监听 UDP :5001，收到 App 的
0x06 广播发现 ping → 回 9 字节 [0]=0x07（区分视觉设备；车是收 0x06 回 0x06）。

平板侧 RobotTransport.discover 收到 [0]=0x07 → Storage.setVisionHost(源IP)，
扫一次设备即自动定位香橙派、取代写死 IP。

与 FastAPI 视觉服务并存、独立运行，**不依赖 NPU/模型**——即使推理服务崩，发现仍可用。
详见 contracts/udp-protocol.md「设备发现」+ contracts/integration-qa.md 2026-06-12。
"""
import socket
import time

DISCOVERY_PORT = 5001
PING_CODE = 0x06      # App→广播 发现请求
RESP_CODE = 0x07      # 香橙派→App 发现响应（视觉设备标识）


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.bind(("0.0.0.0", DISCOVERY_PORT))   # 双网卡都监听（有线 + WiFi）
    print(f"[discovery] listening UDP :{DISCOVERY_PORT} | 0x06 ping -> reply 0x07", flush=True)

    resp = bytes([RESP_CODE] + [0] * 8)      # 9 字节，[0]=0x07 其余 0
    while True:
        try:
            data, addr = sock.recvfrom(64)
            if len(data) >= 1 and data[0] == PING_CODE:
                sock.sendto(resp, addr)
                print(f"[discovery] 0x06 from {addr[0]}:{addr[1]} -> sent 0x07", flush=True)
        except Exception as e:
            print(f"[discovery] err: {e}", flush=True)
            time.sleep(0.5)


if __name__ == "__main__":
    main()
