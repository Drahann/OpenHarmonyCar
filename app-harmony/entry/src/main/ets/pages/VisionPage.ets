/**
 * VisionPage —— 全屏仪表识别页（形态A，主推）：实时视频 + 读数面板。
 *
 * 🔴 **不打扰导航**：本页只连/断**自己的** VisionService（香橙派 WS），不碰 RobotTransport（紫派 UDP 保活）。
 *   从 ControlPage `router.pushUrl` 进来时 ControlPage **不被销毁**（其 aboutToDisappear/stopHeartbeat 仅在 pop 时触发），
 *   导航保活留在 RobotTransport 单例里继续跑——进出视频页，导航控制/建图/地图照常。
 *
 * **WS 生命周期 = 视图可见**：aboutToAppear → connect + subscribe；aboutToDisappear → unsubscribe + disconnect
 *   （省平板流量/电；香橙派常驻推理不受连断影响）。**只读订阅**，不向香橙派发任何控制。
 *
 * 帧显示交 VideoView（自订阅帧流 + 丢帧解码）；本页订阅 meta/state/error 驱动 ReadingPanel（节流刷新）。
 * ⚠️ 未经 DevEco 验证；路由需在 main_pages.json 注册。
 */

import { router } from '@kit.ArkUI';
import { VisionService, VisionStreamListener, VisionConnState } from '../service/VisionService';
import { FrameMeta, Detection } from '../model/vision';
import { Storage } from '../service/storage';
import { VideoView } from '../component/VideoView';
import { ReadingPanel, GaugeReadingVM } from '../component/ReadingPanel';
import { AppColor, FontSize, FontFamily, Space, Radius, TOUCH_MIN } from '../constants/theme';
import { Log, Logger } from '../utils/log';

const log: Logger = Log.scoped('VisionPage');
/** 读数面板刷新节流（ms）：视频可 15fps，但面板文字无需每帧重排，约 4/s 足够。 */
const PANEL_REFRESH_MS: number = 250;

/** 订阅适配器：把 VisionService 回调转发到页面方法（帧流由 VideoView 自管，这里 onFrame no-op）。 */
class VisionPageSink implements VisionStreamListener {
  private readonly onMetaCb: (meta: FrameMeta) => void;
  private readonly onStateCb: (state: VisionConnState) => void;
  private readonly onErrorCb: (message: string) => void;

  constructor(onMetaCb: (meta: FrameMeta) => void, onStateCb: (state: VisionConnState) => void,
    onErrorCb: (message: string) => void) {
    this.onMetaCb = onMetaCb;
    this.onStateCb = onStateCb;
    this.onErrorCb = onErrorCb;
  }

  onFrame(_jpeg: ArrayBuffer): void {
  }

  onMeta(meta: FrameMeta): void {
    this.onMetaCb(meta);
  }

  onStateChange(state: VisionConnState): void {
    this.onStateCb(state);
  }

  onStreamError(message: string): void {
    this.onErrorCb(message);
  }
}

@Entry
@Component
struct VisionPage {
  private service: VisionService = VisionService.getInstance();
  private sink: VisionPageSink | null = null;
  private host: string = Storage.getVisionHost();
  private port: number = Storage.getVisionPort();
  private lastPanelAt: number = 0;

  @State private connState: VisionConnState = VisionConnState.idle;
  @State private fps: number = 0;
  @State private inferenceMs: number = 0;
  @State private numDetections: number = 0;
  @State private readings: GaugeReadingVM[] = [];
  @State private errorMessage: string = '';

  aboutToAppear(): void {
    this.sink = new VisionPageSink(
      (meta: FrameMeta) => this.onMeta(meta),
      (state: VisionConnState) => this.onState(state),
      (message: string) => this.onError(message)
    );
    this.service.subscribe(this.sink);
    this.service.connect(this.host, this.port);  // WS 随视图打开而连
    log.i(`open vision ${this.host}:${this.port}`);
  }

  aboutToDisappear(): void {
    if (this.sink !== null) {
      this.service.unsubscribe(this.sink);
      this.sink = null;
    }
    this.service.disconnect();  // WS 随视图收起而断；香橙派常驻不受影响、导航毫不相干
    log.i('close vision');
  }

