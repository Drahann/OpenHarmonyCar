/**
 * 地图服务：HTTP 拉图 + 文本解析 + 坐标变换参数计算。
 * 整合旧 utils/Pending.ets 的 getTxt/MakeCanvas 与 Common/common.ets 的坐标数学。
 *
 * 关键去全局化：
 *  - 不再持有全局 context: CanvasRenderingContext2D，也不再写全局 Txt2Canvas。
 *  - parseMap 接收画布尺寸为入参，产出纯数据 ParsedMap（含 MapTransform 与渲染所需的格子遍历），
 *    具体绘制交给 UI 组件 MapCanvas（UI 阶段）。
 *  - 用 pollMapUntilReady 的"显式轮询 + 可取消"取代旧 getTxt 里 toast「幻读」+ 递归重试的怪启发式。
 *
 * ⚠️ 首行解析：`defultMap.txt` 首行为 7 值 `range resolution height width metersPerPixel x0 y0`，
 *    **按位置取 `parts[2]`=height(行)、`parts[3]`=width(列)**（与旧可运行版 MakeCanvas 同位，A 源码
 *    `MapServer.cpp::saveProbMap` 实证 + A12 确认）。**绝不"取末两个"**——末两个是 `x0/y0`（世界坐标
 *    偏移、常为负），误取即把偏移当行列 → 空气图（这正是 2026-06 真机 bug 的根因之一）。首行非标准/
 *    缺失（如 fixture `40 40`）则回退按实际数据推断。详见 `docs/map-pipeline.md`。
 *    （注：A 接口文档 §9 仍写"取末两个"，那是 LCM 分片头 `range resolution height width` 4 值路径，
 *      与 App 拉的 7 值 HTTP 文件不同；已在 `contracts/integration-qa.md` 请 A 订正。）
 */

import { http } from '@kit.NetworkKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { MAP_GRID_WALL } from '../constants/protocol';
import { MAP_READY_MIN_BYTES, MAP_POLL_INTERVAL_MS } from '../constants/ui';
import { MapTransform } from '../model/geometry';
import { CoarseGrid } from '../model/mapContour';
import { Log, Logger } from '../utils/log';

const log: Logger = Log.scoped('MapService');

/**
 * 解析后的地图（纯数据，无画布依赖）。
 *
 * 栅格已**归一化**为 `grid[行][列]`（`1`=障碍 / `0`=空旷），屏蔽源文件两种格式（紫派 `MapServer.cpp::saveProbMap`）：
 *   - `defultMap.txt`（App 拉的就是它）：首行 7 值 `range resolution height width metersPerPixel x0 y0`
 *     + **空格分隔**数据，`-1`=障碍 / `0`=空旷；
 *   - `defultMap.txt.txt`：同首行 + **密排单字符**，`1`=障碍 / `0`=空旷。
 * 详见 `docs/map-pipeline.md`。`grid` 与原始行对齐：`grid[0]`=首行(头)占位空行、`grid[1..]`=数据行
 * （与包围盒扫描/`startY` 的行索引一致，保持与旧 App 同一坐标系）。
 */
export class ParsedMap {
  readonly rows: number;      // 地图行数（height，首行 parts[2] 或数据推断）
  readonly cols: number;      // 地图列数（width，首行 parts[3] 或数据推断）
  readonly startX: number;    // 包围盒（正方形化后）左
  readonly startY: number;    // 上
  readonly endX: number;      // 右
  readonly endY: number;      // 下
  readonly squareSize: number;
  readonly gridWidth: number;  // 每格像素宽（依画布）
  readonly gridHeight: number; // 每格像素高
  readonly gridSize: number;   // 平均格尺寸，供坐标换算
  readonly txtAverSize: number;
  private readonly grid: Uint8Array[];  // 归一化栅格，grid[行][列]：1=障碍 0=空旷
  // 墙体「水平行程」缓存（打包三元组 [croppedY, croppedXStart, len, ...]，cell 坐标，与画布尺寸/缩放无关）。
  // 惰性构建：首次 forEachWallSpan 时扫一遍 grid，之后每次重绘（缩放/平移）直接回放，避免反复全图扫描。
  private wallSpans: number[] | undefined = undefined;

