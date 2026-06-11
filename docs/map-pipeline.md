# 地图管线：格式 / 解析 / 坐标变换 / 渲染 / 定位（写清楚）

> 用户 2026-06-08：真机地图渲染/操作有大问题，要求**对照旧 App（能跑通成功渲染）+ 紫派底层代码（雷达↔地图 txt 强相关）**
> 把"坐标转换 / 图像渲染 / 定位处理 / 字符串解析"四块逻辑**写清楚**。本文是这四块的权威说明 + 本轮修复。
> 代码：`app-harmony/.../service/MapService.ets`（解析/渲染数据）、`model/geometry.ets`（坐标变换）、`component/MapCanvas.ets`（绘制）。
> 紫派来源：`origin/purplepi-control` 的 `Navi/map/MapServer.cpp`、`Navi/main.cpp`。旧 App：`W:\CarApp\CarApp-ETS-Refactor\...\features\map\`。

## 1. 真实地图文件格式（紫派写出，权威）

紫派 `cmd32 存图` → `NAVI_SaveMap(unprobdefultMap.txt)` 写未优化图 → `NAVI_OptimizeMap` → `createProbMap` →
**`MapServer::saveProbMap`（`Navi/map/MapServer.cpp:717`）一次写两个文件**：

| 文件 | 首行 | 数据 | 障碍 | App 拉的 |
|---|---|---|---|---|
| **`defultMap.txt`** | `range resolution height width metersPerPixel x0 y0`（**7 值**，空格分隔） | **空格分隔**整数，每个后跟空格 | **`-1`**=障碍、`0`=空旷 | ✅ 当前 `MAP_FILE_NAME` |
| **`defultMap.txt.txt`** | 同上 7 值 | **密排单字符**（无分隔） | **`1`**=障碍、`0`=空旷 | 旧 App 当年拉的就是它 |
| **`zipedMap.txt`** 🆕 | `ZMAP1` + 同上 7 值（在**第 2 行**） | 每行 `rowBitCount wordCount word0…`，64 格/无符号 64 位整数 | 位 **`1`**=障碍 | **App 现首选拉取**（`fetchMapPreferZiped`，BigInt 解压；见 `contracts/map-format.md`「压缩地图格式」） |

- `height`=行数(rows)、`width`=列数(cols)，在**首行第 3、4 个位置**（`parts[2]`、`parts[3]`）。
- `x0 y0`=地图栅格 `[0][0]` 的**世界坐标偏移**（真机常为负，如 `-45 -44`）。`metersPerPixel`≈0.05m/格。
- ⚠️ 证据：`saveProbMap` 对 `defultMap.txt` 写 `outFile << -1 ... << ' '`（空格分隔 -1）；对 `.txt` 写 `outFile << 1`（密排）。

**这解释了真机 bug 的根因**（两处格式错配）：
1. **首行**：App 旧逻辑"取末两个整数"→拿到的是 `x0 y0`（负偏移 -45 -44）当行列 → 负数 → 解析循环不执行 → 空气图。
2. **数据**：App 当 `密排 '0'/'1'` 解析，但 `defultMap.txt` 是**空格分隔 `-1/0`** → `line[x]` 逐字符索引错位、`'1'` 命中 `-1` 里的 `1` → 包围盒乱 → 变换乱 → 渲染/定位全错。

## 2. 字符串解析（`MapService.parseMap` / `parseRow`）—— 本轮已修

- **首行按位置取维度**：`parts[2]=height`、`parts[3]=width`（**对齐旧 App `MapParser.parseHeader`**，旧 App 取 `parts[2]/[3]`）。
  **绝不"取末两个"**（末两个是 x0/y0 偏移）。首行非标准（如 fixture `40 40`，parts[2] 缺）则**回退**按数据推断（行数 / 最宽行）。
- **数据行归一化**（`parseRow`）：自动识别——
  - 含空格 → 空格分隔（`defultMap.txt`）：`split(/\s+/)`，**仅 `<0`（即 -1）为障碍**，`0`/`2` 空旷（`2`=覆盖调试，非障碍）；
  - 无空格 → 密排（`.txt.txt`）：`'1'`=障碍。
  - 统一成 `grid[行][列]`：`1`=障碍 / `0`=空旷。`grid[0]`=首行(头)占位空行，`grid[1..]`=数据行（与包围盒行索引一致）。
- **包围盒**：扫 `grid[y][x]===1` 求 `xMin/yMin/xMax/yMax` → **正方形化**（短边向两侧扩）→ `squareSize`。
- 派生：`gridWidth=画布宽/squareSize`、`gridSize=(gridWidth+gridHeight)/2`、`txtAverSize=(width+height)/2`。

## 3. 坐标变换（`model/geometry.ets`，与旧 App `CoordinateTransform.ets` **逐式一致**）

三套坐标系：① 真实世界（心跳 x/y、目标点，单位 5cm=1格）② 地图数组（行列索引）③ 画布像素。变换（`half`=画布半边 vp）：

```
mapHalf = txtAverSize / 2
mapToCanvas: cx = round((mapX - startX + mapHalf) * gridSize - half)
             cy = round((endY - mapHalf - mapY) * gridSize - half)   // y 轴翻转
