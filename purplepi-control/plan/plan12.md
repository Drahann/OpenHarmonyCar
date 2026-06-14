# plan12：平板双区域覆盖纠偏与代码落地

## 纠偏结论

本轮修正 plan11 初版的错误理解：多机覆盖不是“平板生成 `routes[].points[]` 路径点队列”。当前正确方案是：

1. 平板给两辆车分别下发两个矩形区域。
2. 每辆车收到自己的 `107/108` 区域后，由机器人端 `122` 本地生成覆盖路径 `roadFile.txt`。
3. `123` 读取并完整执行本车生成的整条覆盖路径，不再按 `robot_id` 拆成前半/后半。
4. 平板不生成、不读取、不下发覆盖路径点。

## 本轮代码修改

1. `Navi/NaviInterface.cpp`
   - `subGetMapFromMain()` 现在只拉主机地图：优先 `zipedMap.txt`，失败回退 `defultMap.txt`。
   - 删除子机拉取主机 `roadFile.txt` 的逻辑，避免子机自己的区域路径被主机路径覆盖。
   - 覆盖执行模式 `algNum=2` 不再按 `robot_id` 切分 `roadFile.txt`。
   - `robot_id` 只校验 `0/1` 并作为本车身份保留；执行路径改为完整 `fullPath`。
2. `Navi/main.cpp`
   - 更新 `123` 注释为“读取本车生成的路径文件并执行本车区域全覆盖”。
3. `code-upload/Navi/...`
   - 同步上述代码修改到上传仓库。

## 文档同步

1. 根目录 `README.md`、`code-upload/README.md`、`readme-upload/purplepi-control/README.md` 已统一改为“平板下发双区域，机器人本地全覆盖”。
2. `readme-upload/contracts/multi-robot-collab.md` 升级到 v0.8：
   - `assignments[]` 区域矩形为当前主字段。
   - v0.7 的 `routes[].points[]` 主路径说明作废。
   - 明确 `107/108 -> 122 -> 123` 是当前多机区域覆盖主流程。
3. `readme-upload/contracts/integration-qa.md`、`udp-protocol.md`、`map-format.md`、`dual-car-purplepi-handoff.md`、`contracts/README.md` 已同步纠偏。
4. `plan/plan11.md` 和 `readme-upload/purplepi-control/plan/plan11.md` 已加纠偏说明，避免继续按错误的多点队列口径执行。

## 验证结果

1. `Navi` 编译通过：

```powershell
ninja -C .\Navi\build
```

结果：重新编译 `main.cpp`、`NaviInterface.cpp` 并成功链接 `navigation`。

2. `NewWheelCtrl` 增量构建通过：

```powershell
ninja -C .\NewWheelCtrl\build
```

结果：`no work to do`。

## 新脚本用法

### 上传四个模块

```powershell
.\upload_modules_to_robot.ps1 -Build -ReconnectDelaySeconds 2
```

- 默认读取 `config.txt` 中的机器人 IP。
- 每个 IP 会自动补 `:8710`。
- `hdc tconn` 失败时每 2 秒重试，连接成功后才执行远端建目录、上传和 chmod。
- 只上传某一台车：

```powershell
.\upload_modules_to_robot.ps1 -Robot 192.168.107.173 -ReconnectDelaySeconds 2
```

### 启动机器人并拉日志

```powershell
.\start_robots_and_logs.ps1 -ReconnectDelaySeconds 2
```

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

## 下一步实机验证

1. master 建图保存后，sub 执行 `105 -> 5`，确认只拉地图、不拉 `roadFile.txt`。
2. 平板给 master 下发区域 A：`107 -> 108(robot_id=0)`，确认 master 生成并完整执行自己的 `roadFile.txt`。
3. 平板给 sub 下发区域 B：`107 -> 108(robot_id=1)`，确认 sub 生成并完整执行自己的 `roadFile.txt`。
4. 观察两车靠近时心跳 byte1/byte2 的协同避障状态，确认暂停/恢复不会被平板误判为任务失败。
