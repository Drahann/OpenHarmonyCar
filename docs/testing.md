# 测试与联合测试计划

**原则：每个人都要能在没有另外两台设备时，用 Mock / 录制数据独立开发自测。**
设备物理分散、凑齐困难，所以把"全系统联调"留到固定的**联调日**，平时靠 Mock 顶住。

## 三层测试

### ① 各自单测（每天 · 本机/自设备 + Mock 对端）— 杠杆最高
- **App**：用 `tools/mock-purplepi/`（PC 端假紫派）——按 9 字节协议收指令、回假心跳与假坐标、
  `http.server` 托管示例地图。多数 App 逻辑无需真车即可开发/自测。
  （重构后 `app-harmony`：目标 IP 走 `service/storage`、传输走 `RobotTransport`、地图走 `MapService`；
  连接方式正从"手填 IP"转向"局域网发现+点击连接"——见 `app-refactor-plan.md` §连接与设备发现。
  **App 侧按当前进度的细化测试计划见文末「App 侧细化」。**）
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

---

## App 侧细化（模拟 → 实机 · 2026-06-06）

> 上面三层是全队框架；这里按 **App 当前进度**细化，并定模拟测试工具。

**现状可测边界**：服务/模型/契约层 ✅（`verify.mjs` 17/17 + mock）；**UI 层为空**（仅 `LoadingPage`），`.ets` 未进 DevEco。
→ 现在不能"整 App 点测"（等 UI）；**现在能测 = 协议层（`mock-app` ↔ `mock-purplepi`）+ 服务层逻辑（verify/hypium）**。

**对应到上面三层**：① 各自单测 = `verify.mjs`(纯逻辑) + hypium(DevEco) + `mock-app`↔`mock-purplepi`(协议联调，**无需 App UI**)；② 两两联调 = App↔`mock-purplepi`(等 UI) / `mock-app`↔真紫派；③ 联调日 = App↔真车（单→多）。

**工具状态**：
| 工具 | 状态 | 作用 |
|---|---|---|
| `tools/verify/verify.mjs` | ✅ | 纯逻辑镜像（改算法须同步） |
| `tools/mock-purplepi/` | ✅（升级 2026-06-06） | 假紫派：心跳/坐标 + HTTP 地图 + 发现 + 105-108 覆盖模拟 + roadFile + 生成大图 + 参数化 |
| `tools/mock-app/` | ⛏ 待建 | PC 命令驱动器（发命令/打印心跳/保活急停/脚本）；A 也用 |
| `tools/mock-purplepi/mock_fleet.py` | ⛏ 待建 | 多车（见下绑定） |

**两个必读坑**：
1. **地图就绪阈值** `MAP_READY_MIN_BYTES = 324e4`（~3.24MB，`constants/ui.ets`）按真实 ~1800² 图设；小 fixture（40×40）过不了 `pollMapUntilReady`。→ 模拟用 `mock_purplepi --gen-map 1800x1800` 生成大图，或测 `parseMap` 时直接喂小图绕过。⚠️ 阈值待真图校准。
2. **多车同端口 5001 需多 IP**：Linux `127.0.0.2/3…` 原生；**Windows 默认只 127.0.0.1**（需 loopback 别名/网卡多 IP，建议 WSL/Linux 或真 LAN）；退而用多端口会破坏广播发现。`mock_fleet` 两模式可选。

**发现**：方案 B（局域网发现+点击连接）等 A 答 `integration-qa.md` Q5；模拟先 `cmd0` 广播兜底 + 提案 `0x06` ping。公共热点 AP 隔离会挡广播 → 退回手填 IP。

**模拟 runbook（PC，现在可做协议层）**：
```bash
python tools/mock-purplepi/mock_purplepi.py --id 1 [--gen-map 1800x1800]
python tools/mock-app/mock_app.py --ip 127.0.0.1      # 待建：发命令、看心跳/急停
node tools/verify/verify.mjs ; python tools/mock-purplepi/smoke_test.py
```

**实机 runbook（真车）**：先按 `network.md` 配网/同热点、确认车 IP（或发现）。
单车：建连 `0` → 建图 `0..2` → 拉图（`:8000/defultMap.txt`）→ 导航 `3` → 取消 `4` → 加载 `5`(归零) → 停发 >3s 验急停。
多车：发现/手填 → 各连 → master 建图 → 划矩形 `cmd107` 先 `cmd108` 后 → 各车覆盖；**协同避障会让车自主暂停（非卡死）**。
异常/抓包存 `contracts/fixtures/`，与契约不符处回写 `contracts/` + `integration-qa.md`。
