/**
 * AgentServiceAbility —— 车载 agent 无界面常驻入口（ServiceExtensionAbility）。完整装配：
 *   FleetMission 黑板（软总线）→ Reconciler → 本机 9 字节 UDP（udp2lcm, 127.0.0.1）；
 *   本机心跳 → 写回黑板本车位姿（节流）。设计见 docs/car-agent-plan.md §6。
 *
 * 关键点：
 *  - **agent 是本机唯一 localhost UDP 客户端**（§6.3）：平板经黑板下发、不直连本车 5001。
 *  - **幂等**：Reconciler 仅在本车 phase/assignment/endpoint 变化时产命令；别的车位姿变化不重发（§6.2）。
 *  - **保活恒中性**（pending+stop）：覆盖/导航是一次性 setup 命令（107/108/3），靠保活维持链路、不重复触发。
 *  - 会话用约定 `FLEET_SESSION_ID` 让平板/各车 agent 无协商即入同一黑板（MVP；后续可由平板下发 sessionId）。
 *  - 权限 DISTRIBUTED_DATASYNC 须**预授权**（无界面服务不能弹框，见 integration-qa Q6.3）。
 *
 * ⚠️ 未经 DevEco 验证；DevEco 工程集成（作为模块/独立工程、resources、签名）+ 真机联调待 P5（与成员A）。
 */

import { ServiceExtensionAbility, Want } from '@kit.AbilityKit';
import { Reconciler, CarView, AgentCmd } from '../reconciler/Reconciler';
import { RobotTransport, MessageListener } from '../service/RobotTransport';
import { FleetMissionService, MissionSyncListener } from '../service/FleetMissionService';
import { Mission, MissionSnapshot, RobotRuntime, AssignmentDTO, EndPointDTO } from '../model/mission';
import { UdpSendData, UdpReceiveData, RobotCommand, MoveDirection } from '../model/protocol';
import { Log, Logger } from '../utils/log';

const LOCAL_STACK_IP: string = '127.0.0.1';   // 本机 udp2lcm
const FLEET_SESSION_ID: string = 'OpenHarmonyCarFleetV1';  // 约定会话（MVP 无协商入会）
const WRITEBACK_MIN_MS: number = 300;          // 黑板写回最小间隔（心跳 500ms，避免刷爆软总线）
const CMD_DISPATCH_GAP_MS: number = 300;       // 本机命令对(cmd5→107→108)间隔，避免紫派"先暂存后使用"乱序

export default class AgentServiceAbility extends ServiceExtensionAbility {
  private readonly log: Logger = Log.scoped('AgentService');
  private readonly reconciler: Reconciler = new Reconciler();
  private readonly transport: RobotTransport = RobotTransport.getInstance();
  private readonly mission: Mission = new Mission();   // 本机黑板镜像
  private carId: number = 1;                            // 本车号（可由 Want 覆盖）
  private lastWriteback: number = 0;

  private readonly blackboardListener: MissionSyncListener = (s: MissionSnapshot) => this.onBlackboard(s);
  private readonly localListener: MessageListener = (d: UdpReceiveData, from: string) => this.onLocalHeartbeat(d, from);

  onCreate(want: Want): void {
    const params: Record<string, Object> | undefined = want.parameters;
    if (params !== undefined) {
      const cid: Object = params['carId'];
      if (typeof cid === 'number') {
        this.carId = cid as number;
      }
    }
    this.start();
  }

  /** 起本机 UDP + 入软总线黑板 + 挂订阅。 */
  private start(): void {
    this.transport.bind().then(() => {
      this.transport.subscribe(this.localListener);
      this.transport.startHeartbeat(LOCAL_STACK_IP, AgentServiceAbility.neutral());  // 中性保活
      const fleet: FleetMissionService = FleetMissionService.getInstance();
      fleet.init(this.context);                  // ServiceExtensionContext（权限须预授权）
      fleet.subscribeMission(this.blackboardListener);
      fleet.joinSession(FLEET_SESSION_ID).catch((e: Object) => this.log.e(`joinSession: ${JSON.stringify(e)}`));
      this.log.i(`agent up: car=${this.carId} -> ${LOCAL_STACK_IP}`);
    }).catch((e: Object) => this.log.e(`start failed: ${JSON.stringify(e)}`));
  }

