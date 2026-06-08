import { AbilityConstant, ConfigurationConstant, UIAbility, Want } from '@kit.AbilityKit';
import { window } from '@kit.ArkUI';
import { BusinessError } from '@kit.BasicServicesKit';
import { RobotTransport } from '../service/RobotTransport';
import { FleetMissionService } from '../service/FleetMissionService';
import { Storage } from '../service/storage';
import { Log, Logger } from '../utils/log';

const log: Logger = Log.scoped('EntryAbility');

/**
 * 应用主入口。负责初始化服务层（持久化 / UDP 传输 / 设备协同），不再像旧版那样
 * 在各页面散落 socket.bind 与全局变量。
 */
export default class EntryAbility extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    this.context.getApplicationContext().setColorMode(ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET);
    Storage.init();
    RobotTransport.getInstance().bind()
      .catch((e: BusinessError) => log.e(`udp bind failed: ${e.code} ${e.message}`));
    FleetMissionService.getInstance().init(this.context)
      .catch((e: Object) => log.e(`fleet init failed: ${JSON.stringify(e)}`));
  }

  onDestroy(): void {
    log.i('onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    // 指向真实首页 HomePage（替换占位 LoadingPage）。其余页（ControlPage/SetIPPage）经 router 跳转。
    windowStage.loadContent('pages/HomePage', (err: BusinessError) => {
      if (err.code) {
        log.e(`loadContent failed: ${JSON.stringify(err)}`);
        return;
      }
      log.i('loadContent succeeded');
    });
  }

  onWindowStageDestroy(): void {
    log.i('onWindowStageDestroy');
  }

  onForeground(): void {
  }

  onBackground(): void {
  }
}
