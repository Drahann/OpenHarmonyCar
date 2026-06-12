/**
 * 唯一的 UDP socket 持有者（取代旧 Common/Socket.ets 的全局 udp + 各页面各自 on/off('message')）。
 *
 * 职责：
 *  - 绑定一次、注册一次 on('message')，按订阅者分发解析后的心跳（单点分发，避免监听泄漏/抢占）。
 *  - 收发 9 字节协议（model/protocol）。
 *  - 保活心跳循环：每 1s 重发当前指令，满足"App 至少每 1s 发一次"，避免紫派 3s 未收指令急停。
 *    **支持多目标**：平板直管多车时，每个目标 IP 各一条独立保活循环；车端常驻 agent 只对本机一台。
 *
 * 单例：整个 App 共享一个实例。UI 不直接碰 socket。
 */

import { socket } from '@kit.NetworkKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { UDP_PORT, CONNECT_TIMEOUT_MS, HEARTBEAT_INTERVAL_MS } from '../constants/protocol';
import { UdpSendData, UdpReceiveData, RobotCommand, encodeSend, decodeReceive, commandName } from '../model/protocol';
import { Log, Logger } from '../utils/log';
import { DEBUG_WIRE } from '../constants/debug';

/** 心跳订阅回调：收到一帧（已解析）+ 来源 IP。 */
export type MessageListener = (data: UdpReceiveData, remoteIp: string) => void;

/** 广播地址（方案B 局域网发现，见 docs/app-refactor-plan.md §连接与设备发现）。 */
const BROADCAST_ADDR: string = '255.255.255.255';
/** 发现期重复探测间隔。 */
const DISCOVER_PING_INTERVAL_MS: number = 1000;
/** 香橙派视觉设备的发现响应码（byte0=0x07）：App 广播 0x06 发现，香橙派回 0x07 表明"我是视觉源、不是车"（见 contracts/udp-protocol.md 设备发现）。 */
const VISION_DISCOVERY_RESP: number = 0x07;
/** 心跳新鲜阈值（ms）：保活中且距上次心跳 < 此值判"已连接"；心跳 500ms，留 ~4 帧容差。 */
const HEARTBEAT_FRESH_MS: number = 2000;
/** RX 心跳 trace 节流（ms）：DEBUG_WIRE 开时每 ip 最多约 1/s 打印（心跳 500ms，避免刷屏）。 */
const WIRE_RX_THROTTLE_MS: number = 1000;

/** 发现到的一台车（仅 UDP 可知：源 IP + 最近一帧状态/时间；车号协议不携带，按 IP 标识）。 */
export class DiscoveredRobot {
  ip: string;
  /** 最近一帧心跳首字节（状态码）。 */
  state: number;
  /** 最近收到时间戳（ms）。 */
  lastSeen: number;

  constructor(ip: string, state: number, lastSeen: number) {
    this.ip = ip;
    this.state = state;
    this.lastSeen = lastSeen;
  }
}

/** 发现回调：每收到/更新一台车即回传当前去重列表。 */
export type DiscoverListener = (robots: DiscoveredRobot[]) => void;

/** 连接状态（按 IP 派生：是否在保活 + 心跳新鲜度）。 */
export enum ConnState {
  discovered = 'discovered',   // 已发现：响应过探测，但未建立控制保活链路
  connecting = 'connecting',   // 已起保活，自连接起尚未收到心跳
  connected = 'connected',     // 保活中且心跳新鲜
  lost = 'lost'                // 曾连上，心跳已断（保活仍在）
}

/** 单个目标 IP 的保活循环：定时器句柄 + 当前重发的指令（可中途更新，不打断计时器）。 */
class HeartbeatLoop {
  timer: number;
  payload: UdpSendData;

  constructor(timer: number, payload: UdpSendData) {
    this.timer = timer;
    this.payload = payload;
  }
}

export class RobotTransport {
  private static instance: RobotTransport | null = null;

  static getInstance(): RobotTransport {
    const inst: RobotTransport = RobotTransport.instance ?? new RobotTransport();
    RobotTransport.instance = inst;
    return inst;
  }

