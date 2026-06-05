# mock-purplepi — PC 端假紫派（给 App 用）

让 App 在**没有真车**时也能完整联调控制与地图流程。严格按
[`../../contracts/udp-protocol.md`](../../contracts/udp-protocol.md) 实现。

## 应实现的行为

1. **UDP 服务**：绑定 `0.0.0.0:5001`。
   - 收到 App 命令 0（建连）后，记录来源 IP，开始每 **1s** 向其回 9 字节心跳。
   - 心跳里 `byte0=3` 时带坐标：在 `x/y/r`(byte3-8, int16 大端) 填模拟坐标，可做缓慢移动轨迹。
   - 收到命令 1（运动）解析 `runState/speed`，命令 3（目标点）解析 `endX/endY`，打印出来便于核对。
   - 可选：超过 3s 没收到 App 指令就打印"急停"，验证保活逻辑。
2. **HTTP 服务**：`:8000` 托管一张示例地图（取 `contracts/fixtures/defultMap.txt`），
   URL 路径 `/defultMap.txt`（紫派 web 根=/data/test，URL 无前缀），供 App `createHttp` 拉取。

## 用法（实现后）

```bash
python mock_purplepi.py            # 默认 UDP:5001 + HTTP:8000
```
App 内把目标 IP 设为运行本脚本的机器 IP，`testMapUrl` 指向 `http://<ip>:8000/...`。

> 让 Claude 生成实现：`参照 contracts/udp-protocol.md 与 map-format.md 写 mock_purplepi.py`。