  /** 黑板变更：镜像快照 → 抽本车视图 → reconcile → 翻成本机 UDP 一次性下发。 */
  private onBlackboard(snapshot: MissionSnapshot): void {
    this.mission.applySnapshot(snapshot);
    this.ensureMyRobot();
    const cmds: AgentCmd[] = this.reconciler.reconcile(this.extractView(snapshot));
    // 命令对(cmd5→107→108)间留间隔：紫派"先暂存后使用"，避免乱序/未处理完（对照 ControlPage 同款延时）
    let delay: number = 0;
    for (const c of cmds) {
      const payload: UdpSendData = AgentServiceAbility.toPayload(c);
      setTimeout(() => {
        this.transport.send(LOCAL_STACK_IP, payload);
      }, delay);
      delay += CMD_DISPATCH_GAP_MS;
    }
  }

  /** 本机心跳 → 更新本车位姿 → 节流写回黑板。 */
  private onLocalHeartbeat(data: UdpReceiveData, from: string): void {
    if (!data.hasPose()) {
      return;
    }
    const r: RobotRuntime | undefined = this.mission.robots.find((x: RobotRuntime): boolean => x.index === this.carId);
    if (r === undefined) {
      return;
    }
    r.x = data.x;
    r.y = data.y;
    r.r = data.r;
    r.online = true;
    const now: number = Date.now();
    if (now - this.lastWriteback >= WRITEBACK_MIN_MS) {
      this.lastWriteback = now;
      FleetMissionService.getInstance().publishMission(this.mission);  // 仅写本车位姿（最后写者胜，MVP）
    }
  }

  /** 从快照抽本车视图切片（phase + 本车 assignment + 本车目标点）。 */
  private extractView(s: MissionSnapshot): CarView {
    const mine: AssignmentDTO | undefined =
      s.assignments.find((a: AssignmentDTO): boolean => a.carId === this.carId);
    const ep: EndPointDTO | undefined = s.endPoints.length > 0 ? s.endPoints[0] : undefined;
    return {
      phase: s.phase,
      carId: this.carId,
      assignment: mine !== undefined
        ? { corner1: mine.corner1, corner2: mine.corner2, robotId: mine.robotId }
        : null,
      endPoint: (ep !== undefined && ep.x !== null && ep.y !== null) ? { x: ep.x, y: ep.y } : null
    };
  }

  private ensureMyRobot(): void {
    if (this.mission.robots.find((r: RobotRuntime): boolean => r.index === this.carId) === undefined) {
      this.mission.robots = this.mission.robots.concat(new RobotRuntime(this.carId));
    }
  }

  /** AgentCmd → 9 字节发送帧。cmd108 的 byte1 = robotId（借 runState 位写入，encodeSend 原样落 byte1）。 */
  private static toPayload(c: AgentCmd): UdpSendData {
    const b1: number | undefined = c.robotId ?? c.runState;
    return new UdpSendData(
      c.cmd as RobotCommand,
      b1 === undefined ? undefined : (b1 as MoveDirection),
      c.speed, c.x, c.y
    );
  }

  private static neutral(): UdpSendData {
    return new UdpSendData(RobotCommand.pending, MoveDirection.stop, 0);
  }

  onDestroy(): void {
    this.transport.unsubscribe(this.localListener);
    this.transport.stopHeartbeat();
    const fleet: FleetMissionService = FleetMissionService.getInstance();
    fleet.unsubscribeMission(this.blackboardListener);
    fleet.leaveSession().catch((e: Object) => this.log.e(`leaveSession: ${JSON.stringify(e)}`));
  }
}
