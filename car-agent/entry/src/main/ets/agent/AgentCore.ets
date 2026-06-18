/**
 * AgentCore —— 车载 agent 运行时核心（与 ability 外壳**解耦**，可被任意宿主托管）。
 *   FleetMission 黑板（软总线）→ Reconciler → 本机 9 字节 UDP（udp2lcm, 127.0.0.1）；
 *   本机心跳 → 写回黑板本车位姿（节流）。设计见 docs/car-agent-plan.md §6。
 *
 * 为什么独立成 core：headless 的 `ServiceExtensionAbility` 是**系统 API**，第三方公共 SDK 编不过
 *   （见 docs/car-agent-plan.md §6.5 / contracts/integration-qa.md Q6.1）。当前用 **UIAbility（AgentAbility）**
 *   托管本 core，以便公共 SDK 编译 + 真机联调；将来 agent 作为**紫派系统应用**时，换一个
 *   `ServiceExtensionAbility` 外壳（Full SDK + 系统签名）托管同一个 core 即可——**下面的逻辑一行不动**。
 *
 * 关键点（同原 AgentServiceAbility）：
 *  - agent 是本机唯一 localhost UDP 客户端（§6.3）：平板经黑板下发、不直连本车 5001。
 *  - 幂等：Reconciler 仅在本车 phase/assignment/endpoint 变化时产命令（§6.2）。
 *  - 保活恒中性（pending+stop）：覆盖/导航是一次性 setup 命令（107/108/3），靠保活维持链路。
 *  - 会话用约定 `FLEET_SESSION_ID`；权限 DISTRIBUTED_DATASYNC 须**预授权**（无界面不能弹框）。
 */

import { common } from '@kit.AbilityKit';
import { Reconciler, CarView, AgentCmd, AgentAssignment, AgentPoint } from '../reconciler/Reconciler';
import { RobotTransport, MessageListener } from '../service/RobotTransport';
import { FleetMissionService, MissionSyncListener } from '../service/FleetMissionService';
import { Mission, MissionSnapshot, RobotRuntime, RobotStatus, AssignmentDTO, EndPointDTO } from '../model/mission';
import { UdpSendData, UdpReceiveData, RobotCommand, MoveDirection } from '../model/protocol';
import { FLEET_SESSION_ID, AGENT_LOCAL_PORT, UDP_PORT } from '../constants/protocol';
import { Log, Logger } from '../utils/log';

const LOCAL_STACK_IP: string = '127.0.0.1';   // 本机 udp2lcm
const WRITEBACK_MIN_MS: number = 300;          // 黑板写回最小间隔（心跳 500ms，避免刷爆软总线）
const CMD_DISPATCH_GAP_MS: number = 300;       // 本机命令对(cmd5→107→108)间隔，避免紫派"先暂存后使用"乱序
// X4（方案B）：子车 cmd105 触发紫派 cmd124 wget 拉主机图是**异步、无完成回执**。本应轮询本机 :8000 确认落地，
// 但**本紫派 OH 缺 @ohos.net.http 模块**（真机日志 http.abc not existed）→ agent 无法 HTTP 轮询；沙箱亦可能无 /data/test 读权限。
// 故退化为**固定延时**（可调）。更稳的做法需 A 给 cmd124 一个完成信号（LCM/心跳位），见 contracts/app-purplepi-alignment-audit.md X4。
const MAP_PULL_WAIT_MS: number = 15000;         // 子车 cmd105→cmd5 间隔：留足 wget 时间（保守值，按真机地图大小/网速调）

export class AgentCore {
  private readonly log: Logger = Log.scoped('AgentCore');
  private readonly reconciler: Reconciler = new Reconciler();
  private readonly transport: RobotTransport = RobotTransport.getInstance();
  private readonly mission: Mission = new Mission();   // 本机黑板镜像
  private carId: number = 1;                            // 本车号（可由 Want 覆盖）
  private lastWriteback: number = 0;
  private started: boolean = false;
  private engaged: boolean = false;   // 本车是否已作 udp2lcm 客户端(中性保活中)；仅任务期 true，空闲期回 UDP 静默

  private readonly blackboardListener: MissionSyncListener = (s: MissionSnapshot) => this.onBlackboard(s);
  private readonly localListener: MessageListener = (d: UdpReceiveData, from: string) => this.onLocalHeartbeat(d, from);

  /** 本车号（宿主从 Want.parameters.carId 注入）。 */
  setCarId(carId: number): void {
    this.carId = carId;
  }

