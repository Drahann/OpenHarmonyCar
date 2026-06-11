/**
 * SymbolGlyph 资源名 —— 单一来源（docs/ui-polish-plan.md §2）。
 * 真机编译若某 symbol 名不存在，**只改本文件**（DevEco 资源面板 → Symbol 库挑近似名）。
 * 用法：`SymbolGlyph(Icons.back).fontSize(20).fontColor([AppColor.textBody])`（fontColor 取**数组**）。
 * ⚠️ 意向名未经 DevEco 验证（SDK API 12 自带 sys.symbol 库）。
 */
export class Icons {
  static readonly back: Resource = $r('sys.symbol.chevron_left');
  static readonly forward: Resource = $r('sys.symbol.chevron_right');
  static readonly expand: Resource = $r('sys.symbol.chevron_down');
  static readonly close: Resource = $r('sys.symbol.xmark');
  static readonly plus: Resource = $r('sys.symbol.plus');
  static readonly minus: Resource = $r('sys.symbol.minus');
  /** 地图视图回中（定位/取景语义；备选 sys.symbol.location）。 */
  static readonly recenter: Resource = $r('sys.symbol.dot_viewfinder');
  static readonly video: Resource = $r('sys.symbol.video');
  /** 设备互信（链接/配对语义）。 */
  static readonly trust: Resource = $r('sys.symbol.link');
  static readonly search: Resource = $r('sys.symbol.magnifyingglass');
  static readonly warn: Resource = $r('sys.symbol.exclamationmark_triangle');
}
