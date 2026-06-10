/**
 * 香橙派视觉服务客户端 —— 独立 WebSocket 收视频流/读数 + REST 配置/报告。
 *
 * 🔴 **与导航链路完全解耦**：本服务**不 import** RobotTransport / MapService / FleetMissionService，
 *    自持一条 WebSocket（香橙派 192.168.1.5:8000），与紫派 UDP:5001 / HTTP:8000 互不共享 socket、
 *    互不影响。进出/收起视频**只**连/断本 WS，导航保活/控制/建图照常运行（docs/vision-integration-plan.md §3）。
 * **只读订阅**：对香橙派只收视频/读数 + 用户主动的 REST（配置/报告），不发任何控制。
 *
 * WS 生命周期 = 视图可见性：视图展开/进入时 connect、收起/离开时 disconnect（省平板流量/电）；
 * 香橙派常驻推理，不受 App 连断影响。断开按退避自动重连（契约 §6.3 建议 3s）。
 *
 * 单例：整个 App 共享一个实例。UI 经 component/VideoView、pages/VisionPage 订阅。
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）：`@kit.NetworkKit` 的 webSocket/http API 形参以 SDK 为准，首编按报错修。
 */

import { webSocket, http } from '@kit.NetworkKit';
import { BusinessError } from '@kit.BasicServicesKit';
import {
  wsVideoUrl, visionApiBase,
  API_SUMMARY, API_VIDEO_STATUS, API_HISTORY, API_THRESHOLDS, API_GAUGE_CONFIGS,
  API_DATA_REPORT, API_DATA_REPORTS,
  VISION_RECONNECT_BASE_MS, VISION_RECONNECT_MAX_MS, VISION_HTTP_TIMEOUT_MS, VISION_REPORT_TIMEOUT_MS
} from '../constants/vision';
import {
  FrameMeta, Summary, VideoStatus, GaugeConfig, VisionTextMessage,
  parseVisionText, parseSummary, parseVideoStatus, parseGaugeConfig
} from '../model/vision';
import { Log, Logger } from '../utils/log';

/** WS 连接状态（驱动 UI：连接中 / 已连 / 重连中 / 已关）。 */
export enum VisionConnState {
  idle = 'idle',                 // 未连接（初始 / 主动断开后）
  connecting = 'connecting',     // 首次连接中
  connected = 'connected',       // 已连，正在收帧
  reconnecting = 'reconnecting', // 断开后退避重连中
  closed = 'closed'              // 收到服务器 error / 主动关闭
}

/**
 * 视频流订阅者。视图实现它，connect 前 subscribe、disconnect 时 unsubscribe。
 *  - onFrame：一帧 JPEG（ArrayBuffer）。**调用方负责丢帧/解码/回收**（VideoView 解码进行中丢新帧、只显最新）。
 *  - onMeta：与该帧交替推送的元数据。
 *  - onStateChange：连接态变化（UI 显示"连接中/已连/重连中"）。
 *  - onStreamError：服务器 {"type":"error"} 或 WS 错误（显示提示，随后自动重连）。
 */
export interface VisionStreamListener {
  onFrame(jpeg: ArrayBuffer): void;
  onMeta(meta: FrameMeta): void;
  onStateChange(state: VisionConnState): void;
  onStreamError(message: string): void;
}

export class VisionService {
  private static instance: VisionService | null = null;

  static getInstance(): VisionService {
    const inst: VisionService = VisionService.instance ?? new VisionService();
    VisionService.instance = inst;
    return inst;
  }

  private readonly log: Logger = Log.scoped('VisionService');
  private ws: webSocket.WebSocket | null = null;
  private host: string = '';
  private port: number = 0;
  /** true 表示用户希望保持连接（断开后应自动重连）；disconnect() 置 false 终止重连。 */
  private shouldRun: boolean = false;
  private reconnectTimer: number = -1;
  private backoffMs: number = VISION_RECONNECT_BASE_MS;
  private state: VisionConnState = VisionConnState.idle;
  private listeners: Set<VisionStreamListener> = new Set<VisionStreamListener>();

  private constructor() {
  }

  // ── 订阅 ──────────────────────────────────────────────────────────────────
  subscribe(listener: VisionStreamListener): void {
    this.listeners.add(listener);
    listener.onStateChange(this.state);  // 立刻同步当前态
  }

  unsubscribe(listener: VisionStreamListener): void {
    this.listeners.delete(listener);
  }

