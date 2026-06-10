# purplepi-control 接口功能与对接问题说明

本文档记录 `purplepi-control` 部分的主要接口功能、内部/外部消息格式，并回答仓库其它说明文档中要求紫派侧确认的问题。本文依据根目录 `目录结构.md`、当前 `Lidar`/`Navi`/`NewWheelCtrl` 代码和主目录脚本整理。

## 一、模块组成

`purplepi-control` 由三个核心模块组成：

| 模块 | 主要目录 | 功能 |
| --- | --- | --- |
| UDP/轮控桥接 | `NewWheelCtrl/udp2lcm` | 监听 App UDP 5001 端口，将 9 字节命令转换成 LCM；维护心跳；拉起 `/data/test` 下的 HTTP 地图服务。 |
| 轮控串口 | `NewWheelCtrl/serial` | 订阅 `wheel_ctrl` 与 `PATH`，换算为轮子串口协议，通过 `/dev/ttyUSB1` 控制底盘。 |
| 雷达驱动 | `Lidar` | 通过 `/dev/ttyUSB0` 读取 SLAMTEC/RPLIDAR 数据，封装为 `laser_t`，发布到 `HOKUYO_LIDAR`。 |
| SLAM/导航/覆盖规划 | `Navi` | 订阅雷达、里程计、控制命令；完成建图、加载地图、定位、目标点导航、全路径/多机矩形覆盖。 |

## 二、对外接口功能

### 1. App 到紫派：UDP 5001

`NewWheelCtrl/udp2lcm/udp.c` 在本机 `5001` 端口监听 UDP。收到首包后记录客户端 IP，启动心跳发送线程，并向 `ROBOT_CONTROL` 发布 `commandid=7` 初始化导航配置。之后每次收到包调用 `parseCmd()`。

安全约定：

- 常规控制包按 9 字节解析；`0x06` 发现探测允许至少 1 字节。
- 收到 `0x06` 发现探测时立即回 9 字节发现响应，不建立心跳会话，也不触发导航命令。
- 3 秒无新 UDP 包时，向 `wheel_ctrl` 发布停止命令，底盘急停。
- 紫派每 500ms 向最近一次客户端 IP 回传 9 字节心跳。

### 2. 紫派到 App：HTTP 地图服务

`udp2lcm` 启动时 fork 子进程并执行：

```text
cd /data/test
python3 -u -m http.server 8000 --bind 0.0.0.0 --directory /data/test
```

因此 HTTP 根目录是 `/data/test`，App 拉图 URL 不带 `/data/test` 前缀：

```text
http://<紫派IP>:8000/defultMap.txt
http://<紫派IP>:8000/zipedMap.txt
```

默认地图文件名以代码为准：`defultMap.txt`。保存地图时会先保存未优化文件 `unprobdefultMap.txt`，随后优化生成 `defultMap.txt`、兼容显示文件 `defultMap.txt.txt` 和压缩地图 `zipedMap.txt`。当前 `Navi` 启动时会清理旧地图、路径和覆盖调试产物；建图命令被 `Navi` 接受并进入新一轮建图时，也会再次执行同样清理，不生成 `.bak` 备份。

### 3. 地图与输出文件说明

机器人运行时各模块都放在 `/data/test`，`test.sh run` 会先 `cd /data/test` 再启动 `lidar_driver`、`navigation`、`serial`、`udp2lcm`。因此代码里没有写绝对路径的地图和调试输出，也默认落在 `/data/test`。

地图和路径文件写入规则：

- App 或子机只读取正式文件：`/data/test/defultMap.txt`、`/data/test/roadFile.txt`。
- `Navi` 写主地图、兼容地图和覆盖路径时先写同目录 `.tmp`，写完且文件有效后再替换正式文件；算法运行中不会直接半写正式文件。
- 子机执行 `105`/`'i'` 拉主机文件时也先下载到 `.tmp`，确认非空后再替换本机正式文件；下载失败时保留原正式文件。
- `navigation` 启动和建图命令进入新一轮建图前，都会删除旧的 `defultMap.txt`、`defultMap.txt.txt`、`zipedMap.txt`、`unprobdefultMap.txt`、`roadFile.txt` 和覆盖调试图，避免旧数据影响首次普通建图或重新建图。

