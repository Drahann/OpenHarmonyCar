/**
 * ControlPage —— 单一参数化控制页（mode∈{astar|fullpath|distributed}），取代旧 4 个克隆页
 * （distributedPage / distributedsecondPage / distriFullPathPage / PendingComponent，约 4600 行 → 1 份）。
 *
 * Google Maps 范式：全屏 MapCanvas 打底 + 浮层（顶部连接条、右下缩放 FAB、底部按 mode 变体的操作卡、
 * 右下摇杆）。组合 U3 MapCanvas / U4 Joystick / U5 DeviceList，业务经 service/RobotTransport 多目标下发。
 *
 * 连接策略（避副作用）：进入页面不发 cmd0（=启动建图），仅 startHeartbeat 中性保活(pending+stop)，
 *   收到该 IP 心跳即判"在线"；位姿(state==3)更新 RobotRuntime 并 bump renderVersion 重绘地图。
 *
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）；路由需 U9 注册；fullpath 选房间/distributed 进度细化待真机联调。
 */

import { router } from '@kit.ArkUI';
import { Mission, ControlMode, RobotRuntime, EndPoint, Assignment, RobotStatus } from '../model/mission';
import { Point } from '../model/geometry';
import { RobotCommand, MoveDirection, UdpSendData, UdpReceiveData } from '../model/protocol';
import { RobotTransport, DiscoveredRobot, MessageListener } from '../service/RobotTransport';
import { MapService } from '../service/MapService';
import { mapUrlFor } from '../constants/protocol';
import { SPEED_ASTAR, SPEED_FULLPATH, SPEED_DEFAULT } from '../constants/ui';
import { AppColor, FontSize, FontFamily, Space, Radius, Elevation, TOUCH_MIN } from '../constants/theme';
import { MapCanvas } from '../component/MapCanvas';
import { Joystick } from '../component/Joystick';
import { DeviceList } from '../component/DeviceList';

interface ControlRouteParams {
  mode: string;
  ip: string;
}

@Entry
@Component
struct ControlPage {
  @State private mission: Mission = new Mission();
  @State private mapText: string = '';
  @State private renderVersion: number = 0;
  @State private connectedIps: string[] = [];
  @State private devices: DiscoveredRobot[] = [];
  @State private scanning: boolean = false;
  @State private showDevicePanel: boolean = false;
  @State private mapViewCmd: string = '';
  @State private mapping: boolean = false;  // 建图进行中（cmd0 开始 → cmd2 结束）

  private ip: string = '';
  private mode: ControlMode = ControlMode.astar;
  private transport: RobotTransport = RobotTransport.getInstance();
  private cancelPoll: (() => void) | null = null;
  private stopDiscover: (() => void) | null = null;
  private msgListener: MessageListener | null = null;
  private viewNonce: number = 0;
  private distCorners: Point[] = [];

  // ── 生命周期 ────────────────────────────────────────────────────────────────
  aboutToAppear(): void {
    const p: object | undefined = router.getParams();
    if (p !== undefined) {
      const params: ControlRouteParams = p as ControlRouteParams;
      this.ip = params.ip;
      this.mode = (params.mode as ControlMode) ?? ControlMode.astar;
    }
    this.mission.mode = this.mode;
    if (this.ip.length > 0) {
      this.mission.robots = [new RobotRuntime(1, this.ip)];
    }
    this.initConnection();
  }

  aboutToDisappear(): void {
    if (this.msgListener !== null) {
      this.transport.unsubscribe(this.msgListener);
      this.msgListener = null;
    }
    if (this.cancelPoll !== null) {
      this.cancelPoll();
      this.cancelPoll = null;
    }
    this.endScan();
    this.transport.stopHeartbeat(); // 退出页面停所有保活（单页单车场景）
  }

  private async initConnection(): Promise<void> {
    try {
      await this.transport.bind();
    } catch (e) {
      const err: Error = e as Error;
      console.error(`[ControlPage] bind 失败：${err.message}`);
    }
    this.msgListener = (data: UdpReceiveData, from: string) => this.onMessage(data, from);
    this.transport.subscribe(this.msgListener);
    if (this.ip.length > 0) {
      this.connectTo(this.ip);
      this.startMapPoll(this.ip);
    }
  }

