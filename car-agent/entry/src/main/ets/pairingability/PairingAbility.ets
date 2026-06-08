/**
 * PairingAbility —— 一次性设备互信「配对」UIAbility（仅配网期手动拉起）。
 *
 * 为什么需要它（保险）：分布式互信 `bindTarget` 是**平板=发起方、本车=接受方**。接受方可能要求
 *   **目标包（本 agent，bundleName=com.example.carapp）在前台/活跃**才弹系统 PIN 弹窗。常驻入口
 *   `AgentServiceAbility` 是 headless（无窗口），故另备这个**有界面**的 Ability：配网时车上接 HDMI
 *   显示器+鼠标，手动拉起它 → 本 bundle 活跃 → 平板发起配对 → 本机**系统 PIN 弹窗**确认。
 *
 * PIN 由**系统弹窗**呈现：第三方应用不能自渲染 PIN（`uiStateChange`/`setUserOperation` 是系统 API），
 *   见 docs/distributed-trust.md「官方文档依据」。本页只做"使 bundle 活跃 + 指引 + 列已信任设备"。
 *
 * 互信是一次性、持久（跨重启）；配对完成后本 Ability 可退出，运行时只跑 headless 的 AgentServiceAbility。
 *
 * ⚠️ 未经 DevEco 验证；接受方是否**必须**前台、是否需额外回调，待真机（P5 与成员A）验证。
 */

import { AbilityConstant, UIAbility, Want } from '@kit.AbilityKit';
import { window } from '@kit.ArkUI';
import { BusinessError } from '@kit.BasicServicesKit';
import { FleetMissionService } from '../service/FleetMissionService';
import { Log, Logger } from '../utils/log';

const log: Logger = Log.scoped('PairingAbility');

export default class PairingAbility extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    // 初始化 deviceManager：注册本 bundle 为 DM 客户端 / 可被绑定的目标。
    FleetMissionService.getInstance().init(this.context);
    log.i('pairing ability created');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    windowStage.loadContent('pages/PairingPage', (err: BusinessError) => {
      if (err.code) {
        log.e(`loadContent failed: ${JSON.stringify(err)}`);
      }
    });
  }

  onDestroy(): void {
    log.i('pairing ability destroyed');
  }

  onWindowStageDestroy(): void {
  }

  onForeground(): void {
  }

  onBackground(): void {
  }
}
