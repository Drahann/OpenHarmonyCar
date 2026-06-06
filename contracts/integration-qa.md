# 集成对接 Q&A（App ↔ 紫派，跨端异步备忘）

> 本文给**两侧的人和 AI 异步对接**用：App 侧（owner，`app-harmony/`）与紫派侧（成员A，`purplepi-control/`）。
> 谁有结论/问题就追加带日期的小节，**改了接口先改 `contracts/` 再改两侧代码**（CLAUDE.md 约定）。
> 紫派侧权威接口说明见 `purplepi-control/接口功能与对接问题说明.md`；本文是 App 侧的对账回应 + 开放问题。

---

## 2026-06-05 · App 侧：已据 A 的文档收口（FYI，无需 A 回复）

读了 A 的《接口功能与对接问题说明.md》(`origin/purplepi-control` 的 `1608796`/`59fc335`) + 源码，App 侧据此已改：

1. **地图文件名 bug 修复**：App 之前误取 `defultMap.txt.txt`（双 .txt），会拉图失败。
   按 A 确认（`Navi/main.cpp:517/642` 实际保存 `defultMap.txt`，`.txt.txt` 启动即删 `:1094-1096`），
   已改 `MAP_FILE_NAME = 'defultMap.txt'`，URL = `http://<紫派IP>:8000/defultMap.txt`（web 根=/data/test，无前缀）。
   mock/smoke/契约一并更正。
2. **坐标系收口**：`frame` = 单位 5cm/格、原点=建图初始位姿、`r`=度 [-180,180]、`theta=0`=+X、正角 CCW。
   写入 `app-harmony/.../model/mission.ets` 的 `Frame`/`defaultFrame()` 与 `contracts/map-format.md`。
3. **子机归零**：多机子车入场用 **`cmd 5`** 加载图即归零 (0,0,0)；App/agent 流程按此走（不用 `cmd 2/'j'/'l'`，那几个沿用当前位姿）。
4. **地图传输 = 方案 B**（`cmd124` wget 拉 `defultMap.txt`+`roadFile.txt`）。软总线方案 A 留作后续 agent 增强。
5. **子区域 = 单个轴对齐矩形**（2 对角点 + robot_id，`x1≠x2 ∧ y1≠y2`），时序 `cmd107` 先于 `cmd108`。App `Assignment` 模型与此一致。
6. **协同避障（`59fc335`）**：确认是 `Navi` 内独立 LCM 频道 `COOP_AVOID`、**未改 `udp2lcm`** → **App↔紫派 9 字节 UDP 协议不变，App 无需改协议/命令**。

契约已升版：`map-format.md` v0.2、`multi-robot-collab.md` v0.4、`udp-protocol-crosscheck.md` 补 §十。

---

## 2026-06-05 · App → 紫派：开放问题（请 A 侧确认/答复）

### Q1【建议加协议】协同避障的"暂停"状态如何让 App 可见？

协同避障时被让车会**自主暂停**数秒（保存目标 → `PATH v=0,w=0` → 恢复）。但**当前 9 字节心跳没有"暂停/避让"标志位**
（`byte0=3`，`byte[1..2]` 保留=0，`byte[3..8]`=x/y/r）。平板仅凭心跳**无法区分**"避让暂停"与"缓行"与"卡死/掉线"，
可视化容易把正常避让误报为故障。

- **提议**：在心跳里留一个**状态字节**（如把保留的 `byte[1]` 用作运行状态码：0=正常/1=避让暂停/2=到达/3=异常…）。
  这是 **`udp2lcm` 心跳编码的小改动**（协议变更，需回写 `udp-protocol.md`）。
- 或者：紫派让 agent 能读到避让态（如 agent 订阅 `COOP_AVOID` 或 `Navi` 暴露一个查询），由 agent 写进软总线 `FleetMission.robots[].status`。
- **请 A 选一种**（或说明无需，App 就不显示避让态）。App 这边的 `RobotStatus` 已预留 `error` 等，可加 `avoiding/paused`。

### Q2【确认即可】支持几台车？协同避障的 tie-break 在 >2 车时如何处理？

避障文档按**双车**描述，互请停机时 `robotId=1` 继续、`robotId=0` 停等。
App 的 `FleetMission.assignments/robots` 支持 N 台车（每台一个矩形子区域）。

- 当前系统是否**只支持 2 车**？若支持 >2，避障 tie-break（谁让谁）如何定？
- 若暂时只支持 2 车，App 侧先按 2 车做联调即可，请确认。

### Q3【确认即可】`roadFile.txt` 是否需要 App 关心？

`cmd124` 子机会同时拉 `defultMap.txt` + `roadFile.txt`。App 目前只拉 `defultMap.txt` 做**显示**。
理解 `roadFile.txt` 是**紫派内部**的覆盖路径文件、App 无需读取/展示——请确认（若 App 也该可视化覆盖路径，请说明其格式）。

### Q4【信息同步】方案 A（软总线传地图）若要做，agent 落地路径

将来若做方案 A（demo 亮点：地图经软总线同步），agent 需把整图文本写到紫派栈读取的路径。
按现状应写 `/data/test/defultMap.txt`（紫派 `cmd10` 加载处）。请确认写文件后紫派是否需要额外触发（还是下一次 `cmd 5/10` 加载即可读到）。

### Q5【提案，待 A 确认/实现】局域网设备发现，免手动输入 IP

App 想在同一局域网/热点下**自动发现车、点击即连**，不再手填 IP。理由：`udp2lcm` 收首包才知 App IP，App 又得先知车 IP → 死循环；发现可破之。App 侧已定走 **UDP 广播/组播探测**（见 `docs/app-refactor-plan.md` §连接与设备发现）。需要 A 这边确认/配合：

1. **`udp2lcm` 能否收子网广播**（App 发 `255.255.255.255:5001`）或加入某**组播组**？若都不行，App 只能退回手填 IP / 子网扫描。
2. **新增一个"发现 ping"**——别拿 `cmd 0` 凑合：`cmd 0` 会让车进入受控态并武装 3s 急停，广播一发就连上一片车、随后集体超时急停。提案（9 字节、大端，与现协议同框）：

   | 方向 | byte0 | byte1 | byte2 | byte3..8 | 紫派行为 |
   |---|---|---|---|---|---|
   | App→广播 发现请求 | `0x06` | 0 | 0 | 0 | 探测在线车 |
   | 车→App 发现响应 | `0x06` | `robot_id`/车号 | 状态(0=空闲…) | 可选当前位姿 x,y（或 0） | **仅回一帧标识；不记为受控客户端、不武装 3s 急停、不发 LCM** |

   **关键诉求**：发现响应**不能**让车进入"已连受控"状态（否则没被选中的车会 3s 急停）。`0x06` 仅举例（现协议 0-5、102-108 已占用，6 空闲），最终码听 A 定。

3. 若 A 更愿意走 **mDNS**（紫派跑 responder 广告 `_inspbot._udp`，App 用 `@ohos.net.mdns` 浏览）也行——与 1/2 二选一，看哪个省事。

A 确认后，把最终发现命令写进 `udp-protocol.md`（协议变更）。
