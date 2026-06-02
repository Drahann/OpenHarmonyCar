# app-harmony — 鸿蒙 App（ArkTS）

**负责人：本仓库 owner。** 平板上的鸿蒙应用：控制小车、取数据、地图与识别结果展示。
DevEco Studio + ArkTS + hvigor 构建。原型代码在 `W:\CarApp\CarApp`。

## 迁移既有代码（首次）

从 `W:\CarApp\CarApp` 迁入，**排除构建产物**（robocopy 在 PowerShell 里运行）：

```powershell
robocopy W:\CarApp\CarApp W:\Project\OpenHarmonyCar\app-harmony /E `
  /XD oh_modules node_modules build .hvigor .preview .idea .cxx .test .git `
  /XF local.properties
```

迁入后：DevEco Studio 打开 `app-harmony/`，执行 `ohpm install` 还原依赖（`oh_modules/` 不进 git）。

## 对接的契约

| 契约 | 用途 |
|---|---|
| [`../contracts/udp-protocol.md`](../contracts/udp-protocol.md) | 与紫派的 UDP 控制/心跳（`MakeData.ets` 即此协议的实现） |
| [`../contracts/map-format.md`](../contracts/map-format.md) | 解析/渲染地图、坐标换算 |
| [`../contracts/server-api.md`](../contracts/server-api.md) | 读取处理后视频 + 识别结果展示（待立项） |

## 无真车时的开发

起本地假紫派：见 [`../tools/mock-purplepi/`](../tools/mock-purplepi/)。它按 9 字节协议回心跳、
托管示例地图。App 内把目标 IP 指向本机、`testMapUrl` 指向 mock 的 HTTP 即可。

## 构建 / 测试

- 构建：DevEco Studio 或 `hvigorw assembleHap`。
- 单测：`entry/src/test`（LocalUnit）与 `entry/src/ohosTest`（hypium，需真机/模拟器）。
- 关键源码：`Common/Socket.ets`(UDP)、`Common/MakeData.ets`(协议编解码)、`pages/`(界面)、
  `Common/distributed.ets`(分布式软总线)。
