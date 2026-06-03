/**
 * 设备协同服务：分布式软总线发现/认证（distributedDeviceManager）+ 跨端拉起 +
 * @ohos.data.distributedDataObject 协同状态同步。
 *
 * 取代旧 Common/distributed.ets 的全局 dmClass，以及旧"Want.parameters → export let 全局 → UDP
 * 手工拼一次性快照"的协同写法。现在协同数据（Mission）经 distributedDataObject 在可信设备间自动同步，
 * 跨端拉起仅负责"启动远端控制能力"。
 *
 * 关键修正：跨端拉起统一用 networkId（旧 distributedPage 用 item.deviceId，几乎必拉起失败）。
 *
 * ⚠️ distributedDataObject 与跨端拉起需"两台真机"联调验证；单机阶段先用本服务把逻辑跑通。
 *    同步采用单一 JSON 字段（missionJson）承载 MissionSnapshot，规避嵌套字段同步的边界问题。
 */

import { abilityAccessCtrl, common, Permissions, Want } from '@kit.AbilityKit';
import { distributedDeviceManager } from '@kit.DistributedServiceKit';
import { distributedDataObject } from '@kit.ArkData';
import { BusinessError } from '@kit.BasicServicesKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { Mission, MissionSnapshot } from '../model/mission';

const DOMAIN: number = 0xFF00;
const TAG: string = 'DeviceCollab';
const BUNDLE_NAME: string = 'com.example.carapp';
/** 跨端拉起的目标 Ability（与 module.json5 中一致）。 */
const REMOTE_ABILITY: string = 'RobotRunAbility';
const COLLAB_PERMISSIONS: Permissions[] = ['ohos.permission.DISTRIBUTED_DATASYNC'];

/** distributedDataObject 同步的数据形状：单一 JSON 字段承载 MissionSnapshot。 */
class CollabData {
  missionJson: string = '';
}

export type MissionSyncListener = (snapshot: MissionSnapshot) => void;

export class DeviceCollabService {
  private static instance: DeviceCollabService | null = null;

  static getInstance(): DeviceCollabService {
    const inst: DeviceCollabService = DeviceCollabService.instance ?? new DeviceCollabService();
    DeviceCollabService.instance = inst;
    return inst;
  }

  private context: common.UIAbilityContext | null = null;
  private deviceManager: distributedDeviceManager.DeviceManager | null = null;
  private dataObject: distributedDataObject.DataObject | null = null;
  private sessionId: string = '';
  private listeners: Set<MissionSyncListener> = new Set<MissionSyncListener>();

  private constructor() {
  }

  /** 在 Ability.onCreate 调用：申请权限 + 初始化设备管理与协同对象。 */
  async init(context: common.UIAbilityContext): Promise<void> {
    this.context = context;
    await this.requestPermissions(context);
    this.initDeviceManager();
    this.initDataObject(context);
  }

  private requestPermissions(context: common.UIAbilityContext): Promise<void> {
    return new Promise<void>((resolve: () => void) => {
      const atManager: abilityAccessCtrl.AtManager = abilityAccessCtrl.createAtManager();
      atManager.requestPermissionsFromUser(context, COLLAB_PERMISSIONS)
        .then(() => resolve())
        .catch((err: BusinessError) => {
          hilog.error(DOMAIN, TAG, `request permissions failed: ${err.code} ${err.message}`);
          resolve(); // 不阻塞启动
        });
    });
  }

  private initDeviceManager(): void {
    try {
      this.deviceManager = distributedDeviceManager.createDeviceManager(BUNDLE_NAME);
    } catch (err) {
      hilog.error(DOMAIN, TAG, `createDeviceManager err: ${JSON.stringify(err)}`);
    }
  }

  /** 可信设备列表（移植自旧 getRemoteDeviceInfo）。 */
  listDevices(): distributedDeviceManager.DeviceBasicInfo[] {
    if (this.deviceManager === null) {
      return [];
    }
    try {
      return this.deviceManager.getAvailableDeviceListSync();
    } catch (err) {
      hilog.error(DOMAIN, TAG, `getAvailableDeviceListSync err: ${JSON.stringify(err)}`);
      return [];
    }
  }

  private initDataObject(context: common.UIAbilityContext): void {
    this.dataObject = distributedDataObject.create(context, new CollabData());
    this.dataObject.on('change', (sessionId: string, fields: string[]) => {
      if (fields.includes('missionJson')) {
        this.dispatchRemoteMission();
      }
    });
    this.dataObject.on('status', (sessionId: string, networkId: string, status: string) => {
      hilog.info(DOMAIN, TAG, `peer ${networkId} ${status}`);
    });
  }

  // ── 协同会话 ──────────────────────────────────────────────────────────────
  /** 主机：新建会话并加入，返回 sessionId（传给子机/跨端拉起）。 */
  async newSession(): Promise<string> {
    const id: string = distributedDataObject.genSessionId();
    await this.joinSession(id);
    return id;
  }

  /** 子机：用主机给的 sessionId 加入同一同步网络。 */
  async joinSession(sessionId: string): Promise<void> {
    this.sessionId = sessionId;
    if (this.dataObject !== null) {
      await this.dataObject.setSessionId(sessionId);
    }
  }

  async leaveSession(): Promise<void> {
    this.sessionId = '';
    if (this.dataObject !== null) {
      await this.dataObject.setSessionId();
    }
  }

  getSessionId(): string {
    return this.sessionId;
  }

  // ── Mission 同步 ──────────────────────────────────────────────────────────
  /** 本地 Mission 变化 → 同步到协同设备。 */
  publishMission(mission: Mission): void {
    if (this.dataObject !== null) {
      this.dataObject['missionJson'] = JSON.stringify(mission.toSnapshot());
    }
  }

  subscribeMission(listener: MissionSyncListener): void {
    this.listeners.add(listener);
  }

  unsubscribeMission(listener: MissionSyncListener): void {
    this.listeners.delete(listener);
  }

  private dispatchRemoteMission(): void {
    if (this.dataObject === null) {
      return;
    }
    const raw: Object = this.dataObject['missionJson'] as Object;
    if (typeof raw === 'string' && raw.length > 0) {
      const snapshot: MissionSnapshot = JSON.parse(raw) as MissionSnapshot;
      this.listeners.forEach((l: MissionSyncListener) => l(snapshot));
    }
  }

  // ── 跨端拉起 ──────────────────────────────────────────────────────────────
  /**
   * 拉起远端设备上的控制能力进入协同。
   * @param networkId 目标设备 networkId（来自 listDevices()，不是 deviceId！）
   * @param sessionId 协同会话号；远端据此加入同一 distributedDataObject 网络
   */
  startRemoteControl(networkId: string, sessionId: string): Promise<void> {
    if (this.context === null) {
      return Promise.reject(new Error('DeviceCollabService not initialized'));
    }
    const want: Want = {
      bundleName: BUNDLE_NAME,
      abilityName: REMOTE_ABILITY,
      deviceId: networkId, // 跨端 Want 的 deviceId 字段填 networkId
      parameters: { sessionId }
    };
    return this.context.startAbility(want);
  }
}
