/**
 * DeviceList —— 设备发现列表（方案B：局域网广播发现 + 点击连接），取代旧 3 处克隆的设备列表。
 *
 * 纯展示 + 交互：发现/连接的"动作"由上层注入（onScan 触发 RobotTransport.discover、
 * onConnect 触发 connect+startHeartbeat、onManualAdd 进 SetIPPage 手填兜底）。
 * 本组件只渲染传入的 devices（DiscoveredRobot[]）+ 连接态，不直接持有 socket。
 *
 * 见 docs/app-refactor-plan.md §连接与设备发现、contracts/integration-qa.md Q5。
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）。
 */

import { DiscoveredRobot } from '../service/RobotTransport';
import { AppColor, FontSize, FontFamily, Space, Radius, TOUCH_MIN } from '../constants/theme';

@Component
export struct DeviceList {
  /** 已发现的车（按源 IP 去重）。 */
  @Prop devices: DiscoveredRobot[] = [];
  /** 当前已连接（保活中）的 IP 列表，用于行内"已连接"态。 */
  @Prop connectedIps: string[] = [];
  /** 是否正在扫描（控制按钮态/spinner/空态文案）。 */
  @Prop scanning: boolean = false;
  /** 触发一次扫描发现。 */
  onScan?: () => void;
  /** 点击连接某台车。 */
  onConnect?: (ip: string) => void;
  /** 手动添加 IP 兜底入口。 */
  onManualAdd?: () => void;

  build() {
    Column() {
      // 顶栏：标题 + 扫描按钮
      Row() {
        Text('设备发现')
          .fontSize(FontSize.h3)
          .fontColor(AppColor.textTitle)
          .fontWeight(FontWeight.Bold)
        Blank()
        Button() {
          Row() {
            if (this.scanning) {
              LoadingProgress().width(18).height(18).color(AppColor.onPrimary)
            }
            Text(this.scanning ? '扫描中' : '扫描设备')
              .fontSize(FontSize.bodySm)
              .fontColor(AppColor.onPrimary)
              .margin({ left: this.scanning ? Space.xs : 0 })
          }
        }
        .backgroundColor(AppColor.primary)
        .borderRadius(Radius.pill)
        .height(TOUCH_MIN)
        .padding({ left: Space.md, right: Space.md })
        .enabled(!this.scanning)
        .onClick(() => {
          if (this.onScan !== undefined) {
            this.onScan();
          }
        })
      }
      .width('100%')
      .padding({ bottom: Space.sm })

      // 列表 / 空态
      if (this.devices.length === 0) {
        this.EmptyState()
      } else {
        List({ space: Space.sm }) {
          ForEach(this.devices, (item: DiscoveredRobot) => {
            ListItem() {
              this.DeviceRow(item)
            }
          }, (item: DiscoveredRobot) => item.ip)
        }
        .width('100%')
        .layoutWeight(1)
      }

      // 底部：手动添加 IP 兜底
      Row() {
        Text('+ 手动添加 IP')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.info)
      }
      .width('100%')
      .justifyContent(FlexAlign.Center)
      .padding({ top: Space.sm, bottom: Space.sm })
      .onClick(() => {
        if (this.onManualAdd !== undefined) {
          this.onManualAdd();
        }
      })
    }
    .width('100%')
    .padding(Space.md)
    .backgroundColor(AppColor.surface)
    .borderRadius(Radius.md)
  }

  @Builder
  EmptyState() {
    Column() {
      Text(this.scanning ? '正在搜索同网络下的车辆…' : '同一热点 / 局域网下点"扫描设备"自动发现车辆')
        .fontSize(FontSize.bodySm)
        .fontColor(AppColor.textSecondary)
        .textAlign(TextAlign.Center)
    }
    .width('100%')
    .padding({ top: Space.xl, bottom: Space.xl })
    .layoutWeight(1)
    .justifyContent(FlexAlign.Center)
  }

  @Builder
  DeviceRow(item: DiscoveredRobot) {
    Row() {
      // 在线点
      Circle({ width: 10, height: 10 })
        .fill(AppColor.robotOnline)
        .margin({ right: Space.sm })
      // 名称 + IP
      Column() {
        Text('巡检车')
          .fontSize(FontSize.body)
          .fontColor(AppColor.textBody)
        Text(item.ip)
          .fontSize(FontSize.dataSm)
          .fontFamily(FontFamily.mono)
          .fontColor(AppColor.textSecondary)
      }
      .alignItems(HorizontalAlign.Start)
      Blank()
      // 连接按钮
      Button(this.isConnected(item.ip) ? '已连接' : '连接')
        .fontSize(FontSize.bodySm)
        .fontColor(this.isConnected(item.ip) ? AppColor.onPrimary : AppColor.primary)
        .backgroundColor(this.isConnected(item.ip) ? AppColor.primary : AppColor.surface)
        .borderRadius(Radius.pill)
        .border(this.isConnected(item.ip) ? undefined : { width: 1, color: AppColor.primary })
        .height(36)
        .padding({ left: Space.md, right: Space.md })
        .enabled(!this.isConnected(item.ip))
        .onClick(() => {
          if (this.onConnect !== undefined) {
            this.onConnect(item.ip);
          }
        })
    }
    .width('100%')
    .padding({ top: Space.sm, bottom: Space.sm, left: Space.sm, right: Space.sm })
    .borderRadius(Radius.sm)
    .backgroundColor(AppColor.pageBg)
  }

  private isConnected(ip: string): boolean {
    return this.connectedIps.includes(ip);
  }
}
