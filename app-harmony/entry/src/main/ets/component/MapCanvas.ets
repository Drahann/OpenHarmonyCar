/**
 * MapCanvas —— 可复用「干净建筑图纸」地图组件（取代旧 4 个克隆页里各抄一份的画布逻辑）。
 *
 * 职责：把 SLAM 栅格地图画成一张浅色楼层平面图——浅冷灰未知区 + 白色已扫图纸 + **墨绿平滑矢量墙体**；
 *       叠加覆盖子区域、Maps 泪滴目标 pin、带朝向的机器人圆点；支持双指缩放 + 拖拽平移；点选返回真实坐标。
 *
 * 关键设计——**墙体不再画栅格方块**（旧版逐格/合并行程 fillRect 是"丑的根因"：亚像素碎块/锯齿）：
 *   改成提取**自由/墙体边界**的平滑矢量等高线（`model/mapContour`：marching squares → 角点平滑 Chaikin），
 *   由本组件描边 + 软填充。栅格 → 等高线的纯几何全在 `mapContour` + `MapService.toDisplayPool`，可被
 *   `tools/verify/verify.mjs` 镜像断言（规避 ArkTS 本机不可编译）。等高线提取失败时回退行程软填充，保证有墙可见。
 *
 * 渲染架构（双画布分层）：
 *  - **base**（静态）：背景渐变 + 白图纸 + 极淡比例网格 + 平滑墙体。仅地图/缩放/平移变化时重画。
 *  - **overlay**（动态）：覆盖区 + 目标 pin + 机器人。~20fps ticker 承载**克制**动效（目标地面脉冲、活动车脉冲）。
 *
 * 坐标与缩放：墙体等高线在 base 像素空间（随 ctx 变换缩放/平移）；overlay 标记换算到屏幕空间后以恒定 vp
 *   尺寸绘制（缩放时 pin/投影视觉大小恒定）。点选用逆变换还原回基准空间再 `canvasToMap`。
 *
 * 历史：第一版深色霓虹「指挥中心 HUD」经用户否决（不贴品牌、栅格方块没根治），重做为本浅色图纸方案。
 *   详见 `docs/map-ui-redesign.md`。⚠️ 未经 DevEco 验证。
 */

import { MapService, ParsedMap } from '../service/MapService';
import { Point, MapTransform, mapToCanvas, canvasToMap } from '../model/geometry';
import { Contour, CoarseGrid, extractContours } from '../model/mapContour';
import { RobotRuntime, EndPoint, Assignment, Rect, RobotStatus } from '../model/mission';
import { MapTheme } from '../constants/theme';
import { getScreen } from '../utils/screen';

// ── 墙体等高线 ──────────────────────────────────────────────────────────────
const CONTOUR_MAX_LEN: number = 240;   // 粗网格长边上限（降采样目标）
const CHAIKIN_ITERS: number = 2;       // 角点平滑迭代（2 顺滑；0 = 仅折线）
const WALL_STROKE: number = 2.4;       // 墙体描边宽（屏幕 vp，恒定）
// ── 图纸网格 ────────────────────────────────────────────────────────────────
const GRID_DIV: number = 22;           // 图纸比例网格：间距 = side / GRID_DIV
// ── 机器人标记（屏幕空间 vp，恒定）────────────────────────────────────────────
const DISC_R: number = 7;              // 车体圆点半径
const RING_W: number = 2;              // 白环宽
const HEAD_TIP: number = 9;            // 朝向三角尖端伸出（圆点外）
const HEAD_BASE: number = 5;           // 朝向三角底半宽
const ACTIVE_PULSE: number = 10;       // 活动车（覆盖/扫描）脉冲环外扩
const LABEL_FONT: number = 11;         // 车号字号
// ── 目标 pin（屏幕空间 vp）────────────────────────────────────────────────────
const PIN_R: number = 8;               // 泪滴 pin 头半径
const PIN_H: number = 22;              // pin 尖端到头心距离
const TARGET_PULSE: number = 8;        // 目标地面脉冲外扩
// ── 覆盖子区域 ──────────────────────────────────────────────────────────────
const REGION_LINE_WIDTH: number = 2;
const REGION_CORNER: number = 14;
// ── 缩放 ────────────────────────────────────────────────────────────────────
const SCALE_MIN: number = 0.5;
const SCALE_MAX: number = 8;
// ── 动效 ────────────────────────────────────────────────────────────────────
const ENABLE_MOTION: boolean = true;   // 克制动效总开关（目标地面脉冲 + 活动车脉冲）
const TICK_MS: number = 50;
const PING_PERIOD_MS: number = 1700;
const TWO_PI: number = Math.PI * 2;

