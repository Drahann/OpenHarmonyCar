/**
 * UDP 9 字节协议的编解码 + 命令枚举（与 contracts/udp-protocol.md 逐字段对齐）。
 * 移植自旧 Common/MakeData.ets（字节布局已正确），仅去掉对 dataConstants 的散落依赖，
 * 改用 constants/protocol.ets 的偏移/端序常量。
 */

import {
  UDP_PACKET_BYTES, BIG_ENDIAN,
  OFFSET_STATE, OFFSET_RUNSTATE, OFFSET_SPEED, OFFSET_X, OFFSET_Y, OFFSET_R
} from '../constants/protocol';

/**
 * 命令码（byte0）。取值刻意对齐 ASCII，所以既是 App 状态也是下发紫派的命令码
 * （如 fullpathStartRoute = 102 = 'f'）。取代旧 EnumModels.robotState。
 *
 * ✅ 已与紫派 udp2lcm.c 对账（见 contracts/udp-protocol-crosscheck.md）：
 *   - 5 = loadMap（加载地图；原 App 枚举缺，已补）
 *   - 107/108 = 分布式覆盖矩形的对角点 1/2（108 的 byte1 = robot_id），已改名 distAreaCorner1/2
 *   - 120 = 紫派未实现（死命令），不收录
 *   - 105 主机 IP 打包在 byte[1,2,4,6]；坐标单位 1/20m=5cm，r=度
 *   - 0x06 = 发现 ping（A 2026-06-10 在 udp.c 实现）：紫派收到即回 9 字节 [0]=0x06/[1]robotId/[2]状态/[3..8]位姿，
 *     **不记 clientIP、不起心跳、不武装 3s 急停**（A 当前 [1]/[2] 恒 0、[3..8]=当前位姿）
 */
export enum RobotCommand {
  beforeStart = 0,        // 初始化心跳连接；算法侧 LCM 30 启动建图
  pending = 1,            // 控制运动：用 runState/speed 构造轮控
  afterEnd = 2,           // 结束建图：停轮控；保存地图(32)；加载导航地图(10)
  startRoute = 3,         // 设置目标点(endX,endY)做导航规划 → LCM 20
  endRoute = 4,           // 取消导航 → LCM 23
  loadMap = 5,            // 加载地图：停车→取位姿→加载导航图(LCM 10)（对账新增）
  discoveryPing = 0x06,   // 发现 ping（广播）：A udp.c 收到即回 9 字节发现响应、不建心跳/不武装急停
  fullpathStartRoute = 102, // 'f' 启动全息路径规划(127)
  fullpathStartover = 103,  // 'g' 取消全息路径规划(126)
  fullpathSelect = 104,     // 'h' 单机全路径选房间顶点(125)，byte1=房间号
  distributed = 105,        // 'i' 子机拉主机地图(124)；主机 IP 在 byte[1,2,4,6]
  distributedEnd = 106,     // 'j' 加载地图(10)+接收主机目标点 → LCM 20
  distAreaCorner1 = 107,    // 'k' 分布式覆盖矩形·对角点1（暂存）
  distAreaCorner2 = 108,    // 'l' 对角点2 + robot_id(byte1)：从机规划覆盖(122)+跟踪(123)
  forceCreateMap = 0x6d     // 'm' 强制重新建图（A 8b4af4b 新增）：LCM30+iparams[1]=1(forceNewMap) 清旧图重建。
                            // cmd0「有图时只当心跳」会被 Navi.createMap 忽略，故"开始建图"用本码（见 contracts/udp-protocol.md）
}

/** 运动方向（byte1，仅命令 pending 有意义）。 */
export enum MoveDirection {
  stop = 0,
  go = 1,
  left = 2,
  right = 3
}

/** App → 紫派 发送数据。 */
export class UdpSendData {
  state: RobotCommand;
  runState?: MoveDirection;
  speed?: number;
  endX?: number;
  endY?: number;

  constructor(state: RobotCommand, runState?: MoveDirection, speed?: number, endX?: number, endY?: number) {
    this.state = state;
    this.runState = runState;
    this.speed = speed;
    this.endX = endX;
    this.endY = endY;
  }
}

/** 紫派 → App 接收数据（心跳，含坐标）。 */
export class UdpReceiveData {
  state: number;
  x: number;
  y: number;
  r: number;

  constructor(state: number, x: number, y: number, r: number) {
    this.state = state;
    this.x = x;
    this.y = y;
    this.r = r;
  }

  /** 本帧是否携带有效坐标（首字节 == 3）。 */
  hasPose(): boolean {
    return this.state === 3;
  }
}

/** 编码为 9 字节大端缓冲（= 旧 MakeSendData）。 */
export function encodeSend(d: UdpSendData): ArrayBuffer {
  const buffer = new ArrayBuffer(UDP_PACKET_BYTES);
  const bytes = new Uint8Array(buffer);
  bytes[OFFSET_STATE] = d.state;
  bytes[OFFSET_RUNSTATE] = d.runState ?? 0;
  bytes[OFFSET_SPEED] = d.speed ?? 0;
  const view = new DataView(buffer);
  view.setInt16(OFFSET_X, d.endX ?? 0, BIG_ENDIAN);
  view.setInt16(OFFSET_Y, d.endY ?? 0, BIG_ENDIAN);
  // byte7、byte8 保持 0（发送时未使用）
  return buffer;
}

/** 命令码 → 简短可读名（调试日志用；未知码回退 `cmd<code>`）。与上面 RobotCommand 枚举对齐。 */
export function commandName(code: number): string {
  switch (code) {
    case RobotCommand.beforeStart: return 'beforeStart';
    case RobotCommand.pending: return 'pending';
    case RobotCommand.afterEnd: return 'afterEnd';
    case RobotCommand.startRoute: return 'startRoute';
    case RobotCommand.endRoute: return 'endRoute';
    case RobotCommand.loadMap: return 'loadMap';
    case RobotCommand.discoveryPing: return 'discoveryPing';
    case RobotCommand.fullpathStartRoute: return 'fullpathStartRoute';
    case RobotCommand.fullpathStartover: return 'fullpathStartover';
    case RobotCommand.fullpathSelect: return 'fullpathSelect';
    case RobotCommand.distributed: return 'distributed';
    case RobotCommand.distributedEnd: return 'distributedEnd';
    case RobotCommand.distAreaCorner1: return 'distAreaCorner1';
    case RobotCommand.distAreaCorner2: return 'distAreaCorner2';
    case RobotCommand.forceCreateMap: return 'forceCreateMap';
    default: return `cmd${code}`;
  }
}

/** 解析 9 字节心跳（= 旧 parseReceiveData）。 */
export function decodeReceive(buffer: ArrayBuffer): UdpReceiveData {
  const bytes = new Uint8Array(buffer);
  const view = new DataView(buffer);
  const state: number = bytes[OFFSET_STATE];
  const x: number = view.getInt16(OFFSET_X, BIG_ENDIAN);
  const y: number = view.getInt16(OFFSET_Y, BIG_ENDIAN);
  const r: number = view.getInt16(OFFSET_R, BIG_ENDIAN);
  return new UdpReceiveData(state, x, y, r);
}
