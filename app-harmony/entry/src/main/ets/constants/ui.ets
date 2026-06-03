/**
 * UI / 行为可调参数 —— 取代旧代码里散落的魔法数（5e2、35、20、324e4 等）。
 * 纯展示尺寸（摇杆贴图大小等）留待 UI 阶段补全。
 */

// ── 遥控长按节流 ────────────────────────────────────────────────────────────
/** 长按持续下发命令的间隔（旧 common.ets 里的 5e2）。每个 Joystick 实例各自持有，不再全局共享。 */
export const THROTTLE_INTERVAL_MS: number = 500;

// ── 速度档位（旧 distributedsecondPage / distriFullPathPage 中的硬编码）──────
/** A* 单机导航前进速度 */
export const SPEED_ASTAR: number = 35;
/** 全路径覆盖前进速度 */
export const SPEED_FULLPATH: number = 20;
/** 缺省速度 */
export const SPEED_DEFAULT: number = 20;

// ── 到点判定 ────────────────────────────────────────────────────────────────
const ARRIVE_DISTANCE: number = 4;
/** (arriveDistance² )<<1 —— 旧 dataConstants.calcArriveDistance，避免每帧开方比较。 */
export const ARRIVE_DISTANCE_SQUARED_X2: number = (ARRIVE_DISTANCE ** 2) << 1;

// ── 地图渲染 ────────────────────────────────────────────────────────────────
export const MAP_FREE_COLOR: string = '#ffffff'; // 空旷
export const MAP_WALL_COLOR: string = '#000000'; // 障碍
/**
 * 地图文本"就绪"最小字节数：小于此值认为 SLAM 尚未产出完整地图，需轮询重试。
 * 取代旧 Pending.getTxt 里 324e4 的"幻读"启发式 + toast。⚠️ 阈值待用真实地图校准。
 */
export const MAP_READY_MIN_BYTES: number = 324e4;
/** 轮询地图的间隔 */
export const MAP_POLL_INTERVAL_MS: number = 3000;
