# 地图格式契约 · 紫派 → App

**版本 v0.2（2026-06-05）** — 来源：设计文档 3.3.3 + App 解析代码 + 成员A《接口功能与对接问题说明.md》及 `purplepi-control` 源码确认。

## 文件与传输

- 紫派 SLAM 建图结果保存在 `/data/test/` 下。
- 紫派 `chdir("/data/test")` 后拉起 `python -m http.server`（端口 **8000**），**以 `/data/test` 为 web 根**暴露文件
  （`udp2lcm.c:40-50`）。故 App 拉取 URL = `http://<紫派IP>:8000/<文件名>`，**不带 `/data/test/` 前缀**。
- ✅ **文件名（A确认）= `defultMap.txt`**（单 .txt，`Navi/main.cpp:517/642` 实际保存）。URL = `http://<紫派IP>:8000/defultMap.txt`。
  `defultMap.txt.txt` 是历史残留，紫派启动即删（`main.cpp:1094-1096`）——勿用。
- 分布式：子机经 UDP `cmd105/'i'` → 紫派 `cmd124` 用 `wget` 从主机拉 **`defultMap.txt` + `roadFile.txt`**
  （`NaviInterface.cpp:4799-4818`，URL 无前缀）落到本机 `/data/test/`。
- 全息路径覆盖另会生成 `tmpcoverageMap.txt`（初始）、`coverageMap.txt`（最终）；分布式覆盖的路径文件为 `roadFile.txt`。

## 文本格式（App 侧解析依据）

```
<range> <resolution> <height> <width>   # 第一行 4 值（A确认，Navi/main.cpp 分片读取）；App 取末两个 = <height 行数> <width 列数>
000010...                                # 其后每行密排栅格字符（无分隔），行间换行
011000...
...
```

- `0` = 空旷可通行；`1` = 障碍物。**栅格为密排单字符**（早期示例的空格分隔仅为示意，实际无分隔），App 按字符索引解析。
- **首行 App 取末两个整数**作为 `行数(height) 列数(width)`——兼容旧 2-token 写法与 A 的 4-token `range resolution height width`
  （A 明确背书此折中；见 `app-harmony` `MapService.parseMap`）。地图一般近正方形，约 1800×1800 量级。
- App 解析时按障碍包围盒裁剪 + 两层遍历。

## 坐标系（共三套，换算在 App 侧）

1. **真实世界坐标系** — 来自地图数据文件（紫派建图产出）。机器人坐标、目标点都用它。
2. **屏幕地图坐标系** — 对全图做包围盒裁剪（找 `xmin/ymin/xmax/ymax` 取正方形）后、可平移缩放的显示坐标。
3. **终点选点坐标系** — 用户在屏幕上 touch 选点，先转屏幕地图坐标，再换算回真实世界坐标下发。

> **单位（✅ 已对账 `udp2lcm.c`）**：真实世界坐标 **1 单位 = 1/20 m = 5cm**，与地图格子 **1:1**（建图网格 0.05m）；
> 朝向 `r` = **度**，[-180,180]。UDP 心跳/目标点均用此单位（详见 `udp-protocol.md` 坐标单位节与 `udp-protocol-crosscheck.md`）。

> **原点与 0°（✅ A确认，《接口…说明》§六.2-3）**：地图原点 = 紫派**建图/定位初始位姿**；`theta=0` 指向 **+X**，
> 正角 **CCW** 朝 +Y。多机时子车用 UDP **`cmd 5`** 加载地图会把初始位姿**强制归零到 (0,0,0)**
> （`cmd 2/'j'/'l'` 则沿用当前心跳位姿，不归零）——故"子机从 master 起点出发"须经 `cmd 5`（见 `multi-robot-collab.md` §定位约定）。

> 换算实现见 App `canvas2map / map2canvas`。紫派侧只需保证：**地图文件格式稳定 + 心跳里的 x/y/r
> 与地图同一真实世界坐标系**。坐标系原点/朝向定义若有调整，必须在此文档同步。

## 已确认（成员A 2026-06-05，《接口功能与对接问题说明.md》+ `purplepi-control` 源码）

- [x] 坐标**单位 = 1/20 m = 5cm**，与格子 1:1。
- [x] `r`（朝向）= **度**，[-180,180]；`theta=0` = +X，正角 CCW 朝 +Y。
- [x] **原点 = 建图/定位初始位姿**；子机经 **`cmd 5`** 加载图归零到 (0,0,0)（`cmd 2/'j'/'l'` 沿用当前位姿，不归零）。
- [x] 地图**文件名 = `defultMap.txt`**（`.txt.txt` 启动即删，弃用）。
- [x] 地图**首行 = `range resolution height width`**（App 取末两整数为行列）。
- [x] 分布式地图传输 = **方案 B**（子机 `cmd124` wget 拉 `defultMap.txt` + `roadFile.txt`）；方案 A 需新增 agent 写文件能力后再实现。
