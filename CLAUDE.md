# CLAUDE.md — 给 Claude Code 的项目约定

本仓库由 3 人协作开发一台基于 OpenHarmony 的工业巡检机器人。三块代码运行在不同设备、
不同语言、互不编译依赖。**集成失败几乎都来自接口不一致**，因此协作的中心是 `contracts/`。

## 你（Claude）在本仓库工作时

- **改任何跨设备通信的代码前，先读 `contracts/` 下对应的契约文档**，并保证两侧逐字节/逐字段一致。
  若需要改协议，连同 `contracts/` 一起改，并在说明里提示"这是协议变更，需另一端同步"。
- **接口是两边共同设计的，不必一味迁就某一端的现有代码**：对成员A（紫派）/B（香橙派）有需求可**直接提**；
  他们那边也会按我们的代码改动来适配。目标是**两边同时推进、共建一个优美的项目**，而非单方面将就既有实现。
  需要更优的接口/协议时大胆提，在 `contracts/`（开放问题写进 `contracts/integration-qa.md`）里写明"这是协议变更/接口建议，
  需另一端同步"，异步沟通即可。**想了解 A 的实现，直接参考其分支 `origin/purplepi-control`**——含其完整代码
  （`Lidar/`、`Navi/`、`NewWheelCtrl/`）与权威 `接口功能与对接问题说明.md`。
- 明确当前所在子项目，别跨界改别人的设备代码（除非是对接接口）：
  - `app-harmony/` — ArkTS（鸿蒙 App）。owner 负责。
  - `purplepi-control/` — C/C++ + Python3（紫派 / OpenHarmony 5.0）。成员 A 负责。
  - `orangepi-vision/` — Python / C++（香橙派 / 昇腾）。成员 B 负责。
- **不要提交构建产物或大文件**：`oh_modules/`、`build/`、`.hvigor/`、`.preview/`、交叉编译出的
  `.so/.ko/.a`、模型权重（`.om/.pt/.onnx/...`）。这些已在 `.gitignore` 中。
- 模型权重、大段录制视频走外部存储或 Release 附件；要版本化再启用 Git LFS（见 `.gitattributes`）。

## 关键事实（避免重复发现）

- App↔紫派：UDP 端口 **5001**，自定义 **9 字节**二进制协议。详见 `contracts/udp-protocol.md`。
  字节布局与命令码已从 App 代码 `app-harmony/.../MakeData.ets`、`dataConstants.ets` 中提取核对。
- App 的 `robotState` 枚举值刻意对齐 ASCII（`fullpath_startRoute=102='f'` …），byte0 既是 App 状态
  也是下发给紫派的命令码。
- 地图：紫派 HTTP `:8000`（web 根=`/data/test`）暴露 `defultMap.txt`（URL **不带** `/data/test` 前缀）；文本 0=空旷/1=障碍、
  **密排单字符**，首行 `range resolution height width`（App 取**末两个**=行 列）。详见 `contracts/map-format.md`。
- 紫派内部用 LCM 信道通信（`ROBOT_CONTROL/PATH/POSE/CURRENTPOSE/...`）+ 命令号，见 `contracts/lcm/`。
- 香橙派→服务器→App 的视频/识别数据链路中，**"服务器"归属未定**，见 `contracts/server-api.md`。

## 风格

- 与所在子项目既有代码风格保持一致（命名、注释密度、缩进）。
- 文档与注释用中文，代码标识符用英文。
- 平台：开发机多为 Windows。脚本优先跨平台；必须区分时 `.ps1`(Windows) / `.sh`(设备/Linux)。