| 文件名 | 位置/访问方式 | 用途 |
| --- | --- | --- |
| `defultMap.txt` | `/data/test/defultMap.txt`；HTTP 为 `http://<紫派IP>:8000/defultMap.txt` | 主地图文件。保存地图后由优化流程生成；加载地图、App 拉图、子机从主机拉图时默认使用它。写入过程使用 `defultMap.txt.tmp`。 |
| `unprobdefultMap.txt` | `/data/test/unprobdefultMap.txt` | 未优化地图/中间地图。保存地图命令执行时先写出该文件，再优化生成 `defultMap.txt`。写入过程使用 `unprobdefultMap.txt.tmp`。 |
| `defultMap.txt.txt` | `/data/test/defultMap.txt.txt` | 兼容显示文件，由 `defultMap.txt` 同源生成；首行同为 7 字段，栅格为密排 `1/0`。App 新实现优先读取 `defultMap.txt`。 |
| `zipedMap.txt` | `/data/test/zipedMap.txt`；HTTP 为 `http://<紫派IP>:8000/zipedMap.txt` | 压缩地图文件。保存 `defultMap.txt` 或 `unprobdefultMap.txt` 后生成，障碍位压缩为 64 位整数，适合 App 快速拉图；解压后可恢复为 `defultMap.txt` 的 `-1/0` 文本格式。 |
| `roadFile.txt` | `/data/test/roadFile.txt`；子机可通过 `http://<主机IP>:8000/roadFile.txt` 获取 | 覆盖路径点文件，每行格式为 `x,y`。双车协同覆盖时用于让另一台车复用或跟踪主机生成的覆盖路线。写入和拉取过程使用 `roadFile.txt.tmp`。 |
| `tmpcoverageMap.txt` | `/data/test/tmpcoverageMap.txt` | 覆盖算法初始阶段的临时栅格调试输出，用于查看覆盖图生成前期状态。 |
| `initCoverageMap.txt` | `/data/test/initCoverageMap.txt` | 覆盖算法初始化后的栅格输出，用于检查初始覆盖区域和栅格化结果。 |
| `midMap.txt` | `/data/test/midMap.txt` | 覆盖算法中间过程输出，用于检查坐标变换、区域划分或中间覆盖结果。 |
| `coverageMap.txt` | `/data/test/coverageMap.txt` | 覆盖算法最终或阶段性覆盖栅格输出，用于调试全路径覆盖效果；它不替代主地图 `defultMap.txt`。 |

地图文本格式以 `MapServer::saveMap()` / `saveProbMap()` 为准：

```text
range resolution height width metersPerPixel x0 y0
```

- `height/width` 固定取首行第 3、4 个字段。
- `metersPerPixel` 是栅格分辨率；`x0/y0` 是地图最小角世界坐标，单位为米，可为负。
- `defultMap.txt` 数据区为空格分隔，`-1` 表示障碍，`0` 表示非障碍。
- `defultMap.txt.txt` 数据区为密排 `1/0`，只用于旧显示兼容。

压缩地图 `zipedMap.txt` 格式如下：

```text
ZMAP1
range resolution height width metersPerPixel x0 y0
rowBitCount wordCount word0 word1 ...
```

- 每一行地图对应一行压缩数据，`rowBitCount` 通常等于 `width`。
- 地图栅格先转成二进制序列：障碍为 `1`，非障碍为 `0`；`defultMap.txt` 中的 `-1` 等价于压缩位 `1`。
- 每 64 个 bit 打包为一个无符号 64 位整数，最后不足 64 bit 的部分左对齐并补 0。
- `MapServer::loadZipedMap(zipedMap.txt, defultMap.txt)` 可将压缩文件解压回普通地图文本。

### 4. 子机从主机拉取地图

分布式协同时，App/agent 发送 UDP 命令 `'i'`/105，主机 IP 四段放在 byte `[1] [2] [4] [6]`。紫派转换为 LCM `commandid=124` 后，`Navi` 执行：