  private readonly log: Logger = Log.scoped('RobotTransport');
  private udp: socket.UDPSocket = socket.constructUDPSocketInstance();
  private bound: boolean = false;
  private listeners: Set<MessageListener> = new Set<MessageListener>();
  /** 多目标保活：ip → 该 ip 的保活循环。 */
  private heartbeats: Map<string, HeartbeatLoop> = new Map<string, HeartbeatLoop>();
  /** 每个 ip 最近收到心跳的时间戳（ms）。全局 on('message') 维护，供连接状态派生。 */
  private lastSeenByIp: Map<string, number> = new Map<string, number>();
  /** 每个 ip 起保活的时间戳（ms）。区分 connecting（自连接起未收）与 lost（曾收后断）。 */
  private connectStartedAt: Map<string, number> = new Map<string, number>();
  /** wire TX 去重：某 ip 上一帧 hex（保活只在内容相对上一帧变化时打印，避免刷屏）。 */
  private lastTxHexByIp: Map<string, string> = new Map<string, string>();
  /** wire RX 节流：某 ip 上次打印 RX 心跳的时间戳（ms）。 */
  private lastRxLogAtByIp: Map<string, number> = new Map<string, number>();

  private constructor() {
  }

  /**
   * 绑定本机端口并注册唯一消息分发。重复调用安全。
   * @param localPort 本机收口端口，默认 `UDP_PORT`(5001，平板用)；**车载 agent 须传 `AGENT_LOCAL_PORT`(5002)**，
   *   避免与同机 udp2lcm 抢 5001（见 constants/protocol AGENT_LOCAL_PORT）。发送目标端口不受此影响（恒 UDP_PORT）。
   */
  async bind(localPort: number = UDP_PORT): Promise<void> {
    if (this.bound) {
      return;
    }
    await new Promise<void>((resolve: () => void, reject: (e: BusinessError) => void) => {
      this.udp.bind({ address: '0.0.0.0', port: localPort, family: 1 }, (err: BusinessError) => {
        if (err) {
          reject(err);
          return;
        }
        resolve();
      });
    });
    this.udp.on('message', (info: socket.SocketMessageInfo) => {
      const data: UdpReceiveData = decodeReceive(info.message);
      const ip: string = info.remoteInfo.address;
      const now: number = Date.now();
      this.lastSeenByIp.set(ip, now);
      // RX 心跳每 500ms 一帧 → 仅 DEBUG_WIRE 开且按 WIRE_RX_THROTTLE_MS 节流打印（避免刷屏；显式命令 TX 始终打印）。
      if (DEBUG_WIRE && now - (this.lastRxLogAtByIp.get(ip) ?? 0) >= WIRE_RX_THROTTLE_MS) {
        this.lastRxLogAtByIp.set(ip, now);
        this.log.i(`RX← ${ip} state=${data.state} x=${data.x} y=${data.y} r=${data.r} [${this.hexOf(info.message)}]`);
      }
      this.listeners.forEach((l: MessageListener) => l(data, ip));
    });
    this.bound = true;
  }

  subscribe(listener: MessageListener): void {
    this.listeners.add(listener);
  }

  unsubscribe(listener: MessageListener): void {
    this.listeners.delete(listener);
  }

  /**
   * 立即发送一帧**显式命令**：**始终打印一行 TX 日志**（命令名 + 字段 + hex），让操作者直观看到
   * "实际发了哪 9 个字节、对不对"（info 级，release 由 LOG_MIN_LEVEL 收掉）。
   * 保活/周期重发请走 `sendKeepalive`（默认不打印，避免每帧刷屏）。
   */
  send(ip: string, data: UdpSendData): void {
    const buffer: ArrayBuffer = encodeSend(data);
    const hex: string = this.hexOf(buffer);
    this.log.i(this.txLine(ip, data, hex));
    this.lastTxHexByIp.set(ip, hex);  // 记住本帧，紧随其后的同内容保活不再重复打印
    this.rawSend(ip, buffer);
  }

  /**
   * 保活 / 周期重发（心跳循环、广播发现 ping）：默认**不打印**（避免每 500ms/1s 刷屏）；
   * 仅 `DEBUG_WIRE` 开且**帧内容相对上一帧变化**时打印一行（带"(保活)"）。照常发送。
   */
  private sendKeepalive(ip: string, data: UdpSendData): void {
    const buffer: ArrayBuffer = encodeSend(data);
    if (DEBUG_WIRE) {
      const hex: string = this.hexOf(buffer);
      if (this.lastTxHexByIp.get(ip) !== hex) {
        this.log.i(`${this.txLine(ip, data, hex)} (保活)`);
        this.lastTxHexByIp.set(ip, hex);
      }
    }
    this.rawSend(ip, buffer);
  }

