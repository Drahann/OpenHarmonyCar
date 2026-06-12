# 紫派适配指南：LAN Socket 黑板集成

> **日期**: 2026-06-13  
> **状态**: ⏳ 待成员A 执行  
> **面向读者**: 成员A（紫派 OpenHarmony 开发者）  
> **前置文档**: [`lan-blackboard-impl.md`](lan-blackboard-impl.md)（实施详细）/ [`lan-blackboard-plan.md`](lan-blackboard-plan.md)（设计方案）

---

## 1. 背景

### 1.1 为什么改

平板（MatePad）和紫派之间的**黑板传输层**从鸿蒙软总线 `distributedDataObject` (DDO) 换成了 **LAN TCP Socket**。

**原因**：DDO 在消费级平板 + 紫派组合上无法工作（详见 [`distributed-bringup-status.md`](distributed-bringup-status.md)）：
- DDO 依赖蓝牙、超级终端、DSched、华为账号等系统先决条件
- 这些条件在工业紫派 + 消费平板组合上凑不齐
- 8 份 HiLog 逐层分析确认根因：平板权限不足 + 紫派 DSched 服务不稳定

**LAN Socket 优势**：
- 零系统权限：只需 `ohos.permission.INTERNET`（normal，安装即授）
- 零蓝牙/零超级终端/零账号
- 已验证：机器人 UDP 5001 一直通就是铁证

### 1.2 改动范围

**好消息**：这次改动**只涉及车端 agent**（`car-agent/`），**不涉及紫派主控代码**（`purplepi-control/`）。

你不需要改 `udp2lcm` 或任何其他紫派模块。你只需要：
1. 构建并安装车端 agent app 到紫派
2. 确保 agent 能启动并监听 TCP 端口 5003
3. 了解新的网络拓扑以便排查问题

---

## 2. 新架构

### 2.1 网络拓扑

```
┌─────────────┐         TCP :5003          ┌─────────────┐
│   平板 App  │ ◄────────────────────────► │  车 agent   │
│  (客户端)   │   mission / robot JSON     │  (服务端)   │
└─────────────┘                            │  (紫派上)   │
                                           └─────────────┘
                                                  │
                                                  │ UDP 5002 → 5001
                                                  │ (agent → udp2lcm)
                                                  ▼
                                           ┌─────────────┐
                                           │  紫派 udp2lcm │
                                           │  (端口 5001)  │
                                           └─────────────┘
```

### 2.2 端口规划

| 端口 | 协议 | 方向 | 用途 | 改动 |
|------|------|------|------|------|
| 5001 | UDP | 平板→紫派 / agent→紫派 | udp2lcm 控制指令 | ❌ 不变 |
| 5002 | UDP | 紫派→agent | udp2lcm 心跳回报 | ❌ 不变 |
| **5003** | **TCP** | **平板→agent** | **LAN 黑板（mission/robot JSON）** | ✅ **新增** |
| 8000 | HTTP | 平板→紫派 | 地图文件拉取 | ❌ 不变 |

**关键**：新增 TCP 端口 5003 用于平板↔agent 的黑板通信。你的 `udp2lcm` 代码不需要改。

### 2.3 数据流

```
平板                        车 agent (紫派)                紫派 udp2lcm
  │                              │                              │
  │──── TCP connect ────────────►│                              │
  │                              │                              │
  │──── mission (JSON) ─────────►│                              │
  │                              │──── UDP cmd ────────────────►│
  │                              │     (5002 → 5001)            │
  │                              │                              │
  │                              │◄──── UDP heartbeat ─────────│
  │                              │     (5001 → 5002)            │
  │                              │                              │
  │◄──── robot (JSON) ──────────│                              │
  │                              │                              │
```

**流程**：
1. 平板通过 TCP 5003 连到车 agent
2. 平板发送 `mission` 消息（包含任务阶段、子区域分配、目标点等）
3. Agent 解析 mission，提取本车的指令，通过 UDP 5002 发给 `udp2lcm`（端口 5001）
4. `udp2lcm` 执行指令（建图/导航/覆盖），通过 UDP 5001 回报心跳
5. Agent 收到心跳，打包成 `robot` 消息，通过 TCP 5003 发回平板
6. 平板更新地图上的车辆位置和状态