```text
wget http://<主机IP>:8000/defultMap.txt -O /data/test/defultMap.txt.tmp
wget http://<主机IP>:8000/roadFile.txt -O /data/test/roadFile.txt.tmp
```

下载成功且文件非空后，`Navi` 再把 `.tmp` 替换成正式文件。正式文件始终在 `/data/test` 下，HTTP URL 不包含 `/data/test` 前缀。这说明现有代码默认采用“方案 B：车间 HTTP 拉图”；软总线方案 A 需要新增 agent 写文件能力后再实现。

### 5. 其它模块对接注意事项

其它模块与 `purplepi-control` 交互时，以本节和“外部 UDP 9 字节格式”为准，统一使用当前文件名、URL 和地图头解析规则。

| 对接方 | 注意事项 |
| --- | --- |
| App 地图显示 | 拉取 `http://<紫派IP>:8000/defultMap.txt`，不要拼成 `/data/test/defultMap.txt` URL；解析首行 7 字段，`height/width` 固定取第 3、4 字段。 |
| App 命令发送 | 常规控制包保持 9 字节；`0x06` 可以只发 1 字节做发现探测；`105` 主机 IP 仍放在 byte `[1] [2] [4] [6]`。 |
| App 多机覆盖 | 子机先发 `105`/`'i'` 拉主机 `defultMap.txt` 与 `roadFile.txt`，再发 `0x05`、`106`、`108` 或 `123` 相关命令；`roadFile.txt` 是机器人内部覆盖路径文件，不作为普通地图显示。 |
| ArkTS agent | 建议只做软总线状态同步、本机 UDP 5001 桥接、可选地图文件落地和心跳位姿回写；不要绕过 `udp2lcm` 直接操作 LCM。 |
| mock/契约文档 | `MAP_FILE_NAME` 使用 `defultMap.txt`；地图数据以 `defultMap.txt` 的空格分隔 `-1/0` 为准；`defultMap.txt.txt` 仅作兼容显示文件。 |
| 机器人端脚本 | 所有运行产物和生成文件放在 `/data/test`；HTTP 服务根目录就是 `/data/test`；建图开始后旧地图会被清理，保存完成前拉图可能暂时 404。 |

## 三、内部 LCM 消息格式

### 双车协同避障说明

本次更新在 `Navi` 内新增独立的双车协同避障通道，不修改 `NewWheelCtrl/udp2lcm`。该通道用于两车在协同全路径覆盖时交换当前位姿、当前导航点、停机请求和恢复请求。

| 项 | 内容 |
| --- | --- |
| LCM URI | `udpm://239.255.76.67:7668?ttl=1` |
| 频道 | `COOP_AVOID` |
| 消息类型 | `robot_control_t` |
| 目标过滤 | `robotid` 必须等于本机 `robotId`，且 `iparams[0]` 不能等于本机 `robotId` |

命令号使用负数，避免和原有 `ROBOT_CONTROL` 的 `0..127` 命令冲突：

| `commandid` | 含义 | 主要参数 |
| --- | --- | --- |
| `-40` | 坐标请求 | `iparams[0]=源 robotId`，`iparams[1]=请求序号`，`iparams[2]=是否有当前目标`，`dparams[0..2]=请求方当前位姿` |
| `-39` | 坐标响应 | `dparams[0..2]=响应方当前位姿`；若有目标，`dparams[3..5]=当前导航目标` |
| `-38` | 停机请求 | 请求对方保存当前未到达目标并发布零速度停车 |
| `-37` | 停机确认 | 对方已进入停机等待 |
| `-36` | 恢复请求 | 请求被暂停方恢复保存的导航目标 |
| `-35` | 恢复确认 | 被暂停方已收到恢复请求 |

触发入口：

- A* 规划失败、连续无路径或 DWA 判定不可达时，`NaviInterface` 会触发协同诊断。
- 图匹配跳变达到现有阈值时，也会触发协同诊断。
- 若对方位置或预测路径走廊与本车当前路线距离小于安全半径，则判断为对方车辆阻挡；图匹配跳变场景还要求近期激光/障碍点在对方走廊附近，降低外部障碍误判。

停机与恢复规则：

