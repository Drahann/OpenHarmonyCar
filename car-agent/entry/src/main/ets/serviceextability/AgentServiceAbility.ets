/**
 * AgentServiceAbility —— 车载 agent 无界面常驻入口（ServiceExtensionAbility）。
 * 装配：FleetMission 黑板（软总线）→ Reconciler → 本机 9 字节 UDP（udp2lcm）；本机心跳 → 写回黑板。
 *
 * 🚧 P2 脚手架：Reconciler + translate 已就绪；transport / 黑板接线标 TODO(shared-core)——
 *    待 HAR 抽出 app-harmony 的 RobotTransport / FleetMissionService / model / utils 后启用。
 * ⚠️ 未经 DevEco 验证。设计见 docs/car-agent-plan.md §6。
 */

import { ServiceExtensionAbility } from '@kit.AbilityKit';
import { Reconciler, CarView, AgentCmd } from '../reconciler/Reconciler';
// TODO(shared-core HAR)：
//   import { RobotTransport } from '@shared/service/RobotTransport';
//   import { FleetMissionService } from '@shared/service/FleetMissionService';
//   import { MissionSnapshot, UdpReceiveData } from '@shared/model/...';
//   import { UdpSendData } from '@shared/model/protocol';
//   import { Log } from '@shared/utils/log';

/** translate 输出：9 字节发送帧字段（mirror app-harmony UdpSendData，shared-core 后改用之）。 */
export interface SendFields {
  state: number;     // 命令码 byte0
  runState: number;  // byte1（cmd108 = robotId）
  speed: number;     // byte2
  endX: number;
  endY: number;
}

const LOCAL_STACK_IP: string = '127.0.0.1';  // 本机 udp2lcm

export default class AgentServiceAbility extends ServiceExtensionAbility {
  private reconciler: Reconciler = new Reconciler();
  private carId: number = 1;  // TODO: 从启动参数 / 本机配置读本车号

  onCreate(): void {
    // 装配（待 shared-core）：
    // 1) RobotTransport.getInstance().bind() → 目标 127.0.0.1:5001 起中性保活（pending+stop）。
    // 2) FleetMissionService 入软总线、持 FleetMission 黑板，订阅变更 → this.onBlackboardChange。
    // 3) RobotTransport.subscribe(本机心跳) → this.onLocalHeartbeat（写回黑板，节流）。
  }

  /** 黑板变更：抽本车视图 → reconcile → 翻成本机 UDP 下发。 */
  private onBlackboardChange(): void {
    const view: CarView = this.extractView();
    const cmds: AgentCmd[] = this.reconciler.reconcile(view);
    for (const c of cmds) {
      this.dispatch(AgentServiceAbility.translate(c));
    }
  }

  /** 本机心跳 → 写回黑板本车 RobotRuntime（节流，见 car-agent-plan §6.4）。 */
  private onLocalHeartbeat(): void {
    // TODO(shared-core)：from===127.0.0.1 && hasPose → 更新本车 x/y/r/status → 节流写回黑板单一 missionJson。
  }

  /** 从黑板快照抽出本车视图切片（按 carId 定位 robots/assignments/endPoints/phase）。 */
  private extractView(): CarView {
    // TODO(shared-core)：按 this.carId 从 MissionSnapshot 组装真实视图。
    return { phase: 'idle', carId: this.carId, assignment: null, endPoint: null };
  }

  /** 把一条命令下发到本机栈。 */
  private dispatch(f: SendFields): void {
    // TODO(shared-core)：改用共享 RobotTransport + Log：
    //   const payload = new UdpSendData(f.state, f.runState, f.speed, f.endX, f.endY);
    //   RobotTransport.getInstance().send(LOCAL_STACK_IP, payload);
    //   RobotTransport.getInstance().setHeartbeatPayload(LOCAL_STACK_IP, payload); // 保活重发当前动作
    console.info(`[agent] -> ${LOCAL_STACK_IP} cmd=${f.state} run=${f.runState} x=${f.endX} y=${f.endY}`);
  }

  /** AgentCmd → 9 字节发送字段。cmd108 的 byte1 = robotId；其余 runState/speed 默认 0。 */
  static translate(c: AgentCmd): SendFields {
    return {
      state: c.cmd,
      runState: c.robotId ?? c.runState ?? 0,
      speed: c.speed ?? 0,
      endX: c.x ?? 0,
      endY: c.y ?? 0
    };
  }

  onDestroy(): void {
    // TODO(shared-core)：RobotTransport.getInstance().stopHeartbeat()；退会软总线。
  }
}