  /** 起本机 UDP + 入软总线黑板 + 挂订阅。宿主 onCreate 时调用，传入其 context。 */
  start(context: common.Context): void {
    if (this.started) {
      return;
    }
    this.started = true;
    this.transport.bind(AGENT_LOCAL_PORT).then(() => {   // 绑本机独立端口(5002)，不抢同机 udp2lcm 的 5001
      this.transport.subscribe(this.localListener);
      // ⚠️ 不在此起保活：仅本车**有任务**时才作 udp2lcm 客户端（见 onBlackboard 引擎门控）。
      //   空闲/建图期保持 **UDP 静默** → 平板可直连该车建图/直控，不被 agent 抢端口/抢轮控。
      const fleet: FleetMissionService = FleetMissionService.getInstance();
      // init 内**先申请 DATASYNC 权限、再建 DDO**（顺序关键）；故 subscribe/join 必须等 init 完成再做，
      // 否则 joinSession 时 dataObject 还没建好会被静默跳过（真机表现：入会假成功、实际没连上）。
      fleet.init(context).then(() => {
        fleet.subscribeMission(this.blackboardListener);
        fleet.joinSession(FLEET_SESSION_ID)
          .then(() => this.log.i(`已入软总线会话 ${FLEET_SESSION_ID}（等待平板/它车写黑板）`))
          .catch((e: Object) => this.log.e(`joinSession 失败: ${JSON.stringify(e)}`));
        this.log.i(`agent up: car=${this.carId} → bind 本机 :${AGENT_LOCAL_PORT}（UDP 静默待命，仅任务期接 udp2lcm ${LOCAL_STACK_IP}:${UDP_PORT}）`);
      }).catch((e: Object) => this.log.e(`fleet init 失败: ${JSON.stringify(e)}`));
    }).catch((e: Object) => this.log.e(`start failed: ${JSON.stringify(e)}`));
  }

  /** 退订 + 停保活 + 退会。宿主 onDestroy 时调用。 */
  stop(): void {
    if (!this.started) {
      return;
    }
    this.started = false;
    this.engaged = false;
    this.transport.unsubscribe(this.localListener);
    this.transport.stopHeartbeat();
    const fleet: FleetMissionService = FleetMissionService.getInstance();
    fleet.unsubscribeMission(this.blackboardListener);
    fleet.leaveSession().catch((e: Object) => this.log.e(`leaveSession: ${JSON.stringify(e)}`));
  }

  /** 黑板变更：镜像快照 → 抽本车视图 → reconcile → 翻成本机 UDP 一次性下发。 */
  private onBlackboard(snapshot: MissionSnapshot): void {
    this.mission.applySnapshot(snapshot);
    this.ensureMyRobot();
    const view: CarView = this.extractView(snapshot);
    const cmds: AgentCmd[] = this.reconciler.reconcile(view);
    this.log.i(`黑板更新: phase=${view.phase} 车${this.carId} 区域=${view.assignment !== null ? '有' : '无'} 目标点=${view.endPoint !== null ? '有' : '无'} → 产 ${cmds.length} 条本机命令`);
    // 把本车状态写回黑板（供平板弹提示，让操作者在平板看到 agent 进展）：本轮有下发即 covering，否则 idle。
    const mine: RobotRuntime | undefined = this.mission.robots.find((r: RobotRuntime): boolean => r.index === this.carId);
    if (mine !== undefined) {
      const next: RobotStatus = cmds.length > 0 ? RobotStatus.covering : RobotStatus.idle;
      if (mine.status !== next) {
        mine.status = next;
        FleetMissionService.getInstance().publishMission(this.mission);
        this.log.i(`本车状态 → ${next}（写回黑板供平板提示）`);
      }
    }
    // UDP 引擎门控：仅本车**有任务**(覆盖/导航)时才作 udp2lcm 客户端 + 中性保活；空闲/建图期 UDP 静默，
    // 避免与平板直连抢同机 udp2lcm（端口/单客户端/轮控冲突 —— 真机"建不了图"根因之一）。
    const engaged: boolean = (view.phase === 'covering' && view.assignment !== null) || view.endPoint !== null;
    if (engaged && !this.engaged) {
      this.engaged = true;
      this.transport.startHeartbeat(LOCAL_STACK_IP, AgentCore.neutral());  // 任务期才起中性保活（成为 udp2lcm 客户端）
      this.log.i('本车进入任务 → 起本机中性保活（成为 udp2lcm 客户端）');
    }
    // 命令对(cmd105→cmd5→107→108)间留间隔：紫派"先暂存后使用"，避免乱序/未处理完。
    // cmd105(拉主机图)后给紫派 cmd124 的 wget 留足时间(MAP_PULL_WAIT_MS)再发 cmd5（X4：本机无法 HTTP 轮询，见上注）。
    let delay: number = 0;
    for (const c of cmds) {
      const payload: UdpSendData = AgentCore.toPayload(c);
      setTimeout(() => {
        this.transport.send(LOCAL_STACK_IP, payload);
      }, delay);
      delay += (c.cmd === RobotCommand.distributed) ? MAP_PULL_WAIT_MS : CMD_DISPATCH_GAP_MS;
    }
    if (!engaged && this.engaged) {
      this.engaged = false;
      this.transport.stopHeartbeat(LOCAL_STACK_IP);  // 任务结束 → 停保活回 UDP 静默（让平板可直连建图/直控该车）
      this.log.i('本车任务结束 → 停本机保活（回 UDP 静默）');
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
      this.log.i(`本机心跳 → 写回黑板: 车${this.carId} pose=(${r.x},${r.y},${r.r})`);
    }
  }