- 停机时保存当前 `m_waypoints.front()`，清空导航队列并发布 `PATH v=0,w=0`。
- 恢复时重新下发保存的目标点。
- 两车同时互相请求停机时，`robotId=1` 继续导航，`robotId=0` 停机等待。
- 协同覆盖路径执行循环已改为可暂停的 `while` 下标推进；停机等待时不会跳过当前覆盖转折点。

### 1. LCM 信道

| 信道 | 类型 | 发布者 | 订阅者 | 含义 |
| --- | --- | --- | --- | --- |
| `HOKUYO_LIDAR` | `laser_t` | `Lidar/lidar_driver.cpp` | `Navi/main.cpp` | 一圈雷达扫描数据。 |
| `POSE` | `pose_t` | `NewWheelCtrl/serial/serial.c` | `Navi/main.cpp` | 轮控侧里程计位姿，约 15ms 发布一次。 |
| `CURRENTPOSE` | `pose_t` | `Navi/main.cpp` | `udp2lcm`、`serial` | 导航融合后的当前位姿，也是 App 心跳来源。 |
| `ROBOT_CONTROL` | `robot_control_t` | `udp2lcm`、`Navi` | `Navi`、上层服务 | 通用控制命令与状态回包。 |
| `PATH` | `path_t` | `Navi/main.cpp` | `NewWheelCtrl/serial/serial.c` | 导航输出给轮控的线速度/角速度。 |
| `wheel_ctrl` | `path_ctrl_t` | `udp2lcm` | `serial` | App 手动遥控轮控命令。 |
| `MAPFILE` | `robot_control_t` | `Navi/main.cpp` | 上层服务 | 地图文件分片。 |
| `SERVICE_COMMAND` | `robot_control_t` | `Navi/main.cpp` | 上层服务 | 建图、保存地图、路径分片等服务状态。 |
| `ROBOT_LOG` | `robot_control_t` | `Navi/main.cpp` | 上层服务 | 日志/状态码。 |
| `ALIVE` | `robot_control_t` | `Navi/main.cpp` | 上层服务 | 导航进程 1s 心跳，`commandid=99`。 |

### 2. `robot_control_t`

通用控制消息。`commandid` 类型是 `int8_t`，有效范围为 `[-128, 127]`，新增命令不能超过此范围。

| 字段 | 类型 | 含义 |
| --- | --- | --- |
| `utime` | `int64_t` | 时间戳，部分代码填 0。 |
| `commandid` | `int8_t` | 命令号。 |
| `robotid` | `int8_t` | 目标机器人 ID，多数代码填 0。 |
| `ndparams` / `dparams` | `int8_t` / `double*` | 浮点参数数量与数组。 |
| `niparams` / `iparams` | `int8_t` / `int8_t*` | 整型参数数量与数组。 |
| `nsparams` / `sparams` | `int8_t` / `char**` | 字符串参数数量与数组。 |
| `nbparams` / `bparams` | `int64_t` / `uint8_t*` | 原始字节数量与数组。 |

常用命令：

| `commandid` | 含义 |
| --- | --- |
| `6` | 初始化配置完成回包，由 `7` 触发后发布到 `ROBOT_CONTROL`。 |
| `7` | 初始化导航配置。 |
| `10` | 加载地图并初始化定位；无文件名时加载 `/data/test/defultMap.txt`。 |
| `11` | `ROBOT_LOG` 日志状态码，`bparams[0]` 为状态值。 |
| `13` | 加载地图结果/错误回包，当前代码固定在加载流程后发布。 |
| `20` | 设置导航目标点，`dparams[0..2]=x,y,theta`。 |
| `25` | 无路/规划异常回包，`bparams[0]` 为异常状态。 |
| `23` | 删除/取消当前目标点。 |
| `30` | 开始建图，`dparams[0]` 为分辨率，默认 `0.05m`；`iparams[1]!=0` 或 `bparams[0]!=0` 表示强制新建。普通建图也可作为首次建图使用，因为 `navigation` 启动时已清理旧地图文件。 |
| `32` | 保存地图；无文件名时保存为 `defultMap.txt`。 |
| `51` | `SERVICE_COMMAND` 建图启动结果，`iparams[0]` 为是否成功启动。 |
| `52` | `SERVICE_COMMAND` 保存地图阶段状态。 |
| `73` | `SERVICE_COMMAND` 路径/服务分片，`iparams[0]` 为分片序号，`iparams[1]` 为类型。 |
| `99` | 导航进程存活心跳，同时发布到 `ALIVE` 和 `ROBOT_LOG`。 |
| `101` | 保存地图完成回包，发布到 `ROBOT_CONTROL`。 |
| `102` | 地图名列表回包，`sparams` 为 `/data/test` 下 txt 文件名列表。 |
| `104` | 地图文件分片，发布到 `MAPFILE`。 |
| `105` | 地图更新完成回包。 |
| `107` | 手动地图更新开始回包。 |
| `124` | 子机按主机 IP 拉取地图和路径文件。 |
| `125` | 单机全路径规划选择房间顶点。 |
| `126` | 取消全路径规划。 |
| `127` | 启动全路径规划。 |
| `122` | 按两个对角点生成分布式矩形覆盖路径。 |
| `123` | 读取路径文件并按 `robot_id` 执行分布式覆盖跟踪。 |

