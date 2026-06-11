/**
 * Joystick —— 摇杆遥控组件（取代旧 4 克隆页里各抄一份的 CarControlBuilder）。
 *
 * 关键修复（对照旧实现）：
 *  - **每实例独立节流**：节流计时器是本组件的实例字段（不再是 common.ets 里全局唯一 taskId，
 *    旧写法多个控件互相清定时器、相互抢占）。按住摇杆时每 THROTTLE_INTERVAL_MS 重发一次当前方向；
 *    松手回中即发一次 stop 并清本实例计时器。aboutToDisappear 兜底清理，杜绝泄漏。
 *  - **解耦传输**：本组件只产出 (MoveDirection, speed)，经 onDirection 回调交上层；
 *    由 ControlPage 路由到 service/RobotTransport（**多目标**：所选车各发一份）。组件本身可单测/复用。
 *
 * 方向映射：上=go(前进)、左=left、右=right、中心死区与下半区=stop
 *   （9 字节协议 MoveDirection 仅 stop/go/left/right，无后退，故下半区落 stop）。
 *
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）。
 */

import { MoveDirection } from '../model/protocol';
import { THROTTLE_INTERVAL_MS, SPEED_DEFAULT } from '../constants/ui';
import { AppColor, Elevation, TOUCH_MIN } from '../constants/theme';
import { Motion } from '../constants/motion';

const DEFAULT_DIAMETER: number = 180; // 底盘直径（vp）
const KNOB_RATIO: number = 0.42;      // 摇杆头直径 / 底盘
const DEADZONE_RATIO: number = 0.28;  // 死区半径 / 可移动半径

@Component
export struct Joystick {
  /** 推到边缘时下发的速度档（go/left/right 用）。 */
  @Prop speed: number = SPEED_DEFAULT;
  /** 底盘直径（vp）。 */
  @Prop diameter: number = DEFAULT_DIAMETER;
  /** 禁用（未连接车辆时）。 */
  @Prop disabled: boolean = false;
  /** 方向回调：按住时每节流周期触发一次（dir≠stop），松手触发一次 stop。 */
  onDirection?: (dir: MoveDirection, speed: number) => void;

  // 摇杆头相对中心位移（vp）
  @State private knobX: number = 0;
  @State private knobY: number = 0;
  @State private activeDir: MoveDirection = MoveDirection.stop;
  // 每实例节流计时器（核心：非全局）
  private throttleTimer: number = -1;
  private currentDir: MoveDirection = MoveDirection.stop;

  aboutToDisappear(): void {
    this.stopThrottle();
  }

  build() {
    Stack() {
      // 底盘
      Column()
        .width(this.diameter)
        .height(this.diameter)
        .borderRadius(this.diameter / 2)
        .backgroundColor(AppColor.primarySoft)
        .border({ width: 1, color: AppColor.border })
        .opacity(this.disabled ? 0.4 : 1)

      // 摇杆头
      Column() {
        Text(this.dirGlyph())
          .fontSize(this.knobDiameter() * 0.4)
          .fontColor(AppColor.onPrimary)
          .fontWeight(FontWeight.Bold)
      }
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .width(this.knobDiameter())
      .height(this.knobDiameter())
      .borderRadius(this.knobDiameter() / 2)
      .backgroundColor(this.activeDir === MoveDirection.stop ? AppColor.primary : AppColor.primaryPressed)
      .shadow({
        radius: Elevation.softRadius,
        color: Elevation.softColor,
        offsetX: 0,
        offsetY: Elevation.softOffsetY
      })
      .translate({ x: this.knobX, y: this.knobY })
    }
    .width(this.diameter)
    .height(this.diameter)
    .constraintSize({ minWidth: TOUCH_MIN, minHeight: TOUCH_MIN })
    .gesture(
      PanGesture()
        .onActionStart((e: GestureEvent) => this.onPan(e))
        .onActionUpdate((e: GestureEvent) => this.onPan(e))
        .onActionEnd(() => this.release())
        .onActionCancel(() => this.release())
    )
  }

  // ── 手势 ────────────────────────────────────────────────────────────────────
  private onPan(e: GestureEvent): void {
    if (this.disabled) {
      return;
    }
    const maxR: number = this.maxKnobRadius();
    let dx: number = e.offsetX;
    let dy: number = e.offsetY;
    const mag: number = Math.sqrt(dx * dx + dy * dy);
    if (mag > maxR && mag > 0) {
      // 限制在可移动半径内
      dx = dx / mag * maxR;
      dy = dy / mag * maxR;
    }
    this.knobX = dx;
    this.knobY = dy;
    this.setDirection(this.dirOf(dx, dy, maxR));
  }

  private release(): void {
    // 摇杆头弹簧回中（ui-polish-plan §3）；stop 指令立即下发，不等动画。
    animateTo({ curve: Motion.springSnap }, () => {
      this.knobX = 0;
      this.knobY = 0;
    });
    this.setDirection(MoveDirection.stop);
  }

  // ── 方向 + 节流 ─────────────────────────────────────────────────────────────
  /** 位移 → 方向（屏幕 y 向下，需取负回到数学角）。 */
  private dirOf(dx: number, dy: number, maxR: number): MoveDirection {
    const mag: number = Math.sqrt(dx * dx + dy * dy);
    if (mag < maxR * DEADZONE_RATIO) {
      return MoveDirection.stop;
    }
    const ang: number = Math.atan2(-dy, dx) * 180 / Math.PI; // (-180,180]，上为 +90
    if (ang > 45 && ang <= 135) {
      return MoveDirection.go;     // 上
    }
    if (ang > -45 && ang <= 45) {
      return MoveDirection.right;  // 右
    }
    if (ang > 135 || ang <= -135) {
      return MoveDirection.left;   // 左
    }
    return MoveDirection.stop;     // 下半区：协议无后退 → stop
  }

  private setDirection(dir: MoveDirection): void {
    if (dir === this.currentDir) {
      return;
    }
    this.currentDir = dir;
    this.activeDir = dir;
    if (dir === MoveDirection.stop) {
      this.stopThrottle();
      this.emit(MoveDirection.stop);
    } else {
      this.emit(dir);          // 方向切换即时下发一帧
      this.ensureThrottle();   // 之后按节流周期重发
    }
  }

  private ensureThrottle(): void {
    if (this.throttleTimer !== -1) {
      return;
    }
    this.throttleTimer = setInterval(() => {
      this.emit(this.currentDir);
    }, THROTTLE_INTERVAL_MS);
  }

  private stopThrottle(): void {
    if (this.throttleTimer !== -1) {
      clearInterval(this.throttleTimer);
      this.throttleTimer = -1;
    }
  }

  private emit(dir: MoveDirection): void {
    if (this.onDirection !== undefined) {
      this.onDirection(dir, dir === MoveDirection.stop ? 0 : this.speed);
    }
  }

  // ── 尺寸/字形 ───────────────────────────────────────────────────────────────
  private knobDiameter(): number {
    return this.diameter * KNOB_RATIO;
  }

  private maxKnobRadius(): number {
    return (this.diameter - this.knobDiameter()) / 2;
  }

  private dirGlyph(): string {
    switch (this.activeDir) {
      case MoveDirection.go:
        return '↑';
      case MoveDirection.left:
        return '←';
      case MoveDirection.right:
        return '→';
      default:
        return '●';
    }
  }
}
