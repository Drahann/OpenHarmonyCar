/**
 * 与香橙派（成员B）视觉推理服务对接的常量 —— 唯一来源，与 contracts/vision-stream-api.md v1.0 对齐。
 *
 * 服务部署在**香橙派本机**（FastAPI+Uvicorn，Ascend 310B1），App 直连，无云端/独立服务器。
 * 基地址 `http://<host>:<port>`（默认 192.168.1.5:8000，可在界面改；持久化见 service/storage.ets）。
 *
 * 🔴 与导航链路（紫派 UDP :5001 / HTTP :8000）**完全解耦**：不同设备、不同 IP、不同协议。
 *    注意：香橙派 :8000（视觉，IP 192.168.1.5）与紫派 :8000（地图 HTTP，紫派 IP）**不同设备勿混**。
 * ⚠️ 改这里属于"协议对接"参数，若涉及端点/字段须同步 contracts/vision-stream-api.md 并通知香橙派侧。
 */

// ── 网络（contracts/vision-stream-api.md §1）────────────────────────────────
/** 香橙派默认 IP（局域网静态）。可在界面改，持久化于 storage。 */
export const VISION_DEFAULT_HOST: string = '192.168.1.5';
/** 视觉服务端口。 */
export const VISION_DEFAULT_PORT: number = 8000;

// ── 端点（contracts/vision-stream-api.md §2、§3）────────────────────────────
/** 实时视频流 WebSocket 路径（核心接口）。 */
export const WS_VIDEO_PATH: string = '/ws/video';
/** 实时状态（fps/latency/readings）。 */
export const API_SUMMARY: string = '/api/summary';
/** 摄像头运行状态（running/fps/frame_count）。 */
export const API_VIDEO_STATUS: string = '/api/video/status';
/** 最近 600 条读数记录。 */
export const API_HISTORY: string = '/api/history';
/** 数据统计（可带时间范围）。 */
export const API_DATA_STATS: string = '/api/data/stats';
/** 报警阈值设置（low/high 百分比）。 */
export const API_THRESHOLDS: string = '/api/thresholds';
/** 仪表配置（量程/阈值/显示名）。{type} 拼在后面。 */
export const API_GAUGE_CONFIGS: string = '/api/gauge/configs';
/** 生成 DeepSeek 分析报告（会暂停推理释放 NPU）。 */
export const API_DATA_REPORT: string = '/api/data/report';
/** 历史报告列表（limit/offset）。 */
export const API_DATA_REPORTS: string = '/api/data/reports';

// ── 关键点名（contracts/vision-stream-api.md §5 + orangepi-vision/README.md）─
/**
 * 🔴 **关键点一律按 `name` 字段映射，绝不按数组 index**：
 *   契约表记 `0=center,1=pointer_tip,2=zero_mark,3=full_mark`，但当前部署模型实际输出顺序是
 *   `0=pointer_tip,1=center,2=zero,3=full`（README 已自曝两者不一致）。按 name 取值即可免踩。
 * 已请香橙派把契约表 index 与模型输出统一（见 contracts/integration-qa.md）。
 */
export const KP_CENTER: string = 'center';        // 表盘旋转中心（角度参考原点）
export const KP_POINTER_TIP: string = 'pointer_tip'; // 指针尖端（当前读数方向）
export const KP_ZERO: string = 'zero_mark';       // 零刻度（量程起点 0%）
export const KP_FULL: string = 'full_mark';       // 满刻度（量程终点 100%）

// ── 读数状态（contracts/vision-stream-api.md §3.1 readings[].status）────────
export const READING_NORMAL: string = 'NORMAL';   // 读数在阈值范围内
export const READING_ALARM: string = 'ALARM';     // 读数超阈值 → 触发告警 UI
export const READING_UNKNOWN: string = 'UNKNOWN'; // 无法获取读数

// ── 连接 / 重连（contracts/vision-stream-api.md §6.3）──────────────────────
/** WS 断开后首次重连延迟（契约建议 3s）。 */
export const VISION_RECONNECT_BASE_MS: number = 3000;
/** 重连退避上限（指数退避，断网久了不刷屏）。 */
export const VISION_RECONNECT_MAX_MS: number = 15000;
/** REST 连接/读取超时。 */
export const VISION_HTTP_TIMEOUT_MS: number = 8000;
/** 报告生成会暂停推理、耗时较长，单独给更长超时。 */
export const VISION_REPORT_TIMEOUT_MS: number = 120000;

// ── 视频渲染节流（V2 用；contracts §2 摄像头约 15FPS）────────────────────
/**
 * UI 解码/刷新最小间隔（ms）：平板解码 JPEG 可能跟不上来帧，限制刷新频率（≈33ms→30fps 上限），
 * **解码进行中到来的帧直接丢、只保留最新**，防积压/内存涨/延迟累积。详见 docs/vision-integration-plan.md §4。
 */
export const VIDEO_MIN_RENDER_INTERVAL_MS: number = 33;

/** 由 host/port 拼出视频流 WS URL（ws://<host>:<port>/ws/video）。 */
export function wsVideoUrl(host: string, port: number): string {
  return `ws://${host}:${port}${WS_VIDEO_PATH}`;
}

/** 由 host/port 拼出 REST 基地址（http://<host>:<port>），调用方再接端点路径。 */
export function visionApiBase(host: string, port: number): string {
  return `http://${host}:${port}`;
}
