/**
 * 持久化（取代旧 utils/SetIP.ets）。
 *
 * 改进：
 *  - 英文 key（旧用中文 key `第N号机器人的IP地址`）。
 *  - getter 不再有副作用（旧 getIp 每次读都调用 persistProp）——改为 init() 一次性登记可持久化属性，
 *    之后 set 走 AppStorage.setOrCreate（已与持久化联动），get 仅读取。
 */

import { DEFAULT_TARGET_IP } from '../constants/protocol';
import { VISION_DEFAULT_HOST, VISION_DEFAULT_PORT } from '../constants/vision';

const KEY_ROBOT_COUNT: string = 'robot_count';
const KEY_LOCAL_IP: string = 'local_ip';
const ROBOT_IP_PREFIX: string = 'robot_ip_';
/** 香橙派视觉服务地址（与导航的车 IP 无关——不同设备）。 */
const KEY_VISION_HOST: string = 'vision_host';
const KEY_VISION_PORT: string = 'vision_port';
/** 预登记的最大机器人数量（持久化槽位）。 */
const MAX_ROBOTS: number = 8;

const IP_REGEX: RegExp =
  /^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;

export class Storage {
  /** 应用启动时调用一次（EntryAbility）：登记可持久化属性及默认值。 */
  static init(): void {
    PersistentStorage.persistProp<number>(KEY_ROBOT_COUNT, 1);
    PersistentStorage.persistProp<string>(KEY_LOCAL_IP, '');
    PersistentStorage.persistProp<string>(KEY_VISION_HOST, VISION_DEFAULT_HOST);
    PersistentStorage.persistProp<number>(KEY_VISION_PORT, VISION_DEFAULT_PORT);
    for (let i = 0; i < MAX_ROBOTS; i++) {
      PersistentStorage.persistProp<string>(`${ROBOT_IP_PREFIX}${i}`, DEFAULT_TARGET_IP);
    }
  }

  static getRobotCount(): number {
    return AppStorage.get<number>(KEY_ROBOT_COUNT) ?? 1;
  }

  static setRobotCount(n: number): void {
    AppStorage.setOrCreate<number>(KEY_ROBOT_COUNT, n);
  }

  /** 第 i 台（0 起）机器人的 IP。 */
  static getRobotIp(i: number): string {
    return AppStorage.get<string>(`${ROBOT_IP_PREFIX}${i}`) ?? DEFAULT_TARGET_IP;
  }

  static setRobotIp(i: number, ip: string): void {
    AppStorage.setOrCreate<string>(`${ROBOT_IP_PREFIX}${i}`, ip);
  }

  static getLocalIp(): string {
    return AppStorage.get<string>(KEY_LOCAL_IP) ?? '';
  }

  static setLocalIp(ip: string): void {
    AppStorage.setOrCreate<string>(KEY_LOCAL_IP, ip);
  }

  /** 香橙派视觉服务 host（默认 192.168.1.5；与车 IP 无关，不同设备）。 */
  static getVisionHost(): string {
    return AppStorage.get<string>(KEY_VISION_HOST) ?? VISION_DEFAULT_HOST;
  }

  static setVisionHost(host: string): void {
    AppStorage.setOrCreate<string>(KEY_VISION_HOST, host);
  }

  static getVisionPort(): number {
    return AppStorage.get<number>(KEY_VISION_PORT) ?? VISION_DEFAULT_PORT;
  }

  static setVisionPort(port: number): void {
    AppStorage.setOrCreate<number>(KEY_VISION_PORT, port);
  }

  /** 合法 IPv4 校验（移植自旧 SetIP.IslegalIP）。 */
  static isValidIp(value: string): boolean {
    return IP_REGEX.test(value);
  }

  /** "a.b.c.d" → [a,b,c,d]（移植自旧 SetIP.IP2Number）。 */
  static ipToBytes(ip: string): number[] {
    return ip.split('.').map((p: string): number => parseInt(p, 10));
  }
}
