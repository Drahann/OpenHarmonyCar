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
 *
 * ★ 坐标偏移修复（M1，2026-06-12）：紫派地图首行 `... metersPerPixel x0 y0`，`x0/y0` = 栅格最小角的**世界坐标(米)**
 *   （A12 确认、`MapServer.cpp` 实证）。心跳/UDP 世界坐标(5cm 单位)与栅格索引的关系是
 *   `grid = (world/单位每米 − x0)/metersPerPixel`，**不是** `grid == world`。旧实现把世界坐标直接当栅格索引，
 *   x0/y0≠0 时机器人 pin / 选点下发**整体偏一个常量**。现于 mapToCanvas/canvasToMap 边界做 world↔grid 换算。
 *   x0=y0=0（旧图/fixture）时退化为恒等，行为与旧版完全一致。
 */

import { COORD_UNITS_PER_METER } from '../constants/protocol';

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
  /** 每格 = 米（地图首行 metersPerPixel；缺省 0.05）。 */
  metersPerPixel: number;
  /** 栅格最小角的世界 X 坐标（米，可负；地图首行 x0）。world↔grid 偏移用。 */
  x0: number;
  /** 栅格最小角的世界 Y 坐标（米，可负；地图首行 y0）。 */
  y0: number;

  constructor(startX: number, startY: number, endX: number, endY: number,
    gridSize: number, squareSize: number, txtAverSize: number,
    metersPerPixel: number = 0.05, x0: number = 0, y0: number = 0) {
    this.startX = startX;
    this.startY = startY;
    this.endX = endX;
    this.endY = endY;
    this.gridSize = gridSize;
    this.squareSize = squareSize;
    this.txtAverSize = txtAverSize;
    this.metersPerPixel = metersPerPixel;
    this.x0 = x0;
    this.y0 = y0;
  }
}

/** 紫派世界坐标(UDP 5cm 单位) → 栅格索引：`grid = (world/单位每米 − x0米)/每格米`。x0/y0=0 时退化为恒等。 */
function worldToGrid(m: Point, t: MapTransform): Point {
  const mpp: number = t.metersPerPixel > 0 ? t.metersPerPixel : 0.05;
  return {
    x: (m.x / COORD_UNITS_PER_METER - t.x0) / mpp,
    y: (m.y / COORD_UNITS_PER_METER - t.y0) / mpp
  };
}

/** 栅格索引 → 紫派世界坐标(UDP 5cm 单位)：`world = (grid*每格米 + x0米)*单位每米`。worldToGrid 的逆（取整给 UDP）。 */
function gridToWorld(g: Point, t: MapTransform): Point {
  const mpp: number = t.metersPerPixel > 0 ? t.metersPerPixel : 0.05;
  return {
    x: Math.round((g.x * mpp + t.x0) * COORD_UNITS_PER_METER),
    y: Math.round((g.y * mpp + t.y0) * COORD_UNITS_PER_METER)
  };
}

/**
 * 屏幕地图坐标 → 真实世界坐标。
 * @param c           画布坐标（相对画布中心，单位 vp）
 * @param t           当前地图变换
 * @param halfViewport 画布半宽（旧 screenWidth_vpDiv2），运行时由 display 动态取得
 */
export function canvasToMap(c: Point, t: MapTransform, halfViewport: number): Point {
  // 先按既有几何还原**栅格索引**，再经 gridToWorld 转回紫派世界坐标（含 x0/y0 偏移、取整给 UDP）。
  const g: Point = {
    x: (c.x + halfViewport) / t.gridSize + t.startX,
    // y 取负：屏幕向下为正，栅格/世界向上为正
    y: -((c.y + halfViewport) / t.gridSize) + t.endY
  };
  return gridToWorld(g, t);
}

/**
 * 真实世界坐标 → 屏幕地图坐标（canvasToMap 的逆变换）。
 */
export function mapToCanvas(m: Point, t: MapTransform, halfViewport: number): Point {
  // 紫派世界坐标 → **栅格索引**（含 x0/y0 偏移）→ 既有画布换算。
  const g: Point = worldToGrid(m, t);
  return {
    x: Math.round((g.x - t.startX) * t.gridSize - halfViewport),
    y: Math.round((t.endY - g.y) * t.gridSize - halfViewport)
  };
}