### 3. `pose_t`

| 字段 | 含义 |
| --- | --- |
| `pos[0]` | x，单位 m。 |
| `pos[1]` | y，单位 m。 |
| `pos[2]` | theta，单位 rad。 |
| `vel[0]` | 当前定位状态标记。 |
| `orientation[0..2]` | 定位 rate flag。 |

`CURRENTPOSE` 发布后，`udp2lcm` 将 `x/y` 乘以 20 转为 UDP 整数坐标，将 `theta` 从弧度转为度并归一到 `[-180, 180]`。

### 4. `path_t`

`PATH` 中轮控只读取第一行：

| 字段 | 含义 |
| --- | --- |
| `xyr[0][0]` | 线速度 `v`，单位 m/s。 |
| `xyr[0][1]` | 角速度 `w`，单位 rad/s。 |
| `xyr[0][2]` | 当前未使用。 |

轮控换算：

```text
left_percent  = (v - w * RADIUS) / FULLSPEED * 100
right_percent = (v + w * RADIUS) / FULLSPEED * 100
```

其中 `RADIUS=0.13m`，`FULLSPEED=0.60m/s`。当 `|v|<=0.01` 且 `|w|<=0.01` 时直接停车；`w` 限制在 `[-0.8, 0.8]`，左右轮百分比限制在 `[-60, 60]`。

### 5. `path_ctrl_t`

| 字段 | 类型 | 含义 |
| --- | --- | --- |
| `cmd` | `int8_t` | 手动轮控动作。 |
| `speed` | `int8_t` | 前进速度百分比。 |

`cmd` 含义：

| `cmd` | 含义 |
| --- | --- |
| `0` | 停止。 |
| `1` | 双轮前进，速度取 `speed`。 |
| `2` | 左转，使用 `DEFAULT_SPEED`。 |
| `3` | 右转，使用 `DEFAULT_SPEED`。 |
| `4` | 停止并设置 `stopFlag=true`，临时屏蔽 `PATH`。 |
| `5` | 停止并设置 `stopFlag=false`，恢复 `PATH` 控制。 |

### 6. `laser_t`

| 字段 | 含义 |
| --- | --- |
| `nranges` / `ranges` | 距离数组长度与数组，当前写入毫米距离。 |
| `nintensities` / `intensities` | 当前代码写入角度值，不是原始强度。 |
| `rad0` | 第一束角度，当前为角度制。 |
| `radstep` | 角度步长，当前为角度制。 |

## 四、外部 UDP 9 字节格式

除 `0x06` 发现探测外，App → 紫派常规控制命令按 9 字节固定包解析，多字节整数为大端。

