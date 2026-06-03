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
 * ⚠️ 首行解析：旧 MakeCanvas 读 firstLine[2]=行数、firstLine[3]=列数（隐含首行有前导 token/空白，
 *    实际维度在第 3、4 个位置），而 contracts/map-format.md 记的是"首行为行列数"（2 个数）。这里改成
 *    "取首行最后两个整数为 行数、列数"——既兼容旧可运行版本的 4-token 首行（末两个即原 [2]/[3]），
 *    也兼容契约的 2-数首行。真实地图 fixture 到手后在 contracts 中核对定稿（见 contracts/fixtures）。
 */

import { http } from '@kit.NetworkKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { MAP_GRID_FREE, MAP_GRID_WALL } from '../constants/protocol';
import { MAP_READY_MIN_BYTES, MAP_POLL_INTERVAL_MS } from '../constants/ui';
import { MapTransform } from '../model/geometry';

/** 解析后的地图（纯数据，无画布依赖）。 */
export class ParsedMap {
  readonly rows: number;      // 地图行数
  readonly cols: number;      // 地图列数
  readonly startX: number;    // 包围盒（正方形化后）左
  readonly startY: number;    // 上
  readonly endX: number;      // 右
  readonly endY: number;      // 下
  readonly squareSize: number;
  readonly gridWidth: number;  // 每格像素宽（依画布）
  readonly gridHeight: number; // 每格像素高
  readonly gridSize: number;   // 平均格尺寸，供坐标换算
  readonly txtAverSize: number;
  private readonly lines: string[];

  constructor(rows: number, cols: number, startX: number, startY: number, endX: number, endY: number,
    squareSize: number, gridWidth: number, gridHeight: number, gridSize: number, txtAverSize: number,
    lines: string[]) {
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
    this.lines = lines;
  }

  /** 供坐标换算（geometry.canvasToMap / mapToCanvas）使用。 */
  toTransform(): MapTransform {
    return new MapTransform(this.startX, this.startY, this.endX, this.endY,
      this.gridSize, this.squareSize, this.txtAverSize);
  }

  /**
   * 遍历需要填充的障碍格子，回调其画布矩形（含 y 轴翻转）。供 MapCanvas 渲染，
   * 把绘制原语留给组件、把"画哪些格子"的数学留在服务层。移植自旧 MakeCanvas 的绘制循环。
   */
  forEachWallRect(cb: (px: number, py: number, w: number, h: number) => void): void {
    for (let y = 0; y <= this.squareSize; y++) {
      const row: string | undefined = this.lines[y + this.startY];
      if (row === undefined) {
        continue;
      }
      for (let x = 0; x <= this.squareSize; x++) {
        if (row[x + this.startX] === MAP_GRID_FREE) {
          continue; // 空旷不画
        }
        cb(x * this.gridWidth, (this.squareSize - 1 - y) * this.gridHeight, this.gridWidth, this.gridHeight);
      }
    }
  }
}

export class MapService {
  /** 拉取地图文本。HTTP GET，200 且非空才 resolve；否则 reject。 */
  static fetchMapText(url: string): Promise<string> {
    return new Promise<string>((resolve: (t: string) => void, reject: (e: Error) => void) => {
      const httpRequest: http.HttpRequest = http.createHttp();
      const options: http.HttpRequestOptions = {
        method: http.RequestMethod.GET,
        connectTimeout: 10000,
        readTimeout: 10000,
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
        console.error(`[MapService] poll error: ${e.message}`);
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
    const lines: string[] = text.split('\n');
    // 取首行最后两个整数为 行数、列数（见文件头 ⚠️ 注释）
    const headerNums: number[] = lines[0].trim().split(/\s+/)
      .map((t: string): number => parseInt(t, 10))
      .filter((n: number): boolean => !Number.isNaN(n));
    const height: number = headerNums[headerNums.length - 2]; // 行数
    const width: number = headerNums[headerNums.length - 1];  // 列数

    let yMin: number = Infinity;
    let yMax: number = -Infinity;
    let xMin: number = Infinity;
    let xMax: number = -Infinity;

    for (let y = 1; y < lines.length; y++) {
      const d: string = lines[y];
      if (d.includes(MAP_GRID_WALL)) {
        if (!Number.isFinite(yMin)) {
          yMin = y;
        } else {
          yMax = Math.max(yMax, y);
        }
        for (let x = 0; x < width; x++) {
          if (d[x] === MAP_GRID_WALL) {
            xMin = Math.min(xMin, x);
            xMax = Math.max(xMax, x);
          }
        }
      }
    }

    // 把绘制区域尽量正方形化
    const drawHeight: number = yMax - yMin + 1;
    const drawWidth: number = xMax - xMin + 1;
    const squareSize: number = Math.max(drawWidth, drawHeight);
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
      gridWidth, gridHeight, gridSize, txtAverSize, lines);
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
