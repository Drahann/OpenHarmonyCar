# 全覆盖算法 — 新算法设计：BCD 牛耕分解（Boustrophedon Cellular Decomposition）

> **目标**：替换/补充现有牛耕法（`algNum=0`）与 STC（`algNum=1`），提供一套**覆盖完整、重复率低、
> 支持任意形状房间**的全覆盖路径规划。本文给出算法原理、数据结构、实现步骤、复杂度、与旧算法对比、
> 集成方案与验证计划，供成员 A 施工。旧算法的修复另见
> [`全覆盖算法-旧算法修复说明.md`](./全覆盖算法-旧算法修复说明.md)。

## 0. 为什么选 BCD

现有两套算法的根本短板（详见修复说明 F5/F7 与前期评审）：

| | 牛耕法(0) | STC(1) | **BCD（本方案）** |
|---|---|---|---|
| 覆盖完整性 | 差（扫描方向写死、只前后往返） | 中（受降采样连通性限制） | **高（按障碍分解，逐子区扫净）** |
| 重复率 | ~50%（行距/覆盖宽不匹配） | ~100%（DFS 回溯，非真 STC 绕行） | **低（每条扫描线只走一次）** |
| 房间形状 | 仅凸四边形 | 任意（但窄道被降采样堵死） | **任意非凸、带内部障碍** |
| 规划方式 | 在线反应式（易撞细障碍） | 离线一次性 | **离线一次性，全局可预测** |
| 复杂度 | O(N) 反应式 | 规划 O(N)、执行 O(n²) | 规划 O(N)、执行 O(路径长) |

BCD 是工业扫地/割草机器人的主流方案：**用障碍把可行区域切成若干"无内部障碍"的子区（cell），
每个子区做标准弓字形（牛耕）覆盖，子区之间用最短路径连接**。它把"全局覆盖"这个难题拆成
"分解 + 子区简单覆盖 + 子区排序连接"三个好解决的子问题。

## 1. 输入 / 输出

- **输入**：栅格占据图。直接复用现有 `coverageState`（一维 `int*`，`y*W+x` 索引）：
  - `0 UNCOVERAGE_BUT_IN_ROOM`、`1 COVERGAED_AND_IN_ROOM` → **自由（可走）**
  - `2 OBSTACLE`、`3 OUTOF_ROOM`、`4 WARNINGAREA` → **占据（障碍）**
  - 也可直接读底层 `astarPlanner.m_pGridState`（`Occupied/Near_Obstacle/danger/neardanger` → 障碍）。
- **输出**：一条有序路径点序列 `std::vector<IPoint>`（栅格坐标），交给现有
  `GridToGlobal + setGoal` 逐点导航，与 STC 分支的执行段完全一致。

> 关键好处：**不需要"房间是凸四边形"的假设**，直接在占据图上工作，自然解决修复说明里的 F7。

## 2. 算法原理（三步）

### 第一步：单元分解（Cellular Decomposition，竖直扫描线法）

想象一条**竖直扫描线**从左到右（x = 0 → W-1）扫过地图。在每一列 x 上，自由格被障碍切成若干段
**连续竖直区间（slice）**。随扫描线右移，比较相邻两列的 slice 连通关系，会出现三类事件：

```
列 x-1        列 x
  │            │
 ┌─┐  自由     ┌─┐
 │ │  ──────▶  │ │   连续：slice 1→1 对应          → 同一个 cell 延续
 └─┘          ╱└─┘
              障碍开始
 ┌─┐         ┌─┐
 │ │  ──────▶│ │     IN 事件：1 个 slice 裂成 2     → 关闭 1 个 cell，新开 2 个 cell
 └─┘         └─┘
             └─┘
 ┌─┐         ┌─┐
 │ │ ╲       │ │     OUT 事件：2 个 slice 合成 1     → 关闭 2 个 cell，新开 1 个 cell
 └─┘  ──────▶└─┘
 ┌─┐         （障碍结束）
 │ │ ╱
 └─┘
```

判定规则（对第 x 列每个 slice 与第 x-1 列各 slice 的**行区间是否重叠**）：