  // ── 连接 / 收发 ─────────────────────────────────────────────────────────────
  private connectTo(ip: string): void {
    this.ensureRobot(ip);
    this.transport.startHeartbeat(ip, this.neutralPayload());
  }

  private neutralPayload(): UdpSendData {
    return new UdpSendData(RobotCommand.pending, MoveDirection.stop, 0);
  }

  private onMessage(data: UdpReceiveData, from: string): void {
    const r: RobotRuntime | undefined = this.robotOf(from);
    if (r === undefined) {
      return;
    }
    if (!r.online) {
      r.online = true;
      r.status = RobotStatus.idle;
      if (!this.connectedIps.includes(from)) {
        this.connectedIps = this.connectedIps.concat(from);
      }
    }
    if (data.hasPose()) {
      r.x = data.x;
      r.y = data.y;
      r.r = data.r;
    }
    this.bump();
  }

  private startMapPoll(ip: string): void {
    if (this.cancelPoll !== null) {
      this.cancelPoll();
    }
    this.cancelPoll = MapService.pollMapUntilReady(mapUrlFor(ip), (text: string) => {
      this.mapText = text;
      this.mission.mapReady = true;
      this.bump();
    });
  }

  // ── 摇杆 → 多目标下发 ───────────────────────────────────────────────────────
  private onJoystick(dir: MoveDirection, speed: number): void {
    const payload: UdpSendData = new UdpSendData(RobotCommand.pending, dir, speed);
    const keep: UdpSendData = dir === MoveDirection.stop ? this.neutralPayload() : payload;
    for (const ip of this.connectedIps) {
      this.transport.send(ip, payload);
      this.transport.setHeartbeatPayload(ip, keep); // 保活重发当前动作，避免节流间隙 3s 急停
    }
  }

  // ── 地图点选（按 mode 语义不同）────────────────────────────────────────────
  private onPick(p: Point): void {
    if (this.mode === ControlMode.astar) {
      this.mission.endPoints = [new EndPoint(p.x, p.y)];
      this.bump();
    } else if (this.mode === ControlMode.distributed) {
      this.distCorners.push(p);
      if (this.distCorners.length >= 2) {
        const c1: Point = this.distCorners[0];
        const c2: Point = this.distCorners[1];
        this.distCorners = [];
        this.mission.assignments = [new Assignment(1, c1, c2, 0)];
        this.sendDistArea(c1, c2, 0);
        this.bump();
      }
    }
    // fullpath：点选=选房间，需 cmd104+房间号，待真机房间编号方案；此处暂不处理点选。
  }

  // ── 指令下发 ────────────────────────────────────────────────────────────────
  private sendTo(ip: string, cmd: RobotCommand, runState?: number, x?: number, y?: number): void {
    this.transport.send(ip, new UdpSendData(cmd, runState, undefined, x, y));
  }

  private startAstar(): void {
    const e: EndPoint | undefined = this.mission.endPoints[0];
    if (e === undefined || e.x === null || e.y === null) {
      return;
    }
    this.sendTo(this.ip, RobotCommand.startRoute, undefined, e.x, e.y);
  }

  private cancelRoute(): void {
    this.sendTo(this.ip, RobotCommand.endRoute);
  }

  private startFullpath(): void {
    this.sendTo(this.ip, RobotCommand.fullpathStartRoute);
  }

  private stopFullpath(): void {
    this.sendTo(this.ip, RobotCommand.fullpathStartover);
  }

  // ── 建图（R1 回归：紫派 Navi 启动即删图，必须先建图才有地图，见 docs/feature-parity-review.md）──
  /** 开始建图：cmd0（紫派 LCM30 启动 SLAM）。之后用摇杆驱动小车探索一圈。 */
  private startBuild(): void {
    this.sendTo(this.ip, RobotCommand.beforeStart);
    this.mapping = true;
  }

  /** 结束建图：cmd2（紫派 停轮控 + 存图32 + 加载导航图10）。完成后主动拉一次图。 */
  private finishBuild(): void {
    this.sendTo(this.ip, RobotCommand.afterEnd);
    this.mapping = false;
    this.startMapPoll(this.ip);  // 存图后重新拉图（建完即取，绕开就绪阈值的脆性）
  }

