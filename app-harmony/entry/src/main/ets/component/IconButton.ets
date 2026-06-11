/**
 * IconButton —— 圆形图标按钮（顶栏工具钮 40 / 地图 FAB 44 通用），取代散落的字符按钮
 * （'‹ 返回'、'＋/－/⊙'、'✕' 等，见 docs/ui-polish-plan.md §2/§4）。
 * 白底 + 软阴影 + SymbolGlyph；尺寸/字号/颜色可配，规格默认=顶栏工具钮（40）。
 * ⚠️ 未经 DevEco 验证：SymbolGlyph fontColor 取数组，资源名单点在 constants/icons.ets。
 */
import { AppColor, Elevation } from '../constants/theme';

@Component
export struct IconButton {
  /** symbol 资源（从 constants/icons.ets 的 Icons 取）。 */
  icon: Resource = $r('sys.symbol.xmark');
  /** 按钮直径（vp）：顶栏 40 / FAB 44（TOUCH_MIN）。 */
  size: number = 40;
  /** 图标字号（vp）。 */
  glyph: number = 20;
  /** 图标颜色。 */
  color: string = AppColor.textBody;
  /** 底色（默认白；主色钮传 AppColor.primary 并配 onPrimary 图标色）。 */
  bg: string = AppColor.surface;
  onTap?: () => void;

  build() {
    Button() {
      SymbolGlyph(this.icon)
        .fontSize(this.glyph)
        .fontColor([this.color])
    }
    .type(ButtonType.Circle)
    .width(this.size)
    .height(this.size)
    .backgroundColor(this.bg)
    .shadow({
      radius: Elevation.softRadius,
      color: Elevation.softColor,
      offsetX: 0,
      offsetY: Elevation.softOffsetY
    })
    .onClick(() => {
      if (this.onTap !== undefined) {
        this.onTap();
      }
    })
  }
}