/** hex(`#rrggbb`) → rgba 字符串（给 Canvas 渐变/描边附加透明度）。 */
function rgba(hex: string, a: number): string {
  const h: string = hex.charAt(0) === '#' ? hex.substring(1) : hex;
  const r: number = parseInt(h.substring(0, 2), 16);
  const g: number = parseInt(h.substring(2, 4), 16);
  const b: number = parseInt(h.substring(4, 6), 16);
  return `rgba(${r},${g},${b},${a})`;
}

@Component
export struct MapCanvas {
  // ── 输入（由上层 ControlPage 传入）──────────────────────────────────────────
  /** 地图文本（紫派 defultMap.txt / 密排栅格，首行含维度）。变化即重解析。 */
  @Prop @Watch('onMapChanged') mapText: string = '';
  /** 重绘触发器：上层改动任何动态数据后自增，强制 overlay 重画。 */
  @Prop @Watch('redraw') renderVersion: number = 0;
  /** 视图指令（缩放 FAB 用）：形如 `in:<nonce>` / `out:<nonce>` / `reset:<nonce>`。 */
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
  private baseSettings: RenderingContextSettings = new RenderingContextSettings(true);
  private baseCtx: CanvasRenderingContext2D = new CanvasRenderingContext2D(this.baseSettings);
  private overlaySettings: RenderingContextSettings = new RenderingContextSettings(true);
  private overlayCtx: CanvasRenderingContext2D = new CanvasRenderingContext2D(this.overlaySettings);
  /** 方形画布边长（vp），onAreaChange 取实际短边；未测得前用 screen 兜底。 */
  @State private side: number = getScreen().mapCanvasSideVp();
  private parsed: ParsedMap | undefined = undefined;
  /** 墙体平滑等高线（base 像素坐标），随 parsed 重算；提取失败为 undefined → 回退行程软填充。 */
  private contours: Contour[] | undefined = undefined;
  /** 粗网格（等高线同源）：墙体软填充直接填它的 cell，绝不误填房间（不依赖描边缠绕方向）。 */
  private coarse: CoarseGrid | undefined = undefined;
  private baseReady: boolean = false;
  private overlayReady: boolean = false;
  // 缩放/平移（施加于基准空间）。注：成员不能叫 scale —— 与 ArkUI 通用属性 .scale() 冲突。
  @State private zoom: number = 1;
  @State private panX: number = 0;
  @State private panY: number = 0;
  // 手势基线（onActionStart 记录）
  private zoomAtStart: number = 1;
  private panXAtStart: number = 0;
  private panYAtStart: number = 0;
  // 动效相位（plain 成员，由 ticker 直接驱动 overlay 重画）
  private tickTimer: number = -1;
  private pingPhase: number = 0;   // 0..1 循环：目标地面脉冲 / 活动车脉冲

