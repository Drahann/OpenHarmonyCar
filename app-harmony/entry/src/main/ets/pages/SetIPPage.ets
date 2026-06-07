/**
 * SetIPPage —— 高级/兜底：手动填写车 IP（公共 AP 客户端隔离、固定 IP、调试时用）。
 * 取代旧 utils/SetIP.ets 的中文 key + getter 副作用 —— 走 service/storage（英文 key、纯 get）。
 *
 * 正常路径是 DeviceList 广播发现 + 点击连接（见 §连接与设备发现）；本页是发现不可达时的降级入口。
 *
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）；路由需 U9 注册。
 */

import { router } from '@kit.ArkUI';
import { Storage } from '../service/storage';
import { ControlMode } from '../model/mission';
import { AppColor, FontSize, FontFamily, Space, Radius, TOUCH_MIN } from '../constants/theme';

interface ControlRouteParams {
  mode: string;
  ip: string;
}

@Entry
@Component
struct SetIPPage {
  @State private ip: string = Storage.getRobotIp(0);
  @State private touched: boolean = false;

  build() {
    Column({ space: Space.lg }) {
      // 顶栏
      Row() {
        Button('‹ 返回')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.textBody)
          .backgroundColor(AppColor.surface)
          .borderRadius(Radius.pill)
          .height(40)
          .padding({ left: Space.md, right: Space.md })
          .onClick(() => router.back())
        Blank()
      }
      .width('100%')

      Text('手动连接')
        .fontSize(FontSize.h1)
        .fontColor(AppColor.textTitle)
        .fontWeight(FontWeight.Bold)
        .width('100%')
      Text('发现不可用时（客户端隔离 / 固定 IP / 调试）手填车辆 IP。优先使用首页"发现设备"。')
        .fontSize(FontSize.bodySm)
        .fontColor(AppColor.textSecondary)
        .width('100%')

      // IP 输入
      Column({ space: Space.sm }) {
        Text('车辆 IP')
          .fontSize(FontSize.caption)
          .fontColor(AppColor.textSecondary)
          .width('100%')
        TextInput({ text: this.ip, placeholder: '192.168.x.x' })
          .fontFamily(FontFamily.mono)
          .fontSize(FontSize.dataMd)
          .height(TOUCH_MIN + 8)
          .backgroundColor(AppColor.surface)
          .borderRadius(Radius.sm)
          .border({ width: 1, color: this.borderColor() })
          .onChange((v: string) => {
            this.ip = v;
            this.touched = true;
          })
        if (this.touched && !this.isValid()) {
          Text('IP 格式不正确')
            .fontSize(FontSize.caption)
            .fontColor(AppColor.danger)
            .width('100%')
        }
      }
      .width('100%')

      Blank()

      Button('保存并连接')
        .fontSize(FontSize.body)
        .fontColor(AppColor.onPrimary)
        .backgroundColor(AppColor.primary)
        .borderRadius(Radius.pill)
        .width('100%')
        .height(TOUCH_MIN + 8)
        .enabled(this.isValid())
        .onClick(() => this.saveAndConnect())
    }
    .width('100%')
    .height('100%')
    .padding(Space.lg)
    .backgroundColor(AppColor.pageBg)
  }

  private isValid(): boolean {
    return Storage.isValidIp(this.ip);
  }

  private borderColor(): string {
    if (this.touched && !this.isValid()) {
      return AppColor.danger;
    }
    return AppColor.border;
  }

  private saveAndConnect(): void {
    if (!this.isValid()) {
      return;
    }
    Storage.setRobotIp(0, this.ip);
    const params: ControlRouteParams = { mode: ControlMode.astar as string, ip: this.ip };
    router.replaceUrl({ url: 'pages/ControlPage', params }).catch((e: Error) => {
      console.error(`[SetIPPage] 路由 ControlPage 失败：${e.message}`);
    });
  }
}