canvasToMap: mapX = round((cx + half) / gridSize + startX - mapHalf)
             mapY = -round((cy + half) / gridSize + mapHalf - endY)
```

- 以**包围盒**（startX/endY）+ `txtAverSize/2` 为中心把地图摆到画布中央；y 翻转（栅格行向下、世界 y 向上）。
- **关键：变换用的是"地图数组坐标系"**（startX 是列索引）。心跳 x/y 必须在**同一坐标系**才能落对位置——见 §5。

## 4. 图像渲染（`ParsedMap.forEachWallRect` + `MapCanvas`）

- `forEachWallRect`：遍历 `grid[y+startY][x+startX]===1` 的障碍格 → 回调画布矩形 `(x*gridWidth, (squareSize-1-y)*gridHeight, gridWidth, gridHeight)`（含 y 翻转）。**读归一化 grid，不再逐字符**——屏蔽空格/密排差异。
- `MapCanvas`：障碍格填充 + 叠加层（机器人 pin+朝向、目标点、子区域矩形）按 `mapToCanvas` 定位；双指缩放/平移走 Canvas ctx 变换，叠加层按 1/scale 反缩放。

## 5. 定位处理（机器人 pin / 选点）—— ✅ A 已答 Q12：x0/y0 单位=米

- 机器人 pin：心跳世界坐标 `(x,y)`（5cm 单位）→ `mapToCanvas` → 像素。选点：像素 → `canvasToMap` → 下发 `cmd3(endX,endY)`（紫派 ÷20=米）。
- **旧 App 不用 x0/y0** 也能跑对 → 说明当年地图 `x0≈0`（SLAM 从地图原点起）。**但真机现在首行 `x0 y0` 非 0**：地图数组坐标 = 世界坐标相对**地图最小角 (x0,y0)** 的偏移，不校正则机器人 pin / 选点系统性偏移 = "操作有大问题"的**疑似第二因**。
- **✅ A 答 Q12（2026-06-08，权威，见 `contracts/integration-qa.md` §A12）**：`x0/y0` **单位 = 米**（不是格！），含义 = 地图栅格**左下角 / 最小 (x,y) 的世界坐标**，**不是建图起点、可为负**。世界↔栅格换算：

  ```text
  grid_x = (world_x_米 - x0) / metersPerPixel
  grid_y = (world_y_米 - y0) / metersPerPixel
  ```

  - ⚠️ **修正本节旧假设**：早前以为「`数组列 = 世界格 - x0`」（把 x0 当**格**）。A 确认 x0 是**米**，故偏移量（格）= `x0 / metersPerPixel`（`metersPerPixel`≈0.05），**不是 x0 本身**。
  - App 心跳坐标若是 **5cm 格**：`mapX格 = 心跳x格 - x0/metersPerPixel`、`mapY格 = 心跳y格 - y0/metersPerPixel`；反向（地图点→下发目标）`world_米 = grid·metersPerPixel + x0`，再 ÷0.05 转 5cm 单元打进 `cmd3`。
- **落地状态**：公式已明确，**代码校正待落实**（按 owner 选择，本轮先不改码）。改时动 `geometry`（世界↔数组加 `x0/metersPerPixel` 偏移项）+ `MapCanvas`/`onPick`，并真机用 1 个已知点核对符号（x0/y0 取自紫派 `MapServer.cpp` 的 `globalProbMap`）。`parseMap` 已**打印首行**（含 x0/y0），真机可直接读。

## 6. 本轮修复 + 验证 + 待确认

- ✅ `MapService.parseMap`：首行按位置 `parts[2]/[3]` + 数据 `parseRow` 归一化（空格 -1/0 与密排 1/0）+ 整图兜底；`ParsedMap` 改存归一化 `grid`、`forEachWallRect` 读 grid===1。
- ✅ `ControlPage.mapLooksComplete`：按首行声明行数 `parts[2]` + 非空数据行 ≥ 90% 判就绪（容忍 cmd2 落盘期尾部缺失）。
- ✅ `tools/verify/verify.mjs`：镜像同步 + **新增真机格式用例**（7 值首行 + 空格 -1/0），防回归"取末两个 / 把 -1 当密排"。**30/30**。⚠️ ArkTS 未经 DevEco。
- **待 A（→ integration-qa）**：① App 应拉 `defultMap.txt`（空格 -1/0）还是 `.txt.txt`（密排 1/0）？建议统一 `defultMap.txt`、App 已能解两种。② `x0/y0` 单位（米/格）与符号约定，供 §5 定位校正。
