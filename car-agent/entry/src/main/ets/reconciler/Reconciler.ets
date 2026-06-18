/**
 * Reconciler —— 车载 agent 的「黑板 → 本机命令」**纯决策逻辑**（无 IO、可单测）。
 * 由 AgentServiceAbility 在 FleetMission 黑板变更时调用，产出待下发命令序列；
 * **幂等**：本车相关切片签名未变则返回 []（不每 tick 重发）。状态机见 docs/car-agent-plan.md §6.2。
 *
 * Node 镜像测试见 tools/verify/verify-reconciler.mjs。
 * ⚠️ 未经 DevEco 验证。类型与命令码暂在本文件本地定义（与 app-harmony model/protocol、model/mission 同源），
 *    待 shared-core HAR 抽取后改为 import 去重（car-agent-plan §6.1）。
 *
 * 分布式覆盖时序（**方案B**，2026-06-12 对齐裁决，见 contracts/app-purplepi-alignment-audit.md 裁决记录）：
 *   - **master(robotId=0)**：自有建图+定位，直接 `107 → 108`（**不发 cmd5**，避免把自身位姿归零）。
 *   - **sub(robotId=1)**：`105`(从 master HTTP 拉图，紫派转 cmd124 wget) → `5`(加载拉到的图+归零 0,0,0) → `107 → 108`。
 *     master 地址来自黑板 `map.ref`（→ CarView.masterIp）；未知则本轮不发、等地址到达（签名含 masterIp，到达即重评）。
 */

export interface AgentPoint {
  x: number;
  y: number;
}

export interface AgentAssignment {
  corner1: AgentPoint;
  corner2: AgentPoint;
  robotId: number;
}

/** 本车在黑板里的视图切片（AgentServiceAbility 从 FleetMission 按 carId 抽取）。 */
export interface CarView {
  phase: string;                       // MissionPhase: idle|scanning|dividing|covering|done
  carId: number;
  assignment: AgentAssignment | null;  // 覆盖子区域（对角矩形）
  endPoint: AgentPoint | null;         // 单点目标（nav）
  masterIp: string | null;             // master 地址（黑板 map.ref）：子车 cmd105 拉图用；master/无关时可 null
}

/** 一条待下发命令（AgentServiceAbility 翻成 UdpSendData → RobotTransport.send(127.0.0.1)）。 */
export interface AgentCmd {
  cmd: number;            // RobotCommand 数值
  runState?: number;
  speed?: number;
  x?: number;
  y?: number;
  robotId?: number;       // → encodeSend 的 byte1（cmd108 覆盖）
  masterIp?: string;      // cmd105 拉主机图：master IP（toPayload 打进 byte[1,2,4,6]）
}

// 命令码（与 model/protocol.RobotCommand 同源；shared-core 后改 import）
const CMD_PULL_MAP: number = 105;     // 'i' 子车从 master 拉图（方案B；紫派转 cmd124 wget）
const CMD_LOAD_MAP: number = 5;       // 加载图 + 位姿归零(0,0,0)
const CMD_START_ROUTE: number = 3;    // 设目标点
const CMD_END_ROUTE: number = 4;      // 取消导航
const CMD_AREA_CORNER1: number = 107; // 覆盖矩形对角点1
const CMD_AREA_CORNER2: number = 108; // 对角点2 + robot_id(byte1) → 开始覆盖

export class Reconciler {
  private lastSig: string = '';
  private mapPulled: boolean = false;  // 子车是否已发过 cmd105 拉主机图（每覆盖会话一次）
  private loaded: boolean = false;     // 子车是否已发过 cmd5 归零
  private active: boolean = false;     // 本车是否处于"已下发任务"态（用于只在 active→inactive 时取消）

  /**
   * 黑板视图 → 待发命令序列。幂等：本车相关切片签名未变返回 []。
   * 覆盖：master → `107,108`；sub → `(105 拉图)(5 加载归零) → 107,108`。
   * 导航：cmd3(目标点)。任务撤销/done 且之前在跑：cmd4。
   */
  reconcile(v: CarView): AgentCmd[] {
    const sig: string = Reconciler.signature(v);
    if (sig === this.lastSig) {
      return [];
    }
    this.lastSig = sig;
    const out: AgentCmd[] = [];

    if (v.phase === 'covering' && v.assignment !== null) {
      const isSub: boolean = v.assignment.robotId === 1;
      if (isSub) {
        // 子车需先从 master 拉图。master 地址未到（黑板 map.ref 空）→ 本轮先不发，等地址到达再重评。
        if (v.masterIp === null || v.masterIp.length === 0) {
          return out;  // []：不置 active、不发覆盖，避免无图覆盖
        }
        if (!this.mapPulled) {
          out.push({ cmd: CMD_PULL_MAP, masterIp: v.masterIp });  // cmd105：master IP → byte[1,2,4,6]，紫派 cmd124 wget
          this.mapPulled = true;
        }
        if (!this.loaded) {
          out.push({ cmd: CMD_LOAD_MAP });   // cmd5：加载拉到的图 + 位姿归零 (0,0,0)
          this.loaded = true;
        }
      }
      // master(robotId=0)：自有建图+定位，直接覆盖（不发 cmd5/cmd105，避免归零自身位姿）。
      out.push({ cmd: CMD_AREA_CORNER1, x: v.assignment.corner1.x, y: v.assignment.corner1.y });
      out.push({
        cmd: CMD_AREA_CORNER2, x: v.assignment.corner2.x, y: v.assignment.corner2.y,
        robotId: v.assignment.robotId
      });
      this.active = true;
      return out;
    }

    if (v.endPoint !== null) {
      out.push({ cmd: CMD_START_ROUTE, x: v.endPoint.x, y: v.endPoint.y });
      this.active = true;
      return out;
    }

    // 无任务（含 phase=done / 分配与目标都空）：仅在之前确有任务时取消一次
    if (this.active) {
      out.push({ cmd: CMD_END_ROUTE });
      this.active = false;
    }
    return out;
  }

  /** 断线重连 / 换车 / 重新建图时重置（下次按新黑板全量重发）。 */
  reset(): void {
    this.lastSig = '';
    this.mapPulled = false;
    this.loaded = false;
    this.active = false;
  }

  private static signature(v: CarView): string {
    return JSON.stringify({ p: v.phase, a: v.assignment, e: v.endPoint, m: v.masterIp });
  }
}