  /** 划子区域：107=对角点1，108=对角点2(byte1=robotId)。byte1 经 encodeSend 的 runState 写入。 */
  private sendDistArea(c1: Point, c2: Point, robotId: number): void {
    this.sendTo(this.ip, RobotCommand.distAreaCorner1, undefined, c1.x, c1.y);
    this.sendTo(this.ip, RobotCommand.distAreaCorner2, robotId, c2.x, c2.y);
  }

  // ── 缩放 FAB ────────────────────────────────────────────────────────────────
  private view(kind: string): void {
    this.viewNonce++;
    this.mapViewCmd = `${kind}:${this.viewNonce}`;
  }

  // ── 设备面板 ────────────────────────────────────────────────────────────────
  private startScan(): void {
    this.endScan();
    this.scanning = true;
    this.stopDiscover = this.transport.discover((robots: DiscoveredRobot[]) => {
      this.devices = robots;
    });
    setTimeout(() => {
      this.scanning = false;
    }, 3200);
  }

  private endScan(): void {
    if (this.stopDiscover !== null) {
      this.stopDiscover();
      this.stopDiscover = null;
    }
    this.scanning = false;
  }

  // ── 工具 ────────────────────────────────────────────────────────────────────
  private robotOf(ip: string): RobotRuntime | undefined {
    return this.mission.robots.find((r: RobotRuntime): boolean => r.ip === ip);
  }

  private ensureRobot(ip: string): void {
    if (this.robotOf(ip) === undefined) {
      this.mission.robots = this.mission.robots.concat(new RobotRuntime(this.mission.robots.length + 1, ip));
    }
  }

  private bump(): void {
    this.renderVersion++;
  }

  private speedForMode(): number {
    if (this.mode === ControlMode.astar) {
      return SPEED_ASTAR;
    }
    if (this.mode === ControlMode.fullpath) {
      return SPEED_FULLPATH;
    }
    return SPEED_DEFAULT;
  }

  // ── 视图 ────────────────────────────────────────────────────────────────────
  build() {
    Stack({ alignContent: Alignment.TopStart }) {
      // 底层全屏地图
      MapCanvas({
        mapText: this.mapText,
        renderVersion: this.renderVersion,
        viewCmd: this.mapViewCmd,
        robots: this.mission.robots,
        endPoints: this.mission.endPoints,
        assignments: this.mission.assignments,
        area: this.mission.area,
        enablePick: this.mode !== ControlMode.fullpath,
        onPickPoint: (p: Point) => this.onPick(p)
      })

      // 顶部连接条
      this.TopBar()

      // 右下缩放 FAB
      this.ZoomFabs()

      // 摇杆（右下，连接后启用）
      Joystick({
        speed: this.speedForMode(),
        disabled: this.connectedIps.length === 0,
        onDirection: (dir: MoveDirection, speed: number) => this.onJoystick(dir, speed)
      })
        .position({ x: Space.lg, y: '62%' })

      // 底部卡：无地图 → 建图卡（R1 回归，必须先建图）；有地图 → 按 mode 操作卡
      if (this.mission.mapReady) {
        this.OpCard()
      } else {
        this.BuildCard()
      }

      // 设备发现面板（浮层）
      if (this.showDevicePanel) {
        this.DevicePanel()
      }
    }
    .width('100%')
    .height('100%')
    .backgroundColor(AppColor.pageBg)
  }

