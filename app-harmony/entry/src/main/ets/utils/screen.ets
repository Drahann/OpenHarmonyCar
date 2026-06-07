/**
 * 动态屏幕度量 —— 取代旧 Common/Screen.ets 的写死分辨率（2199×1533）。
 *
 * 旧实现把分辨率硬编码，并在模块加载时一次性派生一堆面板布局常量（screenWidth9/3/1…），
 * 换平板即错位、且无法在运行时刷新。本工具改为：
 *  - 运行时 display.getDefaultDisplaySync() 动态取屏；懒加载 + 缓存（首次访问才查，避免模块加载期
 *    display 子系统尚未就绪而抛错）。查询失败回退到一组合理默认分辨率并打印告警。
 *  - 只暴露「原始度量 + 通用派生」（半宽/半高、短边/长边、方形地图画布边长），不绑定具体页面布局；
 *    布局由各组件（MapCanvas / ControlPage）自行决定，避免重蹈旧版把布局常量焊死在此处的覆辙。
 *  - 旋转 / 分屏 / 多窗变化后调用 refreshScreen() 丢弃缓存并重算。
 *
 * 关键消费者（务必成对使用，保证方形地图画布各向同性）：
 *  - service/MapService.ets 的 parseMap(text, canvasWidth, canvasHeight)：传
 *      getScreen().mapCanvasSideVp(inset) 作为 canvasWidth 与 canvasHeight（同值=正方形）。
 *  - model/geometry.ets 的 canvasToMap / mapToCanvas(c, t, halfViewport)：传
 *      getScreen().halfMapViewportVp(inset)（= 上面边长的一半）作为 halfViewport。
 *
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）；display / px2vp 为 ArkUI 运行时 API，
 *    tools/verify/verify.mjs 不镜像本文件（它只镜像纯算法）。
 */

import { display } from '@kit.ArkUI';
import { BusinessError } from '@kit.BasicServicesKit';

/** display 查询失败时的回退分辨率（px）——沿用旧 Screen.ets 的量级，仅兜底用。 */
const FALLBACK_WIDTH_PX: number = 2200;
const FALLBACK_HEIGHT_PX: number = 1440;

/**
 * 一次屏幕度量快照（不可变）。宽高同时提供 px 与 vp：
 * 布局与坐标换算统一用 vp，少数需要像素精度处（如 Canvas drawImage 原图尺寸）用 px。
 */
export class ScreenMetrics {
  readonly widthPx: number;
  readonly heightPx: number;
  readonly widthVp: number;
  readonly heightVp: number;

  constructor(widthPx: number, heightPx: number) {
    this.widthPx = widthPx;
    this.heightPx = heightPx;
    this.widthVp = px2vp(widthPx);
    this.heightVp = px2vp(heightPx);
  }

  // —— 通用派生（vp）——
  get halfWidthVp(): number {
    return this.widthVp / 2;
  }

  get halfHeightVp(): number {
    return this.heightVp / 2;
  }

  /** 短边：横屏平板上通常是高度，作为正方形内容（地图）边长的天然上限。 */
  get shorterEdgeVp(): number {
    return Math.min(this.widthVp, this.heightVp);
  }

  get longerEdgeVp(): number {
    return Math.max(this.widthVp, this.heightVp);
  }

  /**
   * 全屏地图的方形画布边长（vp）。默认取短边减去对称内边距，使正方形地图在横屏平板上完整可见。
   * 同一个返回值应同时作为 MapService.parseMap 的 canvasWidth 与 canvasHeight。
   * @param insetVp 四周对称留白（默认 0；ControlPage 可传浮层避让量）
   */
  mapCanvasSideVp(insetVp: number = 0): number {
    return Math.max(0, this.shorterEdgeVp - insetVp * 2);
  }

  /** geometry.canvasToMap / mapToCanvas 所需的 halfViewport：方形地图画布的半边长（vp）。 */
  halfMapViewportVp(insetVp: number = 0): number {
    return this.mapCanvasSideVp(insetVp) / 2;
  }
}

/** 缓存的不可变快照（纯记忆化，非跨页可变状态）。 */
let cached: ScreenMetrics | undefined = undefined;

/** 取当前屏幕度量（懒加载 + 缓存）。查询失败回退到 FALLBACK_* 并打印告警。 */
export function getScreen(): ScreenMetrics {
  if (cached === undefined) {
    cached = querySync();
  }
  return cached;
}

/** 旋转 / 分屏 / 多窗变化后调用：丢弃缓存、重算并返回新快照。 */
export function refreshScreen(): ScreenMetrics {
  cached = querySync();
  return cached;
}

function querySync(): ScreenMetrics {
  try {
    const d: display.Display = display.getDefaultDisplaySync();
    // display 偶发在子系统初始化竞态期返回 0/负值，一并回退。
    const w: number = d.width > 0 ? d.width : FALLBACK_WIDTH_PX;
    const h: number = d.height > 0 ? d.height : FALLBACK_HEIGHT_PX;
    return new ScreenMetrics(w, h);
  } catch (e) {
    const err: BusinessError = e as BusinessError;
    console.warn(`[screen] getDefaultDisplaySync 失败，回退 ${FALLBACK_WIDTH_PX}x${FALLBACK_HEIGHT_PX}：code=${err.code}, msg=${err.message}`);
    return new ScreenMetrics(FALLBACK_WIDTH_PX, FALLBACK_HEIGHT_PX);
  }
}