  constructor(rows: number, cols: number, startX: number, startY: number, endX: number, endY: number,
    squareSize: number, gridWidth: number, gridHeight: number, gridSize: number, txtAverSize: number,
    grid: Uint8Array[]) {
    this.rows = rows;
    this.cols = cols;
    this.startX = startX;
    this.startY = startY;
    this.endX = endX;
    this.endY = endY;
    this.squareSize = squareSize;
    this.gridWidth = gridWidth;
    this.gridHeight = gridHeight;
    this.gridSize = gridSize;
    this.txtAverSize = txtAverSize;
    this.grid = grid;
  }

  /** 供坐标换算（geometry.canvasToMap / mapToCanvas）使用。 */
  toTransform(): MapTransform {
    return new MapTransform(this.startX, this.startY, this.endX, this.endY,
      this.gridSize, this.squareSize, this.txtAverSize);
  }

  /**
   * 遍历需要填充的障碍格子，回调其画布矩形（含 y 轴翻转）。供 MapCanvas 渲染。
   * 读归一化 grid（1=障碍）而非原始字符——屏蔽空格分隔/密排两种源格式。移植自旧 MakeCanvas 的绘制循环。
   */
  forEachWallRect(cb: (px: number, py: number, w: number, h: number) => void): void {
    for (let y = 0; y <= this.squareSize; y++) {
      const row: Uint8Array | undefined = this.grid[y + this.startY];
      if (row === undefined) {
        continue;
      }
      for (let x = 0; x <= this.squareSize; x++) {
        if (row[x + this.startX] !== 1) {
          continue; // 只画障碍格
        }
        cb(x * this.gridWidth, (this.squareSize - 1 - y) * this.gridHeight, this.gridWidth, this.gridHeight);
      }
    }
  }

  /**
   * 遍历墙体「水平行程」：把同一行内连续的障碍格合并成一条矩形，回调其画布矩形（含 y 翻转）。
   * 相比逐格 `forEachWallRect`：① 亚像素地图（1800 格压进 ~800vp、每格 < 0.5vp）下相邻墙格合并成连续墙面，
   * 消除发丝/缝隙的"简陋"观感；② fillRect 调用数从「墙格数」降到「行程数」，缩放/平移重绘更跟手。
   * 行程在 cell 坐标系首次构建后缓存，之后只做坐标×格尺寸的回放。供 MapCanvas 渲染墙体使用。
   */
  forEachWallSpan(cb: (px: number, py: number, w: number, h: number) => void): void {
    if (this.wallSpans === undefined) {
      const spans: number[] = [];
      for (let y = 0; y <= this.squareSize; y++) {
        const row: Uint8Array | undefined = this.grid[y + this.startY];
        if (row === undefined) {
          continue;
        }
        let x: number = 0;
        while (x <= this.squareSize) {
          if (row[x + this.startX] === 1) {
            const xStart: number = x;
            while (x <= this.squareSize && row[x + this.startX] === 1) {
              x++;
            }
            spans.push(y, xStart, x - xStart);  // [croppedY, croppedXStart, len]
          } else {
            x++;
          }
        }
      }
      this.wallSpans = spans;
    }
    const s: number[] = this.wallSpans;
    for (let i = 0; i < s.length; i += 3) {
      const y: number = s[i];
      const xStart: number = s[i + 1];
      const len: number = s[i + 2];
      cb(xStart * this.gridWidth, (this.squareSize - 1 - y) * this.gridHeight, len * this.gridWidth, this.gridHeight);
    }
  }

  /**
   * 把裁剪+正方形化后的栅格在 **display 朝向**（与 `forEachWallRect` 的 y 翻转一致）max-pool 成粗网格，
   * 供 `model/mapContour` 提取平滑矢量等高线（取代逐格方块渲染）。max-pool＝任一原格为墙则粗格为墙，
   * 天然降噪、连断点、缩规模。粗格坐标 (cx,cy) 还原到 base px 时 ×f×格尺寸即可（见 mapContour）。
   * @param maxLen 粗网格长边上限（如 240）；降采样因子 f = ceil((squareSize+1) / maxLen)。
   */
  toDisplayPool(maxLen: number): CoarseGrid {
    const ss: number = this.squareSize;
    const span: number = ss + 1;                      // forEachWallRect 的 0..squareSize 含端点
    const f: number = Math.max(1, Math.ceil(span / maxLen));
    const w: number = Math.max(1, Math.ceil(span / f));
    const h: number = w;                              // 正方形
    const b: Uint8Array = new Uint8Array(w * h);
    for (let dy = 0; dy < span; dy++) {
      // display 行 dy → 原裁剪行（y 翻转）：sy = startY + (ss - dy)
      const row: Uint8Array | undefined = this.grid[this.startY + (ss - dy)];
      if (row === undefined) {
        continue;
      }
      const cy: number = Math.floor(dy / f);
      for (let dx = 0; dx < span; dx++) {
        if (row[this.startX + dx] === 1) {
          b[cy * w + Math.floor(dx / f)] = 1;
        }
      }
    }
    return { b, w, h, f };
  }
}

