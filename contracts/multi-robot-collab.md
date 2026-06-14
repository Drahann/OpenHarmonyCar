# 多机协同契约 · 平板直连双车 + 区域覆盖

**版本 v0.8（2026-06-14）** — 纠正 v0.7 的错误理解：当前多机覆盖不是“平板生成路径点/多点队列”，而是**平板给两辆车分别下发区域矩形，机器人端自己生成并执行覆盖路径**。v0.7 中把 `routes[].points[]` 写成主路径的内容作废。

## 当前结论

1. 平板是区域分配者，不是覆盖路径生成器。
2. 每辆车收到自己的矩形区域后，由紫派 `Navi` 在本车本地生成完整覆盖路径 `/data/test/roadFile.txt`。
3. `123` 执行本车生成的整条路径，不再按 `robot_id` 把共享路径切成前半/后半。
4. 子机 `105` 只从主机拉地图：优先 `zipedMap.txt`，失败回退 `defultMap.txt`；不再拉主机 `roadFile.txt`。
5. 协同避障仍在车端 `COOP_AVOID` 内完成；平板通过心跳 byte1/byte2 读取避障状态和事件。

## 任务字段

多机覆盖的正式任务字段为 `assignments`：

```json
{
  "assignments": [
    {
      "carId": 0,
      "robotId": 0,
      "corner1": { "x": 120, "y": 80 },
      "corner2": { "x": 260, "y": 180 },
      "status": "idle|running|avoiding|paused|failed|done"
    },
    {
      "carId": 1,
      "robotId": 1,
      "corner1": { "x": 270, "y": 80 },
      "corner2": { "x": 410, "y": 180 },
      "status": "idle|running|avoiding|paused|failed|done"
    }
  ]
}
```

| 字段 | 含义 |
|---|---|
| `carId` | 平板任务中的车辆索引。 |
| `robotId` | 紫派车端协同 ID，当前双车为 `0/1`。 |
| `corner1` / `corner2` | 本车要完整覆盖的轴对齐矩形的两个对角点。坐标使用 master 地图坐标系，单位 5cm，与 UDP `107/108` 一致。 |
| `status` | 平板可视化状态，由任务阶段、目标执行结果和心跳 byte1/byte2 维护。 |

`routes[]` / `points[]` 不是当前主路径。后续若要做路径预览或人工航点队列，必须另开契约版本，不能默认替代 `assignments`。

## UDP 下发时序

平板对每辆车各发一组矩形区域：

1. `'k'`/`107`：`byte[3..6]=corner1.x/corner1.y`，只暂存第一个对角点。
2. `'l'`/`108`：`byte[1]=robot_id`，`byte[3..6]=corner2.x/corner2.y`。
3. `udp2lcm` 收到完整对角点且 `robot_id` 为 `0/1` 后，先发布 LCM `122`。
4. `Navi::CreateFullPath()` 在本车矩形内生成 `/data/test/roadFile.txt`。
5. `udp2lcm` 随后发布 LCM `123`。
6. `Navi` 读取本车 `roadFile.txt` 并完整执行，不做前半/后半分段。

## 主从流程

1. 平板令 master 建图并保存地图。
2. 平板把 master 的地图作为显示和分区依据，操作员给两辆车分别画两个矩形区域。
3. 对 master：平板直接下发 `107 -> 108(robot_id=0)`，master 本地生成并执行区域 A 的完整覆盖路径。
4. 对 sub：平板先发 `105`，让 sub 从 master `:8000` 拉 `zipedMap.txt` 并解压到本机 `defultMap.txt`，失败时回退 `defultMap.txt`。
5. sub 地图落地后，平板发 `5`，sub 加载地图并把初始位姿归零到 master 原点 `(0,0,0)`。
6. 平板再给 sub 下发 `107 -> 108(robot_id=1)`，sub 本地生成并执行区域 B 的完整覆盖路径。
7. 两车运行中通过心跳回传位姿和 byte1/byte2 协同避障状态；两车之间的让行、暂停、恢复由 `COOP_AVOID` 处理。

## roadFile.txt 语义

- `roadFile.txt` 是机器人端内部覆盖路径文件，每行 `x,y`。
- 它由本车收到 `107/108` 后的 `122` 生成。
- 平板不生成、不读取、不下发它。
- 子机 `105` 拉图不拉主机 `roadFile.txt`，避免把主车区域路径覆盖到子车本地。
- 新区域到来时，本车会重新生成自己的 `roadFile.txt`。

## 心跳状态

`Navi` 通过 `SERVICE_COMMAND 74` 发布 `robotId/status/event`，`udp2lcm` 写入下一帧 9 字节心跳：

| 字节 | 含义 |
|---|---|
| byte1 | `coopStatus`：0 正常，1 协同诊断，2 本车为对车暂停，3 对车被本车暂停，4 本地动态避障兜底。 |
| byte2 | `coopEvent`：0 无，1 停车，2 恢复，3 超时，4 LCM 不可用，5 本地兜底。 |

## 已确认

1. 平板直连两辆车 UDP，车上不启动 ArkTS agent 作为本轮主路径。
2. 坐标单位为 5cm/格；子机 `cmd5` 加载图后归零到 `(0,0,0)`。
3. 地图传输走车间 HTTP/wget：`cmd124` 优先拉 `zipedMap.txt` 并解压，失败回退 `defultMap.txt`。
4. `107/108` 是当前多机区域覆盖主接口，不是旧兼容路径。
5. `robot_id` 用于本车身份和协同避障优先级，不再表示共享覆盖路径的分段。
