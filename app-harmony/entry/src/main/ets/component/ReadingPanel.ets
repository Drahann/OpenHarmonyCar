/**
 * ReadingPanel —— 仪表读数 / 状态 / FPS / 告警面板。**纯展示**（dumb view）：只渲染容器传入的轻量 VM，
 * 不碰 service / model，避免每帧把 model 类对象穿过 @Prop 引发的响应式开销。容器（VisionPage）按节流
 * 频率重建 VM 列表再下传。
 *
 * 复用品牌主题（constants/theme.ets）：告警用 danger、在线用 success，与导航 UI 统一。
 * ⚠️ 未经 DevEco 验证。
 */

import { AppColor, FontSize, FontFamily, Space, Radius, Elevation } from '../constants/theme';

/** 单个仪表读数的展示模型（容器从 FrameMeta / Summary 构造）。 */
export interface GaugeReadingVM {
  label: string;      // 仪表名 / 序号（如 "压力表" 或 "表 1"）
  valueText: string;  // 主读数（如 "80.5 %" 或 "1.29 MPa"）
  sub: string;        // 次要信息（如 "score 0.99" / 百分比）
  alarm: boolean;     // 越限告警 → 红
  unknown: boolean;   // 无法识别（NaN）→ 灰、显示"无法识别"
}

@Component
export struct ReadingPanel {
  @Prop connLabel: string = '';
  @Prop connColor: string = AppColor.textSecondary;
  @Prop fps: number = 0;
  @Prop inferenceMs: number = 0;
  @Prop numDetections: number = 0;
  @Prop readings: GaugeReadingVM[] = [];
  /** 服务器错误（{"type":"error"}）或空串。非空时高亮告警条。 */
  @Prop errorMessage: string = '';

  build() {
    Column({ space: Space.md }) {
      // —— 连接 / 性能行 ——
      Row() {
        Circle({ width: 10, height: 10 }).fill(this.connColor).margin({ right: Space.sm })
        Text(this.connLabel)
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.textBody)
        Blank()
        Text(`${this.fps.toFixed(1)} FPS`)
          .fontSize(FontSize.dataSm)
          .fontFamily(FontFamily.mono)
          .fontColor(AppColor.textSecondary)
      }
      .width('100%')

      Text(`推理 ${this.inferenceMs.toFixed(0)} ms · 检测到 ${this.numDetections} 个仪表`)
        .fontSize(FontSize.caption)
        .fontColor(AppColor.textSecondary)
        .width('100%')

      // —— 错误条 ——
      if (this.errorMessage.length > 0) {
        Text(`⚠ ${this.errorMessage}`)
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.onPrimary)
          .backgroundColor(AppColor.danger)
          .borderRadius(Radius.sm)
          .padding(Space.sm)
          .width('100%')
      }

      Divider().color(AppColor.divider)

      // —— 读数列表 / 空态 ——
      if (this.readings.length === 0) {
        Text('暂无读数')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.textCaption)
          .width('100%')
          .padding({ top: Space.sm, bottom: Space.sm })
      } else {
        ForEach(this.readings, (r: GaugeReadingVM) => {
          this.ReadingRow(r)
        }, (r: GaugeReadingVM, idx: number) => `${idx}-${r.label}`)
      }
    }
    .width('100%')
    .padding(Space.md)
    .backgroundColor(AppColor.surface)
    .borderRadius(Radius.md)
    .shadow({
      radius: Elevation.cardRadius,
      color: Elevation.cardColor,
      offsetX: 0,
      offsetY: Elevation.cardOffsetY
    })
  }

  @Builder
  ReadingRow(r: GaugeReadingVM) {
    Row() {
      Column() {
        Text(r.label)
          .fontSize(FontSize.body)
          .fontColor(AppColor.textBody)
        if (r.sub.length > 0) {
          Text(r.sub)
            .fontSize(FontSize.caption)
            .fontColor(AppColor.textSecondary)
            .margin({ top: 2 })
        }
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)

      Text(r.unknown ? '无法识别' : r.valueText)
        .fontSize(FontSize.dataMd)
        .fontFamily(FontFamily.mono)
        .fontColor(this.valueColor(r))
    }
    .width('100%')
    .padding({ top: Space.sm, bottom: Space.sm })
  }

  private valueColor(r: GaugeReadingVM): string {
    if (r.unknown) {
      return AppColor.textCaption;
    }
    if (r.alarm) {
      return AppColor.danger;
    }
    return AppColor.success;
  }
}