export class MapService {
  /** 拉取地图文本。HTTP GET，200 且非空才 resolve；否则 reject。 */
  static fetchMapText(url: string): Promise<string> {
    return new Promise<string>((resolve: (t: string) => void, reject: (e: Error) => void) => {
      const httpRequest: http.HttpRequest = http.createHttp();
      const options: http.HttpRequestOptions = {
        method: http.RequestMethod.GET,
        connectTimeout: 30000,
        readTimeout: 30000,
        maxLimit: 50 * 1024 * 1024,
        usingCache: false,
        priority: 1000,
        expectDataType: http.HttpDataType.STRING
      };
      httpRequest.request(url, options, (err: BusinessError, data: http.HttpResponse) => {
        httpRequest.destroy();
        if (err) {
          reject(new Error(`map http error: ${err.message}`));
          return;
        }
        if (data.responseCode !== 200 || !data.result) {
          reject(new Error(`map http status ${data.responseCode}`));
          return;
        }
        const result: string | Object | ArrayBuffer = data.result;
        if (typeof result === 'string') {
          resolve(result);
        } else if (result instanceof ArrayBuffer) {
          resolve(MapService.arrayBufferToString(result));
        } else {
          resolve(JSON.stringify(result));
        }
      });
    });
  }

  /**
   * 轮询拉图，直到文本字节数 ≥ minBytes（认为 SLAM 已产出完整地图）才回调 onReady。
   * 返回一个取消函数。取代旧"幻读"toast + 递归 setTimeout。
   */
  static pollMapUntilReady(url: string, onReady: (text: string) => void,
    minBytes: number = MAP_READY_MIN_BYTES): () => void {
    let cancelled: boolean = false;
    let timer: number = -1;
    const schedule = (): void => {
      timer = setTimeout(tick, MAP_POLL_INTERVAL_MS);
    };
    const tick = (): void => {
      if (cancelled) {
        return;
      }
      MapService.fetchMapText(url).then((text: string) => {
        if (cancelled) {
          return;
        }
        if (text.length >= minBytes) {
          onReady(text);
        } else {
          schedule();
        }
      }).catch((e: Error) => {
        log.e(`poll error: ${e.message}`);
        if (!cancelled) {
          schedule();
        }
      });
    };
    tick();
    return (): void => {
      cancelled = true;
      if (timer !== -1) {
        clearTimeout(timer);
      }
    };
  }