  // 建图卡（R1）：地图未就绪时显示。开始建图(cmd0) → 摇杆探索 → 结束建图(cmd2) → 自动拉图。
  @Builder
  BuildCard() {
    Column({ space: Space.sm }) {
      if (!this.mapping) {
        Text('尚无地图')
          .fontSize(FontSize.h3)
          .fontColor(AppColor.textBody)
          .fontWeight(FontWeight.Bold)
          .width('100%')
        Text('小车每次启动后地图会被清空，需先建图。点「开始建图」，再用摇杆驱动小车走一圈环境。')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.textSecondary)
          .width('100%')
        Button('开始建图')
          .fontColor(AppColor.onPrimary)
          .backgroundColor(AppColor.primary)
          .borderRadius(Radius.pill)
          .width('100%')
          .height(TOUCH_MIN)
          .enabled(this.connectedIps.length > 0)
          .onClick(() => this.startBuild())
      } else {
        Row() {
          LoadingProgress().width(20).height(20).color(AppColor.primary)
          Text('建图中… 用摇杆驱动小车探索，走完点「结束建图」')
            .fontSize(FontSize.bodySm)
            .fontColor(AppColor.textBody)
            .margin({ left: Space.sm })
            .layoutWeight(1)
        }
        .width('100%')

        Button('结束建图')
          .fontColor(AppColor.onPrimary)
          .backgroundColor(AppColor.primary)
          .borderRadius(Radius.pill)
          .width('100%')
          .height(TOUCH_MIN)
          .onClick(() => this.finishBuild())
      }
    }
    .width('92%')
    .padding(Space.md)
    .backgroundColor(AppColor.surface)
    .borderRadius(Radius.md)
    .shadow({
      radius: Elevation.cardRadius,
      color: Elevation.cardColor,
      offsetX: 0,
      offsetY: Elevation.cardOffsetY
    })
    .position({ x: '4%', y: '80%' })
  }

  @Builder
  TopBar() {
    Row() {
      Button('‹ 返回')
        .fontSize(FontSize.bodySm)
        .fontColor(AppColor.textBody)
        .backgroundColor(AppColor.surface)
        .borderRadius(Radius.pill)
        .height(40)
        .padding({ left: Space.md, right: Space.md })
        .onClick(() => router.back())
      Row() {
        Circle({ width: 10, height: 10 })
          .fill(this.connectedIps.length > 0 ? AppColor.success : AppColor.robotOffline)
          .margin({ right: Space.sm })
        Text(this.ip.length > 0 ? this.ip : '未连接')
          .fontSize(FontSize.dataSm)
          .fontFamily(FontFamily.mono)
          .fontColor(AppColor.textBody)
      }
      .padding({ left: Space.md, right: Space.md })
      .height(40)
      .backgroundColor(AppColor.surface)
      .borderRadius(Radius.pill)
      .margin({ left: Space.sm })
      Blank()
      Button('设备')
        .fontSize(FontSize.bodySm)
        .fontColor(AppColor.onPrimary)
        .backgroundColor(AppColor.primary)
        .borderRadius(Radius.pill)
        .height(40)
        .padding({ left: Space.md, right: Space.md })
        .onClick(() => {
          this.showDevicePanel = !this.showDevicePanel;
        })
    }
    .width('100%')
    .padding(Space.md)
  }

  @Builder
  ZoomFabs() {
    Column({ space: Space.sm }) {
      this.Fab('＋', () => this.view('in'))
      this.Fab('－', () => this.view('out'))
      this.Fab('⊙', () => this.view('reset'))
    }
    .position({ x: '88%', y: '40%' })
  }

  @Builder
  Fab(label: string, action: () => void) {
    Button(label)
      .fontSize(FontSize.h3)
      .fontColor(AppColor.textBody)
      .backgroundColor(AppColor.surface)
      .width(TOUCH_MIN)
      .height(TOUCH_MIN)
      .borderRadius(TOUCH_MIN / 2)
      .shadow({
        radius: Elevation.softRadius,
        color: Elevation.softColor,
        offsetX: 0,
        offsetY: Elevation.softOffsetY
      })
      .onClick(action)
  }

  @Builder
  OpCard() {
    Column() {
      if (this.mode === ControlMode.astar) {
        this.AstarOps()
      } else if (this.mode === ControlMode.fullpath) {
        this.FullpathOps()
      } else {
        this.DistributedOps()
      }
    }
    .width('92%')
    .padding(Space.md)
    .backgroundColor(AppColor.surface)
    .borderRadius(Radius.md)
    .shadow({
      radius: Elevation.cardRadius,
      color: Elevation.cardColor,
      offsetX: 0,
      offsetY: Elevation.cardOffsetY
    })
    .position({ x: '4%', y: '84%' })
  }

  @Builder
  AstarOps() {
    Row() {
      Column() {
        Text('目标点')
          .fontSize(FontSize.caption)
          .fontColor(AppColor.textSecondary)
        Text(this.targetText())
          .fontSize(FontSize.dataMd)
          .fontFamily(FontFamily.mono)
          .fontColor(AppColor.textBody)
      }
      .alignItems(HorizontalAlign.Start)
      Blank()
      Button('取消')
        .fontColor(AppColor.primary)
        .backgroundColor(AppColor.surface)
        .border({ width: 1, color: AppColor.primary })
        .borderRadius(Radius.pill)
        .height(TOUCH_MIN)
        .margin({ right: Space.sm })
        .onClick(() => this.cancelRoute())
      Button('开始导航')
        .fontColor(AppColor.onPrimary)
        .backgroundColor(AppColor.primary)
        .borderRadius(Radius.pill)
        .height(TOUCH_MIN)
        .enabled(this.hasTarget())
        .onClick(() => this.startAstar())
    }
    .width('100%')
  }

  @Builder
  FullpathOps() {
    Row() {
      Text('全路径覆盖')
        .fontSize(FontSize.body)
        .fontColor(AppColor.textBody)
      Blank()
      Button('停止')
        .fontColor(AppColor.primary)
        .backgroundColor(AppColor.surface)
        .border({ width: 1, color: AppColor.primary })
        .borderRadius(Radius.pill)
        .height(TOUCH_MIN)
        .margin({ right: Space.sm })
        .onClick(() => this.stopFullpath())
      Button('开始覆盖')
        .fontColor(AppColor.onPrimary)
        .backgroundColor(AppColor.primary)
        .borderRadius(Radius.pill)
        .height(TOUCH_MIN)
        .onClick(() => this.startFullpath())
    }
    .width('100%')
  }

  @Builder
  DistributedOps() {
    Column({ space: Space.sm }) {
      Text(this.distHint())
        .fontSize(FontSize.bodySm)
        .fontColor(AppColor.textSecondary)
        .width('100%')
      ForEach(this.mission.robots, (r: RobotRuntime) => {
        Row() {
          Text(`车${r.index}`)
            .fontSize(FontSize.bodySm)
            .fontColor(AppColor.textBody)
            .width(48)
          Progress({ value: Math.round(r.progress * 100), total: 100, type: ProgressType.Linear })
            .layoutWeight(1)
            .color(AppColor.primary)
          Text(`${Math.round(r.progress * 100)}%`)
            .fontSize(FontSize.dataSm)
            .fontFamily(FontFamily.mono)
            .fontColor(AppColor.textSecondary)
            .width(48)
            .textAlign(TextAlign.End)
        }
        .width('100%')
      }, (r: RobotRuntime) => `${r.index}`)
    }
    .width('100%')
  }

  @Builder
  DevicePanel() {
    Column() {
      DeviceList({
        devices: this.devices,
        connectedIps: this.connectedIps,
        scanning: this.scanning,
        onScan: () => this.startScan(),
        onConnect: (ip: string) => {
          this.connectTo(ip);
          this.startMapPoll(ip);
          this.showDevicePanel = false;
        },
        onManualAdd: () => {
          router.pushUrl({ url: 'pages/SetIPPage' }).catch((e: Error) => {
            console.error(`[ControlPage] 路由 SetIPPage 失败：${e.message}`);
          });
        }
      })
    }
    .width('80%')
    .position({ x: '10%', y: '12%' })
  }

  // ── 文案 ────────────────────────────────────────────────────────────────────
  private hasTarget(): boolean {
    const e: EndPoint | undefined = this.mission.endPoints[0];
    return e !== undefined && e.x !== null && e.y !== null;
  }

  private targetText(): string {
    const e: EndPoint | undefined = this.mission.endPoints[0];
    if (e === undefined || e.x === null || e.y === null) {
      return '点地图选点';
    }
    return `(${e.x}, ${e.y})`;
  }

  private distHint(): string {
    if (this.distCorners.length === 1) {
      return '已选对角点1，点第二个对角点完成子区域';
    }
    return '点地图两点划分覆盖子区域（对角矩形）';
  }
}