  build() {
    Stack() {
      // 静态层：背景 + 白图纸 + 网格 + 平滑墙体
      Canvas(this.baseCtx)
        .width(this.side)
        .height(this.side)
        .onReady(() => {
          this.baseReady = true;
          this.reparse();
        })
      // 动态层：覆盖区 + 目标 pin + 机器人（承载手势与动效）
      Canvas(this.overlayCtx)
        .width(this.side)
        .height(this.side)
        .onReady(() => {
          this.overlayReady = true;
          this.startTicker();
          this.drawOverlay();
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
                this.drawBase();
                this.drawOverlay();
              }),
            PinchGesture({ fingers: 2 })
              .onActionStart(() => {
                this.zoomAtStart = this.zoom;
              })
              .onActionUpdate((e: GestureEvent) => {
                this.zoom = this.clampScale(this.zoomAtStart * e.scale);
                this.drawBase();
                this.drawOverlay();
              })
          )
        )
    }
    .width('100%')
    .height('100%')
    .clip(true)
    .backgroundColor(MapTheme.bg)
    .onAreaChange((_o: Area, n: Area) => this.onArea(n))
  }

  aboutToDisappear(): void {
    this.stopTicker();
  }

  // ── @Watch 回调 ─────────────────────────────────────────────────────────────
  onMapChanged(): void {
    this.reparse();
  }

  redraw(): void {
    // 上层改了动态数据（位姿/目标/区域）；地图静态 → 只重画 overlay。
    this.drawOverlay();
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
    this.drawBase();
    this.drawOverlay();
  }

  /** 重置视图到铺满（scale=1, pan=0）。供上层换图/回中调用。 */
  resetView(): void {
    this.zoom = 1;
    this.panX = 0;
    this.panY = 0;
    this.drawBase();
    this.drawOverlay();
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
      this.recomputeWalls(this.parsed);
    } else {
      this.parsed = undefined;
      this.coarse = undefined;
      this.contours = undefined;
    }
    this.drawBase();
    this.drawOverlay();
  }

  /** 栅格 → 粗网格 + 平滑等高线（降采样池化 → marching squares → 平滑 → 映射 base px）。失败回退 undefined（→ 行程软填充）。 */
  private recomputeWalls(p: ParsedMap): void {
    try {
      const cg: CoarseGrid = p.toDisplayPool(CONTOUR_MAX_LEN);
      this.coarse = cg;
      this.contours = extractContours(cg, p.gridWidth, p.gridHeight, CHAIKIN_ITERS);
    } catch (e) {
      this.coarse = undefined;
      this.contours = undefined;
    }
  }

  // ── 动效 ticker（只重画 overlay）────────────────────────────────────────────
  private startTicker(): void {
    if (!ENABLE_MOTION) {
      this.drawOverlay();
      return;
    }
    this.stopTicker();
    this.tickTimer = setInterval(() => {
      this.pingPhase += TICK_MS / PING_PERIOD_MS;
      if (this.pingPhase > 1) {
        this.pingPhase -= 1;
      }
      this.drawOverlay();
    }, TICK_MS);
  }

  private stopTicker(): void {
    if (this.tickTimer !== -1) {
      clearInterval(this.tickTimer);
      this.tickTimer = -1;
    }
  }

  // ── base 层绘制（静态：背景 + 白图纸 + 网格 + 平滑墙体）──────────────────────
  private drawBase(): void {
    if (!this.baseReady) {
      return;
    }
    const ctx: CanvasRenderingContext2D = this.baseCtx;
    const side: number = this.side;
    ctx.clearRect(0, 0, side, side);

    // 1) 背景（未知 / 图纸外）：轻微纵向渐变出景深——屏幕固定，始终铺满
    const bg: CanvasGradient = ctx.createLinearGradient(0, 0, 0, side);
    bg.addColorStop(0, MapTheme.bg);
    bg.addColorStop(1, MapTheme.bgEdge);
    ctx.fillStyle = bg;
    ctx.fillRect(0, 0, side, side);

    // 2) 地图内容（白图纸 + 网格 + 墙体）——随缩放/平移变换
    const p: ParsedMap | undefined = this.parsed;
    if (p !== undefined) {
      ctx.save();
      ctx.translate(this.panX, this.panY);
      ctx.scale(this.zoom, this.zoom);

      // 白色已扫图纸（方形包围盒区）
      ctx.fillStyle = MapTheme.floor;
      ctx.fillRect(0, 0, side, side);

      // 极淡比例网格（裁剪在图纸内，随地图缩放——建筑图纸观感）
      ctx.save();
      ctx.beginPath();
      ctx.rect(0, 0, side, side);
      ctx.clip();
      ctx.strokeStyle = MapTheme.grid;
      ctx.lineWidth = 1 / this.zoom;
      const pitch: number = side / GRID_DIV;
      for (let gx = 0; gx <= side + 0.5; gx += pitch) {
        ctx.beginPath();
        ctx.moveTo(gx, 0);
        ctx.lineTo(gx, side);
        ctx.stroke();
      }
      for (let gy = 0; gy <= side + 0.5; gy += pitch) {
        ctx.beginPath();
        ctx.moveTo(0, gy);
        ctx.lineTo(side, gy);
        ctx.stroke();
      }
      ctx.restore();

      // 平滑矢量墙体
      this.drawWalls(ctx, p);

      // 图纸描边
      const bw: number = 1.5 / this.zoom;
      ctx.strokeStyle = MapTheme.floorEdge;
      ctx.lineWidth = bw;
      ctx.strokeRect(bw * 0.5, bw * 0.5, side - bw, side - bw);
      ctx.restore();
    }
  }

  /** 墙体：粗格实心软填充（绝不误填房间）+ 平滑等高线描边（圆角）。等高线缺失时回退行程软填充。 */
  private drawWalls(ctx: CanvasRenderingContext2D, p: ParsedMap): void {
    const cs: Contour[] | undefined = this.contours;
    const cg: CoarseGrid | undefined = this.coarse;
    if (cs !== undefined && cs.length > 0 && cg !== undefined) {
      // 1) 软填充：直接填粗网格的墙 cell（实心、绝不误填房间内部，不依赖描边缠绕方向）。
      //    粗格边的锯齿由下面的平滑描边盖住；fill 轻微透明，只为厚墙增体积感。
      ctx.fillStyle = MapTheme.wallFill;
      const cwPx: number = cg.f * p.gridWidth;
      const chPx: number = cg.f * p.gridHeight;
      const ov: number = 0.5 / this.zoom; // 轻微重叠闭合 cell 缝
      for (let cy = 0; cy < cg.h; cy++) {
        for (let cx = 0; cx < cg.w; cx++) {
          if (cg.b[cy * cg.w + cx] === 1) {
            ctx.fillRect(cx * cwPx, cy * chPx, cwPx + ov, chPx + ov);
          }
        }
      }
      // 2) 平滑等高线描边（圆角接头/端点消除棱角 → 矢量楼层平面图观感）
      ctx.strokeStyle = MapTheme.wallStroke;
      ctx.lineWidth = WALL_STROKE / this.zoom;
      ctx.lineJoin = 'round';
      ctx.lineCap = 'round';
      ctx.beginPath();
      for (const c of cs) {
        if (c.pts.length < 4) {
          continue;
        }
        ctx.moveTo(c.pts[0], c.pts[1]);
        for (let i = 2; i < c.pts.length; i += 2) {
          ctx.lineTo(c.pts[i], c.pts[i + 1]);
        }
        if (c.closed) {
          ctx.closePath();
        }
      }
      ctx.stroke();
    } else {
      // 回退：行程软填充（等高线提取失败时仍有墙可见）
      ctx.fillStyle = MapTheme.wallFill;
      const minPx: number = 1 / this.zoom;
      p.forEachWallSpan((px: number, py: number, w: number, h: number): void => {
        ctx.fillRect(px, py, Math.max(w, minPx), Math.max(h, minPx));
      });
    }
  }

  // ── overlay 层绘制（动态：覆盖区 + 目标 pin + 机器人，屏幕空间恒定尺寸）────────
  private drawOverlay(): void {
    if (!this.overlayReady) {
      return;
    }
    const ctx: CanvasRenderingContext2D = this.overlayCtx;
    ctx.clearRect(0, 0, this.side, this.side);
    const p: ParsedMap | undefined = this.parsed;
    if (p === undefined) {
      return; // 尚无地图 → overlay 空（浅色背板由 base 提供，等待建图）
    }
    const t: MapTransform = p.toTransform();
    const half: number = this.side / 2;

    // 1) 大区域 + 子区域矩形
    if (this.area !== null) {
      this.drawRegion(ctx, this.worldToScreen(this.area.corner1, t, half), this.worldToScreen(this.area.corner2, t, half), false);
    }
    for (const a of this.assignments) {
      this.drawRegion(ctx, this.worldToScreen(a.corner1, t, half), this.worldToScreen(a.corner2, t, half), true);
    }

    // 2) 目标点（Maps 泪滴 pin + 地面脉冲）
    for (const e of this.endPoints) {
      if (e.x !== null && e.y !== null) {
        const s: Point = this.worldToScreen({ x: e.x, y: e.y }, t, half);
        this.drawTarget(ctx, s.x, s.y);
      }
    }

    // 3) 机器人（带朝向圆点 + 软投影 + 活动脉冲 + 车号）
    for (const r of this.robots) {
      const s: Point = this.worldToScreen({ x: r.x, y: r.y }, t, half);
      this.drawRobot(ctx, s.x, s.y, r);
    }
  }

  // ── 坐标换算 ────────────────────────────────────────────────────────────────
  /** 世界坐标 → 基准画布坐标（左上原点，未施加缩放/平移）。 */
  private worldToBase(m: Point, t: MapTransform, half: number): Point {
    const c: Point = mapToCanvas(m, t, half);
    return { x: c.x + half, y: c.y + half };
  }

  /** 基准坐标 → 屏幕坐标（施加当前缩放/平移）。 */
  private baseToScreen(b: Point): Point {
    return { x: b.x * this.zoom + this.panX, y: b.y * this.zoom + this.panY };
  }

  /** 世界坐标 → 屏幕坐标（overlay 标记定位用）。 */
  private worldToScreen(m: Point, t: MapTransform, half: number): Point {
    return this.baseToScreen(this.worldToBase(m, t, half));
  }

  // ── overlay 绘制子程序（均屏幕空间、恒定 vp 尺寸）─────────────────────────────
  private drawRegion(ctx: CanvasRenderingContext2D, aS: Point, bS: Point, filled: boolean): void {
    const color: string = MapTheme.region;
    const x: number = Math.min(aS.x, bS.x);
    const y: number = Math.min(aS.y, bS.y);
    const w: number = Math.abs(bS.x - aS.x);
    const h: number = Math.abs(bS.y - aS.y);
    if (filled) {
      ctx.globalAlpha = 0.12;
      ctx.fillStyle = color;
      ctx.fillRect(x, y, w, h);
      ctx.globalAlpha = 1;
    }
    ctx.lineWidth = REGION_LINE_WIDTH;
    ctx.strokeStyle = color;
    ctx.strokeRect(x, y, w, h);
    this.drawCorners(ctx, x, y, w, h, color);
  }

  private drawCorners(ctx: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, color: string): void {
    const c: number = REGION_CORNER;
    ctx.strokeStyle = color;
    ctx.lineWidth = REGION_LINE_WIDTH + 1;
    ctx.lineCap = 'round';
    const corner = (cxp: number, cyp: number, dx: number, dy: number): void => {
      ctx.beginPath();
      ctx.moveTo(cxp + dx * c, cyp);
      ctx.lineTo(cxp, cyp);
      ctx.lineTo(cxp, cyp + dy * c);
      ctx.stroke();
    };
    corner(x, y, 1, 1);
    corner(x + w, y, -1, 1);
    corner(x, y + h, 1, -1);
    corner(x + w, y + h, -1, -1);
    ctx.lineCap = 'butt';
  }

  private drawTarget(ctx: CanvasRenderingContext2D, sx: number, sy: number): void {
    // 地面脉冲（gentle）：从 pin 尖端外扩淡出的小环
    if (ENABLE_MOTION) {
      const tp: number = this.pingPhase;
      ctx.strokeStyle = rgba(MapTheme.target, (1 - tp) * 0.5);
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(sx, sy, 2 + tp * TARGET_PULSE, 0, TWO_PI);
      ctx.stroke();
    }
    // 泪滴 pin：尖端在 (sx,sy)，头心在 (sx, sy-PIN_H)
    const cyc: number = sy - PIN_H;
    ctx.save();
    ctx.shadowColor = MapTheme.markerShadow;
    ctx.shadowBlur = 6;
    ctx.shadowOffsetY = 2;
    ctx.fillStyle = MapTheme.target;
    ctx.beginPath();
    ctx.moveTo(sx, sy);
    ctx.quadraticCurveTo(sx - PIN_R, cyc + PIN_R * 0.6, sx - PIN_R, cyc);
    ctx.arc(sx, cyc, PIN_R, Math.PI, 0, false);
    ctx.quadraticCurveTo(sx + PIN_R, cyc + PIN_R * 0.6, sx, sy);
    ctx.closePath();
    ctx.fill();
    ctx.restore();
    // 白心
    ctx.fillStyle = MapTheme.targetInner;
    ctx.beginPath();
    ctx.arc(sx, cyc, PIN_R * 0.42, 0, TWO_PI);
    ctx.fill();
  }

  private drawRobot(ctx: CanvasRenderingContext2D, sx: number, sy: number, r: RobotRuntime): void {
    const col: string = this.statusColor(r.status);
    const active: boolean = r.status === RobotStatus.covering || r.status === RobotStatus.scanning;

    // 活动车脉冲环（覆盖/扫描时）——克制
    if (ENABLE_MOTION && active) {
      const tp: number = this.pingPhase;
      ctx.strokeStyle = rgba(col, (1 - tp) * 0.32);
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(sx, sy, DISC_R + tp * ACTIVE_PULSE, 0, TWO_PI);
      ctx.stroke();
    }

    // 朝向三角（世界 0°=+X、CCW 朝 +Y；屏幕 y 向下 → dy 取负）
    const rRad: number = r.r * Math.PI / 180;
    const hx: number = Math.cos(rRad);
    const hy: number = -Math.sin(rRad);
    const px: number = -hy; // 垂直方向
    const py: number = hx;
    const tipX: number = sx + hx * (DISC_R + HEAD_TIP);
    const tipY: number = sy + hy * (DISC_R + HEAD_TIP);
    const baseX: number = sx + hx * (DISC_R + 1);
    const baseY: number = sy + hy * (DISC_R + 1);
    ctx.fillStyle = col;
    ctx.beginPath();
    ctx.moveTo(tipX, tipY);
    ctx.lineTo(baseX + px * HEAD_BASE, baseY + py * HEAD_BASE);
    ctx.lineTo(baseX - px * HEAD_BASE, baseY - py * HEAD_BASE);
    ctx.closePath();
    ctx.fill();

    // 车体圆点（软投影抬起）
    ctx.save();
    ctx.shadowColor = MapTheme.markerShadow;
    ctx.shadowBlur = 5;
    ctx.shadowOffsetY = 2;
    ctx.fillStyle = col;
    ctx.beginPath();
    ctx.arc(sx, sy, DISC_R, 0, TWO_PI);
    ctx.fill();
    ctx.restore();

    // 白环
    ctx.strokeStyle = MapTheme.robotRing;
    ctx.lineWidth = RING_W;
    ctx.beginPath();
    ctx.arc(sx, sy, DISC_R, 0, TWO_PI);
    ctx.stroke();

    // 车号
    ctx.fillStyle = MapTheme.robotLabel;
    ctx.font = `bold ${LABEL_FONT}vp sans-serif`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(`${r.index}`, sx, sy);
  }

  private statusColor(s: RobotStatus): string {
    switch (s) {
      case RobotStatus.covering:
      case RobotStatus.scanning:
        return MapTheme.robotCovering;
      case RobotStatus.idle:
      case RobotStatus.done:
        return MapTheme.robotOnline;
      case RobotStatus.error:
        return MapTheme.robotError;
      default:
        return MapTheme.robotOffline;
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