  /** 实际把缓冲发到 ip:5001（发送失败打 error）。 */
  private rawSend(ip: string, buffer: ArrayBuffer): void {
    const address: socket.NetAddress = { address: ip, port: UDP_PORT };
    this.udp.send({ address, data: buffer }, (err: BusinessError) => {
      if (err) {
        this.log.e(`send fail code=${err.code} msg=${err.message}`);
      }
    });
  }

  /** 9 字节缓冲 → 空格分隔 hex 串（如 `03 00 00 01 02 ff ff 00 00`）。 */
  private hexOf(buffer: ArrayBuffer): string {
    const bytes: Uint8Array = new Uint8Array(buffer);
    let hex: string = '';
    for (let i = 0; i < bytes.length; i++) {
      hex += bytes[i].toString(16).padStart(2, '0');
      if (i < bytes.length - 1) {
        hex += ' ';
      }
    }
    return hex;
  }

  /** TX 日志行：`TX→ <ip> <命令名>(<码>) run=.. spd=.. x=.. y=.. [hex]`。 */
  private txLine(ip: string, data: UdpSendData, hex: string): string {
    return `TX→ ${ip} ${commandName(data.state)}(${data.state}) ` +
      `run=${data.runState ?? 0} spd=${data.speed ?? 0} x=${data.endX ?? 0} y=${data.endY ?? 0} [${hex}]`;
  }

  /**
   * 建连：向 ip 发 hello（通常命令 0），CONNECT_TIMEOUT_MS 内收到该 ip 回包即判定成功。
   * 取代旧 "发 0 → 在 udp.on 回调里看回包" 的散落逻辑。
   */
  connect(ip: string, hello: UdpSendData): Promise<boolean> {
    return new Promise<boolean>((resolve: (ok: boolean) => void) => {
      let done: boolean = false;
      let timer: number = -1;
      const probe: MessageListener = (_data: UdpReceiveData, from: string) => {
        if (!done && from === ip) {
          done = true;
          this.unsubscribe(probe);
          if (timer !== -1) {
            clearTimeout(timer);
          }
          resolve(true);
        }
      };
      this.subscribe(probe);
      this.send(ip, hello);
      timer = setTimeout(() => {
        if (!done) {
          done = true;
          this.unsubscribe(probe);
          resolve(false);
        }
      }, CONNECT_TIMEOUT_MS);
    });
  }

  /** 启动/重置某个 ip 的保活循环：**立即发一帧** + 每 1s 重发 payload 给该 ip。重复调用同 ip 会重置。 */
  startHeartbeat(ip: string, payload: UdpSendData): void {
    this.stopHeartbeat(ip);
    const loop: HeartbeatLoop = new HeartbeatLoop(-1, payload);
    this.send(ip, payload);  // 立即发首帧：车收首包即捕获 App IP 并开始回心跳，连接态尽快转"已连接"（不等 1s）。显式打印一行
    loop.timer = setInterval(() => {
      this.sendKeepalive(ip, loop.payload);  // 周期重发：默认静默（DEBUG_WIRE 开且内容变化才打印）
    }, HEARTBEAT_INTERVAL_MS);
    this.heartbeats.set(ip, loop);
  }

  /** 更新某 ip 保活重发的指令（如运动→停止）。不打断计时器；该 ip 无循环时忽略。 */
  setHeartbeatPayload(ip: string, payload: UdpSendData): void {
    const loop: HeartbeatLoop | undefined = this.heartbeats.get(ip);
    if (loop !== undefined) {
      loop.payload = payload;
    }
  }

  /** 停止保活：给 ip 则只停该目标；不给 ip 则停全部。 */
  stopHeartbeat(ip?: string): void {
    if (ip !== undefined) {
      const loop: HeartbeatLoop | undefined = this.heartbeats.get(ip);
      if (loop !== undefined) {
        clearInterval(loop.timer);
        this.heartbeats.delete(ip);
      }
      this.connectStartedAt.delete(ip);
      return;
    }
    this.heartbeats.forEach((loop: HeartbeatLoop) => clearInterval(loop.timer));
    this.heartbeats.clear();
    this.connectStartedAt.clear();
  }

  /**
   * 某 IP 的当前连接状态（由"是否在保活"+"心跳新鲜度"派生）。
   * discovered=未保活；connecting=保活但自连接起未收心跳；connected=保活且心跳新鲜；lost=保活但心跳已断。
   */
  connStateOf(ip: string): ConnState {
    if (!this.heartbeats.has(ip)) {
      return ConnState.discovered;
    }
    const last: number | undefined = this.lastSeenByIp.get(ip);
    if (last !== undefined && Date.now() - last < HEARTBEAT_FRESH_MS) {
      return ConnState.connected;
    }
    const startedAt: number = this.connectStartedAt.get(ip) ?? 0;
    return (last !== undefined && last >= startedAt) ? ConnState.lost : ConnState.connecting;
  }