- 当前 slice 恰好与上一列**1 个** slice 重叠，且那个 slice 也恰好只与当前**这 1 个**重叠 → **同一 cell 延续**；
- 上一列某 slice 与当前**多个** slice 重叠 → **IN/分裂**：关闭旧 cell，为每个新 slice 开新 cell；
- 当前某 slice 与上一列**多个** slice 重叠 → **OUT/合并**：关闭那几个旧 cell，开 1 个新 cell；
- 当前 slice 与上一列**0 个**重叠（障碍刚结束露出新空地）→ 新开 cell。

分解结束后，每个自由格都带一个 **cell ID**。每个 cell 的关键性质：**在它覆盖的每一列上，自由区间都是
单段连续的**——这正是它能用一次性弓字形扫净的原因。

### 第二步：子区内弓字形覆盖（Boustrophedon）

竖直分解 → 子区内做**竖直弓字形**：沿 x 方向按步距 `stride` 选列，每列从 `top(c)` 走到 `bottom(c)`，
下一列反向，形成"下→右移→上→右移→下…"的弓字：

```
cell 内（列 c0 c1 c2 …，每列上下端 = 该列自由区间）：
 c0   c1   c2   c3
 ↓    ↑    ↓    ↑
 │    │    │    │
 │    │    │    │
 ↓→→→→↑    ↓→→→→↑
```

- `stride`（列间距）= **机器人有效清扫宽度**（格数），与 `setValue` 印记宽一致——这从设计上根治了
  修复说明 F5 的"行距/覆盖宽不匹配"。
- 每条竖直线**只走一次**，cell 内零重复。

### 第三步：子区排序与连接

1. **排序**：cell 按其最小列 x 升序（扫描线天然从左到右），列相同按 top 升序。也可在 cell 邻接图上做
   贪心/DFS 得到更短的总转移路程（进阶可选）。
2. **连接**：上一个 cell 弓字终点 → 下一个 cell 弓字起点，用**栅格 BFS/A\* 最短路**在自由空间里连接
   （复用现有 `astarPlanner`）。这些连接段是"空驶"，不计入有效覆盖，但占比很小。

最终把"各 cell 弓字路径 + cell 间连接路径"首尾拼成一条全局 `std::vector<IPoint>`。

## 3. 建议数据结构

```cpp
struct Slice { int top, bottom; };                 // 一列上的一个竖直自由区间 [top,bottom]
struct Cell {
    int id;
    int xmin, xmax;                                 // cell 跨越的列范围
    std::map<int, std::pair<int,int>> colSpan;      // 列 -> (top,bottom)，弓字按列取上下端
};
// 分解产物：每个自由格的 cell 归属
std::vector<int> cellOf;        // 大小 = W*H，-1 表示障碍/未分配
std::vector<Cell> cells;
```

## 4. 复杂度

- **分解**：每列 O(H) 求 slice + O(slice 数) 匹配，全图 **O(W·H) = O(N)**，扫描一遍。
- **弓字生成**：每个 cell 的路径长正比于其面积，所有 cell 合计 **O(N)**。
- **cell 连接**：k 个 cell 做 k-1 次 BFS/A\*，每次最坏 O(N)，总 **O(k·N)**；k 通常很小（几个~几十个），
  实际接近 O(N)。
- **执行端**：路径用下标推进遍历，O(路径长)，无旧 STC 的 `erase(begin())` O(n²) 问题。

## 5. 与现有代码的集成方案

新增 `algNum = 2` 分支，**复用 STC 分支已有的执行框架**，改动集中、风险可控：

1. 新增类 `BCDCoverage`（`purplepi-control/Navi/planner/BCDCoverage.h/.cpp`），仿照 `STCCoverage` 风格：
   - `void decompose(const int* coverageState, int W, int H);`     // 第一步，产出 cells / cellOf
   - `void buildBoustrophedonPath(int stride);`                    // 第二步，每个 cell 生成弓字
   - `void connectCells(CAstar& astar);`                           // 第三步，BFS/A* 连接
   - `std::vector<IPoint> fullPath;`                               // 最终输出（等价 STC 的 goPath）
2. 在 `FullPathCoverage.h` 的 `FullCoverageAlg` 里加成员 `BCDCoverage bcdCoverage;`（与 `zigzagCoverage`/
   `stcCoverage` 并列）。
