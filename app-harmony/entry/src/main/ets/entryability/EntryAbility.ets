import { AbilityConstant, ConfigurationConstant, UIAbility, Want } from '@kit.AbilityKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { window } from '@kit.ArkUI';
import { BusinessError } from '@kit.BasicServicesKit';
import { RobotTransport } from '../service/RobotTransport';
import { FleetMissionService } from '../service/FleetMissionService';
import { Storage } from '../service/storage';

const DOMAIN: number = 0x0000;
const TAG: string = 'EntryAbility';

/**
 * 应用主入口。负责初始化服务层（持久化 / UDP 传输 / 设备协同），不再像旧版那样
 * 在各页面散落 socket.bind 与全局变量。
 */
export default class EntryAbility extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    this.context.getApplicationContext().setColorMode(ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET);
    Storage.init();
    RobotTransport.getInstance().bind()
      .catch((e: BusinessError) => hilog.error(DOMAIN, TAG, `udp bind failed: ${e.code} ${e.message}`));
    FleetMissionService.getInstance().init(this.context)
      .catch((e: Object) => hilog.error(DOMAIN, TAG, `fleet init failed: ${JSON.stringify(e)}`));
  }

  onDestroy(): void {
    hilog.info(DOMAIN, TAG, 'onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    // 指向真实首页 HomePage（替换占位 LoadingPage）。其余页（ControlPage/SetIPPage）经 router 跳转。
    windowStage.loadContent('pages/HomePage', (err: BusinessError) => {
      if (err.code) {
        hilog.error(DOMAIN, TAG, `loadContent failed: ${JSON.stringify(err)}`);
        return;
      }
      hilog.info(DOMAIN, TAG, 'loadContent succeeded');
    });
  }

  onWindowStageDestroy(): void {
    hilog.info(DOMAIN, TAG, 'onWindowStageDestroy');
  }

  onForeground(): void {
  }

  onBackground(): void {
  }
}
