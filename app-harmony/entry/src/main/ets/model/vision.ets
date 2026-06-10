/**
 * 香橙派视觉服务的数据模型（DTO）+ JSON 解析 —— 与 contracts/vision-stream-api.md v1.0 逐字段对齐。
 *
 * 设计：**wire 接口**（snake_case，贴 JSON）→ `JSON.parse(...) as Raw*` → **DTO 类**（camelCase，App 用）。
 *
 * 🔴 关键点**按 `name` 映射，绝不按 index**（契约表与部署模型输出顺序不一致，见 constants/vision.ets KP_*）。
 * 🔴 NaN 安全：香橙派若用 Python `json.dumps` 输出 `NaN`/`Infinity` 是**非法 JSON**，`JSON.parse` 会抛——
 *    所有解析都 try/catch，失败即返回空（该帧丢弃），不崩 UI。读数本身也可能是 NaN（契约 §6.3 "无法识别"）。
 */

import {
  KP_CENTER, KP_POINTER_TIP, KP_ZERO, KP_FULL,
  READING_NORMAL, READING_ALARM
} from '../constants/vision';

// ── wire 接口（贴 JSON 原始字段名）──────────────────────────────────────────
interface RawKeypoint {
  name: string;
  x: number;
  y: number;
  conf: number;
}

interface RawDetection {
  bbox: number[];          // [x, y, w, h] 像素
  score: number;
  keypoints: RawKeypoint[];
}

interface RawFrameMeta {
  type: string;            // "frame_meta"
  frame_id: number;
  timestamp: number;
  fps: number;
  inference_time_ms: number;
  yolo_time_ms: number;
  pose_time_ms: number;
  num_detections: number;
  detections: RawDetection[];
  gauge_angles: number[];  // 每表读数百分比 0~100，与 detections 一一对应
}

/** WS 文本消息的判别信封：frame_meta 或 {"type":"error","message":...}。 */
interface RawEnvelope {
  type: string;
  message?: string;
}

interface RawReading {
  value: number;
  unit: string;
  label: string;
  status: string;          // NORMAL / ALARM / UNKNOWN
  gauge_id: number;
}

interface RawSummary {
  fps: number;
  latency_ms: number;
  frame_id: number;
  over_limit: boolean;
  readings: RawReading[];
}

interface RawVideoStatus {
  running: boolean;
  fps: number;
  frame_count: number;
}

interface RawGaugeConfig {
  unit: string;
  min_range: number;
  max_range: number;
  low_threshold: number;
  high_threshold: number;
  display_name: string;
}

// ── DTO（App 内部用，camelCase + 便捷方法）──────────────────────────────────
/** 读数状态。 */
export enum ReadingStatus {
  normal = 'NORMAL',
  alarm = 'ALARM',
  unknown = 'UNKNOWN'
}

/** 单个关键点。 */
export class Keypoint {
  readonly name: string;
  readonly x: number;
  readonly y: number;
  readonly conf: number;

  constructor(name: string, x: number, y: number, conf: number) {
    this.name = name;
    this.x = x;
    this.y = y;
    this.conf = conf;
  }
}

/** 单个仪表检测结果（框 + 关键点）。坐标系 = 该 JPEG 帧像素（xywh）。 */
export class Detection {
  readonly bbox: number[];     // [x, y, w, h]
  readonly score: number;
  readonly keypoints: Keypoint[];

  constructor(bbox: number[], score: number, keypoints: Keypoint[]) {
    this.bbox = bbox;
    this.score = score;
    this.keypoints = keypoints;
  }

  /** 🔴 按 `name` 取关键点（绝不按 index）。找不到返回 undefined。 */
  keypointByName(name: string): Keypoint | undefined {
    for (const kp of this.keypoints) {
      if (kp.name === name) {
        return kp;
      }
    }
    return undefined;
  }

  center(): Keypoint | undefined {
    return this.keypointByName(KP_CENTER);
  }

  pointerTip(): Keypoint | undefined {
    return this.keypointByName(KP_POINTER_TIP);
  }

  zeroMark(): Keypoint | undefined {
    return this.keypointByName(KP_ZERO);
  }

  fullMark(): Keypoint | undefined {
    return this.keypointByName(KP_FULL);
  }
}

/** 一帧的元数据（与交替推送的二进制 JPEG 同步）。 */
export class FrameMeta {
  readonly frameId: number;
  readonly timestamp: number;
  readonly fps: number;
  readonly inferenceTimeMs: number;
  readonly yoloTimeMs: number;
  readonly poseTimeMs: number;
  readonly numDetections: number;
  readonly detections: Detection[];
  readonly gaugeAngles: number[];  // 读数百分比 0~100

  constructor(frameId: number, timestamp: number, fps: number, inferenceTimeMs: number,
    yoloTimeMs: number, poseTimeMs: number, numDetections: number,
    detections: Detection[], gaugeAngles: number[]) {
    this.frameId = frameId;
    this.timestamp = timestamp;
    this.fps = fps;
    this.inferenceTimeMs = inferenceTimeMs;
    this.yoloTimeMs = yoloTimeMs;
    this.poseTimeMs = poseTimeMs;
    this.numDetections = numDetections;
    this.detections = detections;
    this.gaugeAngles = gaugeAngles;
  }
}

/** /api/summary 的单条实时读数。 */
export class Reading {
  readonly value: number;     // 可能为 NaN（无法识别）
  readonly unit: string;
  readonly label: string;
  readonly status: ReadingStatus;
  readonly gaugeId: number;

