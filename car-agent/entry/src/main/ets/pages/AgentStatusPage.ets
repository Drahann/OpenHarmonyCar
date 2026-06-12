/**
 * AgentStatusPage —— 车载 agent 运行时的极简状态页 + **配对入口**。
 *
 * 仅因当前用 UIAbility 外壳托管 AgentCore（headless ServiceExtension 系统 API 第三方编不过的过渡形态），
 * UIAbility 需要一张窗口内容页。日常运行不依赖它；真正的桥接逻辑在后台的 AgentCore。
 *
 * 配网认证：`bindTarget` 接受方可能要求**目标包(com.example.carapp)前台/活跃**才弹系统 PIN、才完成互信。
 * 故这里给一个「打开配对界面」按钮，手动拉起 `PairingAbility`（前台），再去平板发起配对。
 */

import { common, Want } from '@kit.AbilityKit';
import { Log, Logger } from '../utils/log';

const log: Logger = Log.scoped('AgentStatusPage');
const PAIRING_WANT: Want = {
  bundleName: 'com.example.carapp',
  moduleName: 'entry',
  abilityName: 'PairingAbility'
};

@Entry
@Component
struct AgentStatusPage {
  @State private hint: string = '';

  build() {
    Column({ space: 12 }) {
      Text('车载 Agent 运行中')
        .fontSize(20)
        .fontWeight(FontWeight.Bold)
      Text('FleetMission 黑板 ↔ 本机 UDP 桥（agent 绑 :5002 → udp2lcm 127.0.0.1:5001）')
        .fontSize(13)
        .fontColor('#666666')
        .textAlign(TextAlign.Center)
      Button('打开配对界面（配网认证用）')
        .fontSize(15)
        .height(44)
        .margin({ top: 8 })
        .onClick(() => this.openPairing())
      Text('配网：先点上面让本应用前台活跃 → 去平板「设备互信」发起配对 → 本机系统弹 PIN 用鼠标确认。配一次即可。')
        .fontSize(12)
        .fontColor('#999999')
        .textAlign(TextAlign.Center)
      if (this.hint.length > 0) {
        Text(this.hint)
          .fontSize(12)
          .fontColor('#C0392B')
          .textAlign(TextAlign.Center)
      }
    }
    .width('100%')
    .height('100%')
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .padding(24)
  }

  /** 拉起 PairingAbility（前台），使 com.example.carapp 活跃以接受 bindTarget + 系统 PIN。 */
  private openPairing(): void {
    const ctx: common.UIAbilityContext = getContext(this) as common.UIAbilityContext;
    ctx.startAbility(PAIRING_WANT).then(() => {
      this.hint = '';
      log.i('已拉起 PairingAbility（前台），现在去平板发起配对');
    }).catch((e: Object) => {
      this.hint = `打开配对界面失败：${JSON.stringify(e)}`;
      log.e(this.hint);
    });
  }
}
