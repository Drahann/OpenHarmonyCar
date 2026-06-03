# verify — 纯逻辑即时验证（PC / Node）

在没有 DevEco Studio 或真机时，立即验证 App 里几个**纯算法**的正确性。

```bash
node tools/verify/verify.mjs
```

`verify.mjs` 是 `app-harmony` 里这些 `.ets` 纯函数的**逐行镜像**：

| 镜像源 | 验证内容 |
|---|---|
| `model/protocol.ets` `encodeSend/decodeReceive` | 9 字节大端编解码：固定向量、负数、往返 |
| `model/geometry.ets` `canvasToMap/mapToCanvas` | 坐标换算 `map→canvas→map` 互逆 |
| `service/MapService.ets` `parseMap` | 首行行列数解析、障碍包围盒、正方形化 |

数据用 `contracts/fixtures/defultMap.txt`。

> ⚠️ 改了上述 `.ets` 的算法，**请同步改 `verify.mjs`**，保持镜像一致——它不直接 import `.ets`
> （ArkTS 不能在 Node 跑），而是手工复刻同样的算式。正式回归仍以 `entry/src/test` 的 hypium 单测为准。