3. 在 `NaviInterface.cpp::CoverageThreadProc` 的 `switch(algNum)` 增加 `case 2:`：
   - 取房间/占据图 → `decompose` → `buildBoustrophedonPath(stride)` → `connectCells(astarPlanner)`；
   - 用与 STC 分支**相同**的执行循环遍历 `fullPath`：`GridToGlobal → iftargetlegalStatic →
     ifIPointAroundLegal → setGoal → setValue → pathIPoint.push`（用**下标推进**，别用 `erase(begin())`）；
   - 写 `coverageMap.txt` 复用 `writeCoverageMapToFile`。
4. App/UDP 侧若要选择该算法，新增一个命令码经 `NAVI_SetPlanFullPath(2)` 设置——**这是协议变更，
   需与 App owner 同步**，按 `CLAUDE.md` 约定在 `contracts/` 记一笔。先不接 App 也能用本地 demo 验证（见 §7）。

> 复用约束：执行循环里**务必先应用修复说明的 F1（`setValue` 边界检查）**，否则 BCD 在地图边缘同样会崩。

## 6. 参数

| 参数 | 含义 | 建议初值 | 说明 |
|---|---|---|---|
| `stride` | 弓字列间距 = 有效清扫宽度（格） | 与 `setValue` 印记一致（如 ~20 格/2.0m） | 决定重复率，需真机标定 |
| `freeStates` | 视为自由的状态集合 | `{0,1}` | 是否把 WARNINGAREA(4) 算障碍，按安全策略定 |
| `minCellArea` | 过滤碎 cell 的面积阈值 | 经验值 | 太小的 cell 不值得单独弓字，可并入邻接 cell |
| `connectPlanner` | cell 间连接用的规划器 | 复用 `astarPlanner` | BFS 或 A\* |

## 7. 验证计划（独立可运行 demo，先于集成）

为在不依赖激光雷达 / 完整机器人栈的情况下验证算法正确性与效率，**先做一个 standalone C++ demo**：

- **输入**：读现有 `coverageMap.txt` 格式（第 1 行 `L W`；第 2 行路径点，忽略；其后 L 行 ×W 列状态值），
  或内置一个合成房间（带 L 形墙 + 内部障碍）开箱即用。
- **流程**：跑完整 BCD（分解 → 弓字 → 连接），输出：
  - `bcd_path.txt`：有序路径点；
  - `bcd_vis.ppm`：可视化（自由=白、障碍=黑、不同 cell=不同底色、路径=红线、已覆盖=浅绿）；
  - 终端指标：**覆盖率%**、**重复率（总走格数 / 应覆盖格数）**、**转向次数**、**空驶占比**。
- **基线对比**：在同一张图上跑一遍"朴素整图弓字"（不分解，遇障碍跳过）作为基线，用数字证明 BCD 在
  **覆盖率更高、重复率更低**。

demo 验证通过后，再按 §5 把 `BCDCoverage` 集成进 `NaviInterface`（`algNum=2`），最后真机标定 `stride`。

> demo 不进设备构建，建议放独立目录（如 `purplepi-control/Navi/planner/bcd_demo/`）并在 `.gitignore`
> 忽略其编译产物。

## 8. 边界 case 与注意事项

- **多段 slice 的 cell**：分解保证单个 cell 每列单段连续；若占据图噪声导致一列里出现碎洞，先做一次
  形态学闭运算（或按 `minCellArea` 过滤）再分解，避免 cell 爆炸。
- **窄通道**：BCD 在原始分辨率工作，不像旧 STC 那样按 `ROBOTSIZE` 降采样，**不会把门口/走廊整段堵死**；
  但要确保机器人物理宽度能过——连接段用 `ifIPointAroundLegal` 校验。
- **不连通区域**：若房间被障碍分成互不连通的几块，连接段的 BFS/A\* 会失败 → 该块作为独立任务处理
  （或提示无法到达），不要让规划静默丢区。
- **方向选择**：竖直分解 + 竖直弓字是默认；若房间明显横长，可整体转置（按行分解 + 水平弓字）减少转向，
  作为进阶优化。

---

## 小结

BCD 用"按障碍分解 → 子区弓字 → 排序连接"三步，把旧算法的三大痛点（覆盖不全、重复率高、只支持矩形房间）
一次性解决，且**复用现有执行框架**、集成成本低。施工建议：**先按 §7 做独立 demo 用数据验证，再按 §5 集成为
`algNum=2`**；集成前务必先落地修复说明里的 F1（`setValue` 边界检查）。
