# purplepi-control 代码潜在 bug 清单

> 基于 `origin/purplepi-control` 分支源码通读整理。每条给出**位置、现象、根因、修复方向**（不含完整实现）。
> 按模块分组，标注严重度：🔴 会崩溃/数据损坏 · 🟠 逻辑错误/功能失效 · 🟡 健壮性/性能 · ⚪ 设计局限。
>
> 路径以 purplepi-control 分支为准：`Navi/`、`NewWheelCtrl/`、`Lidar/`。行号为通读时所见，施工以实际为准。

---

## A. 全覆盖算法（planner）

### A1 🔴 `setValue` 覆盖标记越界访问
- **位置**：`Navi/planner/FullPathCoverage.cpp` `setValue`（约 323–344 行）
- **现象**：以当前点为中心 ±10 格（21×21）刷"已覆盖"，**无任何边界检查**：
  `coverageState[(y+j)*GMapWidth + (x+i)]`，机器人靠近地图边缘（<10 格）时越界读写堆内存 → 段错误/内存损坏。
- **根因**：`findNextGoal_*`、`ifIPointAroundLegal` 同类邻域扫描都做了边界检查，唯独**每步都调用**的 `setValue` 漏了。
- **影响**：牛耕(0)、STC(1)、分布式覆盖(2) **三套算法共用**此函数，都会触发。
- **修复方向**：循环内加 `if (nx<0||nx>=W||ny<0||ny>=L) continue;` 后再索引。

### A2 🟠 `while(distance<0.25)` 死逻辑（四处）
- **位置**：`Navi/planner/ZigzagCoverage.cpp` `detectFrontPlan/Back/Left/Right` 各一处（约 21、75、129、183 行）
- **现象**：`distance` 在循环外算一次，循环体只改 `tmppose` **不重算 distance** → 若条件成立即**死循环**。
- **根因**：漏写循环内 `distance = LinAlg::DistancePose(...)`。当前常量 `GGridSize=0.1` 下 distance=1.0m>0.25 侥幸不进循环。
- **修复方向**：循环内重算 distance；或直接简化（该平移距离恒为 `10*GGridSize`，本就不会 <0.25，逻辑可疑）。

### A3 🟠 STC 递归 DFS 栈溢出
- **位置**：`Navi/planner/STCCoverage.cpp` `traverseTreeDFS`（约 75–95 行）
- **现象**：递归遍历生成树，深度可达自由粗格总数（大图上万）→ 栈溢出崩溃。
- **修复方向**：改显式栈迭代 DFS，保持相同的"带回溯" `goPath` 序列（设计见旧仓库修复文档）。

### A4 🟡 STC 执行 `goPath.erase(begin())` O(n²)
- **位置**：`Navi/NaviInterface.cpp` STC 分支执行循环（约 6150 行）
- **现象**：`std::vector::erase(begin())` 每次 O(n)，整条路径 O(n²)，大图执行端卡顿。
- **修复方向**：改下标推进或 `std::deque::pop_front()`。

### A5 🟡 牛耕行距与覆盖宽不匹配 → ~50% 重复
- **位置**：换行横移 `Navi/planner/ZigzagCoverage.cpp::detectRightPlan`（`GGridSize*10`）vs `setValue` 印记 ±10 格
- **现象**：覆盖全宽 2.1m，换行只移 1.0m，相邻行重叠 ~50%。
- **修复方向**：换行横移调到≈覆盖全宽（~20 格），并把"覆盖半宽/行距"抽成同一具名常量。

### A6 ⚪ 房间识别仅支持凸四边形
- **位置**：`FullPathCoverage.cpp::isPointInsideRoom` + `manualGetRoomBoundary`（固定 4 顶点）
- **现象**：叉积"同侧"判定要求凸多边形且顶点定序，真实非凸房间误判内外。
- **修复方向**：改"射线交点奇偶法"支持任意多边形；或上 BCD 直接在占据图工作（见算法文档）。

---

## B. 分布式矩形覆盖（CoverageThreadProc `case 2`）

### B1 🟠 `size<2` 检查不生效（防御性失败）
- **位置**：`Navi/NaviInterface.cpp` 约 6217–6224 行
- **现象**：
  ```cpp
  if (gridPath.size() < 2) {
      printf("Path too short.\n");
      pObject->setLastFullPathError(FULLPATH_ROAD_FILE_INVALID);
      // ← 没有 break/continue，继续往下执行！
  }
  vector<IPoint> turnPoints;
  turnPoints.push_back(gridPath[0]);   // size==0 时越界
  for (int i = 1; i < gridPath.size() - 1; ++i) { ... }
  ```
  设了错误码却**不中止**，继续做拐点提取。当前靠上游 `fullPath.size()>=2` 间接保证 `gridPath` 非空，
  但本地检查形同虚设；一旦上游不变量被改，`gridPath[0]` 越界 / 下面循环下溢。
