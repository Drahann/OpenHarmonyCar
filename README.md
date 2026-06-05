# purplepi-control — 紫派主控（C/C++ + Python3）

**负责人：成员 A。** Purple Pi OH（RK3566 / OpenHarmony 5.0）。
职责：烧录系统、LCM 通信、服务器拉起、雷达驱动、轮控、SLAM 导航、全息路径、分布式地图。

## 建议目录

```
purplepi-control/
├── udp2lcm/      # UDP↔LCM 桥：收 App 9 字节指令，翻译成 LCM 命令；1s 心跳；3s 超时急停
├── wheel/        # 轮控：串口驱动板，PATH/wheel_ctrl → 差速；15ms 经 POSE 反馈估算坐标
├── nav/          # 导航/SLAM：HOKUYO_LIDAR/POSE/ROBOT_CONTROL 订阅，CURRENTPOSE/PATH 发布
├── drivers/      # 雷达驱动（LDS-50C-2 建图 / RPLIDAR A2M8 导航）、cp210x 说明
├── scripts/      # HTTP 地图服务(:8000)、启动脚本
└── toolchain/    # 交叉编译说明（不提交大二进制/工具链本体）
```

## 对接的契约

- [`../contracts/udp-protocol.md`](../contracts/udp-protocol.md) — **先按命令码表实现 UDP↔LCM 桥**。
- [`../contracts/lcm/`](../contracts/lcm/) — 信道清单 + 命令号；`.lcm` 字段以你实现为准，改了要升版本号。
- [`../contracts/map-format.md`](../contracts/map-format.md) — 建图产物文件格式 + 坐标系一致性。

## 交叉编译产物不进 git

`lcm / glib / wget / openssl / zlib / cp210x.ko` 等交叉编译结果（`.so/.ko/.a`）已被忽略。
**用 Release 附件或共享盘分发**，在 `toolchain/` 写清编译步骤（工具链、命令、依赖），可复现即可。

## 无 App 时的开发

用 [`../tools/mock-app/`](../tools/mock-app/) 发各命令测桥与轮控；`lcm-logger`/`lcm-logplayer` 录放信道。
