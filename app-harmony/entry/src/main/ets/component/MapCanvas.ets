/**
 * MapCanvas —— 可复用地图组件（取代旧 4 个克隆页里各抄一份的画布逻辑）。
 *
 * 职责：把 SLAM 栅格地图画在 Canvas 上，叠加机器人位姿/朝向、目标点、分布式子区域矩形；
 *       支持双指缩放 + 拖拽平移；点选地图返回**真实世界坐标**给上层（ControlPage）。
 *
 * 设计要点：
 *  - 渲染数学全部复用服务层/模型层纯函数，不在组件里重写：
 *      · 障碍格 = MapService.parseMap(text, side, side).forEachWallRect（左上原点、[0,side] 空间）。
 *      · 世界↔画布 = model/geometry.mapToCanvas / canvasToMap（**中心原点** [-half,+half] 空间）。
 *    两套坐标差一个 half=side/2 的平移：叠加层用 worldToBase() 统一 +half 对齐到墙体的左上原点空间。
 *  - 缩放/平移用 Canvas 2D 上下文变换（translate+scale）施加于「基准空间」绘制；
 *    点选用其逆变换把屏幕触点还原回基准空间，再 canvasToMap 得真实坐标。
 *  - 画布尺寸动态：方形边长 side 由 onAreaChange 取实际可视区短边（不写死分辨率，配合 utils/screen）。
 *  - 叠加层（pin/线宽）尺寸按 1/scale 反缩放，缩放时视觉大小恒定。
 *
 * 刷新约定（Canvas 不随状态自动重绘）：上层每次改动位姿/目标/区域后 **bump renderVersion**（@Prop+@Watch）
 *   触发本组件 redraw；mapText 变化经 @Watch 重新解析后重绘。
 *
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）；纯算法已在 model/geometry + service/MapService 经 verify.mjs 覆盖，
 *    本组件只是其上的 ArkUI 装配 + 手势。
 */

import { MapService, ParsedMap } from '../service/MapService';
import { Point, MapTransform, mapToCanvas, canvasToMap } from '../model/geometry';
import { RobotRuntime, EndPoint, Assignment, Rect, RobotStatus } from '../model/mission';
import { AppColor } from '../constants/theme';
import { getScreen } from '../utils/screen';

// ── 视觉常量（基准空间 vp；叠加层会再除以 scale 保持视觉恒定）──────────────────
const PIN_RADIUS: number = 9;        // 机器人 pin 半径
const PIN_HEADING_LEN: number = 16;  // 朝向指示线长度
const TARGET_RADIUS: number = 7;     // 目标点标记半径
const RECT_LINE_WIDTH: number = 2;   // 子区域/大区域描边
const LABEL_FONT_VP: number = 12;    // 车号角标字号
const SCALE_MIN: number = 0.5;
const SCALE_MAX: number = 8;

@Component
export struct MapCanvas {
  // ── 输入（由上层 ControlPage 传入）──────────────────────────────────────────
  /** 地图文本（密排 0/1 栅格，首行含行列数）。变化即重解析。 */
  @Prop @Watch('onMapChanged') mapText: string = '';
  /** 重绘触发器：上层改动任何动态数据后自增，强制本组件重绘。 */
  @Prop @Watch('redraw') renderVersion: number = 0;
  /** 视图指令（缩放 FAB 用）：形如 `in:<nonce>` / `out:<nonce>` / `reset:<nonce>`，nonce 仅为触发 @Watch。 */
  @Prop @Watch('onViewCmd') viewCmd: string = '';
  /** 参与显示的机器人（位姿/朝向/状态色/车号）。 */
  @Prop robots: RobotRuntime[] = [];
  /** 已选目标点（单机导航 / 全路径）。 */
  @Prop endPoints: EndPoint[] = [];
  /** 分布式子区域分配（对角矩形 + robotId）。 */
  @Prop assignments: Assignment[] = [];
  /** master 大区域边界（对角矩形），无则不画。 */
  @Prop area: Rect | null = null;
  /** 是否允许点选取点。 */
  @Prop enablePick: boolean = true;
  /** 点选回调：返回真实世界坐标（栅格索引）。 */
  onPickPoint?: (p: Point) => void;

  // ── 内部状态 ────────────────────────────────────────────────────────────────
  private settings: RenderingContextSettings = new RenderingContextSettings(true);
  private ctx: CanvasRenderingContext2D = new CanvasRenderingContext2D(this.settings);
  /** 方形画布边长（vp），onAreaChange 取实际短边；未测得前用 screen 兜底。 */
  @State private side: number = getScreen().mapCanvasSideVp();
  private parsed: ParsedMap | undefined = undefined;
  private ready: boolean = false;
  // 缩放/平移（施加于基准空间的上下文变换）。注：成员不能叫 scale —— 与 ArkUI 通用属性 .scale() 冲突。
  @State private zoom: number = 1;
  @State private panX: number = 0;
  @State private panY: number = 0;
  // 手势基线（onActionStart 记录）
  private zoomAtStart: number = 1;
  private panXAtStart: number = 0;
  private panYAtStart: number = 0;

