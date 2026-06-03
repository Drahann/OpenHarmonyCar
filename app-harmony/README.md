# app-harmony — 鸿蒙 App（ArkTS）

**负责人：本仓库 owner。** 平板上的鸿蒙应用：控制小车、取数据、地图与识别结果展示。
DevEco Studio + ArkTS + hvigor 构建。

> **状态（2026-06）**：按 [`../docs/app-refactor-plan.md`](../docs/app-refactor-plan.md) 完成**功能内核**重构
> （分层 + 去全局态 + 地道分布式）。**UI 层（组件/页面）待接入**——当前只有占位 `LoadingPage`。
> 旧原型 `W:\CarApp\CarApp` 原封保留作行为参照，不在本仓库内。

## 架构（`entry/src/main/ets/`）

```
constants/   protocol.ets   端口/字节布局/命令偏移/保活超时/地图 HTTP（与 contracts 对齐，唯一来源）
             ui.ets         节流/速度/到点判定/地图渲染色等可调参数
model/       protocol.ets   RobotCommand 枚举 + UdpSendData/UdpReceiveData + encodeSend/decodeReceive
             geometry.ets   Point/MapTransform + canvasToMap/mapToCanvas（纯函数，可单测互逆）
             mission.ets    @Observed Mission/EndPoint/RobotRuntime + 可序列化快照（协同同步用）
service/     RobotTransport.ets   唯一 UDP socket：收发 + 单点 on('message') 分发 + 1s 保活心跳
             MapService.ets       HTTP 拉图 + 解析 + 坐标变换参数（去全局 context/Txt2Canvas）
             DeviceCollabService.ets 设备发现/跨端拉起(networkId) + distributedDataObject 同步 Mission
             storage.ets          持久化（英文 key、getter 无副作用）
entryability/        EntryAbility        入口：初始化 Storage / RobotTransport / DeviceCollabService
robotrunability/     RobotRunAbility     跨端拉起目标：仅取 sessionId，状态走 distributedDataObject
entrybackupability/  EntryBackupAbility  备份扩展（标准模板）
pages/       LoadingPage.ets   占位首页（UI 阶段替换为 HomePage + 参数化 ControlPage）
utils/       componentUtils.ets PromptActionClass（弹窗助手，UI 阶段复用）
```

服务层与模型层**无 UI 依赖、可单测**。UI 阶段将在其上实现 `MapCanvas / Joystick / DeviceList`
组件与单一参数化 `ControlPage`，取代旧版 4 个克隆页面。

## 打开 / 构建

1. DevEco Studio 打开 `app-harmony/`，`ohpm install` 还原依赖（`oh_modules/` 不进 git）。
2. 签名：`build-profile.json5` 的 `signingConfigs` 故意留空（不提交密钥）。首次构建用
   **Project Structure → Signing Configs → 勾选 Automatically generate signature**（沿用
   bundle `com.example.carapp` 的调试证书）。
3. 构建：DevEco 或 `hvigorw assembleHap`。

## 测试 / 验证

- **ArkTS 单测**（`entry/src/test`，LocalUnit/hypium，需真机或模拟器）：协议编解码往返、坐标互逆、地图解析。
- **PC 上即时验证纯逻辑**（无需 DevEco/真机，镜像上述算法）：
  ```bash
  node ../tools/verify/verify.mjs
  ```
- **无真车联调**：起本地假紫派
  ```bash
  python ../tools/mock-purplepi/mock_purplepi.py     # UDP:5001 + HTTP:8000
  python ../tools/mock-purplepi/smoke_test.py        # 自检 mock
  ```
  App 内目标 IP 指向运行 mock 的机器；地图 URL 用 `http://<ip>:8000/data/test/defultMap.txt`。
  ⚠️ `MapService.pollMapUntilReady` 的就绪阈值是按真实 ~1800² 地图设的（`MAP_READY_MIN_BYTES`），
  用小样例地图联调时请直接调 `fetchMapText`/`parseMap` 或传入较小的 `minBytes`。

## 对接的契约

| 契约 | 用途 | 本侧实现 |
|---|---|---|
| [`../contracts/udp-protocol.md`](../contracts/udp-protocol.md) | UDP 控制/心跳 | `constants/protocol` + `model/protocol` + `service/RobotTransport` |
| [`../contracts/map-format.md`](../contracts/map-format.md) | 地图解析/坐标换算 | `service/MapService` + `model/geometry` |
| [`../contracts/server-api.md`](../contracts/server-api.md) | 视频/识别结果展示（待立项） | 留待 UI 阶段 `VisionService` |

## 与契约的待核对项（重构中浮现，需联调定稿）

- 地图首行格式：契约写"行列数"（2 数），旧代码读第 3/4 token。已折中为"取首行末两个整数"，
  待真实地图 fixture 核对（见 `MapService.parseMap` 注释与 `contracts/fixtures`）。
- 地图栅格行为**密排单字符**（解析按字符索引），契约示例用空格分隔仅为示意。
- 命令码 `5 / 107 / 108`（及旧代码出现过的未文档化 `120`）语义待与紫派确认。