| byte[0] | 参数 | 紫派行为 |
| --- | --- | --- |
| `0x00` | 无 | 心跳/兼容建图。首包只建连并发布 `7`；进入命令循环后，若 `/data/test/defultMap.txt` 不存在且本轮尚未请求建图，则发布 `ROBOT_CONTROL 30`，分辨率 0.05m；否则只作为心跳处理。 |
| `0x01` | `byte[1]=cmd`, `byte[2]=speed` | 发布 `wheel_ctrl path_ctrl_t{cmd,speed}`。 |
| `0x02` | 无 | 停轮控，保存地图 `32`，再加载地图 `10`，初始位姿取当前心跳。 |
| `0x03` | `byte[3..4]=x`, `byte[5..6]=y` | 发布 `20`，目标为 `x/20,y/20,theta=0`。 |
| `0x04` | 无 | 发布 `wheel_ctrl cmd=4`，发布 `23` 取消导航，2 秒后发布 `wheel_ctrl cmd=5`。 |
| `0x05` | 无 | 停车并加载地图 `10`，当前代码把初始位姿设为 `0,0,0`。 |
| `0x06` | 可仅 1 字节 | 发现探测。紫派立即回 9 字节响应，`[0]=0x06`、`[1..2]=0`、`[3..8]` 复用当前心跳位姿字段，不触发 LCM 命令。 |
| `'f'`/102 | `byte[1]=algNum` | 发布 `127`，启动全路径规划。 |
| `'g'`/103 | 无 | 发布 `126`，取消全路径规划。 |
| `'h'`/104 | `byte[1]=vertexId`, `byte[3..6]=x,y` | 发布 `125`，选择房间顶点。 |
| `'i'`/105 | `byte[1] [2] [4] [6]` | 发布 `124`，四字节为主机 IP 四段。 |
| `'j'`/106 | `byte[3..6]=x,y` | 加载地图 `10` 后发布目标点 `20`。 |
| `'k'`/107 | `byte[3..6]=x1,y1` | 暂存分布式覆盖矩形对角点 1。 |
| `'l'`/108 | `byte[1]=robot_id`, `byte[3..6]=x2,y2` | 暂存对角点 2；从机发布 `122` 生成矩形覆盖路径；所有车发布 `123` 执行跟踪。 |
| `'m'`/109 | 无 | 强制重新建图。发布 `ROBOT_CONTROL 30`，`dparams[0]=0.05`，`iparams[1]=1` 表示强制新建；`Navi` 接受建图后会清理旧地图、路径和覆盖调试产物。 |

紫派 → App 心跳同为 9 字节：

| 字节 | 含义 |
| --- | --- |
| `[0]` | 固定 `0x03`，表示当前位姿。 |
| `[1..2]` | 保留，当前为 0。 |
| `[3..4]` | `x * 20`，大端 `int16`。 |
| `[5..6]` | `y * 20`，大端 `int16`。 |
| `[7..8]` | `theta_deg`，大端 `int16`，范围 `[-180,180]`。 |

发现响应也是 9 字节：`[0]=0x06`，`[1..2]=0`，`[3..8]` 与当前心跳的坐标和角度字段相同。

## 五、运行与部署命令

机器人端工作目录固定为 `/data/test`。主目录脚本和机器人端 shell 脚本围绕该目录组织：

| 命令/脚本 | 用途 |
| --- | --- |
| `./test.sh run` | 在机器人端启动屏幕保活应用，检查 `/dev/ttyUSB*` 数量为 2，然后启动 `lidar_driver`、`navigation`、`serial`、`udp2lcm`。 |
| `./test.sh stop` | 向 `lidar_driver`、`navigation`、`serial`、`udp2lcm`、`python` 相关进程发送 `SIGINT`。 |
| `./test.sh clean` | 清理 `/data/test` 下日志和历史 `de*` 文件。 |
| `./newtest.sh newrun` | 只启动 `lidar_driver`、`serial`、`udp2lcm`，不启动 `navigation`，用于部分调试场景。 |
| `.\upload_modules_to_robot.ps1 [-Build] [-Robot <ip>]` | 从本机上传四个产物到机器人 `/data/test`：`lidar_driver`、`navigation`、`serial`、`udp2lcm`；未指定 `-Robot` 时读取 `config.txt`。 |

## 六、轮控串口协议

轮控串口固定为 `/dev/ttyUSB1`，波特率 `115200`，8N1，无流控。`wheelSend(a,a_v,b,b_v)` 发送 7 字节：