  build() {
    Column() {
      // —— 顶栏 ——
      Row() {
        Button('‹ 返回')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.textBody)
          .backgroundColor(AppColor.surface)
          .borderRadius(Radius.pill)
          .height(40)
          .padding({ left: Space.md, right: Space.md })
          .onClick(() => router.back())
        Column() {
          Text('仪表识别')
            .fontSize(FontSize.h2)
            .fontColor(AppColor.textTitle)
            .fontWeight(FontWeight.Bold)
          Text(`${this.host}:${this.port}`)
            .fontSize(FontSize.caption)
            .fontFamily(FontFamily.mono)
            .fontColor(AppColor.textSecondary)
        }
        .alignItems(HorizontalAlign.Start)
        .margin({ left: Space.md })
        Blank()
        Row() {
          Circle({ width: 10, height: 10 }).fill(this.stateColor()).margin({ right: Space.xs })
          Text(this.stateLabel())
            .fontSize(FontSize.bodySm)
            .fontColor(AppColor.textSecondary)
        }
      }
      .width('100%')
      .padding({ top: Space.md, bottom: Space.md })

      // —— 视频区（占主空间）——
      Stack() {
        VideoView({ placeholder: this.videoPlaceholder() })
        // 未连接/重连时的轻提示浮层（不挡画面，仅在无帧态有意义；VideoView 自身也有占位）
        if (this.connState === VisionConnState.connecting || this.connState === VisionConnState.reconnecting) {
          Row() {
            LoadingProgress().width(20).height(20).color(AppColor.onPrimary)
            Text(this.stateLabel())
              .fontSize(FontSize.bodySm)
              .fontColor(AppColor.onPrimary)
              .margin({ left: Space.xs })
          }
          .padding({ left: Space.md, right: Space.md, top: Space.sm, bottom: Space.sm })
          .backgroundColor('rgba(0,0,0,0.45)')
          .borderRadius(Radius.pill)
        }
      }
      .width('100%')
      .layoutWeight(3)
      .borderRadius(Radius.md)
      .clip(true)

      // —— 读数面板 ——
      Scroll() {
        ReadingPanel({
          connLabel: this.stateLabel(),
          connColor: this.stateColor(),
          fps: this.fps,
          inferenceMs: this.inferenceMs,
          numDetections: this.numDetections,
          readings: this.readings,
          errorMessage: this.errorMessage
        })
      }
      .width('100%')
      .layoutWeight(2)
      .margin({ top: Space.md })
      .align(Alignment.Top)
    }
    .width('100%')
    .height('100%')
    .padding({ left: Space.lg, right: Space.lg })
    .backgroundColor(AppColor.pageBg)
  }

  // ── 回调 ────────────────────────────────────────────────────────────────────
  private onMeta(meta: FrameMeta): void {
    const now: number = Date.now();
    if (now - this.lastPanelAt < PANEL_REFRESH_MS) {
      return;  // 节流：面板无需每帧重排
    }
    this.lastPanelAt = now;
    this.fps = meta.fps;
    this.inferenceMs = meta.inferenceTimeMs;
    this.numDetections = meta.numDetections;
    this.readings = this.buildReadings(meta);
  }

  private onState(state: VisionConnState): void {
    this.connState = state;
    if (state === VisionConnState.connected) {
      this.errorMessage = '';  // 连上即清旧错误
    }
  }

  private onError(message: string): void {
    this.errorMessage = message;
  }

  /** FrameMeta → 读数 VM 列表（V3：百分比 + 检测置信度；物理量纲/告警阈值留 V4 接 REST 富化）。 */
  private buildReadings(meta: FrameMeta): GaugeReadingVM[] {
    const list: GaugeReadingVM[] = [];
    for (let i = 0; i < meta.gaugeAngles.length; i++) {
      const percent: number = meta.gaugeAngles[i];
      const det: Detection | undefined = i < meta.detections.length ? meta.detections[i] : undefined;
      const unknown: boolean = Number.isNaN(percent);
      list.push({
        label: `表 ${i + 1}`,
        valueText: `${percent.toFixed(1)} %`,
        sub: det !== undefined ? `score ${det.score.toFixed(2)}` : '',
        alarm: false,
        unknown
      });
    }
    return list;
  }

  // ── 连接态展示 ────────────────────────────────────────────────────────────
  private stateLabel(): string {
    switch (this.connState) {
      case VisionConnState.connecting:
        return '连接中…';
      case VisionConnState.connected:
        return '已连接';
      case VisionConnState.reconnecting:
        return '重连中…';
      case VisionConnState.closed:
        return '已断开';
      default:
        return '未连接';
    }
  }

  private stateColor(): string {
    switch (this.connState) {
      case VisionConnState.connected:
        return AppColor.success;
      case VisionConnState.connecting:
      case VisionConnState.reconnecting:
        return AppColor.warning;
      case VisionConnState.closed:
        return AppColor.danger;
      default:
        return AppColor.textCaption;
    }
  }

  private videoPlaceholder(): string {
    if (this.connState === VisionConnState.connected) {
      return '等待视频帧…';
    }
    return `${this.stateLabel()}（香橙派 ${this.host}:${this.port}）`;
  }
}
