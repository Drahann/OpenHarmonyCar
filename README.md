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

## 方案 A 平板直连说明

多机协同当前改为方案 A：平板作为中央协调器，直接向每辆车的 `udp2lcm:5001` 发送 9 字节命令并接收心跳。车端不启动 `car-agent`，不使用 LAN TCP `5003`，也不使用本机 UDP `5002` 桥接。子机地图仍由平板触发 `cmd105`，再由紫派 `cmd124` 通过 HTTP/wget 从主机 `:8000` 优先拉取 `zipedMap.txt` 并解压为 `defultMap.txt`；压缩图失败时回退 `defultMap.txt`，地图失败时不继续拉路径文件。

## 多机覆盖新主设计

plan9 后的新主语义是“平板为每台车分配不同的多个选点队列”。车端复用普通目标点导航逐点执行，路线交汇和临时让行继续由 `Navi` 的 `COOP_AVOID` 协同避障处理。旧 `107/108` 矩形分布式覆盖仍保留为兼容路径，`roadFile.txt` 只服务该旧路径。

## 交叉编译产物不进 git

`lcm / glib / wget / openssl / zlib / cp210x.ko` 等交叉编译结果（`.so/.ko/.a`）已被忽略。
**用 Release 附件或共享盘分发**，在 `toolchain/` 写清编译步骤（工具链、命令、依赖），可复现即可。

## 无 App 时的开发

用 [`../tools/mock-app/`](../tools/mock-app/) 发各命令测桥与轮控；`lcm-logger`/`lcm-logplayer` 录放信道。

## 常用脚本

- `.\upload_modules_to_robot.ps1 [-Build] [-Robot <ip>] [-ReconnectDelaySeconds 2]`：上传四个固定产物到机器人 `/data/test`；连接失败会持续重试，直到 `hdc tconn` 成功。
- `.\start_robots_and_logs.ps1 [-NoStart] [-NoViewer] [-ReconnectDelaySeconds 2]`：连接机器人、按需启动 `$RemoteDir/test.sh run` 并持续拉取日志；启动前和后台拉日志时都会自动重连。

## 分布式覆盖路径生成

`108`/`'l'` 保持 `122 -> 123` 顺序：`122` 在 `Navi` 内同步生成 `/data/test/roadFile.txt`，生成前清理旧的 `roadFile.txt/.tmp`，避免失败后误走上一轮路径；`123` 只读取该文件并按 `robot_id` 分段执行。`robot_id=0` 是 master，走前半段；`robot_id=1` 是 sub，走后半段逆序。若 122 打印 `Create full path ... failed`，优先检查框选区域大小和地图/A* 栅格状态，不需要在 App 侧加延时；无效路径文件只结束本轮覆盖，不再退出覆盖线程。

## 紫派显示保活

`test.sh run` 启动 C/C++ 进程前会执行 `power-shell wakeup` 和 `power-shell timeout -o 2147483647`，防止 OpenHarmony 默认 30 秒息屏；`test.sh stop` 会执行 `power-shell timeout -r` 恢复默认超时。

这只能保证已有 screen 不自动锁屏。若要“不插 HDMI 也认为有显示器”，需要在系统镜像启动参数或设备树 `/chosen/bootargs` 追加 `video=HDMI-A-1:1920x1080@60D`，再用 `cat /sys/class/drm/card0-HDMI-A-1/status` 和 `hidumper -s RenderService -a screen` 验证。
