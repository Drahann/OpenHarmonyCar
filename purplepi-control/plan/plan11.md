# plan11：routes 多点队列、协同避障降敏与脚本重连

## 本轮已完成

1. 阅读 `全局须知.md`、`plan/plan9.md`、`plan/plan10.md`，按 plan10 的“下一步工作”继续执行。
2. 正式化多机覆盖 `routes[]` 线协议字段：
   - `readme-upload/contracts/multi-robot-collab.md` 升级到 v0.7。
   - 当前主路径明确为“平板直连双车 UDP + `routes[].points[]` 多点队列”。
   - 平板维护 `carId/robotId/points/cursor/status`，逐点翻译为现有 UDP `cmd3 -> ROBOT_CONTROL 20`，车端不接收整条队列。
   - 旧 `assignments + 107/108 + roadFile.txt` 保留为矩形覆盖兼容路径。
3. 新增机器人“避障暂停/恢复”状态回传：
   - `Navi` 通过 `SERVICE_COMMAND 74` 发布 `robotId/status/event`。
   - `udp2lcm` 订阅 `SERVICE_COMMAND`，把状态写入心跳 byte1，把事件写入 byte2。
   - `udp-protocol.md` 升级到 v0.3，明确 byte1/byte2 的状态枚举。
4. 协同避障降敏与本地兜底：
   - 安全半径下限从偏大的值降为 `0.20m`，机器人额外余量降为 `0.05m`。
   - 只检查本车约 `0.8m` 前瞻路线和对车约 `0.8m` 前瞻路线，不再用“对车当前位置到最终目标”的整条长线误判远距离交叉。
   - 协同请求初发加 4 次重发，共 5 次仍无业务响应时，打印 `TIMEOUT` 并切回本地动态避障。
   - `coop_lcm` 不可用时，不再等待协同通信，直接进入本地动态避障兜底状态。
5. 检查并加固 DWA 动态避障：
   - `bifsave()`、`bifsavelaseronly()` 对预测轨迹点增加 A* 地图边界检查，避免地图边缘越界读栅格。
   - `bifsavelaseronly()` 也尊重静态 A* 地图障碍和地图外区域。
   - DWA 评分分母为 0 时直接回到重规划，不继续除零选速度。
6. 启动/加载脚本状态：
   - `upload_modules_to_robot.ps1` 已支持 `-ReconnectDelaySeconds`，每次 `hdc tconn` 失败都会持续重试，不再连接失败后继续上传。
   - `start_robots_and_logs.ps1` 已支持 `-ReconnectDelaySeconds`，启动前和后台拉日志时都会持续重连。
   - `newtest.sh` 移除旧 `car-agent` 启动逻辑，和当前“平板直连、不启 agent”方案一致。
7. 文档同步：
   - 根目录 `README.md` 与 `readme-upload/purplepi-control/README.md` 已同步心跳状态、`SERVICE_COMMAND 74`、协同避障降敏、DWA 边界保护和脚本用法。
   - `readme-upload/contracts/README.md`、`dual-car-purplepi-handoff.md`、`integration-qa.md` 同步当前结论。
8. 同步到上传仓库：
   - 根目录代码已按 `code-upload` 跟踪文件清单同步到 `code-upload`，包含本轮修改和此前根目录已有的压缩地图、脚本、建图相关改动。

## 验证结果

1. `Navi` 增量编译通过：
   - `ninja -C .\Navi\build`
   - 已重新链接 `navigation`。
2. `NewWheelCtrl` 增量编译通过：
   - `ninja -C .\NewWheelCtrl\build`
   - 已重新链接 `NewWheelCtrl\bin\udp2lcm`。
3. `Lidar` 增量编译通过：
   - `ninja -C .\Lidar\build`
   - 输出为 `no work to do`。
4. 根目录 `README.md`、`code-upload/README.md`、`readme-upload/purplepi-control/README.md` 内容已保持一致。

## 新脚本用法

### 上传四个模块

```powershell
.\upload_modules_to_robot.ps1 -Build -ReconnectDelaySeconds 2
```

说明：

- 默认读取 `config.txt` 中的机器人 IP。
- 每个 IP 会自动补 `:8710`。
- `hdc tconn` 失败时会每 2 秒重试，直到连接成功后才执行远端建目录、上传和 chmod。
- 只上传某一台车：

```powershell
.\upload_modules_to_robot.ps1 -Robot 192.168.107.173 -ReconnectDelaySeconds 2
```

### 启动机器人并拉日志

```powershell
.\start_robots_and_logs.ps1 -ReconnectDelaySeconds 2
```

说明：

- 默认读取 `config.txt` 中的多台机器人。
- 未加 `-NoStart` 时，会先重连成功再执行 `/data/test/test.sh run`。
- 后台日志接收任务如果断连，会持续 `hdc tconn` 重试，连接恢复后继续拉 `navi.log`、`serial.log`、`udp2lcm.log`。
- 只拉日志不启动：

```powershell
.\start_robots_and_logs.ps1 -NoStart -ReconnectDelaySeconds 2
```

- 不打开日志查看器，只后台拉日志：

```powershell
.\start_robots_and_logs.ps1 -NoViewer -ReconnectDelaySeconds 2
```

## 下一步需要审批 / 实机验证

1. 实机验证平板多点队列主路径：master 建图后，sub 执行 `105 -> 5`，然后两车分别按 `routes[].points[]` 逐点接收 `cmd3`。
2. 实机观察心跳 byte1/byte2：
   - 正常应为 `0/0`。
   - 协同诊断时应为 `1/1` 或 `1/2`。
   - 为对车暂停时应为 `2/1`。
   - 对车被本车暂停时应为 `3/1`。
   - 通信超时或无 LCM 兜底时应为 `4/3` 或 `4/5`。
3. 若两车间隔很远仍触发协同避障，继续审批进一步降敏：把前瞻距离从 `0.8m` 降到 `0.5m`，或要求对车当前位置距离本车路线也在近场范围内才进入停机协商。
4. 若 `COOP_AVOID` 多播仍频繁超时，再审批“平板中转协同避障消息”；本轮按 plan10 要求未实现中转。
5. 是否把 `config.txt` 固定为双车实机 IP 推到代码分支，或改为示例文件避免不同场地反复改 tracked 配置。
