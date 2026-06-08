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
import { Mission, ControlMode, MissionPhase, RobotRuntime, RobotRuntimeDTO, MissionSnapshot,
  EndPoint, Assignment, RobotStatus } from '../model/mission';
import { Point } from '../model/geometry';
import { RobotCommand, MoveDirection, UdpSendData, UdpReceiveData } from '../model/protocol';
import { FleetMissionService, MissionSyncListener } from '../service/FleetMissionService';
import { RobotTransport, DiscoveredRobot, MessageListener, ConnState } from '../service/RobotTransport';
import { MapService } from '../service/MapService';
import { mapUrlFor, FLEET_SESSION_ID } from '../constants/protocol';
import { SPEED_ASTAR, SPEED_FULLPATH, SPEED_DEFAULT } from '../constants/ui';
import { AppColor, FontSize, FontFamily, Space, Radius, Elevation, TOUCH_MIN } from '../constants/theme';
import { MapCanvas } from '../component/MapCanvas';
import { Joystick } from '../component/Joystick';
import { DeviceList } from '../component/DeviceList';
import { Log, Logger } from '../utils/log';

const log: Logger = Log.scoped('ControlPage');

interface ControlRouteParams {
  mode: string;
  ip: string;
}

// 命令时序：紫派对"先暂存后使用"的命令对需要处理间隔，避免 UDP 乱序/未处理完（对照旧 App 的命令间延时）。
const CMD_SEQ_GAP_MS: number = 300;            // 107→108 之间
const FULLPATH_START_DELAY_MS: number = 1000;  // 选完 4 顶点(104)到启动全路径(102)，对照旧 App 1s

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
  @State private joySpeed: number = SPEED_DEFAULT;  // 摇杆前进速度(%)，可调（cmd1 的 byte2）
  @State private fpAlg: number = 0;                 // 全路径算法：0=牛耕，1=最小生成树（cmd102 的 byte1）
  @State private fpVertices: Point[] = [];          // 全路径已选顶点（4 个：左下/右下/右上/左上）
  @State private joyHidden: boolean = false;        // 收起摇杆
  @State private connState: ConnState = ConnState.discovered;  // 本车实时连接状态（心跳新鲜度派生，驱动命令门控）

  private ip: string = '';
  private mode: ControlMode = ControlMode.astar;
  private transport: RobotTransport = RobotTransport.getInstance();
  private cancelPoll: (() => void) | null = null;
  private stopDiscover: (() => void) | null = null;
  private msgListener: MessageListener | null = null;
  private viewNonce: number = 0;
  private connRefreshTimer: number = -1;  // 周期刷新连接状态，使"心跳丢失"能及时反映到命令门控/TopBar
  private distCorners: Point[] = [];
  // 分布式：FleetMission 软总线黑板（平板写 assignments → 各车 agent 读自己的）
  private fleet: FleetMissionService = FleetMissionService.getInstance();
  private remoteListener: MissionSyncListener | null = null;
  @State private distUseAgent: boolean = true;  // true=经 agent(黑板)；false=平板直发兜底(cmd107/108)

  // ── 生命周期 ────────────────────────────────────────────────────────────────
  aboutToAppear(): void {
    const p: object | undefined = router.getParams();
    if (p !== undefined) {
      const params: ControlRouteParams = p as ControlRouteParams;
      this.ip = params.ip;
      this.mode = (params.mode as ControlMode) ?? ControlMode.astar;
    }
    this.mission.mode = this.mode;
    this.joySpeed = this.speedForMode();  // 摇杆速度按 mode 给默认值，用户可在滑块调
    if (this.ip.length > 0) {
      this.mission.robots = [new RobotRuntime(1, this.ip)];
    }
    this.initConnection();
    // 周期刷新连接状态（心跳新鲜度/丢失是时间派生，需定时重算以更新命令门控与 TopBar）。
    this.connRefreshTimer = setInterval(() => this.refreshConn(), 700);
  }

  aboutToDisappear(): void {
    if (this.connRefreshTimer !== -1) {
      clearInterval(this.connRefreshTimer);
      this.connRefreshTimer = -1;
    }
    if (this.msgListener !== null) {
      this.transport.unsubscribe(this.msgListener);
      this.msgListener = null;
    }
    if (this.remoteListener !== null) {
      this.fleet.unsubscribeMission(this.remoteListener);
      this.remoteListener = null;
      this.fleet.leaveSession().catch((e: Object) => log.e(`leaveSession：${JSON.stringify(e)}`));
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
      log.e(`bind 失败：${err.message}`);
    }
    this.msgListener = (data: UdpReceiveData, from: string) => this.onMessage(data, from);
    this.transport.subscribe(this.msgListener);
    // 分布式：入软总线会话 + 订阅黑板（各车 agent 回写的位姿/进度）
    if (this.mode === ControlMode.distributed) {
      this.remoteListener = (s: MissionSnapshot) => this.onRemoteMission(s);
      this.fleet.subscribeMission(this.remoteListener);
      this.fleet.joinSession(FLEET_SESSION_ID).catch((e: Object) => {
        log.e(`joinSession 失败：${JSON.stringify(e)}`);
      });
    }
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
    this.refreshConn();  // 收到心跳即时刷新连接状态（连接中→已连接）
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
        this.assignArea(c1, c2);
        this.bump();
      }
    } else if (this.mode === ControlMode.fullpath) {
      this.selectVertex(p);
    }
  }

  /** 全路径选顶点（4 个：左下→右下→右上→左上）。每点发 cmd104(byte1=序号)；满 4 个自动启动 cmd102(byte1=算法)。 */
  private selectVertex(p: Point): void {
    if (this.fpVertices.length >= 4) {
      return; // 已满，需先「重选顶点」
    }
    const idx: number = this.fpVertices.length;
    this.fpVertices = this.fpVertices.concat(p);
    this.mission.endPoints = this.fpVertices.map((v: Point): EndPoint => new EndPoint(v.x, v.y)); // 地图上显示已选顶点
    this.sendTo(this.ip, RobotCommand.fullpathSelect, idx, p.x, p.y); // cmd104：byte1=顶点序号
    if (this.fpVertices.length >= 4) {
      // 等 4 个顶点(104)都被紫派处理完再启动(102)——对照旧 App 启动前留 1s
      setTimeout(() => {
        this.sendTo(this.ip, RobotCommand.fullpathStartRoute, this.fpAlg); // cmd102：byte1=算法号
      }, FULLPATH_START_DELAY_MS);
    }
    this.bump();
  }

  private resetVertices(): void {
    this.fpVertices = [];
    this.mission.endPoints = [];
    this.bump();
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
    this.sendTo(this.ip, RobotCommand.distAreaCorner1, undefined, c1.x, c1.y); // cmd107 先暂存对角点1
    // 隔开再发 108：紫派要先处理 107 暂存，108 才能据两点生成覆盖矩形（122）
    setTimeout(() => {
      this.sendTo(this.ip, RobotCommand.distAreaCorner2, robotId, c2.x, c2.y);
    }, CMD_SEQ_GAP_MS);
  }

  /** 划好一块覆盖矩形：默认经车载 agent（写 FleetMission 黑板）；distUseAgent=false 时平板直发兜底。 */
  private assignArea(c1: Point, c2: Point): void {
    const r: RobotRuntime | undefined = this.robotOf(this.ip);
    const carId: number = r !== undefined ? r.index : 1;
    const robotId: number = carId <= 1 ? 0 : 1;  // 主/单车=0，从车=1（A 文档 robotId 主从）
    this.mission.assignments = [new Assignment(carId, c1, c2, robotId)];
    if (this.distUseAgent) {
      // 写黑板：phase=covering 让各车 agent 的 reconcile 进入覆盖；agent 读到自己 carId 的 assignment 后本机发 cmd107/108
      this.mission.phase = MissionPhase.covering;
      this.fleet.publishMission(this.mission);
    } else {
      this.sendDistArea(c1, c2, robotId);  // 平板直发兜底（无 agent / 软总线不通时）
    }
  }

  /** 黑板远端变更（各车 agent 回写位姿/进度）：只取 robots 更新显示；assignments 由平板掌权、不被覆盖。 */
  private onRemoteMission(s: MissionSnapshot): void {
    this.mission.robots = s.robots.map((d: RobotRuntimeDTO): RobotRuntime => {
      const rr: RobotRuntime = new RobotRuntime(d.index, d.ip, d.networkId);
      rr.applyDTO(d);
      return rr;
    });
    this.bump();
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

  /** 刷新本车实时连接状态（@State，驱动命令门控/TopBar 重渲染）。仅状态变化才赋值，省渲染。 */
  private refreshConn(): void {
    const s: ConnState = this.ip.length > 0 ? this.transport.connStateOf(this.ip) : ConnState.discovered;
    if (s !== this.connState) {
      this.connState = s;
    }
  }

  /** 实时连接判据：本车心跳新鲜（取代粘滞 connectedIps 做命令门控，见 docs/safety-flow-review.md §1）。 */
  private liveConnected(): boolean {
    return this.connState === ConnState.connected;
  }

  /** TopBar 连接点颜色：按实时连接状态。 */
  private connDotColor(): string {
    switch (this.connState) {
      case ConnState.connected:
        return AppColor.success;
      case ConnState.connecting:
        return AppColor.warning;
      case ConnState.lost:
        return AppColor.danger;
      default:
        return AppColor.robotOffline;
    }
  }

  /** TopBar 连接文案。 */
  private connText(): string {
    if (this.ip.length === 0) {
      return '未连接';
    }
    switch (this.connState) {
      case ConnState.connected:
        return this.ip;
      case ConnState.connecting:
        return `${this.ip} · 连接中`;
      case ConnState.lost:
        return `${this.ip} · 心跳丢失`;
      default:
        return `${this.ip} · 未连接`;
    }
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
        enablePick: this.mission.mapReady && this.liveConnected(),
        onPickPoint: (p: Point) => this.onPick(p)
      })

      // 顶部连接条
      this.TopBar()

      // 右下缩放 FAB
      this.ZoomFabs()

      // 摇杆区（可隐藏）：速度滑块 + 摇杆
      if (!this.joyHidden) {
        this.SpeedControl()
        Joystick({
          speed: this.joySpeed,
          disabled: !this.liveConnected(),
          onDirection: (dir: MoveDirection, speed: number) => this.onJoystick(dir, speed)
        })
          .position({ x: Space.lg, y: '62%' })
      }
      // 隐藏/显示摇杆切换
      Button(this.joyHidden ? '显示摇杆' : '收起摇杆')
        .fontSize(FontSize.label)
        .fontColor(AppColor.textBody)
        .backgroundColor(AppColor.surface)
        .borderRadius(Radius.pill)
        .height(32)
        .padding({ left: Space.md, right: Space.md })
        .position({ x: Space.lg, y: '44%' })
        .onClick(() => {
          this.joyHidden = !this.joyHidden;
        })

      // 底部卡：建图中 / 无地图 → 建图卡（R1）；有地图且非建图 → 按 mode 操作卡
      if (this.mission.mapReady && !this.mapping) {
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
          .enabled(this.liveConnected())
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
          .enabled(this.liveConnected())
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
          .fill(this.connDotColor())
          .margin({ right: Space.sm })
        Text(this.connText())
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
      if (this.mission.mapReady && !this.mapping) {
        Button('重新建图')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.primary)
          .backgroundColor(AppColor.surface)
          .border({ width: 1, color: AppColor.primary })
          .borderRadius(Radius.pill)
          .height(40)
          .margin({ right: Space.sm })
          .padding({ left: Space.md, right: Space.md })
          .enabled(this.liveConnected())
          .onClick(() => this.startBuild())
      }
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
        .enabled(this.liveConnected())
        .onClick(() => this.cancelRoute())
      Button('开始导航')
        .fontColor(AppColor.onPrimary)
        .backgroundColor(AppColor.primary)
        .borderRadius(Radius.pill)
        .height(TOUCH_MIN)
        .enabled(this.hasTarget() && this.liveConnected())
        .onClick(() => this.startAstar())
    }
    .width('100%')
  }

  @Builder
  FullpathOps() {
    Column({ space: Space.sm }) {
      // ① 选算法（牛耕 / 最小生成树 → cmd102 byte1）
      Row({ space: Space.sm }) {
        this.AlgChip('牛耕算法', 0)
        this.AlgChip('最小生成树', 1)
      }
      .width('100%')
      // ② 点地图选 4 个顶点（左下→右下→右上→左上 → cmd104），满 4 个自动启动
      Text(this.fpHint())
        .fontSize(FontSize.bodySm)
        .fontColor(AppColor.textBody)
        .width('100%')
      Row() {
        Button('重选顶点')
          .fontColor(AppColor.primary)
          .backgroundColor(AppColor.surface)
          .border({ width: 1, color: AppColor.primary })
          .borderRadius(Radius.pill)
          .height(TOUCH_MIN)
          .onClick(() => this.resetVertices())
        Blank()
        Button('停止')
          .fontColor(AppColor.onPrimary)
          .backgroundColor(AppColor.primary)
          .borderRadius(Radius.pill)
          .height(TOUCH_MIN)
          .enabled(this.liveConnected())
          .onClick(() => this.stopFullpath())
      }
      .width('100%')
    }
    .width('100%')
  }

  @Builder
  AlgChip(label: string, alg: number) {
    Text(label)
      .fontSize(FontSize.bodySm)
      .fontColor(this.fpAlg === alg ? AppColor.onPrimary : AppColor.primary)
      .backgroundColor(this.fpAlg === alg ? AppColor.primary : AppColor.surface)
      .border(this.fpAlg === alg ? undefined : { width: 1, color: AppColor.primary })
      .borderRadius(Radius.pill)
      .height(TOUCH_MIN)
      .layoutWeight(1)
      .textAlign(TextAlign.Center)
      .onClick(() => {
        this.fpAlg = alg;
      })
  }

  // 速度控制（摇杆上方）：调 cmd1 的 speed（前进速度 %）。
  @Builder
  SpeedControl() {
    Column({ space: Space.xs }) {
      Text(`速度 ${this.joySpeed}%`)
        .fontSize(FontSize.caption)
        .fontColor(AppColor.textSecondary)
      Slider({ value: this.joySpeed, min: 10, max: 100, step: 5 })
        .width(150)
        .blockColor(AppColor.primary)
        .selectedColor(AppColor.primary)
        .trackColor(AppColor.divider)
        .onChange((v: number) => {
          this.joySpeed = Math.round(v);
        })
    }
    .padding(Space.sm)
    .backgroundColor(AppColor.surface)
    .borderRadius(Radius.md)
    .shadow({
      radius: Elevation.softRadius,
      color: Elevation.softColor,
      offsetX: 0,
      offsetY: Elevation.softOffsetY
    })
    .position({ x: Space.lg, y: '50%' })
  }

  private fpHint(): string {
    const n: number = this.fpVertices.length;
    if (n >= 4) {
      return '已选 4 顶点，全路径覆盖已启动';
    }
    return `点地图选第 ${n + 1} 个顶点（${this.fpCornerName(n)}），共需 4 个`;
  }

  private fpCornerName(i: number): string {
    if (i === 0) {
      return '左下';
    }
    if (i === 1) {
      return '右下';
    }
    if (i === 2) {
      return '右上';
    }
    return '左上';
  }

  @Builder
  DistributedOps() {
    Column({ space: Space.sm }) {
      // 下发方式：经车载 agent(写黑板) 或 平板直发兜底
      Row() {
        Text(this.distUseAgent ? '经车载 agent（黑板）' : '平板直发（兜底）')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.textBody)
        Blank()
        Toggle({ type: ToggleType.Switch, isOn: this.distUseAgent })
          .selectedColor(AppColor.primary)
          .onChange((on: boolean) => {
            this.distUseAgent = on;
          })
      }
      .width('100%')
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
            log.e(`路由 SetIPPage 失败：${e.message}`);
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
