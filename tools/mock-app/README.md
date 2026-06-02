# mock-app — 假 App（给紫派用）

让紫派在**没有真平板**时验证 UDP↔LCM 桥与轮控。严格按
[`../../contracts/udp-protocol.md`](../../contracts/udp-protocol.md) 实现。

## 应实现的行为

- 向紫派 `172.168.11.99:5001`（可配）发送 9 字节指令：
  - 命令 0 建连 → 应能收到紫派心跳。
  - 命令 1 + `runState/speed` 遥控（支持持续发，模拟长按）。
  - 命令 2 结束建图、命令 3 目标点(endX,endY)、命令 4 取消导航。
  - 全息/分布式：102-106（'f'/'g'/'h'/'i'/'j'）。
- 接收并打印紫派心跳的 `state/x/y/r`，核对坐标回传。
- 至少每 1s 发一次（含空指令），验证保活；可故意停发 >3s 验证急停。

## 用法（实现后）

```bash
python mock_app.py --ip 172.168.11.99 --cmd go     # 发一条；或进入交互模式逐条发
```

> 让 Claude 生成实现：`参照 contracts/udp-protocol.md 写 mock_app.py（交互式发各命令并打印心跳）`。
