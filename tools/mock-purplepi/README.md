# mock-purplepi — PC 端假紫派（给 App 用）

让 App / `mock-app` 在**没有真车**时联调控制与地图流程。按
[`udp-protocol.md`](../../contracts/udp-protocol.md)、[`udp-protocol-crosscheck.md`](../../contracts/udp-protocol-crosscheck.md)、
[`map-format.md`](../../contracts/map-format.md)、[`multi-robot-collab.md`](../../contracts/multi-robot-collab.md) 实现。
整体测试策略见 [`docs/testing.md`](../../docs/testing.md)。

## 功能（已实现）

- **UDP `<bind>:5001`**：命令 0 建连后每 **500ms** 回 9 字节心跳（`byte0=3`，`x/y/r` int16 大端）。
  位姿**随命令变化**：3/106 导航到目标点、1 遥控(go/left/right)、108 朝对角点2"覆盖"、5 归零；3s 未收指令打印"急停"。
- **命令感知**：0 建连 / 1 遥控 / 2 结束建图 / 3 目标点 / 4 取消 / 5 加载图(归零) /
  102-104 全路径 / 105 子机拉主机图(主机IP在 `byte[1,2,4,6]`) / 106 目标点 / 107 覆盖矩形对角点1 / 108 对角点2+robot_id。
- **发现**：① 命令 0 走广播也能建连（兜底）；② **`0x06` 发现 ping**（提案）→ 回一帧身份（`byte1`=车号），
  但**不建连、不武装 3s 急停**（见 `contracts/integration-qa.md` Q5，待 A 定）。
- **HTTP `<bind>:8000`**：`/defultMap.txt`（web 根=/data/test，**URL 无前缀**）+ `/roadFile.txt`（cmd124 子机会一起拉）。
  `--gen-map 行x列` 生成边框大图（首行 `range resolution height width`）；1800×1800 ~3.3MB，可过 App
  就绪阈值 `MAP_READY_MIN_BYTES=324e4`——小 fixture（40×40）过不了 `pollMapUntilReady`，见 `docs/testing.md` §四。

## 用法

```bash
python mock_purplepi.py                               # 单车，取 fixtures/defultMap.txt（小图）
python mock_purplepi.py --id 2 --gen-map 1800x1800    # 车号2 + 大图（过就绪阈值）
python mock_purplepi.py --bind 127.0.0.2              # 多车：各绑不同 IP（见 docs/testing.md §五）
python smoke_test.py                                  # 自检：HTTP + 心跳 + 发现 ping + 生成图
```

参数：`--id`(车号，发现响应回传) `--bind`(默认 0.0.0.0) `--udp-port`(5001) `--http-port`(8000)
`--map`(地图文件) `--gen-map`(行x列，生成大图) `--start-x/--start-y`(初始位姿)。

App 把目标 IP 设为运行本脚本的机器；地图 URL `http://<ip>:8000/defultMap.txt`。
多车一次起多辆见 `mock_fleet.py`（待建，见 `docs/testing.md` §五 多车绑定）。