  /** 从快照抽本车视图切片（phase + 本车 assignment + 本车目标点）。 */
  private extractView(s: MissionSnapshot): CarView {
    const mine: AssignmentDTO | undefined =
      s.assignments.find((a: AssignmentDTO): boolean => a.carId === this.carId);
    const ep: EndPointDTO | undefined = s.endPoints.length > 0 ? s.endPoints[0] : undefined;
    // 显式构造带接口类型的对象：ArkTS `arkts-no-untyped-obj-literals` 要求对象字面量对应已声明接口/类。
    let assignment: AgentAssignment | null = null;
    if (mine !== undefined) {
      const c1: AgentPoint = { x: mine.corner1.x, y: mine.corner1.y };
      const c2: AgentPoint = { x: mine.corner2.x, y: mine.corner2.y };
      assignment = { corner1: c1, corner2: c2, robotId: mine.robotId };
    }
    let endPoint: AgentPoint | null = null;
    if (ep !== undefined && ep.x !== null && ep.y !== null) {
      endPoint = { x: ep.x, y: ep.y };
    }
    // master 地址：黑板 map.ref（平板写本机 IP）。子车 reconcile 据此发 cmd105 拉图（方案B）。
    const masterIp: string | null =
      (s.map !== undefined && s.map.ref !== undefined && s.map.ref.length > 0) ? s.map.ref : null;
    const view: CarView = {
      phase: s.phase, carId: this.carId, assignment: assignment, endPoint: endPoint, masterIp: masterIp
    };
    return view;
  }

  private ensureMyRobot(): void {
    if (this.mission.robots.find((r: RobotRuntime): boolean => r.index === this.carId) === undefined) {
      this.mission.robots = this.mission.robots.concat(new RobotRuntime(this.carId));
    }
  }

  /**
   * AgentCmd → 9 字节发送帧。cmd108 的 byte1 = robotId（借 runState 位写入，encodeSend 原样落 byte1）。
   * cmd105 特例：master IP 四段 → byte[1,2,4,6]（runState/speed=IP[0]/[1]；endX/endY=IP[2]/[3]，encodeSend
   * 大端写 int16、高字节恒 0 → 低字节恰落 byte[4]/[6]，与紫派 'i' iparams[0..3]=byte[1,2,4,6] 逐字节吻合）。
   */
  private static toPayload(c: AgentCmd): UdpSendData {
    if (c.cmd === RobotCommand.distributed && c.masterIp !== undefined) {
      const oct: number[] = AgentCore.ipOctets(c.masterIp);
      return new UdpSendData(RobotCommand.distributed, oct[0] as MoveDirection, oct[1], oct[2], oct[3]);
    }
    const b1: number | undefined = c.robotId ?? c.runState;
    return new UdpSendData(
      c.cmd as RobotCommand,
      b1 === undefined ? undefined : (b1 as MoveDirection),
      c.speed, c.x, c.y
    );
  }

  /** "a.b.c.d" → [a,b,c,d]（每段 0..255；非法段 0）。供 cmd105 主机 IP 打包进 byte[1,2,4,6]。 */
  private static ipOctets(ip: string): number[] {
    const parts: string[] = ip.split('.');
    const oct: number[] = [0, 0, 0, 0];
    for (let i = 0; i < 4 && i < parts.length; i++) {
      const n: number = parseInt(parts[i], 10);
      oct[i] = Number.isNaN(n) ? 0 : (n & 0xff);
    }
    return oct;
  }

  private static neutral(): UdpSendData {
    return new UdpSendData(RobotCommand.pending, MoveDirection.stop, 0);
  }
}