---

## 3. 你需要做什么

### 3.1 构建车端 agent

车端 agent 代码在仓库的 `car-agent/` 目录，是一个 OpenHarmony app（UIAbility 外壳）。

**步骤**：

```bash
# 1. 切换到 app-harmony-core 分支（包含 LAN 黑板改动）
git checkout app-harmony-core

# 2. 进入 car-agent 目录
cd car-agent

# 3. 用 DevEco Studio 打开项目，构建 HAP 包
#    - 选择 Release 或 Debug 模式
#    - Build → Build Hap(s)
#    - 产物在 car-agent/entry/build/default/outputs/default/entry-default-signed.hap

# 4. 把 HAP 包传到紫派并安装
hdc file send entry-default-signed.hap /data/local/tmp/
hdc shell bm install -p /data/local/tmp/entry-default-signed.hap
```

### 3.2 启动 agent

```bash
# 启动 agent（UIAbility）
hdc shell aa start -a AgentAbility -b com.example.carapp

# 验证启动成功
hdc shell ps -ef | grep carapp
# 应该看到 com.example.carapp 进程

# 验证 TCP 端口监听成功
hdc shell netstat -tlnp | grep 5003
# 应该看到 0.0.0.0:5003 LISTEN
```

### 3.3 查看 agent 日志

```bash
# 实时查看 agent 日志
hdc shell hilog | grep -E "FleetMission|AgentCore"

# 预期日志（启动成功）
# FleetMission(agent): FleetMissionService(LAN agent) init, port=5003, session=OpenHarmonyCarFleetV1
# FleetMission(agent): joinSession: OpenHarmonyCarFleetV1 → 启动 TCP 服务端 :5003
# FleetMission(agent): TCP 服务端已启动: 0.0.0.0:5003
# FleetMission(agent): LAN 黑板就绪 OpenHarmonyCarFleetV1（TCP 服务端 :5003，等待平板连入）
# AgentCore: agent up: car=1 → bind 本机 :5002（UDP 静默待命，仅任务期接 udp2lcm 127.0.0.1:5001）
```

### 3.4 确保网络可达

**平板需要能连到紫派的 TCP 5003 端口**。

**检查清单**：

1. **同一 WiFi**：平板和紫派连同一个 WiFi 热点
2. **无客户端隔离**：热点不能开启"AP 隔离"或"客户端隔离"（家用路由器一般没问题，公共/企业网络可能有）
3. **防火墙**：如果紫派有防火墙，放行 TCP 5003

**验证网络可达**（在平板上用 hdc shell 测试）：

```bash
# 获取紫派 IP（假设是 192.168.43.2）
hdc shell ifconfig wlan0 | grep inet

# 从平板测试 TCP 连接（如果有 nc 命令）
hdc shell nc -zv 192.168.43.2 5003
# 应该看到 Connection to 192.168.43.2 5003 port [tcp/*] succeeded!
```

---

## 4. 联调测试

### 4.1 测试步骤

1. **紫派启动 udp2lcm**（你的现有流程）
2. **紫派启动 agent**（按 §3.2）
3. **平板启动 app-harmony**
4. **平板进分布式模式**：
   - 首页 → 点击"分布式"按钮
   - UDP 0x06 广播发现车（应该看到紫派 IP）
   - 点击紫派 IP，建立 UDP 连接
5. **平板划覆盖区域**：
   - 建图完成后，进入"覆盖模式"
   - 在地图上划一个矩形区域
   - 点击"开始覆盖"

### 4.2 预期日志

**紫派 agent**（`hdc shell hilog | grep FleetMission`）：