| 字节 | 含义 |
| --- | --- |
| `[0]` | 固定 `0x53`。 |
| `[1]` | 固定长度 `0x05`。 |
| `[2]` | 左轮方向：`0x00=停`，`0x01=正转`，`0x02=反转`。 |
| `[3]` | 左轮速度百分比。 |
| `[4]` | 右轮方向：`0x00=停`，`0x01=正转`，`0x02=反转`。 |
| `[5]` | 右轮速度百分比。 |
| `[6]` | 前 6 字节逐字节异或校验。 |

## 七、对其它说明文档待确认项的回答

### 1. 地图文件名

确认：当前紫派默认地图文件为 `defultMap.txt`，HTTP URL 为：

```text
http://<紫派IP>:8000/defultMap.txt
```

`defultMap.txt.txt` 是兼容显示输出文件，不是 App 新实现的首选地图。当前 `navigation` 启动时会删除它；一旦建图命令被 `Navi` 接受并进入新建图流程，也会和旧 `defultMap.txt`、`zipedMap.txt`、`unprobdefultMap.txt`、`roadFile.txt` 一起清理。

### 2. 坐标单位、原点、0 度方向

确认：

- 内部导航坐标单位为米，`pose_t.pos[0..1]` 是米，`pose_t.pos[2]` 是弧度。
- 对外 UDP/App 坐标单位为 `1/20m = 5cm`，即 `x/y` 整数值与 0.05m 栅格 1:1。
- 对外朝向为度，范围 `[-180,180]`。
- 地图原点与导航坐标原点以建图/定位初始化位姿为准。命令 `0x05` 加载地图时明确传入 `0,0,0`，适合“子机从 master 同一物理起点、同一朝向出发”的约定。
- 0 度方向对应导航内部 `theta=0` 的方向；轮控里 `x += v*cos(theta)*dt`、`y += v*sin(theta)*dt`，因此在数学坐标意义上，`theta=0` 时沿 +X 方向前进，正角度按代码更新到 +Y 方向旋转。

### 3. 子机加载图后是否归零到原点

确认：使用 UDP 命令 `0x05` 加载地图时，`udp2lcm` 发布 `ROBOT_CONTROL 10`，并将 `dparams[4..6]` 设为 `0,0,0`。因此该路径会把加载地图后的初始位姿归零到地图原点。

注意：命令 `0x02`、`'j'`、`'l'` 中的加载地图逻辑会使用当前心跳位姿作为初始位姿，不是强制归零。多机协同若要求子机从 master 原点出发，应优先使用 `0x05` 或在 agent 中确保发给 `10` 的初始位姿为 `0,0,0`。

### 4. 多机地图传输选 A 还是 B

当前代码已经实现方案 B：子机按主机 IP 通过 HTTP/wget 拉取 `defultMap.txt` 和 `roadFile.txt`。方案 A（软总线传 `map.text` 后由 agent 落地）当前没有在 `purplepi-control` C/C++ 栈中实现，需要新增紫派 ArkTS agent 的文件写入与本机 UDP 桥接。

### 5. 紫派能否常驻 ArkTS agent

当前 `purplepi-control` 仓库没有 ArkTS agent 代码，无法从现有代码证明 HAP 常驻能力。但从接口边界看，agent 只需要：

- 加入软总线并维护 `FleetMission`。
- 通过 localhost UDP 5001 调用现有 9 字节协议。
- 可选地把软总线地图文本写入 `/data/test/defultMap.txt`。
- 从紫派心跳回包读取位姿并回写共享对象。

也就是说，C/C++ 机器人栈不需要改造即可被 agent 桥接；常驻能力属于 OpenHarmony/HAP 部署问题，不属于当前 C/C++ 栈能力。

### 6. 子区域是否只能矩形、能否多块/不规则

当前分布式覆盖接口只支持一个轴对齐矩形区域，由两个对角点确定。`CreateFullPath()` 明确按 `x1,y1,x2,y2` 计算矩形，并要求 `x1 != x2` 且 `y1 != y2`。

因此：

- 当前支持：单块矩形。
- 当前不支持：多块区域、不规则多边形。
- 下发时序：先发 `'k'`/107 暂存对角点 1，再发 `'l'`/108 携带对角点 2 与 `robot_id`，否则从机无法生成 `122` 覆盖路径。

