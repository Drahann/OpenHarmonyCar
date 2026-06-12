# Plan6 App-紫派接口对齐改动方案

> **状态（2026-06-12）**：本文件只整理改动方案，未改紫派源码、App 源码或 agent 源码。
> **范围**：App(`app-harmony/`) + 车载 agent(`car-agent/`) ↔ 紫派(`purplepi-control`) 的分布式覆盖、地图传输、坐标换算接口对齐。
> **依据**：[`../contracts/app-purplepi-alignment-audit.md`](../contracts/app-purplepi-alignment-audit.md)、`plan/plan6.md`、`全局须知.md`。
> **本轮硬约束**：分布式覆盖传图强制使用鸿蒙分布式软总线；旧 `cmd105/124/wget` 跨车拉图路径只保留兼容说明，不作为新流程。

---

## 0. 本轮结论

| 编号 | 模块 | 严重度 | 改动结论 |
|---|---|---|---|
| P1 | 紫派 `cmd108/'l'` | 高 | master/sub 都必须按 `107 -> 108` 生成覆盖路径并启动执行；`108` 内不再加载地图。 |
| P2 | 分布式传图 | 高 | master 地图通过分布式软总线同步给 sub，sub agent 落地 `/data/test/defultMap.txt` 后才发 `cmd5`。 |
| P3 | agent 命令编排 | 高 | master 覆盖只发 `107,108`；sub 覆盖在地图落地后发 `5,107,108`。 |
| P4 | 平板连接 | 中 | distributed 模式平板不直连紫派 UDP 5001，避免和 master-agent 抢占紫派单一心跳客户端。 |
| P5 | App 坐标 | 高 | App 解析地图头 `metersPerPixel/x0/y0`，统一世界坐标与栅格坐标换算。 |
| P6 | 紫派 UDP 健壮性 | 低 | 后续可刷新 `clientIP` 心跳目标，解决换平板后能控但收不到心跳的问题。 |

整体时序收敛为：

```text
master agent 保存 /data/test/defultMap.txt
  -> 分布式软总线同步地图文本/版本/校验
  -> sub agent 校验并原子落地 /data/test/defultMap.txt
  -> sub agent 发 cmd5 加载地图并归零
  -> master agent 发 107 -> 108(robot_id=0)
  -> sub agent 发 107 -> 108(robot_id=1)
```

## 1. 紫派侧改动

### 1.1 重写 `cmd108/'l'` 的分布式覆盖语义

**涉及位置**：`NewWheelCtrl/udp2lcm/udp2lcm.c::parseCmd`

当前不一致点：

- `robot_id==1` 才发布 `ROBOT_CONTROL 122` 生成覆盖路径，`robot_id==0` 只发布 `123`，导致 master 没有覆盖路径可执行。
- `cmd108/'l'` 内部再次发布 `ROBOT_CONTROL 10` 加载地图，并使用当前心跳位姿参与加载，会覆盖 sub 前置 `cmd5` 设定的 `0,0,0` 初始位姿。

改动要求：

1. `cmd108/'l'` 只处理“对角点 2 + 启动覆盖”，不再发布 `ROBOT_CONTROL 10`。
2. `diag_pt1`、`diag_pt2` 都有效，且 `robot_id` 为 `0` 或 `1` 时，master/sub 都发布 `ROBOT_CONTROL 122`。
3. `122` 的 `dparams[0..2]` 使用当前心跳位姿，`dparams[3..6]` 使用两个对角点。
4. `122` 发布后再发布 `ROBOT_CONTROL 123`，`iparams[0]=robot_id`。
5. 对角点不完整或 `robot_id` 非法时，本次 `108` 直接拒绝并打印日志，不能继续发 `123` 复用旧路径。
6. 成功处理一次 `108` 后清空 `diag_pt1/diag_pt2` 暂存，降低下一轮丢包误用旧点的风险。

配套文档更新：紫派 README 中 `108` 应描述为“master/sub 都生成 122 并执行 123；地图加载由前置 `cmd5` 或既有定位流程负责”。

### 1.2 保留 `cmd5` 作为软总线落地图后的加载入口

**涉及位置**：`NewWheelCtrl/udp2lcm/udp2lcm.c::parseCmd`、`Navi/main.cpp::case 10`

紫派 C/C++ 栈不再承担跨车拉图。新流程由 App/agent 通过分布式软总线把 master 地图同步给 sub，并由 sub agent 将地图落地到：

```text
/data/test/defultMap.txt
```

紫派侧保留现有 `cmd5` 语义即可：停车，发布 `ROBOT_CONTROL 10`，并把初始位姿设为 `0,0,0`。这个命令只表示“地图已经在本机落地，现在加载并归零”。

旧 `cmd105/'i'`、`ROBOT_CONTROL 124`、`NAVI_SubGetMapFromMain()`、`wget` 路径不进入新分布式覆盖流程。

### 1.3 低优先刷新 UDP 心跳目标

**涉及位置**：`NewWheelCtrl/udp2lcm/udp.c`

当前 `clientIP` 只在首个非发现包时记录一次。后续平板换 IP 或重新连接时，可能出现命令可达但心跳仍发往旧 IP 的情况。

后续可在命令循环收到有效非发现包时刷新心跳目标地址，并让发送线程使用可更新的目标地址。仅更新字符串不够，发送线程中的 `serverAddr.sin_addr` 也要同步更新。

## 2. App / agent 侧改动

### 2.1 distributed 模式平板不直连 master UDP

**涉及位置**：`app-harmony/entry/src/main/ets/pages/ControlPage.ets`

distributed 模式下，平板只加入 `FleetMission` 黑板并接收 agent 写回的位姿、进度、地图状态，不再调用 `connectTo(this.ip)`，也不启动到 master 的 UDP heartbeat。