  currentState(): VisionConnState {
    return this.state;
  }

  // ── 连接生命周期 ───────────────────────────────────────────────────────────
  /** 连接香橙派视频流。视图可见时调用；重复调用同地址且已连则忽略。 */
  connect(host: string, port: number): void {
    if (this.ws !== null && this.host === host && this.port === port &&
      (this.state === VisionConnState.connected || this.state === VisionConnState.connecting)) {
      return;
    }
    // 地址变更或重新打开：先彻底关掉旧连接
    this.teardownSocket();
    this.host = host;
    this.port = port;
    this.shouldRun = true;
    this.backoffMs = VISION_RECONNECT_BASE_MS;
    this.openSocket();
  }

  /** 主动断开（视图收起/离开）。停重连、关 socket。香橙派常驻不受影响。 */
  disconnect(): void {
    this.shouldRun = false;
    if (this.reconnectTimer !== -1) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = -1;
    }
    this.teardownSocket();
    this.setState(VisionConnState.idle);
    this.log.i('disconnected (view hidden)');
  }

  /** 关掉并释放当前 socket（不改 shouldRun/state）。 */
  private teardownSocket(): void {
    const ws: webSocket.WebSocket | null = this.ws;
    this.ws = null;
    if (ws !== null) {
      try {
        ws.off('message');
        ws.off('open');
        ws.off('close');
        ws.off('error');
        ws.close((err: BusinessError) => {
          if (err) {
            this.log.w(`ws close err code=${err.code}`);
          }
        });
      } catch (e) {
        this.log.w(`teardown error: ${JSON.stringify(e)}`);
      }
    }
  }

  /** 新建一条 WS 并连接，注册回调。 */
  private openSocket(): void {
    if (this.reconnectTimer !== -1) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = -1;
    }
    const url: string = wsVideoUrl(this.host, this.port);
    this.setState(this.backoffMs > VISION_RECONNECT_BASE_MS ? VisionConnState.reconnecting : VisionConnState.connecting);
    const ws: webSocket.WebSocket = webSocket.createWebSocket();
    this.ws = ws;

    ws.on('open', (_err: BusinessError, _value: Object) => {
      this.backoffMs = VISION_RECONNECT_BASE_MS;  // 连上即复位退避
      this.setState(VisionConnState.connected);
      this.log.i(`ws connected ${url}`);
    });

    // 每帧两条交替：二进制 ArrayBuffer = JPEG；文本 string = frame_meta / error
    ws.on('message', (_err: BusinessError, value: string | ArrayBuffer) => {
      if (value instanceof ArrayBuffer) {
        this.listeners.forEach((l: VisionStreamListener) => l.onFrame(value));
        return;
      }
      if (typeof value === 'string') {
        const msg: VisionTextMessage = parseVisionText(value);
        if (msg.meta !== null) {
          this.listeners.forEach((l: VisionStreamListener) => l.onMeta(msg.meta as FrameMeta));
        } else if (msg.error !== null) {
          this.log.w(`server error: ${msg.error}`);
          this.listeners.forEach((l: VisionStreamListener) => l.onStreamError(msg.error as string));
        }
      }
    });

    ws.on('close', (_err: BusinessError, value: webSocket.CloseResult) => {
      this.log.i(`ws closed code=${value?.code ?? -1} reason=${value?.reason ?? ''}`);
      this.ws = null;
      this.onConnectionDropped();
    });

    ws.on('error', (err: BusinessError) => {
      this.log.w(`ws error code=${err?.code} msg=${err?.message}`);
      // error 之后通常紧跟 close；若未触发 close 则在此兜底重连
      if (this.ws === ws) {
        this.ws = null;
        this.onConnectionDropped();
      }
    });

    ws.connect(url, (err: BusinessError, _value: boolean) => {
      if (err) {
        this.log.w(`ws connect fail code=${err.code} msg=${err.message}`);
        if (this.ws === ws) {
          this.ws = null;
          this.onConnectionDropped();
        }
      }
    });
  }

  /** 连接掉线：若仍希望保活则退避重连，否则归 idle。 */
  private onConnectionDropped(): void {
    if (!this.shouldRun) {
      this.setState(VisionConnState.idle);
      return;
    }
    if (this.reconnectTimer !== -1) {
      return;  // 已排程
    }
    this.setState(VisionConnState.reconnecting);
    const delay: number = this.backoffMs;
    this.log.i(`reconnect in ${delay}ms`);
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = -1;
      if (this.shouldRun) {
        this.openSocket();
      }
    }, delay);
    this.backoffMs = Math.min(this.backoffMs * 2, VISION_RECONNECT_MAX_MS);
  }

  private setState(s: VisionConnState): void {
    if (this.state === s) {
      return;
    }
    this.state = s;
    this.listeners.forEach((l: VisionStreamListener) => l.onStateChange(s));
  }

  // ── REST（只读 + 用户主动配置；与导航无关）────────────────────────────────
  /** 实时状态（fps/latency/readings）。 */
  async getSummary(): Promise<Summary | null> {
    const text: string = await this.httpGet(API_SUMMARY);
    return parseSummary(text);
  }

  /** 摄像头运行状态。 */
  async getVideoStatus(): Promise<VideoStatus | null> {
    const text: string = await this.httpGet(API_VIDEO_STATUS);
    return parseVideoStatus(text);
  }

  /** 最近 600 条读数记录（原始 JSON，绘图/格式化交上层 V4）。 */
  getHistoryRaw(): Promise<string> {
    return this.httpGet(API_HISTORY);
  }

  /** 设置报警阈值（百分比）。成功返回 true。 */
  async setThresholds(low: number, high: number): Promise<boolean> {
    const body: string = `{"low":${low},"high":${high}}`;
    const text: string = await this.httpPost(API_THRESHOLDS, body, VISION_HTTP_TIMEOUT_MS);
    return text.indexOf('"ok"') >= 0 && text.indexOf('true') >= 0;
  }

  /** 所有仪表配置（原始 JSON）。 */
  getGaugeConfigsRaw(): Promise<string> {
    return this.httpGet(API_GAUGE_CONFIGS);
  }

  /** 指定类型仪表配置。 */
  async getGaugeConfig(gaugeType: string): Promise<GaugeConfig | null> {
    const text: string = await this.httpGet(`${API_GAUGE_CONFIGS}/${gaugeType}`);
    return parseGaugeConfig(text);
  }

  /**
   * 生成 DeepSeek 分析报告（原始 JSON）。⚠️ 生成期间香橙派暂停摄像头释放 NPU，
   * 视频会停——上层须给用户"生成中、视频暂停"提示。耗时较长，用长超时。
   */
  generateReportRaw(hours: number = 24): Promise<string> {
    return this.httpPost(API_DATA_REPORT, `{"hours":${hours}}`, VISION_REPORT_TIMEOUT_MS);
  }

  /** 历史报告列表（原始 JSON）。 */
  getReportsRaw(limit: number = 10, offset: number = 0): Promise<string> {
    return this.httpGet(`${API_DATA_REPORTS}?limit=${limit}&offset=${offset}`);
  }

  // ── HTTP 底座（独立 http 实例，每次用完 destroy；与 MapService 同风格但互不共享）──
  private httpGet(path: string, timeoutMs: number = VISION_HTTP_TIMEOUT_MS): Promise<string> {
    return this.request(path, http.RequestMethod.GET, undefined, timeoutMs);
  }

  private httpPost(path: string, body: string, timeoutMs: number): Promise<string> {
    return this.request(path, http.RequestMethod.POST, body, timeoutMs);
  }

  private request(path: string, method: http.RequestMethod, body: string | undefined,
    timeoutMs: number): Promise<string> {
    const url: string = `${visionApiBase(this.host, this.port)}${path}`;
    return new Promise<string>((resolve: (t: string) => void, reject: (e: Error) => void) => {
      const req: http.HttpRequest = http.createHttp();
      const header: Record<string, string> = { 'Content-Type': 'application/json' };
      const options: http.HttpRequestOptions = {
        method,
        header,
        extraData: body,
        connectTimeout: VISION_HTTP_TIMEOUT_MS,
        readTimeout: timeoutMs,
        usingCache: false,
        expectDataType: http.HttpDataType.STRING
      };
      req.request(url, options, (err: BusinessError, data: http.HttpResponse) => {
        req.destroy();
        if (err) {
          reject(new Error(`vision http error: ${err.message}`));
          return;
        }
        if (data.responseCode < 200 || data.responseCode >= 300) {
          reject(new Error(`vision http status ${data.responseCode}`));
          return;
        }
        const result: string | Object | ArrayBuffer = data.result;
        resolve(typeof result === 'string' ? result : JSON.stringify(result));
      });
    });
  }
}