  /** 距上次收到该 IP 心跳的毫秒数（从未收到则 -1）。供 UI 显示"刚刚 / N 秒前"。 */
  msSinceLastSeen(ip: string): number {
    const last: number | undefined = this.lastSeenByIp.get(ip);
    return last === undefined ? -1 : Date.now() - last;
  }

  /**
   * 独占连接：停掉其它所有目标的保活（避免多车被半连接、随后各自 3s 急停的混乱），仅对 ip 起中性保活。
   * payload 必须是中性帧（pending+stop），不触发建图/运动。收到该 ip 心跳后 connStateOf 即转 connected。
   * ⚠️ 被切换掉的旧车会在 3s 后自行急停（预期的安全行为：不再被控的车停下）。
   */
  connectExclusive(ip: string, payload: UdpSendData): void {
    const others: string[] = [];
    this.heartbeats.forEach((_loop: HeartbeatLoop, key: string) => {
      if (key !== ip) {
        others.push(key);
      }
    });
    others.forEach((key: string) => this.stopHeartbeat(key));
    this.startHeartbeat(ip, payload);
    this.connectStartedAt.set(ip, Date.now());
  }

  /**
   * 局域网广播发现（方案B）。开广播 → 周期性向 255.255.255.255:5001 发探测包 → 收集回包车（按源 IP 去重），
   * 每次更新回调 onUpdate；durationMs 后自动停止。返回手动停止函数。
   *
   * 探测命令 = `0x06` 发现 ping（A 2026-06-10 已在 `udp2lcm/udp.c` 实现）：紫派收到只回 9 字节发现响应
   *    （`[0]=0x06`、`[3..8]`=当前位姿），**不记 clientIP、不起心跳、不武装 3s 急停** → 广播发现不会误连/误急停。
   *    （此前用 `cmd0` 兜底会让全网车判"被连"、3s 收不到后续指令即急停；A 实现 0x06 后已切换。见 contracts/udp-protocol.md。）
   */
  discover(onUpdate: DiscoverListener, durationMs: number = CONNECT_TIMEOUT_MS,
    ping: RobotCommand = RobotCommand.discoveryPing, onVision?: (ip: string) => void): () => void {
    const found: Map<string, DiscoveredRobot> = new Map<string, DiscoveredRobot>();
    let pinger: number = -1;
    let ender: number = -1;
    let active: boolean = true;

    const probe: MessageListener = (data: UdpReceiveData, ip: string) => {
      if (!active) {
        return;
      }
      if (data.state === VISION_DISCOVERY_RESP) {     // 香橙派视觉(0x07)：不是车，单独回调（设视觉 host）
        if (onVision !== undefined) {
          onVision(ip);
        }
        return;
      }
      const now: number = Date.now();
      const exist: DiscoveredRobot | undefined = found.get(ip);
      if (exist === undefined) {
        found.set(ip, new DiscoveredRobot(ip, data.state, now));
      } else {
        exist.state = data.state;
        exist.lastSeen = now;
      }
      onUpdate(Array.from(found.values()));
    };

    const stop = (): void => {
      if (!active) {
        return;
      }
      active = false;
      this.unsubscribe(probe);
      if (pinger !== -1) {
        clearInterval(pinger);
      }
      if (ender !== -1) {
        clearTimeout(ender);
      }
      const off: socket.UDPExtraOptions = { broadcast: false };
      this.udp.setExtraOptions(off, (_e: BusinessError) => {});
    };

    this.subscribe(probe);
    const on: socket.UDPExtraOptions = { broadcast: true };
    this.udp.setExtraOptions(on, (err: BusinessError) => {
      if (err) {
        this.log.e(`enable broadcast fail code=${err.code} msg=${err.message}`);
      }
    });
    this.send(BROADCAST_ADDR, new UdpSendData(ping));  // 首发显式打印一行
    pinger = setInterval(() => this.sendKeepalive(BROADCAST_ADDR, new UdpSendData(ping)), DISCOVER_PING_INTERVAL_MS);
    ender = setTimeout(stop, durationMs);
    return stop;
  }

  close(): void {
    this.stopHeartbeat();
    this.listeners.clear();
    try {
      this.udp.close();
    } catch (e) {
      this.log.e(`close error: ${JSON.stringify(e)}`);
    }
    this.bound = false;
  }
}
