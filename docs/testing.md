# 测试与联合测试计划

**原则：每个人都要能在没有另外两台设备时，用 Mock / 录制数据独立开发自测。**
设备物理分散、凑齐困难，所以把"全系统联调"留到固定的**联调日**，平时靠 Mock 顶住。

## 三层测试

### ① 各自单测（每天 · 本机/自设备 + Mock 对端）— 杠杆最高
- **App**：用 `tools/mock-purplepi/`（PC 端假紫派）——按 9 字节协议收指令、回假心跳与假坐标、
  `http.server` 托管一张示例地图。90% 的 App 逻辑无需真车即可在模拟器/真机上开发。
  （App 已有 `judgmentPreview` 区分真机/模拟器、`testMapUrl` 可换 mock 地址。）
- **紫派**：用 `tools/mock-app/`（假 App 脚本）狂发各命令测 UDP↔LCM 与轮控；
  用 `lcm-logger` 录信道、`lcm-logplayer` 回放，单测导航/轮控。
- **香橙派**：用 `contracts/fixtures/` 里的样图/样片**离线**测推理，断言读数误差；上报部分 mock 掉服务器。

### ② 两两联调（每块完成后，先两台一组）
- App ↔ 紫派：UDP 控制 + 心跳坐标 + HTTP 拉地图。
- 紫派 ↔ 香橙派/服务器：视频 + 识别数据上报链路。
- 服务器 ↔ App：读取处理后视频 + 读数展示。

### ③ 全系统联合测试（联调日 · 全员 + 真车 + 同一局域网）
真车跑完整流程：建图 → 选点导航 → 全路径覆盖 → 仪表识别回传 → App 统一展示。
按 `docs/network.md` 配好网络后进行。

## 录制即回放（fixtures）

一次录制、长期复用，放 `contracts/fixtures/`：示例地图、UDP 抓包、LCM 日志、仪表样图+期望读数、
样例识别 JSON。视觉样本兼作**精度回归**基线。

## CI（GitHub Actions，明天建仓后加）

| 子项目 | CI 内容 |
|---|---|
| app-harmony | 构建 `.hap`；跑 hypium 单测（已有 `ohosTest`）；ArkTS lint |
| purplepi-control | 对协议解析等**可在 host 编译**的部分做编译/单测；交叉编译因自定义工具链通常本地做 |
| orangepi-vision | Python 单测；样图推理冒烟（CPU 回退）；读数误差断言 |
| contracts | 校验 `calib.schema.json` 合法；可加"协议字段对账"脚本 |

## 联调日检查清单

- [ ] 各端代码已合入 `main` 且各自单测通过。
- [ ] 契约版本一致（看 `contracts/README.md` 版本表）。
- [ ] 网络按 `docs/network.md` 就绪，设备互通。
- [ ] 现场录一份新 fixtures，回灌仓库供下次离线复测。
- [ ] 记录问题到 Issues，标注归属端。
