/**
 * VideoView —— 视频帧显示（JPEG ArrayBuffer → PixelMap → Image），可复用于全屏 VisionPage 与收起式 PiP 小窗。
 *
 * 性能要点（docs/vision-integration-plan.md §4）：
 *  - **只显最新帧 + 解码进行中丢帧**：来帧只覆盖 `latest`，解码完成后取最新待解帧，旧帧直接丢，
 *    防积压 / 内存涨 / 延迟累积（香橙派 ~15FPS，平板解码可能跟不上）。
 *  - **节流刷新**：两次渲染间隔 ≥ VIDEO_MIN_RENDER_INTERVAL_MS（≈30fps 上限）。
 *  - **及时回收**：换帧即 `release()` 旧 PixelMap + ImageSource，避免 native 内存泄漏。
 *
 * 自订阅 VisionService 的**帧流**（onFrame）；连接/断开由容器（VisionPage / PiP）按视图可见性驱动，
 * 本组件只负责显示，不 connect/disconnect（保证 WS 生命周期单点管理）。
 *
 * ⚠️ 未经 DevEco 验证：`@kit.ImageKit` 的 createImageSource/createPixelMap/release 形参以 SDK 为准，首编按报错修。
 */

import { image } from '@kit.ImageKit';
import { VisionService, VisionStreamListener, VisionConnState } from '../service/VisionService';
import { FrameMeta } from '../model/vision';
import { VIDEO_MIN_RENDER_INTERVAL_MS } from '../constants/vision';
import { AppColor, FontSize } from '../constants/theme';
import { Log, Logger } from '../utils/log';

/** 只取帧流的订阅适配器（其余回调 no-op，避免与容器的 meta/state 订阅重复处理）。 */
class FrameSink implements VisionStreamListener {
  private readonly onFrameCb: (jpeg: ArrayBuffer) => void;

  constructor(onFrameCb: (jpeg: ArrayBuffer) => void) {
    this.onFrameCb = onFrameCb;
  }

  onFrame(jpeg: ArrayBuffer): void {
    this.onFrameCb(jpeg);
  }

  onMeta(_meta: FrameMeta): void {
  }

  onStateChange(_state: VisionConnState): void {
  }

  onStreamError(_message: string): void {
  }
}

@Component
export struct VideoView {
  /** 无帧时的占位文案（容器可按连接态定制）。 */
  @Prop placeholder: string = '等待视频…';

  private readonly log: Logger = Log.scoped('VideoView');
  private service: VisionService = VisionService.getInstance();
  private sink: FrameSink | null = null;

  @State private pixelMap: image.PixelMap | undefined = undefined;
  /** 始终只保留**最新**待解码帧（解码期间来的新帧覆盖它、旧帧丢弃）。 */
  private latest: ArrayBuffer | null = null;
  private decoding: boolean = false;
  private lastRenderAt: number = 0;
  private throttleTimer: number = -1;

  aboutToAppear(): void {
    this.sink = new FrameSink((jpeg: ArrayBuffer) => this.acceptFrame(jpeg));
    this.service.subscribe(this.sink);
  }

  aboutToDisappear(): void {
    if (this.sink !== null) {
      this.service.unsubscribe(this.sink);
      this.sink = null;
    }
    if (this.throttleTimer !== -1) {
      clearTimeout(this.throttleTimer);
      this.throttleTimer = -1;
    }
    this.latest = null;
    this.decoding = false;
    this.releasePixelMap();
  }

  build() {
    Stack() {
      if (this.pixelMap !== undefined) {
        Image(this.pixelMap)
          .objectFit(ImageFit.Contain)
          .width('100%')
          .height('100%')
      } else {
        Text(this.placeholder)
          .fontSize(FontSize.body)
          .fontColor(AppColor.textCaption)
      }
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#000000')  // 视频底色黑，留边（Contain）不突兀
  }

  // ── 丢帧解码（只显最新帧）──────────────────────────────────────────────────
  private acceptFrame(jpeg: ArrayBuffer): void {
    this.latest = jpeg;  // 覆盖：始终只留最新
    if (this.decoding || this.throttleTimer !== -1) {
      return;  // 正在解码 / 正在等节流：本帧已入 latest，等下轮统一取最新
    }
    const wait: number = VIDEO_MIN_RENDER_INTERVAL_MS - (Date.now() - this.lastRenderAt);
    if (wait > 0) {
      this.throttleTimer = setTimeout(() => {
        this.throttleTimer = -1;
        this.decodeLatest();
      }, wait);
    } else {
      this.decodeLatest();
    }
  }

  private decodeLatest(): void {
    const buf: ArrayBuffer | null = this.latest;
    this.latest = null;
    if (buf === null) {
      this.decoding = false;
      return;
    }
    this.decoding = true;
    const src: image.ImageSource = image.createImageSource(buf);
    src.createPixelMap().then((pm: image.PixelMap) => {
      const old: image.PixelMap | undefined = this.pixelMap;
      this.pixelMap = pm;
      if (old !== undefined) {
        old.release();  // 回收旧帧 native 内存
      }
      src.release();
      this.lastRenderAt = Date.now();
      // 解码期间若有更新帧到来，继续解最新；否则收尾
      if (this.latest !== null) {
        this.decodeLatest();
      } else {
        this.decoding = false;
      }
    }).catch((e: Object) => {
      this.log.w(`decode fail: ${JSON.stringify(e)}`);
      src.release();
      this.decoding = false;
    });
  }

  private releasePixelMap(): void {
    if (this.pixelMap !== undefined) {
      this.pixelMap.release();
      this.pixelMap = undefined;
    }
  }
}