```
# 平板连接进来
FleetMission(agent): 平板已连入
FleetMission(agent): hello 握手成功: role=pad

# 收到任务
FleetMission(agent): 收到 mission: phase=covering assignments=1
FleetMission(agent): 黑板更新: phase=covering 车1 区域=有 目标点=无 → 产 3 条本机命令

# 发指令给 udp2lcm
AgentCore: TX→ 127.0.0.1:5001 cmd=107 (0x6b) run=0 spd=0 x=100 y=200
AgentCore: TX→ 127.0.0.1:5001 cmd=108 (0x6c) run=1 spd=0 x=300 y=400
AgentCore: TX→ 127.0.0.1:5001 cmd=3 (0x03) run=0 spd=0 x=0 y=0

# 收到 udp2lcm 心跳，回报给平板
FleetMission(agent): 本机心跳 → 写回黑板: 车1 pose=(120,340,45)
FleetMission(agent): sendRaw: {"t":"robot","robot":{"index":1,"ip":"192.168.43.2",...}}
```

**平板**（`hdc shell hilog | grep FleetMission`）：

```
# 连接车 agent
FleetMission: 连接车 agent: 192.168.43.2:5003
FleetMission: TCP 连接成功: 192.168.43.2
FleetMission: hello 握手成功: 192.168.43.2 role=car carId=1

# 发送任务
FleetMission: publishMission: phase=covering assignments=1
FleetMission: sendTo: 192.168.43.2 {"t":"mission","snapshot":{...}}

# 收到车辆状态更新
FleetMission: 合并 robot: index=1 pos=(120,340) status=covering
```

### 4.3 验收判据

- ✅ 紫派 agent 日志出现 `平板已连入` + `收到 mission` + `黑板更新: phase=covering`
- ✅ 紫派 agent 日志出现 `TX→ 127.0.0.1:5001 cmd=107/108/3`（发给 udp2lcm）
- ✅ 平板日志出现 `合并 robot: index=1 pos=(...)`（收到车辆状态）
- ✅ 平板地图上车**变色 + 随心跳移动**
- ✅ **全程不需要 bindTarget/互信/同账号/系统权限**

---

## 5. 常见问题排查

### 5.1 Agent 启动失败

**症状**：`hdc shell aa start` 失败，或进程启动后立刻退出

**排查**：
```bash
# 查看详细错误
hdc shell hilog | grep -E "AgentAbility|AgentCore|FleetMission"

# 常见错误 1：端口被占用
# FleetMission(agent): startServer 异常: {"code":98,"message":"Address already in use"}
# 解决：检查是否有其他进程占用 5003
hdc shell netstat -tlnp | grep 5003
hdc shell kill -9 <PID>

# 常见错误 2：权限不足
# FleetMission(agent): startServer 异常: {"code":13,"message":"Permission denied"}
# 解决：检查 module.json5 是否声明 INTERNET 权限
# 重新构建并安装 HAP
```

### 5.2 平板连不上 agent

**症状**：平板日志出现 `连接车 agent: 192.168.43.2:5003` 但没有 `TCP 连接成功`

**排查**：
```bash
# 1. 确认 agent 在监听
hdc shell netstat -tlnp | grep 5003
# 应该看到 0.0.0.0:5003 LISTEN

# 2. 确认网络可达
# 在平板上测试（如果有 nc）
hdc shell nc -zv 192.168.43.2 5003

# 3. 检查防火墙（如果紫派有）
hdc shell iptables -L -n | grep 5003
# 如果有 DROP 规则，放行
hdc shell iptables -I INPUT -p tcp --dport 5003 -j ACCEPT

# 4. 检查 WiFi 客户端隔离
# 家用路由器一般没问题，公共/企业网络可能有
# 换一个热点试试
```

### 5.3 Agent 收不到 mission

**症状**：平板显示已发送 mission，但紫派 agent 没有 `收到 mission` 日志

