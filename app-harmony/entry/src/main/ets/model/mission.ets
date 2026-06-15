/**
 * 协同共享状态模型。
 *
 * 用 @Observed 类（配合子组件 @ObjectLink）取代旧代码用平行数组 EndPointX[]/EndPointY[]
 * 绕过"@State 对象数组成员属性不刷新"的写法，使对象数组成员属性可响应式更新。
 *
 * 多机协同时，FleetMissionService 通过 @ohos.data.distributedDataObject 在可信设备间同步
 * 本模型的"纯数据快照"（MissionSnapshot / *DTO，无方法、可序列化）。在 UI 侧仍以 @Observed
 * 类驱动刷新；服务层负责快照 ↔ 模型的桥接（见 service/FleetMissionService.ets）。
 *
 * ★ 当 mode=distributed 时，本 Mission 即 contracts/multi-robot-collab.md 的 **FleetMission 共享黑板**：
 *   phase / frame / map / area / assignments / robots 为协同字段，整块经软总线同步到各车 agent。
 *   **快照字段名与该契约保持一致**（各车 agent 反序列化同一 JSON）——改这些字段须同步契约
 *   （CLAUDE.md：跨设备通信代码与 contracts/ 逐字段一致）。
 */

import { Point } from './geometry';

/** 控制模式：取代旧 IndexPara.Mode 布尔数组的隐式编码。 */
export enum ControlMode {
  astar = 'astar',          // 单机 A* 导航（原 distributedsecondPage）
  fullpath = 'fullpath',    // 全路径覆盖（原 distriFullPathPage）
  distributed = 'distributed' // 多机分布式协同（原 distributedPage）
}

/** 任务阶段（FleetMission.phase）：master 建图 → 平板划分 → 各车覆盖。 */
export enum MissionPhase {
  idle = 'idle',
  scanning = 'scanning',   // master 建图中
  dividing = 'dividing',   // 平板在共享地图上划分子区域中
  covering = 'covering',   // 各车覆盖遍历中
  done = 'done'
}

/** 单车运行状态（FleetMission.robots[].status）：online 表链路通断，status 表"在做什么"。 */
export enum RobotStatus {
  offline = 'offline',
  idle = 'idle',
  scanning = 'scanning',
  covering = 'covering',
  done = 'done',
  error = 'error'
}

// ── 坐标系元数据 frame（✅ 成员A确认，见 contracts/map-format.md 与协同契约 §坐标系元数据）──
/**
 * 地图坐标系三要素。锚定在 master 建图/定位初始位姿——子车从 master 同一物理起点、同一朝向出发，
 * 故其初始位姿按构造 = (0,0,0°)（紫派对 UDP `cmd 5` 加载地图时强制把初始位姿设为 0,0,0；
 * 接口功能与对接问题说明.md §六.3）。
 * ✅ A确认：分辨率 0.05m/格（5cm，UDP 整数坐标与格 1:1）；朝向用度、范围 [-180,180]、
 *   `theta=0` 指向 +X、正角朝 +Y（CCW，数学正向）；原点 = 建图初始位姿。
 */
export interface Frame {
  originX: number;       // 地图 (0,0) 对应的现实原点 = master 建图初始位姿（master 坐标系）
  originY: number;
  resolution: number;    // 每格 = 米（= 0.05m = 5cm，✅ 对账 udp2lcm.c + A确认）
  headingZero: number;   // 0°指向（度）：0 = +X 方向，正角 CCW 朝 +Y
}

/** 默认 frame（A确认值；origin 以实际建图初始位姿为准，多机用 master 起点）。 */
export function defaultFrame(): Frame {
  return { originX: 0, originY: 0, resolution: 0.05, headingZero: 0 };
}

/** 地图来源（FleetMission.map）。text 仅"方案A 经软总线同步地图"时携带（见契约 §地图传输）。 */
export interface MapRef {
  ref: string;           // master 地图来源标识（如 master networkId / URL）
  text?: string;         // 可选：整图文本（方案A）
}

/** 对角矩形（master 坐标系）：用于 area 大区域与 assignment 子区域。 */
export interface Rect {
  corner1: Point;
  corner2: Point;
}

/** 一条机器人行走轨迹（按车号 + 世界坐标点序列）。供 MapCanvas 画轨迹折线（导航/覆盖均用）。 */
export interface RobotTrail {
  index: number;
  pts: Point[];
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

// ── 子区域分配 assignment ───────────────────────────────────────────────────
export interface AssignmentDTO {
  carId: number;
  corner1: Point;
  corner2: Point;
  robotId: number;
}

/**
 * 一台子车的覆盖子区域 = 对角矩形 + robotId（0=主/1=从）。平板规划器写入；子车 agent 读到
 * 自己的项后经本机 UDP `cmd107`(对角点1) + `cmd108`(对角点2, byte1=robotId) 下发，
 * 紫派据矩形规划 FullRoad 覆盖(LCM 122)+跟踪(123)。见 contracts/udp-protocol-crosscheck.md。
 */
@Observed
export class Assignment {
  /** 子车车号（= RobotRuntime.index） */
  carId: number;
  corner1: Point;
  corner2: Point;
  robotId: number;

