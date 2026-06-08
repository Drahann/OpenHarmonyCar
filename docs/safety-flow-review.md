# App 流程 / 连接安全性复审（2026-06-08，用户驱动）

> 用户提出：① 单连接设计与多机协同（多车同时心跳）矛盾；② App 操作需按连接/心跳状态门控（没建立心跳就不该能进操作界面/下发命令）——**整个流程需重审，找出所有此类安全问题**；
> ③ 多机协同是覆盖还是单点导航？④ 全路径覆盖为何没有算法选择选项？
> 本文是审计结论 + 答复 + 修复计划。关联 `feature-parity-review.md`、`multi-robot-collab.md`、`contracts/integration-qa.md`。

## 1. 连接/心跳状态门控审计（核心安全问题）

**现状：只有摇杆 + "开始建图" 按连接门控，其余命令都没门控；且"已连接"是粘滞的（不会因心跳丢失而清除）。**

| 命令 | 触发 | 连接门控？ | 问题 |
|---|---|---|---|
| 摇杆 cmd1 | Joystick | ✅ `disabled: connectedIps.length===0` | 用粘滞 connectedIps，丢心跳不更新 |
| 开始建图 cmd0 | BuildCard | ✅ `enabled(connectedIps>0)` | 同上（粘滞） |
| 结束建图 cmd2 | BuildCard | ❌ 无 | 未连接也能点 |
| 重新建图 cmd0 | TopBar | ❌ 无 | 未连接也能点 |
| 开始导航 cmd3 | AstarOps | ❌ 仅 `hasTarget()` | 未连接也能下发目标 |
| 取消 cmd4 | AstarOps | ❌ 无 | — |
| 选顶点/启动 cmd104/102 | 地图点选 | ❌ 仅 `mapReady` | 未连接也能下发顶点 |
| 停止全路径 cmd103 | FullpathOps | ❌ 无 | — |
| 划区 cmd107/108 | 地图点选 | ❌ 仅 `mapReady` | 未连接也能下发 |
| 地图点选 | MapCanvas | ⚠️ 仅 `enablePick: mapReady` | 未连接也能选点→触发下发 |

**🔴 关键缺陷**：
1. **`connectedIps` 只增不减**（`onMessage` 收到首帧即加入、永不移除）→ 心跳中途断了，UI 仍显示"已连接 + 绿点"，命令照发。**与新加的 `RobotTransport.connStateOf`（带新鲜度/丢失判定）脱节**——ControlPage 没用它。
2. **大多数命令未按连接门控**（仅 cmd0/摇杆有）。`mapReady` 是 HTTP 拉图结果、≠ 心跳活着——map 就绪但心跳已断时，astar/fullpath/distributed 命令仍可下发。
3. TopBar 连接点用 `connectedIps.length>0`（粘滞），同样不反映真实心跳。
4. ✅ 进入 ControlPage 已被 HomePage 门控（仅"已连接"才有「进入控制」），但**页内心跳丢失不再门控**。

**修复（§4 P1）**：ControlPage 改用 `connStateOf(this.ip)===connected` 作**实时**连接判据；所有下发型命令 + 地图点选都加该门控；TopBar 点/文案反映实时状态；加刷新定时器使"丢失"能及时变灰禁用。

## 2. 单连接 vs 多机协同（架构澄清——用户问得对）

**我此前加的 `connectExclusive`（连新断旧）对单车模式没错，但暴露了多机连接模型的两个真问题：**

- **意图架构（`multi-robot-collab` / `car-agent-plan` §6.3）**：每车一个 **agent**，是该车 `udp2lcm` 的**唯一 localhost 客户端**、各自维持本车心跳；平板**持黑板**写 assignments、各 agent 读自己那块驱动本机；各 agent 把位姿/进度写回黑板，平板**读黑板可视化所有车**（不直连从车 UDP）。→ **多车的"同时心跳"发生在各车 agent 上、不在平板**；平板单连接（甚至零直连）本身没错。
- **🔴 问题 A**：ControlPage 在 distributed 模式仍 `connectTo(this.ip)` **直连保活 master**。但 master 也有自己的 agent（localhost 唯一客户端）→ **平板 + master-agent 两个客户端抢 master 的 udp2lcm 单 client 记录 + 3s 急停判定**（Q6.2 的本机端口互斥）。**distributed 模式平板不应直连任何车（含 master），全经黑板。**
- **🔴 问题 B**：**"平板直发兜底"（distUseAgent=false）只对 `this.ip` 一台发 cmd107/108**（`sendDistArea` 只 this.ip）→ **平板直接驱动多车的路径根本没实现**。agent 未部署时，多车协同**端到端走不通**。
- **结论**：单连接不是错；错在 distributed 模式的连接语义未定。**待决策**（§4 决策点）：distributed 模式平板是 (a) 纯黑板不直连任何车（依赖 agent，最干净，合意图）／(b) 直连 master + 黑板带从车（与 master-agent 冲突，需 A 定 localhost 让路）／(c) 平板直连并保活所有车（无 agent 的测试路径，需多连接 + 对每车发 107/108）。

## 3. 多机协同 = 覆盖（不是单点导航）+ 算法选择缺失（用户问得对）

- **多机协同 = 多车矩形覆盖**（每车领一个轴对齐矩形子区域 → `cmd107/108` → 紫派 LCM **122**(生成矩形覆盖路径)+**123**(分布式跟踪)）。概念上是"全路径覆盖按矩形切给多车"，**不是单点导航**（单点导航是 astar 模式 cmd3）。三模式：astar=单点导航 / fullpath=单车全路径覆盖 / distributed=多车协同覆盖。
- **🔴 算法选择缺失**：单车 fullpath 有算法选择（`cmd102` byte1 = 牛耕0/最小生成树1 → LCM127）；但**多车覆盖（cmd107/108 → LCM122）的协议里没有算法参数**（A 文档 §三 122 无 algNum）。所以要么多车覆盖**固定算法**、要么**应当也支持选择**——这是**接口不一致**，需问 A：LCM122 是否支持覆盖算法选择？若支持，在 107/108 哪个空字节带 algNum？（→ integration-qa）。**接口是两边共同设计的**——若该选项有价值，大胆提 A 加。

## 4. 修复计划（优先级）

- **P1（本轮做·安全·决策无关）**：ControlPage 连接门控改实时——`connStateOf(this.ip)` 驱动所有命令/点选的 enabled + TopBar 状态 + 刷新定时器；修粘滞 `connectedIps`。
- **P2（待决策→再做）**：distributed 模式连接语义（§2 决策点 a/b/c）。倾向 (a) 纯黑板 + (c) 作无 agent 测试兜底（需补"平板直发对每车"）。定了再改 ControlPage distributed 连接 + HomePage 多选车。
- **P3（待 A）**：多车覆盖算法选择（§3）——A 确认 LCM122 能否选算法；能则 App 在 DistributedOps 加算法 chip（复用 fullpath 的 AlgChip）+ 协议带 algNum。
- **P4**：多机 HomePage 流程——distributed 模式下选**多台**车（而非一台）进入协同界面；与 P2 连接语义一起设计。

## 5. 待成员A（→ integration-qa）

- **Q（新）多车覆盖算法**：LCM122（分布式矩形覆盖）是否支持像单车 LCM127 那样选覆盖算法（牛耕/最小生成树…）？若支持，App 想在 107/108 带 algNum（哪个空字节？）——这样单/多车算法选择一致。
- **Q6.2 复核**：distributed 模式 master 是否由其 agent 独占 localhost、平板不直连？（确认平板 distributed 不该 connectTo master。）