**排查**：
```bash
# 1. 确认 TCP 连接建立
hdc shell hilog | grep "平板已连入"

# 2. 确认握手成功
hdc shell hilog | grep "hello 握手成功"
# 如果看到 "hello 握手失败: session 不匹配"，说明平板和 agent 的 FLEET_SESSION_ID 不一致
# 检查 constants/protocol.ets 中的 FLEET_SESSION_ID 是否同步

# 3. 查看完整收包日志
hdc shell hilog | grep -E "onReceive|processRecvBuffer|handleMessage"
# 如果看到 "JSON 解析失败"，说明协议格式不对
# 检查平板和 agent 的代码版本是否一致（都在 app-harmony-core 分支）
```

### 5.4 Agent 收不到 udp2lcm 心跳

**症状**：紫派 agent 有 `收到 mission` 但没有 `本机心跳 → 写回黑板`

**排查**：
```bash
# 1. 确认 udp2lcm 在运行
hdc shell ps -ef | grep udp2lcm

# 2. 确认 agent 绑定了 UDP 5002
hdc shell netstat -ulnp | grep 5002
# 应该看到 0.0.0.0:5002

# 3. 确认 udp2lcm 配置了回报端口 5002
# 检查 udp2lcm 的配置文件或启动参数
# 应该把心跳回报到 127.0.0.1:5002（不是 5001）

# 4. 手动测试 UDP 连通
hdc shell nc -u 127.0.0.1 5002
# 输入任意字符，agent 日志应该出现 "onLocalHeartbeat"
```

### 5.5 平板收不到 robot 状态

**症状**：紫派 agent 有 `本机心跳 → 写回黑板`，但平板没有 `合并 robot`

**排查**：
```bash
# 1. 确认 agent 发送了 robot 消息
hdc shell hilog | grep "sendRaw.*robot"
# 应该看到 {"t":"robot","robot":{"index":1,...}}

# 2. 确认 TCP 连接还在
hdc shell netstat -tnp | grep 5003
# 应该看到 192.168.43.x:5003 ESTABLISHED

# 3. 查看平板完整收包日志
hdc shell hilog | grep -E "onReceive|processRecvBuffer|handleMessage"
# 如果看到 "JSON 解析失败"，说明协议格式不对
# 如果看到 "未知消息类型"，说明消息类型不对
```

---

## 6. 你的代码不需要改

**重要**：你的 `udp2lcm` 代码**不需要任何改动**。

Agent 对 `udp2lcm` 的接口完全不变：
- Agent 通过 UDP 5002 发指令到 `udp2lcm` 的 5001
- `udp2lcm` 通过 UDP 5001 回报心跳到 agent 的 5002

唯一的变化是：
- 以前 agent 通过 DDO 收平板的指令（不通）
- 现在 agent 通过 TCP 5003 收平板的指令（通了）

Agent 收到指令后，还是走 UDP 发给 `udp2lcm`，你的代码完全无感。

---

## 7. 后续优化（可选）

以下优化不影响当前功能，后续有空再做：

### 7.1 Agent 常驻后台

当前 agent 是 UIAbility（有界面），会被系统回收。后续可以改成 ServiceExtensionAbility（无界面，常驻后台），需要：
- Full SDK（不是 Public SDK）
- 系统签名（不是三方签名）

详见 [`distributed-bringup-status.md`](distributed-bringup-status.md) §6。

### 7.2 开机自启

当前 agent 需要手动启动。后续可以配置开机自启：
- `module.json5` 加 `BOOT_COMPLETED` 广播接收
- 或紫派系统侧配置 init 脚本

### 7.3 应用级心跳

当前靠 TCP + 重连做保活。后续可以加应用级心跳（每 2s 发一条 `ping`），做死链检测。

---

## 8. 联系与支持

遇到问题随时在群里问，或直接在仓库提 Issue。

**相关文档**：
- [`lan-blackboard-impl.md`](lan-blackboard-impl.md)：实施详细（完整代码逻辑）
- [`lan-blackboard-plan.md`](lan-blackboard-plan.md)：设计方案（为什么这么设计）
- [`distributed-bringup-status.md`](distributed-bringup-status.md)：DDO 失败根因分析
- [`multi-robot-collab.md`](../contracts/multi-robot-collab.md)：多机协同契约（FleetMission 黑板语义）

---

**文档结束**
