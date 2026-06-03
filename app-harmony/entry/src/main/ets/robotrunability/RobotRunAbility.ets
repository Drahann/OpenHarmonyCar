import { AbilityConstant, UIAbility, Want } from '@kit.AbilityKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { window } from '@kit.ArkUI';
import { BusinessError } from '@kit.BasicServicesKit';
import { RobotTransport } from '../service/RobotTransport';
import { DeviceCollabService } from '../service/DeviceCollabService';
import { Storage } from '../service/storage';

const DOMAIN: number = 0x0000;
const TAG: string = 'RobotRunAbility';

/**
 * 跨端拉起目标：另一台设备经分布式软总线启动本 Ability 进入协同控制。
 *
 * 相比旧 robotRunAbility.ets 的根本变化：不再用 `export let EndX/EndY/ip/StartX/StartY/indexEachCar`
 * 这些模块级全局变量从 Want.parameters 里手工搬运一次性快照；现在只从 Want 取 sessionId，
 * 协同状态经 distributedDataObject 自动同步（见 DeviceCollabService）。
 */
export default class RobotRunAbility extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    Storage.init();
    RobotTransport.getInstance().bind()
      .catch((e: BusinessError) => hilog.error(DOMAIN, TAG, `udp bind failed: ${e.code} ${e.message}`));

    let sessionId: string = '';
    const params: Record<string, Object> | undefined = want.parameters;
    if (params !== undefined && typeof params['sessionId'] === 'string') {
      sessionId = params['sessionId'] as string;
    }

    const collab: DeviceCollabService = DeviceCollabService.getInstance();
    collab.init(this.context).then(() => {
      if (sessionId.length > 0) {
        collab.joinSession(sessionId);
      }
    }).catch((e: Object) => hilog.error(DOMAIN, TAG, `collab init failed: ${JSON.stringify(e)}`));
  }

  onDestroy(): void {
    hilog.info(DOMAIN, TAG, 'onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    // UI 阶段：改为加载 ControlPage(mode=distributed)。功能内核阶段先占位。
    windowStage.loadContent('pages/LoadingPage', (err: BusinessError) => {
      if (err.code) {
        hilog.error(DOMAIN, TAG, `loadContent failed: ${JSON.stringify(err)}`);
      }
    });
  }

  onWindowStageDestroy(): void {
  }

  onForeground(): void {
  }

  onBackground(): void {
  }
}
