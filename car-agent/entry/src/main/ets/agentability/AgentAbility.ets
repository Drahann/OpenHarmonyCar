/**
 * AgentAbility —— 车载 agent 的**运行时入口（UIAbility 外壳）**，托管 AgentCore（黑板↔本机 UDP 桥）。
 *
 * 为什么是 UIAbility 而非 headless ServiceExtensionAbility：后者是**系统 API**，第三方公共 SDK 编不过
 *   （见 docs/car-agent-plan.md §6.5 / contracts/integration-qa.md Q6.1）。本壳用公共 SDK 即可编译/装机/联调；
 *   逻辑全在 `AgentCore`，将来上紫派系统应用时换 ServiceExtension 壳托管同一 core 即可。
 *
 * 窗口只挂一张极简状态页 AgentStatusPage（过渡形态：非纯无界面）；配网认证仍走独立的 PairingAbility。
 * ⚠️ 未经真机验证；开机自起/常驻仍需紫派系统侧配置（integration-qa Q6.1）。
 */

import { AbilityConstant, UIAbility, Want } from '@kit.AbilityKit';
import { window } from '@kit.ArkUI';
import { BusinessError } from '@kit.BasicServicesKit';
import { AgentCore } from '../agent/AgentCore';
import { Log, Logger } from '../utils/log';

const log: Logger = Log.scoped('AgentAbility');

export default class AgentAbility extends UIAbility {
  private readonly core: AgentCore = new AgentCore();

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    const params: Record<string, Object> | undefined = want.parameters;
    if (params !== undefined) {
      const cid: Object = params['carId'];
      if (typeof cid === 'number') {
        this.core.setCarId(cid as number);
      }
    }
    this.core.start(this.context);   // UIAbilityContext 即 common.Context，可供 distributedDataObject 使用
    log.i('agent ability created');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    windowStage.loadContent('pages/AgentStatusPage', (err: BusinessError) => {
      if (err.code) {
        log.e(`loadContent failed: ${JSON.stringify(err)}`);
      }
    });
  }

  onDestroy(): void {
    this.core.stop();
    log.i('agent ability destroyed');
  }

  onWindowStageDestroy(): void {
  }

  onForeground(): void {
  }

  onBackground(): void {
  }
}
