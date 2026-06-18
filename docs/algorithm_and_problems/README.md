# algorithm_and_problems

OpenHarmonyCar 全覆盖算法与分布式双车的**思考与问题分析**文档集。代码层面只指出问题与方向，不含完整实现。

依据：`covering_alfor` 仓库 `origin/purplepi-control` 分支源码 + 根目录 `接口功能与对接问题说明.md`。

## 文档索引

| 文档 | 内容 | 关键结论 |
|---|---|---|
| [01-更优全覆盖算法-持续思考.md](./01-更优全覆盖算法-持续思考.md) | 由易到难 5 层演进：BCD → +TSP/方向自适应 → 真 Spiral-STC → frontier 在线+动静分层 → 双车空间分区 | 单车先做 BCD+TSP；双车改"空间分区"而非"路径对半" |
| [02-purplepi-control代码潜在bug.md](./02-purplepi-control代码潜在bug.md) | 通读源码的 bug 清单，按模块分组、带 `file:line` 与严重度 | 先修 `setValue` 越界、ttl 埋雷、robotId 默认 0 |
| [03-双车分布式建图与全覆盖bug分析.md](./03-双车分布式建图与全覆盖bug分析.md) | "为什么双车一起跑就乱"的 7 条系统性根因 + 因果图 | 坐标系不统一、对方成幽灵墙、roadFile 无单一事实源 是三大地基问题 |
| [04-数据传输提速.md](./04-数据传输提速.md) | 传输瓶颈定位与分层优化 | 删 MAPFILE 每行 50ms sleep + HTTP 开 gzip/多线程，建图上送几十秒→几秒 |

## 阅读建议
- 想**改算法** → 01 → 02 的 A 节。
- 想**修双车 bug** → 03（根因）配合 02 的 B/C/D 节（具体位置）。
- 想**提速** → 04。

## 备注
- `algNum` 已占用：`0`=牛耕、`1`=STC、`2`=分布式矩形覆盖；新算法（BCD）应从 `algNum=3` 起。
- 涉及地图格式 / laser 编码 / 新命令码的改动属**协议变更**，需按仓库 `CLAUDE.md` 在 `contracts/` 与 App/上层同步。
