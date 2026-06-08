/**
 * 车队任务服务：分布式软总线发现/认证（distributedDeviceManager）+
 * @ohos.data.distributedDataObject 共享黑板同步（contracts/multi-robot-collab.md 的 FleetMission）。
 *
 * 取代旧 Common/distributed.ets 的全局 dmClass，以及旧"Want.parameters → export let 全局 → UDP
 * 手工拼一次性快照"的协同写法。
 *
 * ★ 核心原则：共享黑板，不是远程拉起。
 *   所有节点（平板 + 每台车的常驻无界面 agent）常驻加入同一 distributedDataObject 会话，
 *   共享一块 Mission(=FleetMission) 黑板；协同 = 反应式读写。**不再用 startAbility 跨端拉起**
 *   （旧 DeviceCollabService.startRemoteControl / RobotRunAbility 已废除，agent 角色移交紫派常驻代理）。
 *
 * ⚠️ distributedDataObject 需"两台真机"联调验证；单机阶段先用本服务把逻辑跑通。
 *    同步采用单一 JSON 字段（missionJson）承载 MissionSnapshot，规避嵌套字段同步的边界问题。
 */

import { abilityAccessCtrl, common, Permissions } from '@kit.AbilityKit';
import { distributedDeviceManager } from '@kit.DistributedServiceKit';
import { distributedDataObject } from '@kit.ArkData';
import { BusinessError } from '@kit.BasicServicesKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { Mission, MissionSnapshot } from '../model/mission';

const DOMAIN: number = 0xFF00;
const TAG: string = 'FleetMission';
const BUNDLE_NAME: string = 'com.example.carapp';
/**
 * 互信弹窗上显示的发起方名称（对端**系统 PIN 弹窗**展示用，非 bundle）。
 * ⚠️ distributedDataObject 跨设备同步要求两端**同 bundleName** → 车载 agent 亦须打包为 `com.example.carapp`
 *    （= BUNDLE_NAME），故 bindTarget 的 targetPkgName 用 BUNDLE_NAME。见 docs/distributed-trust.md / Q9。
 */
const APP_BIND_DISPLAY_NAME: string = '巡检机器人平板';
const COLLAB_PERMISSIONS: Permissions[] = ['ohos.permission.DISTRIBUTED_DATASYNC'];

/** distributedDataObject 同步的数据形状：单一 JSON 字段承载 MissionSnapshot。 */
class CollabData {
  missionJson: string = '';
}

export type MissionSyncListener = (snapshot: MissionSnapshot) => void;

/** distributedDeviceManager 'discoverSuccess' 事件数据（形状以真机 SDK 为准）。 */
interface DiscoverSuccessData {
  device: distributedDeviceManager.DeviceBasicInfo;
}

export class FleetMissionService {
  private static instance: FleetMissionService | null = null;

  static getInstance(): FleetMissionService {
    const inst: FleetMissionService = FleetMissionService.instance ?? new FleetMissionService();
    FleetMissionService.instance = inst;
    return inst;
  }

  private deviceManager: distributedDeviceManager.DeviceManager | null = null;
  private dataObject: distributedDataObject.DataObject | null = null;
  private sessionId: string = '';
  private listeners: Set<MissionSyncListener> = new Set<MissionSyncListener>();

  private constructor() {
  }

  /** 在 Ability.onCreate 调用：申请权限 + 初始化设备管理与协同对象。 */
  async init(context: common.UIAbilityContext): Promise<void> {
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

  // ── 设备互信（账号无关路径②，按官方 distributedDeviceManager；见 docs/distributed-trust.md / Q9）──
  // ⚠️ startDiscovering/bindTarget 的形参与事件名以真机 SDK 为准；未经 DevEco 校验。
  /** 开始发现同网 OH 设备；每发现一台回调其 DeviceBasicInfo。 */
  startDiscovering(onFound: (device: distributedDeviceManager.DeviceBasicInfo) => void): void {
    if (this.deviceManager === null) {
      return;
    }
    try {
      this.deviceManager.on('discoverSuccess', (data: DiscoverSuccessData) => onFound(data.device));
      this.deviceManager.on('discoverFailure', (data: object) =>
        hilog.error(DOMAIN, TAG, `discoverFailure: ${JSON.stringify(data)}`));
      this.deviceManager.startDiscovering({ 'discoverTargetType': 1 });
    } catch (err) {
      hilog.error(DOMAIN, TAG, `startDiscovering err: ${JSON.stringify(err)}`);
    }
  }

  stopDiscovering(): void {
    if (this.deviceManager === null) {
      return;
    }
    try {
      this.deviceManager.off('discoverSuccess');
      this.deviceManager.stopDiscovering();
    } catch (err) {
      hilog.error(DOMAIN, TAG, `stopDiscovering err: ${JSON.stringify(err)}`);
    }
  }

  /**
   * 发起绑定（安全认证 bindType=1，无账号 PIN 码）与目标设备建立**持久**互信（跨重启）。
   * 对端确认：紫派**配网期接 HDMI 显示器**，在**系统 PIN 弹窗**上确认（团队既有流程；老 App 未自渲染 PIN，系统弹窗代劳）。
   * `targetPkgName=BUNDLE_NAME`：DDO 同步需两端同 bundle，车载 agent 亦为 com.example.carapp。见 docs/distributed-trust.md / Q9。
   * ⚠️ 形参与回调以真机 SDK 为准；未经 DevEco 校验。
   */
  bindDevice(deviceId: string, onResult: (ok: boolean, msg: string) => void): void {
    if (this.deviceManager === null) {
      onResult(false, 'no deviceManager');
      return;
    }
    const bindParam: Record<string, Object> = {
      'bindType': 1,                      // 1 = 无账号 PIN 码认证
      'targetPkgName': BUNDLE_NAME,       // 对端目标包（车载 agent 同 bundle）
      'appName': APP_BIND_DISPLAY_NAME,   // 系统弹窗显示的发起方名
      'appOperation': '认证巡检车',
      'customDescription': '与巡检车建立分布式互信（软总线）'
    };
    try {
      this.deviceManager.bindTarget(deviceId, bindParam, (err: BusinessError) => {
        if (err) {
          onResult(false, `${err.code} ${err.message}`);
        } else {
          onResult(true, 'bound');
        }
      });
    } catch (err) {
      const e: BusinessError = err as BusinessError;
      onResult(false, `${e.code} ${e.message}`);
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
  /** 平板（总控）：新建会话并加入，返回 sessionId（带给各车 agent 加入同一同步网络）。 */
  async newSession(): Promise<string> {
    const id: string = distributedDataObject.genSessionId();
    await this.joinSession(id);
    return id;
  }

  /** 车端 agent / 其它端：用同一 sessionId 加入同步网络（常驻入会，不被远程拉起）。 */
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

  // ── FleetMission 黑板同步 ────────────────────────────────────────────────
  /** 本地 Mission(黑板) 变化 → 广播到协同设备。 */
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
}