  constructor(value: number, unit: string, label: string, status: ReadingStatus, gaugeId: number) {
    this.value = value;
    this.unit = unit;
    this.label = label;
    this.status = status;
    this.gaugeId = gaugeId;
  }

  isAlarm(): boolean {
    return this.status === ReadingStatus.alarm;
  }
}

/** /api/summary 响应。 */
export class Summary {
  readonly fps: number;
  readonly latencyMs: number;
  readonly frameId: number;
  readonly overLimit: boolean;
  readonly readings: Reading[];

  constructor(fps: number, latencyMs: number, frameId: number, overLimit: boolean, readings: Reading[]) {
    this.fps = fps;
    this.latencyMs = latencyMs;
    this.frameId = frameId;
    this.overLimit = overLimit;
    this.readings = readings;
  }
}

/** /api/video/status 响应。 */
export class VideoStatus {
  readonly running: boolean;
  readonly fps: number;
  readonly frameCount: number;

  constructor(running: boolean, fps: number, frameCount: number) {
    this.running = running;
    this.fps = fps;
    this.frameCount = frameCount;
  }
}

/** 仪表量程/阈值/显示名配置。 */
export class GaugeConfig {
  readonly unit: string;
  readonly minRange: number;
  readonly maxRange: number;
  readonly lowThreshold: number;
  readonly highThreshold: number;
  readonly displayName: string;

  constructor(unit: string, minRange: number, maxRange: number,
    lowThreshold: number, highThreshold: number, displayName: string) {
    this.unit = unit;
    this.minRange = minRange;
    this.maxRange = maxRange;
    this.lowThreshold = lowThreshold;
    this.highThreshold = highThreshold;
    this.displayName = displayName;
  }

  /** 把读数百分比(0~100)按量程换算为物理量（如 MPa）。 */
  percentToPhysical(percent: number): number {
    return this.minRange + (this.maxRange - this.minRange) * (percent / 100);
  }
}

/** WS 文本消息解析结果：要么是一帧元数据，要么是服务器错误，要么都为 null（解析失败/丢弃）。 */
export class VisionTextMessage {
  readonly meta: FrameMeta | null;
  readonly error: string | null;

  constructor(meta: FrameMeta | null, error: string | null) {
    this.meta = meta;
    this.error = error;
  }
}

// ── 解析 ────────────────────────────────────────────────────────────────────
/** 字符串状态 → ReadingStatus（未知一律归 unknown）。 */
function toStatus(s: string): ReadingStatus {
  if (s === READING_NORMAL) {
    return ReadingStatus.normal;
  }
  if (s === READING_ALARM) {
    return ReadingStatus.alarm;
  }
  return ReadingStatus.unknown;
}

function toKeypoint(r: RawKeypoint): Keypoint {
  return new Keypoint(r.name, r.x, r.y, r.conf);
}

function toDetection(r: RawDetection): Detection {
  const kps: Keypoint[] = (r.keypoints ?? []).map((k: RawKeypoint) => toKeypoint(k));
  return new Detection(r.bbox ?? [], r.score, kps);
}

function toFrameMeta(r: RawFrameMeta): FrameMeta {
  const dets: Detection[] = (r.detections ?? []).map((d: RawDetection) => toDetection(d));
  return new FrameMeta(r.frame_id, r.timestamp, r.fps, r.inference_time_ms,
    r.yolo_time_ms, r.pose_time_ms, r.num_detections, dets, r.gauge_angles ?? []);
}

/**
 * 解析 WS 文本消息：frame_meta → meta；{"type":"error"} → error；其余/解析失败 → 双 null（调用方丢弃）。
 * 全程 try/catch（防 NaN 等非法 JSON 直接崩）。
 */
export function parseVisionText(text: string): VisionTextMessage {
  try {
    const env: RawEnvelope = JSON.parse(text) as RawEnvelope;
    if (env.type === 'error') {
      return new VisionTextMessage(null, env.message ?? '未知错误');
    }
    if (env.type === 'frame_meta') {
      return new VisionTextMessage(toFrameMeta(JSON.parse(text) as RawFrameMeta), null);
    }
    return new VisionTextMessage(null, null);
  } catch (e) {
    return new VisionTextMessage(null, null);
  }
}

/** 解析 /api/summary。失败返回 null。 */
export function parseSummary(text: string): Summary | null {
  try {
    const r: RawSummary = JSON.parse(text) as RawSummary;
    const readings: Reading[] = (r.readings ?? []).map((x: RawReading) =>
      new Reading(x.value, x.unit, x.label, toStatus(x.status), x.gauge_id));
    return new Summary(r.fps, r.latency_ms, r.frame_id, r.over_limit, readings);
  } catch (e) {
    return null;
  }
}

/** 解析 /api/video/status。失败返回 null。 */
export function parseVideoStatus(text: string): VideoStatus | null {
  try {
    const r: RawVideoStatus = JSON.parse(text) as RawVideoStatus;
    return new VideoStatus(r.running, r.fps, r.frame_count);
  } catch (e) {
    return null;
  }
}

/** 解析单个仪表配置。失败返回 null。 */
export function parseGaugeConfig(text: string): GaugeConfig | null {
  try {
    const r: RawGaugeConfig = JSON.parse(text) as RawGaugeConfig;
    return new GaugeConfig(r.unit, r.min_range, r.max_range, r.low_threshold, r.high_threshold, r.display_name);
  } catch (e) {
    return null;
  }
}