- **修复方向**：检查后立即 `break`（与上游 `fullPath<2` 的处理一致）。

### B2 🟡 有符号/无符号混用，潜在越界
- **位置**：`Navi/NaviInterface.cpp` 约 6226 行 `for (int i = 1; i < gridPath.size() - 1; ++i)`
- **现象**：`gridPath.size()` 是 `size_t`。当 size==0 时 `size()-1` 下溢成 `SIZE_MAX`，循环越界访问。
- **修复方向**：先保证 size≥2 再循环，或写成 `for (size_t i=1; i+1 < gridPath.size(); ++i)`。

### B3 🟠 路径对半切分的注释与 robotId 约定相反
- **位置**：`Navi/NaviInterface.cpp` 约 6210–6214 行
- **现象**：
  ```cpp
  if (robotId == 1) {
      subPath.assign(fullPath.begin(), fullPath.begin()+mid);  // 注释写"主机从前半段"
  } else {
      subPath.assign(fullPath.rbegin(), fullPath.rbegin()+(size-mid)); // 注释"副机从后半段"
  }
  ```
  但 `main.cpp:900` 与协议文档都定义 **0=主机、1=副机**。这里注释把 `robotId==1` 称作"主机"，自相矛盾。
- **影响**：维护者极易据错误注释把角色接反 → 两车都走同半段或都不走另一半。
- **修复方向**：统一 robotId 语义并更正注释；最好抽成 `enum {MASTER=0, SUB=1}`。

### B4 🟠 主/副读各自本地 `roadFile.txt`，可能不是同一份 → 覆盖不拼合
- **位置**：`Navi/NaviInterface.cpp` 约 6192–6202 行
- **现象**：只有 `robotId==1` 调 `CreateFullPath` 写**本地** `/data/test/roadFile.txt`；随后**两车各自**
  `ReadFullPathFromFile("/data/test/roadFile.txt")` 再对半切。对半切要正确**前提是两车读到完全相同的 fullPath**。
  但另一台车的本地 roadFile 来自何处？协议里子机靠命令 124 从**主机**拉 `roadFile.txt`（方向：主→子），
  而这里却是 `robotId==1` 生成——**生成方与拉取方向不一致**，主机很可能读到一份**旧的/不同的** roadFile。
- **影响**：两车基于不同路径切分 → 覆盖出现重叠或漏扫，是双车覆盖出 bug 的直接来源之一。
- **修复方向**：明确"谁生成、谁分发、谁消费"单一数据流；生成后用文件哈希/版本号校验两车一致，再开始执行。
  详见 `03-双车分布式建图与全覆盖bug分析.md`。

### B5 🟠 两车从两端相向而行，中点必然相遇
- **位置**：`Navi/NaviInterface.cpp` 6210–6214（前半 vs 后半逆序）
- **现象**：主取后半逆序（从尾走向中点），副取前半（从头走向中点）→ **两车朝同一中点收敛**，
  在中段高概率相遇，强依赖 `COOP_AVOID` 频繁让行。
- **修复方向**：改"空间分区"分解，让两车覆盖空间分离的区域（见算法文档第 5 层）。

---

## C. 分布式基础设施 / 协同避障（COOP_AVOID）

### C1 🔴 被注释掉的 ttl=1 主总线（埋雷）
- **位置**：`Navi/main.cpp` 975 行 `lcm = lcm_create(NULL);`；978 行注释 `// lcm = lcm_create("udpm://239.255.76.67:7667?ttl=1");`
- **现象**：主总线现用 `NULL` → 默认 `udpm://239.255.76.67:7667` 且 **ttl=0**（仅本机，车间隔离，正确）。
  但注释里那行 ttl=1 一旦被启用（或有人设 `LCM_DEFAULT_URL` 为 ttl≥1），两车主总线进入**同一组播组**，
  会互相收到对方的 `HOKUYO_LIDAR/POSE/CURRENTPOSE/ROBOT_CONTROL/PATH/wheel_ctrl` → 建图、定位、轮控全面串话。
- **修复方向**：删除该误导注释；若需车间监控，另开只读旁路而非抬高主总线 ttl。详见分布式文档。

### C2 🟠 `robotId` 运行时下发、默认 0 → 两车可能都当主机
- **位置**：`Navi/NaviInterface.cpp:326` `robotId = 0;`（默认）；`Navi/main.cpp:900` 由 `ROBOT_CONTROL.iparams[0]` 设置
- **现象**：robotId 不是开机就确定，而是等一条命令来设。若该命令丢失/延迟，两车都保持默认 0。
  此时 `handleCoopAvoidMessage` 的 `targetRobotId != robotId` 过滤、对半切分、停机让行 tie-break 全部失效。
- **修复方向**：robotId 改为开机参数/配置文件强制指定，启动时校验 0/1 唯一；未确定前不进入协同流程。

