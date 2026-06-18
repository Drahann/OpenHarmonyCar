/**
 * PairingPage —— PairingAbility 的界面：配网指引 + 已信任设备列表。
 *
 * 作用纯粹是"让 agent 的 bundle 在配网期处于前台、给操作者指引"。**不渲染 PIN**——PIN 由系统弹窗
 *   呈现（第三方应用拿不到 PIN 值，见 docs/distributed-trust.md「官方文档依据」）。
 *
 * 不依赖 app-harmony 的 theme.ets（car-agent 未含），用品牌色字面量（墨绿 #1A3C34 / 页底 #EDF4F1）。
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）。
 */

import { distributedDeviceManager } from '@kit.DistributedServiceKit';
import { FleetMissionService } from '../service/FleetMissionService';

type DeviceInfo = distributedDeviceManager.DeviceBasicInfo;

@Entry
@Component
struct PairingPage {
  @State private trusted: DeviceInfo[] = [];
  private fleet: FleetMissionService = FleetMissionService.getInstance();

  aboutToAppear(): void {
    this.refresh();
  }

  build() {
    Column({ space: 16 }) {
      Text('配对模式')
        .fontSize(28)
        .fontWeight(FontWeight.Bold)
        .fontColor('#1A3C34')

      Text('① 平板与本车连同一 WiFi / 热点\n② 在平板「设备互信」里发起配对\n③ 本机弹出系统 PIN 码时，用鼠标按提示确认\n配对一次即可，之后 agent 自动入会同步，无需再开本界面。')
        .fontSize(15)
        .fontColor('#5A6B66')
        .lineHeight(24)

      Divider().color('#D5E2DC')

      Row() {
        Text(`已信任设备（${this.trusted.length}）`)
          .fontSize(18)
          .fontColor('#1A3C34')
          .fontWeight(FontWeight.Medium)
        Blank()
        Button('刷新')
          .fontSize(14)
          .fontColor('#1A3C34')
          .backgroundColor('#FFFFFF')
          .border({ width: 1, color: '#1A3C34' })
          .borderRadius(20)
          .height(36)
          .padding({ left: 16, right: 16 })
          .onClick(() => this.refresh())
      }
      .width('100%')

      if (this.trusted.length === 0) {
        Text('尚无已信任设备。请在平板上发起配对。')
          .fontSize(14)
          .fontColor('#5A6B66')
          .width('100%')
          .padding({ top: 12, bottom: 12 })
      } else {
        List({ space: 8 }) {
          ForEach(this.trusted, (d: DeviceInfo) => {
            ListItem() {
              Row() {
                Text(d.deviceName !== undefined && d.deviceName.length > 0 ? d.deviceName : '已配对设备')
                  .fontSize(16)
                  .fontColor('#1A3C34')
                Blank()
                Text('已信任')
                  .fontSize(14)
                  .fontColor('#2E7D5B')
              }
              .width('100%')
              .padding(12)
              .backgroundColor('#F2F6F4')
              .borderRadius(10)
            }
          }, (d: DeviceInfo) => d.deviceId)
        }
        .layoutWeight(1)
        .width('100%')
      }
    }
    .width('100%')
    .height('100%')
    .padding(24)
    .backgroundColor('#EDF4F1')
    .alignItems(HorizontalAlign.Start)
  }

  private refresh(): void {
    this.trusted = this.fleet.listDevices();
  }
}
