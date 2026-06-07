/**
 * 坐标 / 画布纯类型 + 三套坐标系换算（无 UI、无全局态，可单测）。
 *
 * 三套坐标系见 contracts/map-format.md：
 *   1. 真实世界坐标（地图文件、机器人位姿、下发目标点）
 *   2. 屏幕地图坐标（包围盒裁剪成正方形后，可平移缩放的显示坐标）
 *   3. 终点选点坐标（屏幕 touch）
 *
 * 换算数学逐字移植自旧 Common/common.ets 的 canvas2map / map2canvas，
 * 但把原先依赖的全局 `Txt2Canvas` 与 `screenWidth_vpDiv2` 改为显式入参，
 * 因而 canvasToMap(mapToCanvas(p)) ≈ p 可被往返测试覆盖。
 */

export interface Point {
  x: number;
  y: number;
}

/**
 * 地图变换参数：由 MapService.parseMap 依据地图文本 + 画布尺寸算出，
 * 取代旧全局可变对象 Txt2Canvas（Txt2CanvasParaModel）。
 */
export class MapTransform {
  /** 障碍包围盒（真实世界栅格索引） */
  startX: number;
  startY: number;
  endX: number;
  endY: number;
  /** 单格在画布上的平均像素尺寸（依赖画布大小） */
  gridSize: number;
  /** 裁剪后的正方形边长（格数） */
  squareSize: number;
  /** (地图行数 + 列数)/2，用于半幅偏移 */
  txtAverSize: number;

  constructor(startX: number, startY: number, endX: number, endY: number,
    gridSize: number, squareSize: number, txtAverSize: number) {
    this.startX = startX;
    this.startY = startY;
    this.endX = endX;
    this.endY = endY;
    this.gridSize = gridSize;
    this.squareSize = squareSize;
    this.txtAverSize = txtAverSize;
  }
}

/**
 * 屏幕地图坐标 → 真实世界坐标。
 * @param c           画布坐标（相对画布中心，单位 vp）
 * @param t           当前地图变换
 * @param halfViewport 画布半宽（旧 screenWidth_vpDiv2），运行时由 display 动态取得
 */
export function canvasToMap(c: Point, t: MapTransform, halfViewport: number): Point {
  const halfTxt: number = t.txtAverSize / 2;
  return {
    x: Math.round((c.x + halfViewport) / t.gridSize + t.startX - halfTxt),
    // y 取负：屏幕向下为正，真实世界向上为正
    y: -Math.round((c.y + halfViewport) / t.gridSize + halfTxt - t.endY)
  };
}

/**
 * 真实世界坐标 → 屏幕地图坐标（canvasToMap 的逆变换）。
 */
export function mapToCanvas(m: Point, t: MapTransform, halfViewport: number): Point {
  const halfTxt: number = t.txtAverSize / 2;
  return {
    x: Math.round((m.x - t.startX + halfTxt) * t.gridSize - halfViewport),
    y: Math.round((t.endY - halfTxt - m.y) * t.gridSize - halfViewport)
  };
}