### C3 🟠 协同避障"同时互发停机"单会话状态竞争
- **位置**：`Navi/NaviInterface.cpp` `triggerCoopAvoidance`/`handleCoopAvoidMessage`（约 4753、4946 行）
- **现象**：同一车既可能是**发起方**（自己的 session，占用 `m_coopState/m_coopActiveSeq/m_coopActivePeer`），
  又可能是对方 session 的**响应方**。这些状态是**单组变量**，无法同时表达两种角色。两车几乎同时触发时，
  状态被互相覆盖，依赖 `robotId==1 续行 / 0 停` 的 tie-break 在状态错乱下不一定成立 → 可能双停或双不停。
- **修复方向**：把"我发起的会话"与"我响应的会话"分成两套独立状态；或用带 `(initiatorId, seq)` 主键的会话表。

### C4 🟡 协同序号 `seq` 用 int8、120 回绕，跨角色匹配脆弱
- **位置**：`Navi/NaviInterface.cpp` `m_coopSeq` 自增、`>120` 归 1，放入 `int8_t iparams`
- **现象**：seq 空间小且回绕；POSE_RESPONSE→STOP_REQUEST→STOP_ACK 链路里 seq 在不同角色间传递，
  高频触发或丢包重传时易发生**旧 seq 误匹配**。
- **修复方向**：seq 至少用更大范围 + 会话主键（见 C3）；对超时会话显式作废。

### C5 🟡 协同避障把"图匹配跳变"当"对方挡路"
- **位置**：`Navi/NaviInterface.cpp:925–933`，`m_ijumpnum>=6` 时 `triggerCoopAvoidance(MATCH_JUMP)`
- **现象**：图匹配跳变可能源于**定位漂移/动态障碍/对方车入侵激光**，并不等于"对方挡住我的路"。
  虽然 `peerLikelyBlocksCurrentRoute` 对 MATCH_JUMP 追加了激光证据校验，但仍会在定位本身抖动时误触发停车。
- **修复方向**：把"定位健康"与"协同让行"解耦；MATCH_JUMP 优先走重定位，而非协同停机。

---

## D. 建图 / SLAM

### D1 🟠 无动态障碍过滤 → 另一辆车被烤进静态地图
- **位置**：`Navi/map/`（占据更新流程，未见 log-odds / hit-miss / dynamic 过滤关键字）
- **现象**：占据栅格把任何激光回波当静态障碍。双车同场时，A 的激光打到 B（移动中）→ B 的轨迹被写成**幽灵墙**；
  全覆盖在该图上规划 → 把对方走过处当墙绕开 → 漏扫 + 误避障。
- **修复方向**：引入 log-odds 占据 + 时间衰减；或用 `COOP_AVOID` 已知的对方位姿，在建图时**掩膜**掉对方所在区域。
  详见分布式文档。

### D2 🟡 `laser_t` 字段被复用承载非语义数据
- **位置**：见 `接口功能与对接问题说明.md` §三.6：`intensities` 实际写**角度**、`ranges` 写**毫米**、`rad0/radstep` 为**角度制**
- **现象**：字段名与内容不符，跨模块/跨人协作极易误用（例如有人按"强度"过滤）。
- **修复方向**：要么改回语义一致，要么在契约里强标注并加封装函数，禁止裸字段访问。

---

## E. 通用 / 工程

### E1 🟡 多处忙等待空转
- **位置**：`Navi/NaviInterface.cpp` 如 `while(algNum==-1){;}`、`while(m_waypoints.size()>0){;}`
- **现象**：空循环吃满一个核（代码里 `pthread_cond_wait` 被注释，本意是条件变量）。
- **修复方向**：换条件变量/信号量，或循环内 `usleep`。

### E2 🟡 `algNum` 取值已被占用，新算法勿复用 2
- **位置**：`CoverageThreadProc` switch：`0`=牛耕、`1`=STC、`2`=分布式矩形覆盖
- **现象**：之前 BCD 设计文档建议 `algNum=2`，与现有 case 2 冲突。
- **修复方向**：BCD 用 `algNum=3` 起；并在协议文档登记。

### E3 🟡 启动期文件删除假设固定路径
- **位置**：`接口功能与对接问题说明.md` §二.2：Navi 启动删 `defultMap.txt` 等，依赖运行目录恒为 `/data/test`
- **现象**：若 `test.sh` 未 `cd /data/test` 就起 navigation，删除/读写落到错目录 → 拉图 404、覆盖输出找不到。
- **修复方向**：路径集中配置；启动时校验工作目录与文件可写。

---

## 优先级建议
1. **先 A1 / C1 / C2**（崩溃 + 串话埋雷 + 角色错乱，影响面最大）；
2. 再 A2/A3/B1/B2（崩溃/越界类）；
3. B3/B4/B5/D1（双车覆盖正确性，配合 `03` 文档一起改）；
4. 其余健壮性/性能项排期处理。

> 关联：双车场景的系统性根因分析见 `03-双车分布式建图与全覆盖bug分析.md`；通信瓶颈见 `04-数据传输提速.md`。
