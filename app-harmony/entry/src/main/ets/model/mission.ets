/**
 * 协同共享状态模型。
 *
 * 用 @Observed 类（配合子组件 @ObjectLink）取代旧代码用平行数组 EndPointX[]/EndPointY[]
 * 绕过"@State 对象数组成员属性不刷新"的写法，使对象数组成员属性可响应式更新。
 *
 * 多机协同时，DeviceCollabService 通过 @ohos.data.distributedDataObject 在可信设备间同步
 * 本模型的"纯数据快照"（MissionSnapshot / *DTO，无方法、可序列化）。在 UI 侧仍以 @Observed
 * 类驱动刷新；服务层负责快照 ↔ 模型的桥接（见 service/DeviceCollabService.ets）。
 */

/** 控制模式：取代旧 IndexPara.Mode 布尔数组的隐式编码。 */
export enum ControlMode {
  astar = 'astar',          // 单机 A* 导航（原 distributedsecondPage）
  fullpath = 'fullpath',    // 全路径覆盖（原 distriFullPathPage）
  distributed = 'distributed' // 多机分布式协同（原 distributedPage）
}

// ── 终点选点 ────────────────────────────────────────────────────────────────
export interface EndPointDTO {
  x: number | null;
  y: number | null;
}

@Observed
export class EndPoint {
  x: number | null;
  y: number | null;

  constructor(x: number | null = null, y: number | null = null) {
    this.x = x;
    this.y = y;
  }

  toDTO(): EndPointDTO {
    return { x: this.x, y: this.y };
  }
}

// ── 单台机器人运行态 ────────────────────────────────────────────────────────
export interface RobotRuntimeDTO {
  index: number;
  ip: string;
  networkId: string;
  online: boolean;
  x: number;
  y: number;
  r: number;
  command: number;
}

@Observed
export class RobotRuntime {
  /** 车号（1 起） */
  index: number;
  /** UDP 目标 IP */
  ip: string;
  /** 分布式软总线 networkId —— 跨端拉起用（取代旧 distributedPage 误用的 deviceId） */
  networkId: string;
  online: boolean;
  /** 最新位姿（真实世界坐标 + 朝向） */
  x: number;
  y: number;
  r: number;
  /** 最近下发的命令码 */
  command: number;

  constructor(index: number, ip: string = '', networkId: string = '') {
    this.index = index;
    this.ip = ip;
    this.networkId = networkId;
    this.online = false;
    this.x = 0;
    this.y = 0;
    this.r = 0;
    this.command = 0;
  }

  toDTO(): RobotRuntimeDTO {
    return {
      index: this.index, ip: this.ip, networkId: this.networkId, online: this.online,
      x: this.x, y: this.y, r: this.r, command: this.command
    };
  }

  applyDTO(d: RobotRuntimeDTO): void {
    this.index = d.index;
    this.ip = d.ip;
    this.networkId = d.networkId;
    this.online = d.online;
    this.x = d.x;
    this.y = d.y;
    this.r = d.r;
    this.command = d.command;
  }
}

// ── 任务（协同共享根） ──────────────────────────────────────────────────────
export interface MissionSnapshot {
  mode: string;
  mapReady: boolean;
  endPoints: EndPointDTO[];
  robots: RobotRuntimeDTO[];
}

@Observed
export class Mission {
  mode: ControlMode;
  /** 地图是否已就绪（建图完成 / 已拉到图） */
  mapReady: boolean;
  /** 已选目标点 */
  endPoints: EndPoint[];
  /** 参与协同的机器人及其运行态 */
  robots: RobotRuntime[];

  constructor(mode: ControlMode = ControlMode.astar) {
    this.mode = mode;
    this.mapReady = false;
    this.endPoints = [];
    this.robots = [];
  }

  toSnapshot(): MissionSnapshot {
    return {
      mode: this.mode,
      mapReady: this.mapReady,
      endPoints: this.endPoints.map((e: EndPoint): EndPointDTO => e.toDTO()),
      robots: this.robots.map((r: RobotRuntime): RobotRuntimeDTO => r.toDTO())
    };
  }

  /** 用远端同步来的快照覆盖本地（distributedDataObject change 回调里调用）。 */
  applySnapshot(s: MissionSnapshot): void {
    this.mode = s.mode as ControlMode;
    this.mapReady = s.mapReady;
    this.endPoints = s.endPoints.map((e: EndPointDTO): EndPoint => new EndPoint(e.x, e.y));
    this.robots = s.robots.map((d: RobotRuntimeDTO): RobotRuntime => {
      const r: RobotRuntime = new RobotRuntime(d.index);
      r.applyDTO(d);
      return r;
    });
  }
}