  constructor(carId: number, corner1: Point, corner2: Point, robotId: number) {
    this.carId = carId;
    this.corner1 = corner1;
    this.corner2 = corner2;
    this.robotId = robotId;
  }

  toDTO(): AssignmentDTO {
    return { carId: this.carId, corner1: this.corner1, corner2: this.corner2, robotId: this.robotId };
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
  progress: number;
  status: string;
}

@Observed
export class RobotRuntime {
  /** 车号（1 起），= 协同黑板里的 carId */
  index: number;
  /** UDP 目标 IP */
  ip: string;
  /** 分布式软总线 networkId —— 跨端拉起用（取代旧 distributedPage 误用的 deviceId） */
  networkId: string;
  online: boolean;
  /** 最新位姿（真实世界坐标 + 朝向），对应契约 robots[].pose 的 x/y/r */
  x: number;
  y: number;
  r: number;
  /** 最近下发的命令码 */
  command: number;
  /** 覆盖进度 0..1（契约 robots[].progress） */
  progress: number;
  /** 运行状态（契约 robots[].status） */
  status: RobotStatus;

  constructor(index: number, ip: string = '', networkId: string = '') {
    this.index = index;
    this.ip = ip;
    this.networkId = networkId;
    this.online = false;
    this.x = 0;
    this.y = 0;
    this.r = 0;
    this.command = 0;
    this.progress = 0;
    this.status = RobotStatus.offline;
  }

  toDTO(): RobotRuntimeDTO {
    return {
      index: this.index, ip: this.ip, networkId: this.networkId, online: this.online,
      x: this.x, y: this.y, r: this.r, command: this.command,
      progress: this.progress, status: this.status
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
    this.progress = d.progress;
    this.status = d.status as RobotStatus;
  }
}

// ── 任务（协同共享根 = FleetMission 黑板） ────────────────────────────────────
export interface MissionSnapshot {
  mode: string;
  phase: string;
  mapReady: boolean;
  frame: Frame;
  map: MapRef;
  area: Rect | null;
  endPoints: EndPointDTO[];
  assignments: AssignmentDTO[];
  robots: RobotRuntimeDTO[];
}

@Observed
export class Mission {
  mode: ControlMode;
  /** 协同阶段（单机模式恒为 idle/covering 等价语义，主要供 distributed 用） */
  phase: MissionPhase;
  /** 地图是否已就绪（建图完成 / 已拉到图） */
  mapReady: boolean;
  /** 坐标系元数据（master 锚定） */
  frame: Frame;
  /** 地图来源 */
  map: MapRef;
  /** master 扫出的大区域边界（对角矩形，master 坐标系）；未知时 null */
  area: Rect | null;
  /** 已选目标点（单机导航 / 全路径选点） */
  endPoints: EndPoint[];
  /** 子区域分配（多机覆盖） */
  assignments: Assignment[];
  /** 参与协同的机器人及其运行态 */
  robots: RobotRuntime[];

  constructor(mode: ControlMode = ControlMode.astar) {
    this.mode = mode;
    this.phase = MissionPhase.idle;
    this.mapReady = false;
    this.frame = defaultFrame();
    this.map = { ref: '' };
    this.area = null;
    this.endPoints = [];
    this.assignments = [];
    this.robots = [];
  }

  toSnapshot(): MissionSnapshot {
    return {
      mode: this.mode,
      phase: this.phase,
      mapReady: this.mapReady,
      frame: this.frame,
      map: this.map,
      area: this.area,
      endPoints: this.endPoints.map((e: EndPoint): EndPointDTO => e.toDTO()),
      assignments: this.assignments.map((a: Assignment): AssignmentDTO => a.toDTO()),
      robots: this.robots.map((r: RobotRuntime): RobotRuntimeDTO => r.toDTO())
    };
  }

  /** 用远端同步来的快照覆盖本地（distributedDataObject change 回调里调用）。 */
  applySnapshot(s: MissionSnapshot): void {
    this.mode = s.mode as ControlMode;
    this.phase = s.phase as MissionPhase;
    this.mapReady = s.mapReady;
    this.frame = s.frame;
    this.map = s.map;
    this.area = s.area;
    this.endPoints = s.endPoints.map((e: EndPointDTO): EndPoint => new EndPoint(e.x, e.y));
    this.assignments = s.assignments.map((a: AssignmentDTO): Assignment =>
      new Assignment(a.carId, a.corner1, a.corner2, a.robotId));
    this.robots = s.robots.map((d: RobotRuntimeDTO): RobotRuntime => {
      const r: RobotRuntime = new RobotRuntime(d.index);
      r.applyDTO(d);
      return r;
    });
  }
}
