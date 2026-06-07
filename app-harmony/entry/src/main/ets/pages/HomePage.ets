/**
 * HomePage —— 首页：机器人列表 + 模式选择，取代旧 god-page Index。
 *
 * 修旧 bug：旧 `Index.onPageShow` 每次显示都 `arr.push` 不清空 → 机器人列表累积重复。
 *   这里列表始终由 @State `devices` **整体重建**（RobotTransport.discover 已按 IP 去重并每次回传全量），
 *   绝不在生命周期里向数组 push 追加。
 *
 * 交互：选模式（astar/fullpath/distributed chip）→ 点某台车卡片 → 路由 ControlPage(mode, ip)。
 *   发现走方案B 广播（service/RobotTransport.discover）。
 *
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）；ControlPage 路由需 U9 在 main_pages.json5 注册后可达。
 */

import { router } from '@kit.ArkUI';
import { ControlMode } from '../model/mission';
import { RobotTransport, DiscoveredRobot } from '../service/RobotTransport';
import { AppColor, FontSize, FontFamily, Space, Radius, Elevation, TOUCH_MIN } from '../constants/theme';

/** 传给 ControlPage 的路由参数。 */
interface ControlRouteParams {
  mode: string;
  ip: string;
}

/** 模式 chip 数据。 */
interface ModeOption {
  mode: ControlMode;
  label: string;
}

@Entry
@Component
struct HomePage {
  @State private selectedMode: ControlMode = ControlMode.astar;
  @State private devices: DiscoveredRobot[] = [];
  @State private scanning: boolean = false;
  private stopDiscover: (() => void) | null = null;
  private readonly modes: ModeOption[] = [
    { mode: ControlMode.astar, label: '单机导航' },
    { mode: ControlMode.fullpath, label: '全路径覆盖' },
    { mode: ControlMode.distributed, label: '多机协同' }
  ];

  aboutToAppear(): void {
    this.startScan();
  }

  aboutToDisappear(): void {
    this.endScan();
  }

  build() {
    Column() {
      // 顶栏
      Row() {
        Text('巡检控制')
          .fontSize(FontSize.h1)
          .fontColor(AppColor.textTitle)
          .fontWeight(FontWeight.Bold)
        Blank()
        Button(this.scanning ? '搜索中…' : '发现设备')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.onPrimary)
          .backgroundColor(AppColor.primary)
          .borderRadius(Radius.pill)
          .height(TOUCH_MIN)
          .padding({ left: Space.md, right: Space.md })
          .enabled(!this.scanning)
          .onClick(() => this.startScan())
      }
      .width('100%')
      .padding({ top: Space.lg, bottom: Space.md })

      // 模式选择
      Row() {
        ForEach(this.modes, (m: ModeOption) => {
          this.ModeChip(m)
        }, (m: ModeOption) => m.mode as string)
      }
      .width('100%')
      .justifyContent(FlexAlign.SpaceBetween)
      .padding({ bottom: Space.md })

      // 机器人卡片列表 / 空态
      if (this.devices.length === 0) {
        Column() {
          Text(this.scanning ? '正在搜索同网络下的车辆…' : '未发现车辆。确保平板与车在同一热点/局域网，再点"发现设备"。')
            .fontSize(FontSize.body)
            .fontColor(AppColor.textSecondary)
            .textAlign(TextAlign.Center)
        }
        .width('100%')
        .layoutWeight(1)
        .justifyContent(FlexAlign.Center)
        .padding(Space.xl)
      } else {
        List({ space: Space.md }) {
          ForEach(this.devices, (item: DiscoveredRobot) => {
            ListItem() {
              this.RobotCard(item)
            }
          }, (item: DiscoveredRobot) => item.ip)
        }
        .width('100%')
        .layoutWeight(1)
      }
    }
    .width('100%')
    .height('100%')
    .padding({ left: Space.lg, right: Space.lg })
    .backgroundColor(AppColor.pageBg)
  }

  @Builder
  ModeChip(m: ModeOption) {
    Text(m.label)
      .fontSize(FontSize.bodySm)
      .fontColor(this.selectedMode === m.mode ? AppColor.onPrimary : AppColor.primary)
      .backgroundColor(this.selectedMode === m.mode ? AppColor.primary : AppColor.surface)
      .border(this.selectedMode === m.mode ? undefined : { width: 1, color: AppColor.primary })
      .borderRadius(Radius.pill)
      .height(TOUCH_MIN)
      .layoutWeight(1)
      .textAlign(TextAlign.Center)
      .margin({ right: m.mode === ControlMode.distributed ? 0 : Space.sm })
      .onClick(() => {
        this.selectedMode = m.mode;
      })
  }

  @Builder
  RobotCard(item: DiscoveredRobot) {
    Row() {
      Circle({ width: 12, height: 12 })
        .fill(AppColor.robotOnline)
        .margin({ right: Space.md })
      Column() {
        Text('巡检车')
          .fontSize(FontSize.h3)
          .fontColor(AppColor.textBody)
        Text(item.ip)
          .fontSize(FontSize.dataSm)
          .fontFamily(FontFamily.mono)
          .fontColor(AppColor.textSecondary)
      }
      .alignItems(HorizontalAlign.Start)
      Blank()
      Text('〉')
        .fontSize(FontSize.h2)
        .fontColor(AppColor.textCaption)
    }
    .width('100%')
    .padding(Space.md)
    .backgroundColor(AppColor.surface)
    .borderRadius(Radius.md)
    .shadow({
      radius: Elevation.cardRadius,
      color: Elevation.cardColor,
      offsetX: 0,
      offsetY: Elevation.cardOffsetY
    })
    .onClick(() => this.enterControl(item.ip))
  }

  // ── 发现 ────────────────────────────────────────────────────────────────────
  private startScan(): void {
    this.endScan();
    this.scanning = true;
    const transport: RobotTransport = RobotTransport.getInstance();
    this.stopDiscover = transport.discover((robots: DiscoveredRobot[]) => {
      // 整体替换（去重已在 discover 内完成），绝不 push 追加 —— 修旧累积 bug。
      this.devices = robots;
    });
    // 发现窗口结束后复位扫描态（discover 内部 durationMs 到点自停，这里同步 UI）。
    setTimeout(() => {
      this.scanning = false;
    }, 3200);
  }

  private endScan(): void {
    if (this.stopDiscover !== null) {
      this.stopDiscover();
      this.stopDiscover = null;
    }
    this.scanning = false;
  }

  // ── 路由 ────────────────────────────────────────────────────────────────────
  private enterControl(ip: string): void {
    this.endScan();
    const params: ControlRouteParams = { mode: this.selectedMode as string, ip };
    router.pushUrl({ url: 'pages/ControlPage', params }).catch((e: Error) => {
      console.error(`[HomePage] 路由 ControlPage 失败：${e.message}`);
    });
  }
}