目标效果：

- master-agent 成为 master 紫派 UDP 5001 的唯一客户端。
- 平板不再和 master-agent 抢紫派 `udp2lcm` 的单一 `clientIP`。
- 地图显示、位姿显示从软总线黑板或单机既有流程取数。

### 2.2 使用分布式软总线传图并落地

**涉及位置**：`car-agent/entry/src/main/ets/agent/AgentCore.ets`、`model/mission.ets`、`FleetMission` 黑板相关代码

改动要求：

1. master agent 在地图保存完成后读取 `/data/test/defultMap.txt`。
2. master agent 将地图文本写入 `FleetMission.map.text`，并附带 `map.version`、`map.ready`、`map.checksum` 或等价字段。
3. sub agent 监听到新地图后，先写入 `/data/test/defultMap.txt.tmp`。
4. sub agent 校验地图非空、首行合法、校验值一致后，原子替换为 `/data/test/defultMap.txt`。
5. sub agent 只有在地图落地成功后，才允许发送 `cmd5`。
6. 如果当前 agent 不能写 `/data/test`，应解决 agent 部署、授权或新增本机文件落地接口，不能回退到旧跨车拉图路径。

初版直接传普通 `defultMap.txt` 文本。若软总线负载过大，再在同一软总线字段中传 `zipedMap.txt` 文本，并由 agent 解码后落地为 `defultMap.txt`。

需要补的测试：

- master 地图文本写入黑板后，sub 能落地为 `/data/test/defultMap.txt`。
- 空文本、首行非法、校验不匹配时不发送 `cmd5`。
- 地图版本未变化时不重复落地或重复触发覆盖流程。

### 2.3 区分 master/sub 覆盖命令

**涉及位置**：`car-agent/entry/src/main/ets/reconciler/Reconciler.ets`、`car-agent/entry/src/main/ets/agent/AgentCore.ets`

命令编排改为：

| 车辆 | 前置条件 | 命令序列 |
|---|---|---|
| master (`robotId==0`) | 已有本机建图结果与定位状态 | `107 -> 108` |
| sub (`robotId==1`) | 已通过软总线落地 master 地图 | `5 -> 107 -> 108` |

`loaded` 状态只表示 sub 已执行过 `cmd5`。master 不因分布式覆盖任务触发 `cmd5`，避免覆盖任务开始时把 master 位姿归零。

需要补的测试：

- master 覆盖只产生命令 `107,108`。
- sub 地图未落地时不产生命令。
- sub 地图落地后产生命令 `5,107,108`。

### 2.4 App 地图坐标补 `x0/y0`

**涉及位置**：`MapService.ets`、`geometry.ets`、`MapCanvas.ets`、相关单测

紫派地图首行为：

```text
range resolution height width metersPerPixel x0 y0
```

`x0/y0` 是地图最小角世界坐标，单位为米。App 需要解析并传递 `metersPerPixel/x0/y0`，不能只使用 `height/width`。

统一换算公式：

```text
gridX = (worldX / 20 - x0) / metersPerPixel
gridY = (worldY / 20 - y0) / metersPerPixel

worldX = (gridX * metersPerPixel + x0) * 20
worldY = (gridY * metersPerPixel + y0) * 20
```

落地要求：

1. `MapService.parseMap()` 解析 `metersPerPixel`、`x0`、`y0`。
2. `ParsedMap.toTransform()` 将三个字段传给 `MapTransform`。
3. `mapToCanvas()` 接收紫派心跳/UDP 世界坐标时，先转栅格坐标再走现有画布换算。
4. `canvasToMap()` 返回业务层前，从栅格坐标转回紫派 UDP 世界坐标。
5. 分布式矩形角点、A* 目标点、机器人 pin 使用同一套坐标换算。

需要补的测试：

- `x0/y0` 为负数时，`mapToCanvas(canvasToMap())` 保持互逆。
- 同一心跳位姿在有偏移地图上不再出现整体常量偏移。
- A* 目标点与分布式矩形角点下发值符合紫派世界坐标。

## 3. 不进入本轮的事项

- 不改 `old_code`。
- 不动结束建图后从队列回放建图逻辑。
- 不把 `cmd105/124/wget` 作为新分布式覆盖传图路径。
- 不删除 `106/'j'` 源码，仅在文档中标注为旧路径或兼容路径。
- 不改 `Lidar` 与 `serial` 主流程；中性保活不打断自主运动的问题已由 `PATH` 与 `wheel_ctrl` 分离、`parseCmd` 幂等性排除。

## 4. 推荐实施顺序

1. 紫派先改 `cmd108/'l'`，解决 master 不动、sub 位姿被覆盖的核心问题。
2. App/agent 同步改 distributed 模式连接、软总线传图落地、master/sub 命令编排。
3. App 补 `x0/y0` 坐标换算，解决机器人 pin、A* 目标点、分布式矩形角点整体偏移。
4. 真机联调后再决定是否补紫派 `clientIP` 刷新。

## 5. 验收清单

- master：收到 `107 -> 108(robot_id=0)` 后能生成覆盖路径并运动。
- sub：先通过分布式软总线收到并落地 master 地图，再执行 `5 -> 107 -> 108(robot_id=1)`，能加载、归零、生成覆盖路径并运动。
- 平板 distributed 模式下不再成为紫派 UDP 心跳客户端，位姿来自 agent 黑板回写。
- 非零 `x0/y0` 地图上，机器人 pin、A* 目标点、分布式矩形角点不再出现整体常量偏移。
- 旧 `cmd105/124/wget` 路径不参与新分布式覆盖主流程。
