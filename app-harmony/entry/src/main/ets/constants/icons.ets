/**
 * SymbolGlyph 资源名 —— 单一来源（docs/ui-polish-plan.md §2）。
 * 真机编译若某 symbol 名不存在，**只改本文件**（DevEco 资源面板 → Symbol 库挑近似名）。
 * 用法：`SymbolGlyph(Icons.back).fontSize(20).fontColor([AppColor.textBody])`（fontColor 取**数组**）。
 * ✅ 首轮 hvigor 已校验（2026-06-11）：本表全部名称通过编译（scope 系对照 SDK
 *    `ets-loader/sysResource.js` symbol 段核实后替换原 dot_viewfinder）。
 */
export class Icons {
  static readonly back: Resource = $r('sys.symbol.chevron_left');
  static readonly forward: Resource = $r('sys.symbol.chevron_right');
  static readonly expand: Resource = $r('sys.symbol.chevron_down');
  static readonly close: Resource = $r('sys.symbol.xmark');
  static readonly plus: Resource = $r('sys.symbol.plus');
  static readonly minus: Resource = $r('sys.symbol.minus');
  /** 地图视图回中（十字准星；已对 SDK sysResource.js symbol 段核实存在；备选 local / circle_viewfinder）。 */
  static readonly recenter: Resource = $r('sys.symbol.scope');
  static readonly video: Resource = $r('sys.symbol.video');
  /** 设备互信（链接/配对语义）。 */
  static readonly trust: Resource = $r('sys.symbol.link');
  static readonly search: Resource = $r('sys.symbol.magnifyingglass');
  static readonly warn: Resource = $r('sys.symbol.exclamationmark_triangle');
}