  build() {
    Stack() {
      Canvas(this.ctx)
        .width(this.side)
        .height(this.side)
        .onReady(() => {
          this.ready = true;
          this.reparse();
        })
        .gesture(
          GestureGroup(GestureMode.Parallel,
            TapGesture({ count: 1 })
              .onAction((e: GestureEvent) => this.onTap(e)),
            PanGesture()
              .onActionStart(() => {
                this.panXAtStart = this.panX;
                this.panYAtStart = this.panY;
              })
              .onActionUpdate((e: GestureEvent) => {
                this.panX = this.panXAtStart + e.offsetX;
                this.panY = this.panYAtStart + e.offsetY;
                this.draw();
              }),
            PinchGesture({ fingers: 2 })
              .onActionStart(() => {
                this.zoomAtStart = this.zoom;
              })
              .onActionUpdate((e: GestureEvent) => {
                this.zoom = this.clampScale(this.zoomAtStart * e.scale);
                this.draw();
              })
          )
        )
    }
    .width('100%')
    .height('100%')
    .clip(true)
    .backgroundColor(AppColor.mapFree)
    .onAreaChange((_o: Area, n: Area) => this.onArea(n))
  }

  // ── @Watch 回调 ─────────────────────────────────────────────────────────────
  onMapChanged(): void {
    this.reparse();
  }

  // ── 布局/解析 ───────────────────────────────────────────────────────────────
  private onArea(n: Area): void {
    const w: number = this.toNum(n.width);
    const h: number = this.toNum(n.height);
    const next: number = Math.max(0, Math.min(w, h));
    if (next > 0 && Math.abs(next - this.side) > 0.5) {
      this.side = next;
      this.reparse();
    }
  }

  private reparse(): void {
    if (this.mapText.length > 0 && this.side > 0) {
      this.parsed = MapService.parseMap(this.mapText, this.side, this.side);
    } else {
      this.parsed = undefined;
    }
    this.draw();
  }

  /** 重置视图到铺满（scale=1, pan=0）。供上层在换图/回中按钮调用（经 renderVersion 间接也可）。 */
  resetView(): void {
    this.zoom = 1;
    this.panX = 0;
    this.panY = 0;
    this.draw();
  }

  // ── 绘制 ────────────────────────────────────────────────────────────────────
  redraw(): void {
    this.draw();
  }

  /** 响应上层缩放 FAB 指令（`in`/`out`/`reset`，后缀 nonce 仅触发变更）。 */
  onViewCmd(): void {
    const kind: string = this.viewCmd.split(':')[0];
    if (kind === 'in') {
      this.zoom = this.clampScale(this.zoom * 1.25);
    } else if (kind === 'out') {
      this.zoom = this.clampScale(this.zoom / 1.25);
    } else if (kind === 'reset') {
      this.zoom = 1;
      this.panX = 0;
      this.panY = 0;
    } else {
      return;
    }
    this.draw();
  }

  private draw(): void {
    if (!this.ready) {
      return;
    }
    const ctx: CanvasRenderingContext2D = this.ctx;
    ctx.clearRect(0, 0, this.side, this.side);
    const p: ParsedMap | undefined = this.parsed;
    if (p === undefined) {
      return;
    }
    const t: MapTransform = p.toTransform();
    const half: number = this.side / 2;

    ctx.save();
    ctx.translate(this.panX, this.panY);
    ctx.scale(this.zoom, this.zoom);

    // 1) 障碍格（左上原点空间，直接画）
    ctx.fillStyle = AppColor.mapWall;
    p.forEachWallRect((px: number, py: number, w: number, h: number): void => {
      ctx.fillRect(px, py, w, h);
    });

    // 2) 大区域 + 子区域矩形
    if (this.area !== null) {
      this.drawWorldRect(ctx, t, half, this.area.corner1, this.area.corner2, AppColor.primary, false);
    }
    for (const a of this.assignments) {
      this.drawWorldRect(ctx, t, half, a.corner1, a.corner2, AppColor.primary, true);
    }

    // 3) 目标点
    for (const e of this.endPoints) {
      if (e.x !== null && e.y !== null) {
        this.drawTarget(ctx, t, half, { x: e.x, y: e.y });
      }
    }

    // 4) 机器人位姿
    for (const r of this.robots) {
      this.drawRobot(ctx, t, half, r);
    }

    ctx.restore();
  }

  /** 世界坐标 → 基准画布坐标（左上原点）。 */
  private worldToBase(m: Point, t: MapTransform, half: number): Point {
    const c: Point = mapToCanvas(m, t, half);
    return { x: c.x + half, y: c.y + half };
  }

