# mock-app — 假 App（PC 命令驱动器）

无需真平板、无需鸿蒙 App UI，即可驱动 9 字节 UDP 协议联调紫派 / mock-purplepi。
也给成员A 在没有真平板时验证 UDP↔LCM 桥与轮控。严格按
[`../../contracts/udp-protocol.md`](../../contracts/udp-protocol.md) 实现，字节布局与
`app-harmony/.../model/protocol.ets`、`tools/mock-purplepi` 完全一致（`>BBBhhh`，大端，9 字节）。

## 用法

```bash
# 交互模式（逐条发，输 help 看全部命令，quit 退出）
python mock_app.py --ip 127.0.0.1

# 单条命令后监听心跳 2s
python mock_app.py --ip 127.0.0.1 --cmd connect
python mock_app.py --ip 127.0.0.1 --cmd "goto 120 80"
python mock_app.py --ip 127.0.0.1 --cmd "go 30" --keepalive   # 持续遥控(每1s重发，模拟长按)

# 广播发现（0x06 提案 ping）后退出
python mock_app.py --discover
```

## 命令（交互 / `--cmd`）

| 命令 | 码 | 说明 |
|---|---|---|
| `connect` / `hello` | 0 | 建连（紫派开始回心跳） |
| `go [speed]` / `left` / `right` / `stop` | 1 | 遥控（runState+speed，默认 speed=20） |
| `endmap` | 2 | 结束建图 |
| `goto X Y` | 3 | 设目标点(endX,endY) |
| `cancel` | 4 | 取消导航 |
| `loadmap` | 5 | 加载地图/位姿归零 |
| `fpstart` / `fpstop` / `fpselect ROOM` | 102/103/104 | 全路径开始/停止/选房间 |
| `dist HOSTIP` / `distend X Y` | 105/106 | 子机拉主机图（IP 打包 byte[1,2,4,6]）/ 目标点 |
| `corner1 X Y` / `corner2 X Y [ROBOTID]` | 107/108 | 覆盖矩形对角点1 / 对角点2+robot_id |
| `raw STATE [B1 B2 X Y]` | 任意 | 手工构帧 |
| `discover` | 6 | 广播 0x06 发现 ping，列出回应车 |
| `keepalive on\|off` | — | 每 1s 重发最近帧（**off 可验证 3s 失联急停**） |

接收方向：后台线程打印紫派心跳 `state/x/y/r`（`state=3` 带坐标）与 0x06 发现回应（`car_id`）。

## 保活 / 急停验证

App 至少每 1s 发一次（哪怕空指令），否则紫派 3s 未收指令急停。`keepalive on` 后每 1s 重发最近帧；
`keepalive off` 停发 → 对端 >3s 后打印"急停"。

## 自检

```bash
python smoke_test.py        # 退出码 0 通过
```
白盒验证编解码（9 字节大端、负数、往返、cmd105 IP 拆段），黑盒起 `mock-purplepi` 子进程跑
**connect→收心跳 / goto→位姿推进 / 0x06 发现→回身份** 的最小协议闭环（当前 8/8）。
> ⚠️ 真机默认走广播 `255.255.255.255`；本机自检用单播 `127.0.0.1`（广播跨虚拟网卡不一定回环到 mock，
> 属网络环境问题，非 mock-app 逻辑）。发现 ping `0x06` 仍待成员A 确认（`contracts/integration-qa.md` Q5）。
