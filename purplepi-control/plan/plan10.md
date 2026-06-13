# plan10：平板多点协同覆盖设计与子车压缩图拉取

## 本轮已完成

1. 阅读 `全局须知.md`、`plan/plan9.md`、根目录 `README.md` 以及 `readme-upload/contracts` 中的多机协同、地图格式和 UDP 协议文档。
2. 根据 plan9 的新方向，把多机协同覆盖主设计从“车端按一个矩形自动拆分覆盖路线”调整为“平板为每台车分配不同的多个选点队列，车端逐点执行普通导航并协同避障”。
3. 修改 `Navi/NaviInterface.cpp::subGetMapFromMain()`：
   - 子车收到 `cmd105 -> cmd124` 后，先从主车拉 `zipedMap.txt`。
   - 拉取成功后用 `MapServer::loadZipedMap()` 校验 `ZMAP1` 并解压生成本机 `/data/test/defultMap.txt`。
   - 同时将 `/data/test/zipedMap.txt.tmp` 原子替换为正式 `/data/test/zipedMap.txt`。
   - 若压缩图不存在、为空或解压失败，再回退旧的 `defultMap.txt` 拉取逻辑。
   - 地图失败时仍直接返回，不继续拉 `roadFile.txt`，避免旧图或空图误执行。
4. 更新根目录 `README.md` 和 contracts：
   - App/子车均优先使用 `zipedMap.txt`，失败回退 `defultMap.txt`。
   - `roadFile.txt` 明确为旧 `107/108` 矩形兼容路径使用。
   - `readme-upload/contracts/multi-robot-collab.md` 升级到 v0.6，新增 `routes[].points[]` 多点队列主语义。
   - `readme-upload/contracts/map-format.md` 与 `udp-protocol.md` 同步 `cmd105` 压缩图优先语义。

## 两台车日志判断

### 192.168.107.173

- `udp2lcm.log` 显示主车收到了 `107` 和 `108`，并发布了 `122 -> 123`。
- `navi.log` 显示主车生成了覆盖路径并开始执行：`Robot 0 will follow path with 17 points`，随后出现 `astar planning succeed !`。
- 后续出现 `can not arrive !`，再进入 `COOP_AVOID TRIGGER`、多次 `COOP_AVOID RETRY`，最后 `COOP_AVOID TIMEOUT`。

结论：173 不是完全没跑，它已经进入旧矩形覆盖执行，后面卡在动态到达失败和协同避障通信超时。

### 192.168.107.251

- `udp2lcm.log` 显示从车收到了 `cmd105`、`cmd5`、`107`、`108`。
- `navi.log` 中 `cmd124` 仍在执行 `wget http://192.168.107.173:8000/defultMap.txt`，没有拉 `zipedMap.txt`。
- 大图拉取多次尝试会拖慢从车进入加载地图和覆盖执行的时机。

结论：251 的主要问题之一是子车仍走未压缩地图拉取。本轮已改为压缩图优先。

### 综合判断

当前“车不跑”不是单一原因：

- 旧矩形覆盖思路与 plan9 提到的平板多点队列思路确实不一致，需要把新主设计切到平板分配 `routes[].points[]`。
- 现有日志里，173 已按旧矩形路径运行过，因此本次实机问题也包含动态避障/协同通信问题。
- 251 明确暴露出地图传输仍走未压缩 `defultMap.txt`，会影响从车启动覆盖的稳定性和时序。

## 新多机协同避障设计

1. 平板作为全局规划器，在同一张地图上给每台车生成独立的多点队列：
   - `routes[carId].points[] = [{x,y}, ...]`
   - 坐标仍使用紫派地图坐标系，单位仍为 5cm。
2. master 已有地图和定位时直接执行自己的点队列。
3. sub 入场流程固定为：
   - `cmd105`：从 master 优先拉 `zipedMap.txt` 并解压，失败回退 `defultMap.txt`。
   - `cmd5`：加载本机 `defultMap.txt` 并将初始位姿归零到 `(0,0,0)`。
   - 逐点接收普通目标点命令。
4. 多点执行优先复用现有 `cmd3 -> ROBOT_CONTROL 20`：
   - 平板或 agent 等待当前点到达、失败或超时后，再下发下一个点。
   - 不把多点列表强行塞进旧 `107/108`，避免 9 字节协议语义混乱。
5. 车端避障继续由 `Navi` 内的 `COOP_AVOID` 处理：
   - A* 失败、DWA 不可达、定位跳变或两车预测走廊接近时触发。
   - 车间交换位姿和让行请求。
   - 互相请求停车时继续保持 `robotId=1` 优先、`robotId=0` 停等的 tie-break。
6. 若 `COOP_AVOID` 多播仍不稳定，再审批“平板作为协同避障通信中转”方案，而不是直接替换当前车端避障状态机。

## 下一步需要审批

1. 是否新增正式的 `routes` 线协议字段，并由 App/agent 逐点翻译为 `cmd3`。这是当前推荐方案，改动小，也不破坏 9 字节协议。
2. 是否新增机器人“避障暂停/恢复”状态回传。可选路径是心跳预留字节，或 agent/LAN `robot.status`。
3. 若实机再次出现 `COOP_AVOID TIMEOUT`，是否审批平板中转协同避障消息，记录 `relay_rx/relay_ack/relay_forward/relay_retry/relay_timeout`。
4. 是否完全停用旧 `107/108` 矩形分布式覆盖，还是保留到 App 多点队列稳定后再删除。
