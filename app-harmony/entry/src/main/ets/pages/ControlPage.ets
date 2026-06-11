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

import { router, promptAction } from '@kit.ArkUI';
import { Mission, ControlMode, MissionPhase, RobotRuntime, RobotRuntimeDTO, MissionSnapshot,
  EndPoint, Assignment, RobotStatus } from '../model/mission';
import { Point } from '../model/geometry';
import { RobotCommand, MoveDirection, UdpSendData, UdpReceiveData } from '../model/protocol';
import { FleetMissionService, MissionSyncListener } from '../service/FleetMissionService';
import { RobotTransport, DiscoveredRobot, MessageListener, ConnState } from '../service/RobotTransport';
import { MapService } from '../service/MapService';
import { VisionService } from '../service/VisionService';
import { Storage } from '../service/storage';
import { VideoView } from '../component/VideoView';
import { mapUrlFor, FLEET_SESSION_ID } from '../constants/protocol';
import { SPEED_ASTAR, SPEED_FULLPATH, SPEED_DEFAULT } from '../constants/ui';
import { AppColor, FontSize, FontFamily, Space, Radius, Elevation, TOUCH_MIN } from '../constants/theme';
import { Motion } from '../constants/motion';
import { Icons } from '../constants/icons';
import { IconButton } from '../component/IconButton';
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
  @State private mapLoading: boolean = false;   // 正在拉取地图（结束建图后阻塞遮罩）
  @State private mapLoadFailed: boolean = false; // 阻塞式拉图失败（弹错误提示 + 重试）
  @State private joySpeed: number = SPEED_DEFAULT;  // 摇杆前进速度(%)，可调（cmd1 的 byte2）
  @State private fpAlg: number = 0;                 // 全路径算法：0=牛耕，1=最小生成树（cmd102 的 byte1）
  @State private fpVertices: Point[] = [];          // 全路径已选顶点（4 个：左下/右下/右上/左上）
  @State private joyHidden: boolean = false;        // 收起摇杆
  @State private astarNavigating: boolean = false;  // astar：已下发目标、导航中（分步引导用）
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
  // 仪表识别 PiP 小窗（形态B）：独立 WS 连香橙派，与导航链路解耦；引用计数 acquire/release。
  private vision: VisionService = VisionService.getInstance();
  @State private pipExpanded: boolean = false;

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
    if (this.pipExpanded) {
      this.vision.release();  // 真正离开本页（pop）时释放视频流；放大进 VisionPage 是 push，不会走到这里
      this.pipExpanded = false;
    }
  }

  /** 页面转场（docs/ui-polish-plan.md §3）：进场右移淡入，返场淡出。 */
  pageTransition() {
    PageTransitionEnter({ duration: Motion.slow, curve: Motion.curveStd })
      .translate({ x: 24 })
      .opacity(0)
    PageTransitionExit({ duration: Motion.base, curve: Motion.curveStd })
      .opacity(0)
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
      this.loadMap(this.ip, 1, 0, false);  // 进页查既有图（A 开机保留上次地图：有则直接用；点「开始建图」会清并重建）；非阻塞、不轮询
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

  /**
   * 拉取地图（HTTP GET）。**只在"地图应已存在"时调用**：① 进页/切车——查既有图（紫派开机即删图，
   * 可能没有）；② **结束建图(cmd2)后**——存图落盘后拉。**绝不在开始建图(cmd0)时拉**（那时还没存图）。
   * bounded 重试等存图落盘；按"栅格行数齐全"判完整（取代脆弱字节阈值，见 docs/safety-flow-review.md §6 / Q8）。
   * attempts 次、间隔 delayMs；全失败=无完整图 → 显示建图卡。可经 cancelPoll 取消（退页/再次调用）。
   */
  private loadMap(ip: string, attempts: number, delayMs: number, blocking: boolean): void {
    if (this.cancelPoll !== null) {
      this.cancelPoll();
      this.cancelPoll = null;
    }
    let cancelled: boolean = false;
    this.cancelPoll = (): void => {
      cancelled = true;
    };
    if (blocking) {
      animateTo({ duration: Motion.fast, curve: Motion.curveStd }, () => {
        this.mapLoading = true;   // 阻塞遮罩（结束建图后等存图落盘）
        this.mapLoadFailed = false;
      });
      this.bump();
    }
    const giveUp = (): void => {
      this.cancelPoll = null;
      animateTo({ duration: Motion.fast, curve: Motion.curveStd }, () => {
        this.mapLoading = false;
        if (blocking) {
          this.mapLoadFailed = true;  // 阻塞式失败 → 弹错误提示+重试；非阻塞(进页查既有)失败 → 静默回建图卡
        }
      });
      this.bump();
    };
    const attempt = (left: number): void => {
      if (cancelled) {
        return;
      }
      // 临时「先跑通」：结束建图后的阻塞拉图给 10 分钟读超时——A 的 `python -m http.server`（单线程）在 RK3566 上
      // 跑 SLAM 时送 ~6.5MB 大图仅 ~47KB/s，30s 拉不完（真机 log errCode 2300028 超时）。最终修在 A 侧
      // （gzip / 多线程 server / 缩图，见 contracts/integration-qa.md）。进页"查既有图"仍 30s，避免长占 A 的单线程 server。
      MapService.fetchMapText(mapUrlFor(ip), blocking ? 600000 : 30000).then((text: string) => {
        if (cancelled) {
          return;
        }
        if (this.mapLooksComplete(text)) {
          this.cancelPoll = null;
          animateTo({ duration: Motion.base, curve: Motion.curveStd }, () => {
            this.mapText = text;
            this.mission.mapReady = true;
            this.mapLoading = false;
            this.mapLoadFailed = false;
          });
          this.bump();
        } else if (left > 1) {
          setTimeout(() => attempt(left - 1), delayMs);
        } else {
          giveUp();
        }
      }).catch(() => {
        if (cancelled) {
          return;
        }
        if (left > 1) {
          setTimeout(() => attempt(left - 1), delayMs);
        } else {
          giveUp();
        }
      });
    };
    attempt(attempts);
  }

  /**
   * 地图是否完整（就绪判据）。真机首行 = `range resolution height width metersPerPixel x0 y0`，
   * 按位置取 parts[2]=声明行数；统计非空数据行，要求 ≥ 声明行数的 90%（容忍 cmd2 存图落盘期间尾部缺失）。
   * 首行非标准（如 fixture `40 40`，parts[2] 缺失）则回退要求 ≥5 行。见 docs/map-pipeline.md。
   */
  private mapLooksComplete(text: string): boolean {
    const rawLines: string[] = text.split('\n');
    if (rawLines.length < 3) {
      log.w(`[mapLooksComplete] 行数不足 ${rawLines.length}`);
      return false;
    }
    const headerH: number = parseInt(rawLines[0].trim().split(/\s+/)[2], 10); // 首行声明行数
    let dataRows: number = 0;
    for (let y = 1; y < rawLines.length; y++) {
      if (rawLines[y].trim().length > 0) {
        dataRows++;
      }
    }
    const expected: number = (Number.isNaN(headerH) || headerH <= 0) ? 5 : Math.floor(headerH * 0.9);
    const ok: boolean = dataRows >= expected && dataRows >= 2;
    log.i(`[mapLooksComplete] 首行="${rawLines[0].trim()}" headerH=${headerH} dataRows=${dataRows} expected=${expected} ok=${ok}`);
    return ok;
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
      this.astarNavigating = false;  // 新目标 → 回到"待开始导航"
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
    this.toast(`已选第 ${idx + 1}/4 个顶点（${this.fpCornerName(idx)}）`);
    if (this.fpVertices.length >= 4) {
      this.toast('4 个顶点已选，全路径覆盖启动中…');
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
    this.astarNavigating = true;
    this.toast('已下发目标点，开始导航');
  }

  private cancelRoute(): void {
    this.sendTo(this.ip, RobotCommand.endRoute);
    this.astarNavigating = false;
    this.toast('已取消导航');
  }

  private stopFullpath(): void {
    this.sendTo(this.ip, RobotCommand.fullpathStartover);
    this.toast('已停止全路径覆盖');
  }

  // ── 建图（A 现在开机保留上次地图、新建图被接受后才删；无图时必须先建图，见 docs/feature-parity-review.md）──
  /** 开始建图：cmd0（紫派 LCM30 启动 SLAM）。之后用摇杆驱动小车探索一圈。 */
  private startBuild(): void {
    // 进入新一轮建图前，取消进页可能仍在途的"查既有图"拉取，并清掉已显示的旧地图：
    // A 现在开机**保留**上次地图、仅在新建图被接受后才删（README 0db78ed/8b4af4b）。若不清，建图期间会一直
    // 显示上一次的**过期静态地图**（且建图无实时预览，Q7/A7）——正是"开始建图后地图不变"的现象。清后建图阶段只显示"建图中…"。
    if (this.cancelPoll !== null) {
      this.cancelPoll();
      this.cancelPoll = null;
    }
    // 用 'm'/0x6d **强制重新建图**：A 的 cmd0「有图时只当心跳」会被 Navi.createMap 忽略（→ 旧图不变 + 定位漂移，
    // 真机实证），必须 forceCreateMap(forceNewMap=true) 才会清空旧图从头建。见 contracts/udp-protocol.md。
    this.sendTo(this.ip, RobotCommand.forceCreateMap);
    animateTo({ duration: Motion.base, curve: Motion.curveStd }, () => {
      this.mapText = '';
      this.mission.mapReady = false;
      this.mapping = true;
    });
    this.bump();
    this.toast('已开始建图——用摇杆驱动小车扫一圈，完成后点「结束建图」');
  }

  /** 结束建图：cmd2（紫派 停轮控 + 存图32 + 加载导航图10）。完成后主动拉一次图。 */
  private finishBuild(): void {
    this.sendTo(this.ip, RobotCommand.afterEnd);
    animateTo({ duration: Motion.base, curve: Motion.curveStd }, () => {
      this.mapping = false;
    });
    this.loadMap(this.ip, 8, 1500, true);  // cmd2 存图后拉图：**阻塞遮罩**重试等存图落盘（按行数齐全判完整，非字节阈值）
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
    this.toast(this.distUseAgent ? '已写入黑板：各车 agent 将执行覆盖' : '已下发覆盖区域（平板直发兜底）');
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
      // 状态点/文案变色带过渡（ui-polish-plan §3）
      animateTo({ duration: 200, curve: Motion.curveStd }, () => {
        this.connState = s;
      });
    }
  }

  /** 实时连接判据：本车心跳新鲜（取代粘滞 connectedIps 做命令门控，见 docs/safety-flow-review.md §1）。 */
  private liveConnected(): boolean {
    return this.connState === ConnState.connected;
  }

  /** 轻量反馈：命令是 fire-and-forget UDP（紫派不回 ack）→ 用 toast 告知"指令已下发"，给操作者明确反馈。 */
  private toast(msg: string): void {
    try {
      promptAction.showToast({ message: msg, duration: 2000 });
    } catch (e) {
      log.e(`toast: ${JSON.stringify(e)}`);
    }
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

      // 仪表识别 PiP 小窗（形态B：边导航边瞥仪表）。绝对定位在左上，避开 TopBar/摇杆/FAB/底卡；
      // 收起态=小胶囊，展开态=小视频卡（可放大进全屏 VisionPage）。独立 WS，不打扰导航。
      this.VisionPip()

      // 摇杆区：**仅建图阶段显示**（建图=遥操作驱动小车扫图；非建图阶段不该手动开车——用户反馈）。
      // 导航/覆盖是自主执行，不用摇杆；连接但未建图时也不该能开车。
      if (this.mapping) {
        if (!this.joyHidden) {
          this.SpeedControl()
          Joystick({
            speed: this.joySpeed,
            disabled: !this.liveConnected(),
            onDirection: (dir: MoveDirection, speed: number) => this.onJoystick(dir, speed)
          })
            .position({ x: Space.lg, y: '62%' })
        }
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
      }

      // 底部操作/建图 sheet：**bottom-docked**（内容向上生长，永不超出界面）；三态内容由 BottomSheet 内部切换。
      this.BottomSheet()

      // 设备发现面板（浮层）
      if (this.showDevicePanel) {
        this.DevicePanel()
      }

      // 地图载入阻塞遮罩（结束建图后拉图）：载入中→全屏阻塞；失败→提示 + 重试/关闭
      if (this.mapLoading || this.mapLoadFailed) {
        this.MapLoadOverlay()
      }
    }
    .width('100%')
    .height('100%')
    .backgroundColor(AppColor.pageBg)
  }

  // 地图载入遮罩：结束建图后拉图期间全屏阻塞（吃掉点击），成功自动消失转操作卡；失败给明确提示 + 重试。
  @Builder
  MapLoadOverlay() {
    Column() {
      Column({ space: Space.md }) {
        if (this.mapLoadFailed) {
          Text('地图获取失败')
            .fontSize(FontSize.h3)
            .fontColor(AppColor.danger)
            .fontWeight(FontWeight.Bold)
          Text('结束建图后未拉到完整地图。请确认与车的连接（HTTP :8000）正常，或重试；也可关闭后重新建图。')
            .fontSize(FontSize.bodySm)
            .fontColor(AppColor.textSecondary)
            .textAlign(TextAlign.Center)
          Row({ space: Space.sm }) {
            Button('关闭')
              .fontColor(AppColor.primary)
              .backgroundColor(AppColor.surface)
              .border({ width: 1, color: AppColor.primary })
              .borderRadius(Radius.pill)
              .height(TOUCH_MIN)
              .onClick(() => {
                animateTo({ duration: Motion.fast, curve: Motion.curveStd }, () => {
                  this.mapLoadFailed = false;
                });
              })
            Button('重试')
              .fontColor(AppColor.onPrimary)
              .backgroundColor(AppColor.primary)
              .borderRadius(Radius.pill)
              .height(TOUCH_MIN)
              .onClick(() => this.loadMap(this.ip, 8, 1500, true))
          }
        } else {
          LoadingProgress().width(48).height(48).color(AppColor.primary)
          Text('正在载入地图…')
            .fontSize(FontSize.h3)
            .fontColor(AppColor.textBody)
            .fontWeight(FontWeight.Bold)
          Text('结束建图后正在拉取存好的地图，请稍候')
            .fontSize(FontSize.bodySm)
            .fontColor(AppColor.textSecondary)
            .textAlign(TextAlign.Center)
        }
      }
      .width('80%')
      .padding(Space.lg)
      .backgroundColor(AppColor.surface)
      .borderRadius(Radius.md)
    }
    .width('100%')
    .height('100%')
    .backgroundColor('rgba(0,0,0,0.45)')
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .onClick(() => {})
    .transition(TransitionEffect.OPACITY)
  }

  // 底部操作/建图 sheet —— **bottom-docked**（docked 到底部、内容向上生长，**永不超出界面**：修复旧 `.position(y:80%/84%)`
  // 在全覆盖/多机等高内容时溢出屏幕底部）。全屏透明包裹层 `hitTestBehavior=None`：空白区把触摸透传给底层地图
  // （缩放/平移/选点不被挡），仅卡片本体拦截。顶部小抓手 + 圆角 + 软阴影 → 像 Maps/iOS 底部卡，消除旧版浮在半空的"突兀"感。
  @Builder
  BottomSheet() {
    Column() {
      Column({ space: Space.sm }) {
        // 抓手（暗示这是底部卡）
        Row()
          .width(40)
          .height(4)
          .borderRadius(2)
          .backgroundColor(AppColor.border)
        // 内容：有完整地图且非建图/非载入 → 操作卡；否则建图卡（建图中 / 尚无地图）
        if (this.mission.mapReady && !this.mapping && !this.mapLoading) {
          this.OpCardContent()
        } else {
          this.BuildCardContent()
        }
      }
      .width('92%')
      .padding({ left: Space.md, right: Space.md, top: Space.sm, bottom: Space.md })
      .backgroundColor(AppColor.surface)
      .borderRadius(Radius.lg)
      .constraintSize({ maxHeight: '62%' }) // 兜底封顶（极端高内容也不顶到屏幕外），常规内容按需自适应
      .shadow({
        radius: Elevation.cardRadius,
        color: Elevation.cardColor,
        offsetX: 0,
        offsetY: Elevation.cardOffsetY
      })
    }
    .width('100%')
    .height('100%')
    .justifyContent(FlexAlign.End)
    .padding({ bottom: Space.lg })
    .hitTestBehavior(HitTestMode.None)
  }

  // 建图卡内容（三态：建图中 / 尚无地图；载入中由 MapLoadOverlay 阻塞遮罩接管）。仅内容、不自带定位/卡片 chrome（由 BottomSheet 提供）。
  // 流程：开始建图(cmd0) → 摇杆探索 → 结束建图(cmd2，紫派存图) → 拉图(loadMap) → 有图转操作卡内容。
  @Builder
  BuildCardContent() {
    Column({ space: Space.sm }) {
      if (this.mapping) {
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
      } else {
        Text('尚无地图')
          .fontSize(FontSize.h3)
          .fontColor(AppColor.textBody)
          .fontWeight(FontWeight.Bold)
          .width('100%')
        Text('还没有地图，需先建图。点「开始建图」，再用摇杆驱动小车走一圈环境。')
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
      }
    }
    .width('100%')
    .transition(TransitionEffect.OPACITY.combine(TransitionEffect.translate({ y: 12 })))
  }

  // 顶栏（ui-polish-plan §5）：返回图标钮 + 状态胶囊（dot+IP+模式，点击=设备面板）+ 重新建图 + 视频图标钮。
  // 「设备」按钮并入状态胶囊点击，顶栏从 5 个胶囊收敛为 3 个元素。
  @Builder
  TopBar() {
    Row() {
      IconButton({ icon: Icons.back, onTap: () => router.back() })
      Row() {
        Circle({ width: 10, height: 10 })
          .fill(this.connDotColor())
          .margin({ right: Space.sm })
        Text(this.connText())
          .fontSize(FontSize.dataSm)
          .fontFamily(FontFamily.mono)
          .fontColor(AppColor.textBody)
        Text(this.modeLabel())
          .fontSize(FontSize.caption)
          .fontColor(AppColor.textSecondary)
          .margin({ left: Space.sm })
        SymbolGlyph(Icons.expand)
          .fontSize(13)
          .fontColor([AppColor.textSecondary])
          .margin({ left: Space.xs })
      }
      .padding({ left: Space.md, right: Space.md })
      .height(40)
      .backgroundColor(AppColor.surface)
      .borderRadius(Radius.pill)
      .margin({ left: Space.sm })
      .shadow({
        radius: Elevation.softRadius,
        color: Elevation.softColor,
        offsetX: 0,
        offsetY: Elevation.softOffsetY
      })
      .onClick(() => this.toggleDevicePanel())
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
      // 随时展开仪表识别视频（形态A）：独立 WS 连香橙派，与导航 UDP/HTTP 链路解耦——
      // pushUrl 不销毁本页（aboutToDisappear/stopHeartbeat 仅 pop 时触发），保活留在 RobotTransport 单例继续跑，
      // 进出视频页导航毫不受影响。见 docs/vision-integration-plan.md §3。
      IconButton({
        icon: Icons.video,
        onTap: () => {
          router.pushUrl({ url: 'pages/VisionPage' }).catch((e: Error) => {
            log.e(`路由 VisionPage 失败：${e.message}`);
          });
        }
      })
    }
    .width('100%')
    .padding(Space.md)
  }

  /** 顶栏状态胶囊里的模式名。 */
  private modeLabel(): string {
    if (this.mode === ControlMode.astar) {
      return '单机导航';
    }
    if (this.mode === ControlMode.fullpath) {
      return '全路径覆盖';
    }
    return '多机协同';
  }

  /** 设备面板开合（带过渡）。 */
  private toggleDevicePanel(): void {
    animateTo({ duration: Motion.base, curve: Motion.curveStd }, () => {
      this.showDevicePanel = !this.showDevicePanel;
    });
  }

  // 仪表识别 PiP 小窗（形态B）。收起=胶囊；展开=小视频卡 + 放大/关闭。WS 生命周期=展开态（引用计数）。
  // 胶囊⇄小窗形变走软弹簧 + 两分支 scale/opacity 过渡（ui-polish-plan §3）。
  @Builder
  VisionPip() {
    Column() {
      if (this.pipExpanded) {
        Column() {
          Row() {
            Text('实时仪表')
              .fontSize(FontSize.label)
              .fontColor(AppColor.textBody)
              .layoutWeight(1)
            Button('放大')
              .fontSize(FontSize.label)
              .fontColor(AppColor.primary)
              .backgroundColor(AppColor.surface)
              .borderRadius(Radius.pill)
              .height(32)
              .padding({ left: Space.sm, right: Space.sm })
              .onClick(() => this.expandPipToFull())
            Button() {
              SymbolGlyph(Icons.close)
                .fontSize(13)
                .fontColor([AppColor.textSecondary])
            }
            .type(ButtonType.Circle)
            .width(28)
            .height(28)
            .backgroundColor(AppColor.surfaceMuted)
            .margin({ left: Space.xs })
            .onClick(() => this.closePip())
          }
          .width('100%')
          .padding({ bottom: Space.xs })

          Stack() {
            VideoView({ placeholder: '连接香橙派…' })
          }
          .width(192)
          .height(120)
          .borderRadius(Radius.sm)
          .clip(true)
        }
        .padding(Space.sm)
        .backgroundColor(AppColor.surface)
        .borderRadius(Radius.md)
        .shadow({
          radius: Elevation.cardRadius,
          color: Elevation.cardColor,
          offsetX: 0,
          offsetY: Elevation.cardOffsetY
        })
        .transition(TransitionEffect.OPACITY.combine(TransitionEffect.scale({ x: 0.9, y: 0.9 })))
      } else {
        Button() {
          Row() {
            SymbolGlyph(Icons.video)
              .fontSize(16)
              .fontColor([AppColor.primary])
            Text('仪表视频')
              .fontSize(FontSize.label)
              .fontColor(AppColor.primary)
              .margin({ left: Space.xs })
          }
        }
        .height(40)
        .backgroundColor(AppColor.surface)
        .border({ width: 1, color: AppColor.primary })
        .borderRadius(Radius.pill)
        .padding({ left: Space.md, right: Space.md })
        .onClick(() => this.openPip())
        .transition(TransitionEffect.OPACITY.combine(TransitionEffect.scale({ x: 0.9, y: 0.9 })))
      }
    }
    .position({ x: '3%', y: '12%' })
  }

  /** 展开 PiP：起视频流（引用计数 acquire），形变走软弹簧。 */
  private openPip(): void {
    if (this.pipExpanded) {
      return;
    }
    this.vision.acquire(Storage.getVisionHost(), Storage.getVisionPort());
    animateTo({ duration: Motion.slow, curve: Motion.springSoft }, () => {
      this.pipExpanded = true;
    });
  }

  /** 收起 PiP：释放视频流（计数归零才真正断开）。 */
  private closePip(): void {
    if (!this.pipExpanded) {
      return;
    }
    this.vision.release();
    animateTo({ duration: Motion.slow, curve: Motion.springSoft }, () => {
      this.pipExpanded = false;
    });
  }

  /** 放大到全屏 VisionPage（push 不销毁本页，PiP 仍持流；VisionPage 再 acquire 共享，返回不误断）。 */
  private expandPipToFull(): void {
    router.pushUrl({ url: 'pages/VisionPage' }).catch((e: Error) => {
      log.e(`路由 VisionPage 失败：${e.message}`);
    });
  }

  @Builder
  ZoomFabs() {
    Column({ space: Space.sm }) {
      IconButton({ icon: Icons.plus, diameter: TOUCH_MIN, glyph: 22, onTap: () => this.view('in') })
      IconButton({ icon: Icons.minus, diameter: TOUCH_MIN, glyph: 22, onTap: () => this.view('out') })
      IconButton({ icon: Icons.recenter, diameter: TOUCH_MIN, glyph: 22, onTap: () => this.view('reset') })
    }
    .position({ x: '88%', y: '40%' })
  }

  // 操作卡内容（按 mode 变体）。仅内容、不自带定位/卡片 chrome（由 BottomSheet 提供）。
  @Builder
  OpCardContent() {
    Column() {
      if (this.mode === ControlMode.astar) {
        this.AstarOps()
      } else if (this.mode === ControlMode.fullpath) {
        this.FullpathOps()
      } else {
        this.DistributedOps()
      }
    }
    .width('100%')
    .transition(TransitionEffect.OPACITY.combine(TransitionEffect.translate({ y: 12 })))
  }

  @Builder
  AstarOps() {
    Column({ space: Space.sm }) {
      // 分步指示：① 点图选点 → ② 开始导航（导航中提示取消/重选）
      Text(this.astarHint())
        .fontSize(FontSize.bodySm)
        .fontColor(AppColor.textBody)
        .width('100%')
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
        Button(this.astarNavigating ? '重新导航' : '开始导航')
          .fontColor(AppColor.onPrimary)
          .backgroundColor(AppColor.primary)
          .borderRadius(Radius.pill)
          .height(TOUCH_MIN)
          .enabled(this.hasTarget() && this.liveConnected())
          .onClick(() => this.startAstar())
      }
      .width('100%')
    }
    .width('100%')
  }

  @Builder
  FullpathOps() {
    Column({ space: Space.sm }) {
      // ① 选算法（牛耕 / 最小生成树 → cmd102 byte1）
      Text('① 选覆盖算法')
        .fontSize(FontSize.caption)
        .fontColor(AppColor.textSecondary)
        .width('100%')
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
      return '③ 4 顶点已选，全路径覆盖已启动（点「停止」结束）';
    }
    return `② 点地图选第 ${n + 1}/4 个顶点（${this.fpCornerName(n)}）`;
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
      // ① 选下发方式：经车载 agent(写黑板·各车自主) 或 平板直发兜底(直接发本车)
      Text('① 选下发方式')
        .fontSize(FontSize.caption)
        .fontColor(AppColor.textSecondary)
        .width('100%')
      Row() {
        Text(this.distUseAgent ? '经车载 agent（黑板·各车自主覆盖）' : '平板直发（兜底·直接发本车）')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.textBody)
          .layoutWeight(1)
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
      Text('③ 各车覆盖进度')
        .fontSize(FontSize.caption)
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

  // 设备面板（ui-polish-plan §6.4）：全屏遮罩（点击关闭）+ 居中卡片（自身消费点击），出没带过渡。
  @Builder
  DevicePanel() {
    Column() {
      Column() {
        DeviceList({
          devices: this.devices,
          connectedIps: this.connectedIps,
          scanning: this.scanning,
          onScan: () => this.startScan(),
          onConnect: (ip: string) => {
            this.connectTo(ip);
            this.loadMap(ip, 1, 0, false);  // 切车查既有图（非阻塞、不轮询）
            this.toggleDevicePanel();
          },
          onManualAdd: () => {
            router.pushUrl({ url: 'pages/SetIPPage' }).catch((e: Error) => {
              log.e(`路由 SetIPPage 失败：${e.message}`);
            });
          }
        })
      }
      .width('80%')
      .backgroundColor(AppColor.surface)
      .borderRadius(Radius.lg)
      .clip(true)
      .shadow({
        radius: Elevation.cardRadius,
        color: Elevation.cardColor,
        offsetX: 0,
        offsetY: Elevation.cardOffsetY
      })
      .onClick(() => {})
      .transition(TransitionEffect.OPACITY.combine(TransitionEffect.scale({ x: 0.96, y: 0.96 })))
    }
    .width('100%')
    .height('100%')
    .backgroundColor('rgba(0,0,0,0.40)')
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .onClick(() => this.toggleDevicePanel())
    .transition(TransitionEffect.OPACITY)
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

  /** astar 分步引导文案：未选点 → 已选点待导航 → 导航中。 */
  private astarHint(): string {
    if (this.astarNavigating) {
      return '导航中…（点「取消」停止；到达后可重新点图选目标）';
    }
    if (this.hasTarget()) {
      return `目标已选 ${this.targetText()} · 点「开始导航」`;
    }
    return '① 点地图选目标点 → ② 点「开始导航」';
  }

  private distHint(): string {
    if (this.distCorners.length === 1) {
      return '② 已选对角点1，点第二个对角点完成子区域';
    }
    return '② 点地图选两个对角点划分覆盖子区域（对角矩形）';
  }
}
