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
import { UdpSendData, UdpReceiveData, RobotCommand, encodeSend, decodeReceive } from '../model/protocol';
import { Log, Logger } from '../utils/log';
import { DEBUG_WIRE } from '../constants/debug';

/** 心跳订阅回调：收到一帧（已解析）+ 来源 IP。 */
export type MessageListener = (data: UdpReceiveData, remoteIp: string) => void;

/** 广播地址（方案B 局域网发现，见 docs/app-refactor-plan.md §连接与设备发现）。 */
const BROADCAST_ADDR: string = '255.255.255.255';
/** 发现期重复探测间隔。 */
const DISCOVER_PING_INTERVAL_MS: number = 1000;

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

  private constructor() {
  }

  /** 绑定本机端口并注册唯一消息分发。重复调用安全。 */
  async bind(): Promise<void> {
    if (this.bound) {
      return;
    }
    await new Promise<void>((resolve: () => void, reject: (e: BusinessError) => void) => {
      this.udp.bind({ address: '0.0.0.0', port: UDP_PORT, family: 1 }, (err: BusinessError) => {
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
      if (DEBUG_WIRE) {
        this.traceFrame('RX <-', ip, info.message, `state=${data.state} x=${data.x} y=${data.y} r=${data.r}`);
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

  /** 立即发送一帧。 */
  send(ip: string, data: UdpSendData): void {
    const buffer: ArrayBuffer = encodeSend(data);
    if (DEBUG_WIRE) {
      this.traceFrame('TX ->', ip, buffer,
        `cmd=${data.state} run=${data.runState ?? 0} spd=${data.speed ?? 0} x=${data.endX ?? 0} y=${data.endY ?? 0}`);
    }
    const address: socket.NetAddress = { address: ip, port: UDP_PORT };
    this.udp.send({ address, data: buffer }, (err: BusinessError) => {
      if (err) {
        this.log.e(`send fail code=${err.code} msg=${err.message}`);
      }
    });
  }

  /** 线缆 trace（仅 DEBUG_WIRE 开时由调用点触发）：hex + 解码摘要。 */
  private traceFrame(dir: string, ip: string, buffer: ArrayBuffer, summary: string): void {
    const bytes: Uint8Array = new Uint8Array(buffer);
    let hex: string = '';
    for (let i = 0; i < bytes.length; i++) {
      hex += bytes[i].toString(16).padStart(2, '0');
      if (i < bytes.length - 1) {
        hex += ' ';
      }
    }
    this.log.d(`${dir} ${ip} [${hex}] ${summary}`);
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

  /** 启动/重置某个 ip 的保活循环：每 1s 重发 payload 给该 ip。重复调用同 ip 会重置。 */
  startHeartbeat(ip: string, payload: UdpSendData): void {
    this.stopHeartbeat(ip);
    const loop: HeartbeatLoop = new HeartbeatLoop(-1, payload);
    loop.timer = setInterval(() => {
      this.send(ip, loop.payload);
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
      return;
    }
    this.heartbeats.forEach((loop: HeartbeatLoop) => clearInterval(loop.timer));
    this.heartbeats.clear();
  }

  /**
   * 局域网广播发现（方案B）。开广播 → 周期性向 255.255.255.255:5001 发探测包 → 收集回包车（按源 IP 去重），
   * 每次更新回调 onUpdate；durationMs 后自动停止。返回手动停止函数。
   *
   * ⚠️ 探测命令暂用 `cmd 0`（beforeStart）作兜底：A 确认 integration-qa.md Q5（新增不触发 3s 急停的
   *    0x06 发现 ping）前，cmd0 广播会让子网内每台车都判为"被连"，未被选中的车 3s 收不到后续指令即急停
   *    ——**仅联调可用**。A 确认后把默认 ping 换成专用发现码并同步 contracts/udp-protocol.md。
   */
  discover(onUpdate: DiscoverListener, durationMs: number = CONNECT_TIMEOUT_MS,
    ping: RobotCommand = RobotCommand.beforeStart): () => void {
    const found: Map<string, DiscoveredRobot> = new Map<string, DiscoveredRobot>();
    let pinger: number = -1;
    let ender: number = -1;
    let active: boolean = true;

    const probe: MessageListener = (data: UdpReceiveData, ip: string) => {
      if (!active) {
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
    const sendPing = (): void => this.send(BROADCAST_ADDR, new UdpSendData(ping));
    sendPing();
    pinger = setInterval(sendPing, DISCOVER_PING_INTERVAL_MS);
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
