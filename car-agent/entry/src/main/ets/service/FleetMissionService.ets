/**
 * 车队任务服务（**car-agent 无界面适配版**）—— 与 app-harmony 的同名服务**同源**，差异：
 *   1. 入参 context 用基类 `common.Context`（ServiceExtensionAbility 的上下文，非 UIAbilityContext）。
 *   2. **不做交互式权限申请**（无界面服务弹不出对话框）——`ohos.permission.DISTRIBUTED_DATASYNC`
 *      须在紫派侧预授权（系统应用 / 预置授权，见 contracts/integration-qa.md Q6.3）。
 * 其余（distributedDataObject 单一 missionJson 黑板同步、会话加入、发布/订阅）与 app-harmony 一致。
 * ⚠️ 复制自 app-harmony，shared-core HAR 后去重；改一处改两处。未经 DevEco 验证。
 */

import { abilityAccessCtrl, common, Permissions, PermissionRequestResult } from '@kit.AbilityKit';
import { distributedDeviceManager } from '@kit.DistributedServiceKit';
import { distributedDataObject } from '@kit.ArkData';
import { BusinessError } from '@kit.BasicServicesKit';
import { Mission, MissionSnapshot } from '../model/mission';
import { Log, Logger } from '../utils/log';

const log: Logger = Log.scoped('FleetMission(agent)');
const BUNDLE_NAME: string = 'com.example.carapp';
const COLLAB_PERMISSIONS: Permissions[] = ['ohos.permission.DISTRIBUTED_DATASYNC'];

/** distributedDataObject 同步的数据形状：单一 JSON 字段承载 MissionSnapshot。 */
class CollabData {
  missionJson: string = '';
}

export type MissionSyncListener = (snapshot: MissionSnapshot) => void;

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

  /**
   * 宿主 onCreate 调用：**申请 DATASYNC 权限** + 初始化设备管理与协同对象。
   * ⚠️ 当前 UIAbility 壳(AgentAbility/PairingAbility)**有窗口、可弹权限框**；将来换 headless ServiceExtension
   *    弹不出框时，DATASYNC 须紫派侧预授权（系统应用/预置）。真机若 `authResults=[-1]`，到紫派设置手动授予。
   */
  async init(context: common.Context): Promise<void> {
    await this.requestPermissions(context);
    this.initDeviceManager();
    this.initDataObject(context);
  }

  /** 申请 DISTRIBUTED_DATASYNC，并打印授权结果（0=授权/-1=拒绝；未授 → DDO 协同失败 29360174 / permission:-1）。 */
  private requestPermissions(context: common.Context): Promise<void> {
    return new Promise<void>((resolve: () => void) => {
      try {
        const atManager: abilityAccessCtrl.AtManager = abilityAccessCtrl.createAtManager();
        atManager.requestPermissionsFromUser(context as common.UIAbilityContext, COLLAB_PERMISSIONS)
          .then((res: PermissionRequestResult) => {
            const allGranted: boolean = res.authResults.length > 0 &&
              res.authResults.every((r: number): boolean => r === 0);
            log.i(`权限请求结果: [${res.permissions.join(', ')}] authResults=[${res.authResults.join(', ')}] → ` +
              (allGranted ? '✅ 全部已授权' : '❌ 有未授权！DDO 协同会失败——请到紫派 设置→应用→com.example.carapp→权限 授予 DISTRIBUTED_DATASYNC'));
            resolve();
          })
          .catch((err: BusinessError) => {
            log.e(`requestPermissionsFromUser 失败(上下文不支持弹框?): ${err.code} ${err.message}；DATASYNC 须紫派设置手动授予`);
            resolve();
          });
      } catch (err) {
        log.e(`requestPermissions 异常: ${JSON.stringify(err)}`);
        resolve();
      }
    });
  }

  private initDeviceManager(): void {
    try {
      this.deviceManager = distributedDeviceManager.createDeviceManager(BUNDLE_NAME);
    } catch (err) {
      log.e(`createDeviceManager err: ${JSON.stringify(err)}`);
    }
  }

  listDevices(): distributedDeviceManager.DeviceBasicInfo[] {
    if (this.deviceManager === null) {
      return [];
    }
    try {
      return this.deviceManager.getAvailableDeviceListSync();
    } catch (err) {
      log.e(`getAvailableDeviceListSync err: ${JSON.stringify(err)}`);
      return [];
    }
  }

  private initDataObject(context: common.Context): void {
    this.dataObject = distributedDataObject.create(context, new CollabData());
    this.dataObject.on('change', (sessionId: string, fields: string[]) => {
      if (fields.includes('missionJson')) {
        this.dispatchRemoteMission();
      }
    });
    this.dataObject.on('status', (sessionId: string, networkId: string, status: string) => {
      log.i(`peer ${networkId} ${status}`);
    });
  }

  // ── 协同会话 ──────────────────────────────────────────────────────────────
  /** 车端 agent 用约定 sessionId 加入同步网络（常驻入会，不被远程拉起）。 */
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