### 7. App 侧地图首行格式与栅格格式

地图文件首行实际为 7 个字段：

```text
range resolution height width metersPerPixel x0 y0
```

`Navi/main.cpp` 的地图分片发送逻辑读取首行前 4 个字段计算进度，完整地图文件仍固定保留 7 个字段。App 解析必须按位置取第 3、4 个字段作为 `height/width`，第 5 个字段为 `metersPerPixel`，第 6、7 个字段为地图最小角世界坐标 `x0/y0`。

地图栅格内容以文件名区分：`defultMap.txt` 是空格分隔 `-1/0`，`defultMap.txt.txt` 是兼容密排 `1/0`。App 新实现优先拉取并解析 `defultMap.txt`。

### 8. 命令 5 / 106 / 108 中加载地图的来源

- 命令 `5`：加载本机 `/data/test/defultMap.txt`，初始位姿为 `0,0,0`。
- 命令 `106`/`'j'`：先加载本机地图，再接收主机下发的目标点。
- 命令 `108`/`'l'`：主机路径中会加载本机地图；从机路径在已有对角点时加载本机地图、生成矩形覆盖路径，再执行分布式跟踪。
- 子机地图来自命令 `105`/`'i'` 触发的 HTTP 拉图，文件落到 `/data/test/defultMap.txt`。

### 9. 其它模块对接结论汇总

- `MAP_FILE_NAME` 使用 `defultMap.txt`。紫派 HTTP 根目录已经是 `/data/test`，所以 App 拉图 URL 为 `http://<紫派IP>:8000/defultMap.txt`。
- 子机如果还没有主机地图，应先发 `105`/`'i'`，让子机从主机拉取 `/defultMap.txt` 和 `/roadFile.txt`，再执行 `5`、`106`、`108` 或 `123` 相关导航/覆盖命令。
- `roadFile.txt` 是覆盖路径点文件，每行 `x,y`。它不是地图文件，但在双车协同覆盖中会和主地图一起被子机从主机拉取。
- 地图首行必须按 `range resolution height width metersPerPixel x0 y0` 解析，`height/width` 固定取第 3、4 个字段。
- `defultMap.txt` 栅格为空格分隔 `-1/0`；`defultMap.txt.txt` 是兼容密排 `1/0`，不作为新 App 默认地图。
- 当前 C/C++ 栈已实现的是车间 HTTP/wget 拉图。若采用软总线同步地图文本，需要 ArkTS agent 将文件安全落到 `/data/test/defultMap.txt`，并继续通过本机 UDP 触发加载地图。
- ArkTS agent 建议只做软总线状态同步、UDP 5001 命令桥接、可选地图文件落地和心跳位姿回写，不建议直接绕过 `udp2lcm` 操作 LCM。
- 子区域当前只支持一个矩形，由 `107`/`'k'` 的对角点 1 与 `108`/`'l'` 的对角点 2 表示；`108` 的 byte1 携带 `robot_id`，当前语义为 `0=主机/master`、`1=从机/sub`。
- 协同避障不新增 App 侧 UDP 命令，也不需要改 `udp2lcm`。两车靠近后的坐标请求、停机、恢复都在 `Navi` 新增的 `COOP_AVOID` LCM 通道内完成。

## 八、维护建议

- 新增 UDP 命令时，必须记录 9 字节布局、字节序、缩放比例和对应 LCM 命令。
- 新增 `ROBOT_CONTROL` 命令时，必须确认 `commandid` 不超过 `int8_t` 范围。
- 涉及停车/取消/任务结束的逻辑，应保证最终产生 `PATH v=0,w=0` 或 `wheel_ctrl cmd=0/4/5`。
- 修改地图文件名或压缩格式时，要同时修改 `Navi` 保存/加载、`udp2lcm` HTTP 暴露说明、App `MAP_FILE_NAME`、mock 工具和契约文档。
- 修改机器人端启动流程时，要同步检查 `test.sh`、`newtest.sh`、`upload_modules_to_robot.ps1` 和 `/data/test` 下四个固定产物名。