  /**
   * 解析地图文本，按给定画布尺寸算出包围盒、正方形化、格尺寸与变换参数。
   * 移植自旧 MakeCanvas 的几何部分（去掉画布绘制副作用）。
   */
  static parseMap(text: string, canvasWidth: number, canvasHeight: number): ParsedMap {
    const rawLines: string[] = text.split('\n');
    // 归一化所有行为栅格：grid[0]=首行(头)占位空行、grid[y>=1]=数据行（与包围盒/startY 同索引系）。
    const grid: Uint8Array[] = [];
    for (let y = 0; y < rawLines.length; y++) {
      grid.push(y === 0 ? new Uint8Array(0) : MapService.parseRow(rawLines[y]));
    }

    // 首行**按位置**取 height/width（真机 7 值首行 `range resolution height width metersPerPixel x0 y0`；
    // 旧 App 即取 parts[2]/[3]）。注意：不能"取末两个"——末两个是 x0/y0（世界坐标偏移，常为负）。
    const headerParts: string[] = (rawLines.length > 0 ? rawLines[0] : '').trim().split(/\s+/);
    let height: number = MapService.toInt(headerParts[2]);
    let width: number = MapService.toInt(headerParts[3]);
    if (!(height > 0) || !(width > 0)) {
      // 回退：首行非标准/缺失（如 fixture 的 `40 40`）→ 按实际数据推断
      let maxCols: number = 0;
      let dataRows: number = 0;
      for (let y = 1; y < grid.length; y++) {
        if (grid[y].length > 0) {
          dataRows++;
          if (grid[y].length > maxCols) {
            maxCols = grid[y].length;
          }
        }
      }
      height = dataRows;
      width = maxCols;
    }
    log.i(`[parseMap] 首行="${headerParts.join(' ')}" → height=${height} width=${width} rows=${rawLines.length}`);

    let yMin: number = Infinity;
    let yMax: number = -Infinity;
    let xMin: number = Infinity;
    let xMax: number = -Infinity;
    for (let y = 1; y < grid.length; y++) {
      const row: Uint8Array = grid[y];
      const w: number = Math.min(width, row.length);
      let rowHasWall: boolean = false;
      for (let x = 0; x < w; x++) {
        if (row[x] === 1) {
          rowHasWall = true;
          if (x < xMin) {
            xMin = x;
          }
          if (x > xMax) {
            xMax = x;
          }
        }
      }
      if (rowHasWall) {
        if (!Number.isFinite(yMin)) {
          yMin = y;
          yMax = y;
        } else {
          yMax = Math.max(yMax, y);
        }
      }
    }
    // 无障碍格（空图/异常）→ 用整图包围盒兜底，避免 NaN 变换
    if (!Number.isFinite(xMin) || !Number.isFinite(yMin)) {
      log.w(`[parseMap] 未发现障碍格，按整图兜底（width=${width} height=${height}）`);
      xMin = 0;
      xMax = Math.max(0, width - 1);
      yMin = 1;
      yMax = Math.max(1, height);
    }

    // 把绘制区域尽量正方形化
    const drawHeight: number = yMax - yMin + 1;
    const drawWidth: number = xMax - xMin + 1;
    const squareSize: number = Math.max(1, Math.max(drawWidth, drawHeight));
    const difference: number = (drawWidth - drawHeight) / 2;
    if (difference > 0) {
      yMin -= Math.trunc(difference);
      yMax += Math.ceil(difference);
    } else {
      xMin += Math.trunc(difference);
      xMax -= Math.floor(difference);
    }

    const gridWidth: number = canvasWidth / squareSize;
    const gridHeight: number = canvasHeight / squareSize;
    const gridSize: number = (gridWidth + gridHeight) / 2;
    const txtAverSize: number = (width + height) / 2;

    return new ParsedMap(height, width, xMin, yMin, xMax, yMax, squareSize,
      gridWidth, gridHeight, gridSize, txtAverSize, grid);
  }

  /**
   * 一行原始文本 → 归一化栅格行（`1`=障碍 / `0`=空旷）。**自动识别**两种源格式：
   *   - 含空格 → 紫派 `defultMap.txt` 的空格分隔（`-1`=障碍、`0`=空旷、`2`=覆盖非障碍）：仅 `<0` 视为障碍；
   *   - 无空格 → `defultMap.txt.txt` 的密排单字符：`'1'`=障碍。
   */
  private static parseRow(line: string): Uint8Array {
    const t: string = line.trim();
    if (t.length === 0) {
      return new Uint8Array(0);
    }
    if (t.indexOf(' ') >= 0) {
      const toks: string[] = t.split(/\s+/);
      const cells: Uint8Array = new Uint8Array(toks.length);
      for (let i = 0; i < toks.length; i++) {
        cells[i] = parseInt(toks[i], 10) < 0 ? 1 : 0;  // 仅 -1 为障碍；0/2 为空旷
      }
      return cells;
    }
    const cells: Uint8Array = new Uint8Array(t.length);
    for (let i = 0; i < t.length; i++) {
      cells[i] = t[i] === MAP_GRID_WALL ? 1 : 0;  // 密排：'1'=障碍
    }
    return cells;
  }

  /** 安全 parseInt（NaN 透传，便于上层判定）。 */
  private static toInt(s: string | undefined): number {
    if (s === undefined) {
      return NaN;
    }
    return parseInt(s, 10);
  }

  private static arrayBufferToString(buf: ArrayBuffer): string {
    const bytes: Uint8Array = new Uint8Array(buf);
    let s: string = '';
    for (let i = 0; i < bytes.length; i++) {
      s += String.fromCharCode(bytes[i]);
    }
    return s;
  }
}
