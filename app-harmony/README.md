# app-harmony — 鸿蒙 App（ArkTS）

**负责人：本仓库 owner。** 平板上的鸿蒙应用：控制小车、取数据、地图与识别结果展示。
DevEco Studio + ArkTS + hvigor 构建。

> **状态（2026-06）**：按 [`../docs/app-refactor-plan.md`](../docs/app-refactor-plan.md) 完成**功能内核**重构
> （分层 + 去全局态 + 地道分布式）+ **UI 层 U1–U11 全量实现**（统一主题 token、动态屏幕、
> `MapCanvas/Joystick/DeviceList` 组件、单一参数化 `ControlPage`、`HomePage/SetIPPage`、路由入口）。
> ⚠️ **全部 ArkUI 代码未经 DevEco 真编译**——以 DevEco 构建 + 真机校验为准（进度见 `../docs/archive/ui-progress.md`）。
> 旧原型 `W:\CarApp\CarApp` 原封保留作行为参照，不在本仓库内。

## 架构（`entry/src/main/ets/`）

```
constants/   protocol.ets   端口/字节布局/命令偏移/保活超时/地图 HTTP（与 contracts 对齐，唯一来源）
             ui.ets         节流/速度/到点判定/地图渲染色等可调参数
model/       protocol.ets   RobotCommand 枚举 + UdpSendData/UdpReceiveData + encodeSend/decodeReceive
             geometry.ets   Point/MapTransform + canvasToMap/mapToCanvas（纯函数，可单测互逆）
             mission.ets    @Observed Mission/EndPoint/RobotRuntime/Assignment + 协同字段(phase/frame/map/area/assignments)；其快照 = contracts 的 FleetMission 黑板
service/     RobotTransport.ets   唯一 UDP socket：收发 + 单点 on('message') 分发 + **多目标** 1s 保活心跳
             MapService.ets       HTTP 拉图 + 解析 + 坐标变换参数（去全局 context/Txt2Canvas）
             FleetMissionService.ets 设备发现(networkId) + distributedDataObject 同步 FleetMission 黑板（**共享黑板，不再 startAbility 跨端拉起**）
             storage.ets          持久化（英文 key、getter 无副作用）
             theme.ets      统一主题 token（AppColor/FontSize/FontFamily/Space/Radius/Elevation/TOUCH_MIN；融合见 ../docs/archive/ui-design.md）
entryability/        EntryAbility        入口：初始化 Storage / RobotTransport / FleetMissionService；loadContent→HomePage
entrybackupability/  EntryBackupAbility  备份扩展（标准模板）
component/   MapCanvas.ets  地图渲染 + 缩放/平移 + 选点 + 多车位姿叠加（复用 MapService/geometry 纯函数）
             Joystick.ets   摇杆遥控，**每实例独立节流**（取代旧全局 taskId），产 (MoveDirection,speed) 回调
             DeviceList.ets 设备发现列表（方案B 广播发现+点击连接，配 RobotTransport.discover）
pages/       LoadingPage.ets  占位（入口已改指 HomePage，保留作启动占位）
             HomePage.ets     机器人列表 + 模式选择（修旧 onPageShow 累积 bug）→ 路由 ControlPage(mode,ip)
             ControlPage.ets  **单一参数化页**（mode∈{astar|fullpath|distributed}）组合三组件，取代旧 4 克隆页
             SetIPPage.ets    手填 IP 兜底（走 storage、isValidIp 校验）
utils/       componentUtils.ets PromptActionClass（弹窗助手）
             screen.ets         动态屏幕度量（display 取屏 + 懒加载缓存 + 方形地图画布边长/半视口派生，取代写死分辨率）
```

服务层与模型层**无 UI 依赖、可单测**。UI 层 `component/` + `pages/` 在其上装配，统一从 `constants/theme.ets`
取设计 token；图标用字形/自绘、配色用 token，故 `resources/media` 无需新增第三方资源。
⚠️ ArkUI 装配（组件/页面/手势）**未经 DevEco 编译**，待真机校验。

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
  App 内目标 IP 指向运行 mock 的机器；地图 URL 用 `http://<ip>:8000/defultMap.txt`（web 根=/data/test，无前缀）。
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
- 命令码 `5 / 107 / 108` 已与紫派对账（见 [`../contracts/udp-protocol-crosscheck.md`](../contracts/udp-protocol-crosscheck.md)：
  5=加载地图，107/108=覆盖矩形对角点1/2，旧 `120`=死命令）；**仍待 A** 的是坐标系 `frame`（单位/原点/0°方向）。