  private drawWorldRect(ctx: CanvasRenderingContext2D, t: MapTransform, half: number,
    c1: Point, c2: Point, color: string, filled: boolean): void {
    const a: Point = this.worldToBase(c1, t, half);
    const b: Point = this.worldToBase(c2, t, half);
    const x: number = Math.min(a.x, b.x);
    const y: number = Math.min(a.y, b.y);
    const w: number = Math.abs(b.x - a.x);
    const h: number = Math.abs(b.y - a.y);
    if (filled) {
      ctx.globalAlpha = 0.12;
      ctx.fillStyle = color;
      ctx.fillRect(x, y, w, h);
      ctx.globalAlpha = 1;
    }
    ctx.lineWidth = RECT_LINE_WIDTH / this.zoom;
    ctx.strokeStyle = color;
    ctx.strokeRect(x, y, w, h);
  }

  private drawTarget(ctx: CanvasRenderingContext2D, t: MapTransform, half: number, m: Point): void {
    const b: Point = this.worldToBase(m, t, half);
    const rad: number = TARGET_RADIUS / this.zoom;
    ctx.strokeStyle = AppColor.mapTarget;
    ctx.lineWidth = 2 / this.zoom;
    // 环 + 十字
    ctx.beginPath();
    ctx.arc(b.x, b.y, rad, 0, Math.PI * 2);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(b.x - rad, b.y);
    ctx.lineTo(b.x + rad, b.y);
    ctx.moveTo(b.x, b.y - rad);
    ctx.lineTo(b.x, b.y + rad);
    ctx.stroke();
  }

  private drawRobot(ctx: CanvasRenderingContext2D, t: MapTransform, half: number, r: RobotRuntime): void {
    const b: Point = this.worldToBase({ x: r.x, y: r.y }, t, half);
    const color: string = this.statusColor(r.status);
    const rad: number = PIN_RADIUS / this.zoom;

    // 朝向指示线（世界 0°=+X、CCW 朝 +Y；画布 y 向下，故 dy 取负）
    const rRad: number = r.r * Math.PI / 180;
    const hx: number = b.x + Math.cos(rRad) * (PIN_HEADING_LEN / this.zoom);
    const hy: number = b.y - Math.sin(rRad) * (PIN_HEADING_LEN / this.zoom);
    ctx.strokeStyle = color;
    ctx.lineWidth = 3 / this.zoom;
    ctx.beginPath();
    ctx.moveTo(b.x, b.y);
    ctx.lineTo(hx, hy);
    ctx.stroke();

    // 车体圆点
    ctx.fillStyle = color;
    ctx.beginPath();
    ctx.arc(b.x, b.y, rad, 0, Math.PI * 2);
    ctx.fill();
    // 白描边增强对比
    ctx.strokeStyle = AppColor.surface;
    ctx.lineWidth = 1.5 / this.zoom;
    ctx.stroke();

    // 车号角标
    ctx.fillStyle = AppColor.onPrimary;
    ctx.font = `${LABEL_FONT_VP / this.zoom}vp sans-serif`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(`${r.index}`, b.x, b.y);
  }

  private statusColor(s: RobotStatus): string {
    switch (s) {
      case RobotStatus.covering:
      case RobotStatus.scanning:
        return AppColor.robotCovering;
      case RobotStatus.idle:
      case RobotStatus.done:
        return AppColor.robotOnline;
      case RobotStatus.error:
        return AppColor.robotError;
      default:
        return AppColor.robotOffline;
    }
  }

  // ── 点选（屏幕 → 真实世界）─────────────────────────────────────────────────
  private onTap(e: GestureEvent): void {
    if (!this.enablePick || this.parsed === undefined || this.onPickPoint === undefined) {
      return;
    }
    const fingers: FingerInfo[] = e.fingerList;
    if (fingers === undefined || fingers.length === 0) {
      return;
    }
    const lx: number = fingers[0].localX;
    const ly: number = fingers[0].localY;
    // 逆变换：屏幕(组件)坐标 → 基准空间 → 中心原点 → 真实世界
    const bx: number = (lx - this.panX) / this.zoom;
    const by: number = (ly - this.panY) / this.zoom;
    const half: number = this.side / 2;
    const c: Point = { x: bx - half, y: by - half };
    const m: Point = canvasToMap(c, this.parsed.toTransform(), half);
    this.onPickPoint(m);
  }

  // ── 工具 ────────────────────────────────────────────────────────────────────
  private clampScale(s: number): number {
    return Math.min(SCALE_MAX, Math.max(SCALE_MIN, s));
  }

  private toNum(v: Length): number {
    if (typeof v === 'number') {
      return v;
    }
    return parseFloat(`${v}`);
  }
}
