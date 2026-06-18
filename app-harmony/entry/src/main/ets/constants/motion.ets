/**
 * 动效 token —— 全 App 动画时长/曲线单一来源（docs/ui-polish-plan.md §3）。
 * 取向：快进慢停、克制（无弹跳炫技、不与地图 overlay ~20fps 动效叠加）。
 * 用法：状态变更包 `animateTo({ duration: Motion.base, curve: Motion.curveStd }, () => {...})`；
 *      条件分支组件挂 `.transition(...)` 后，需在动画上下文（animateTo）内翻转条件才有过渡。
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）。curves 形参以 SDK 为准。
 */
import { curves } from '@kit.ArkUI';

export class Motion {
  /** 快（状态点变色 / 遮罩出没）。 */
  static readonly fast: number = 150;
  /** 标准（卡片/面板出没、内容切换）。 */
  static readonly base: number = 250;
  /** 慢（页面转场、PiP 形变）。 */
  static readonly slow: number = 350;
  /** 标准出入曲线（快进慢停）。 */
  static readonly curveStd: Curve = Curve.FastOutSlowIn;
  /** 软弹簧：PiP 胶囊⇄小窗等卡片形变。 */
  static readonly springSoft: curves.ICurve = curves.springMotion(0.4, 0.85);
  /** 快弹簧：摇杆松手回中（快、略弹）。 */
  static readonly springSnap: curves.ICurve = curves.springMotion(0.3, 0.65);
}
