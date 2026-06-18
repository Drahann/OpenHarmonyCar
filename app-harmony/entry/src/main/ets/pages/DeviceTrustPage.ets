/**
 * DeviceTrustPage —— 设备互信（软总线组网）面板。平板侧一次性配对入口。
 *
 * 职责：发现同网 OH 设备 → 发起 `bindTarget`（账号无关 PIN 认证）→ 与车建立**持久**互信，
 * 之后 distributedDataObject（FleetMission 黑板）才能跨设备同步、车载 agent 才能入会。
 *
 * 关键事实（见 docs/distributed-trust.md「决策」+ contracts/integration-qa.md Q9）：
 *  - **平板=发起方**（本页 `bindTarget`），**紫派=接受方**：紫派配网期接 **HDMI 显示器 + 鼠标**，
 *    在**系统 PIN 弹窗**上确认（本 App 不自渲染 PIN——与老 App 一致，系统弹窗代劳）。
 *  - 互信是**一次性、持久**（跨重启）；建立后无界面 agent 直接用，本页只在配网时用。
 *  - DDO 同步要求两端**同 bundleName** → 车载 agent 亦为 com.example.carapp（bindTarget 的 targetPkgName）。
 *
 * 取代老 App 的 distributedPage 发现/绑定 UI（按官方 distributedDeviceManager 重写、用统一主题）。
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）；distributedDeviceManager 形参/事件以真机 SDK 为准。
 */

import { router } from '@kit.ArkUI';
import { distributedDeviceManager } from '@kit.DistributedServiceKit';
import { FleetMissionService } from '../service/FleetMissionService';
import { AppColor, FontSize, FontFamily, Space, Radius, Elevation, TOUCH_MIN } from '../constants/theme';
import { Log, Logger } from '../utils/log';

type DeviceInfo = distributedDeviceManager.DeviceBasicInfo;

@Entry
@Component
struct DeviceTrustPage {
  @State private trusted: DeviceInfo[] = [];
  @State private discovered: DeviceInfo[] = [];
  @State private discovering: boolean = false;
  /** 正在绑定的 deviceId（''=无），用于行内 spinner/禁用。 */
  @State private bindingId: string = '';
  /** 底部状态提示（认证中/结果）。 */
  @State private statusMsg: string = '';

  private fleet: FleetMissionService = FleetMissionService.getInstance();
  private log: Logger = Log.scoped('DeviceTrust');
  /** 发现去重（deviceId → info），重建 @State discovered。 */
  private discoveredMap: Map<string, DeviceInfo> = new Map<string, DeviceInfo>();

  aboutToAppear(): void {
    this.refreshTrusted();
    this.startDiscover();
  }

  aboutToDisappear(): void {
    this.stopDiscover();
  }

  build() {
    Column() {
      // 顶栏
      Row() {
        Text('〈')
          .fontSize(FontSize.h2)
          .fontColor(AppColor.primary)
          .onClick(() => router.back())
        Text('设备互信')
          .fontSize(FontSize.h1)
          .fontColor(AppColor.textTitle)
          .fontWeight(FontWeight.Bold)
          .margin({ left: Space.md })
        Blank()
        Button(this.discovering ? '搜索中…' : '重新搜索')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.onPrimary)
          .backgroundColor(AppColor.primary)
          .borderRadius(Radius.pill)
          .height(TOUCH_MIN)
          .padding({ left: Space.md, right: Space.md })
          .enabled(!this.discovering)
          .onClick(() => {
            this.refreshTrusted();
            this.startDiscover();
          })
      }
      .width('100%')
      .padding({ top: Space.lg, bottom: Space.md })

      // 操作说明卡
      this.NoteCard()

      // 已信任设备
      this.SectionTitle('已信任设备', this.trusted.length)
      if (this.trusted.length === 0) {
        this.Hint('尚无已信任设备。在下方发现列表里选中车辆配对。')
      } else {
        List({ space: Space.sm }) {
          ForEach(this.trusted, (item: DeviceInfo) => {
            ListItem() {
              this.DeviceRow(item, true)
            }
          }, (item: DeviceInfo) => item.deviceId)
        }
        .width('100%')
      }

      // 发现的新设备
      this.SectionTitle('发现的新设备', this.discovered.length)
      if (this.discovered.length === 0) {
        this.Hint(this.discovering ? '正在搜索同网络下的 OpenHarmony 设备…' : '未发现新设备。确保平板与车在同一 WiFi/热点。')
      } else {
        List({ space: Space.sm }) {
          ForEach(this.discovered, (item: DeviceInfo) => {
            ListItem() {
              this.DeviceRow(item, false)
            }
          }, (item: DeviceInfo) => item.deviceId)
        }
        .width('100%')
        .layoutWeight(1)
      }

      // 状态提示
      if (this.statusMsg.length > 0) {
        Text(this.statusMsg)
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.textSecondary)
          .width('100%')
          .textAlign(TextAlign.Center)
          .padding({ top: Space.sm, bottom: Space.sm })
      }
    }
    .width('100%')
    .height('100%')
    .padding({ left: Space.lg, right: Space.lg })
    .backgroundColor(AppColor.pageBg)
  }

  @Builder
  NoteCard() {
    Column() {
      Text('配对步骤')
        .fontSize(FontSize.bodySm)
        .fontColor(AppColor.textTitle)
        .fontWeight(FontWeight.Bold)
      Text('① 平板与车连同一 WiFi/热点　② 车上接显示器+鼠标　③ 点车辆「配对」→ 车屏弹出系统 PIN 码 → 按平板系统提示输入该 PIN。配对一次即可，之后自动互信。')
        .fontSize(FontSize.caption)
        .fontColor(AppColor.textSecondary)
        .margin({ top: Space.xs })
    }
    .width('100%')
    .alignItems(HorizontalAlign.Start)
    .padding(Space.md)
    .backgroundColor(AppColor.surface)
    .borderRadius(Radius.md)
    .margin({ bottom: Space.md })
  }

  @Builder
  SectionTitle(title: string, count: number) {
    Row() {
      Text(title)
        .fontSize(FontSize.h3)
        .fontColor(AppColor.textTitle)
        .fontWeight(FontWeight.Bold)
      Text(`（${count}）`)
        .fontSize(FontSize.bodySm)
        .fontColor(AppColor.textCaption)
    }
    .width('100%')
    .padding({ top: Space.sm, bottom: Space.xs })
  }

  @Builder
  Hint(text: string) {
    Text(text)
      .fontSize(FontSize.bodySm)
      .fontColor(AppColor.textSecondary)
      .width('100%')
      .padding({ top: Space.sm, bottom: Space.sm })
  }

  @Builder
  DeviceRow(item: DeviceInfo, isTrusted: boolean) {
    Row() {
      Circle({ width: 10, height: 10 })
        .fill(isTrusted ? AppColor.robotOnline : AppColor.textCaption)
        .margin({ right: Space.sm })
      Column() {
        Text(item.deviceName !== undefined && item.deviceName.length > 0 ? item.deviceName : 'OpenHarmony 设备')
          .fontSize(FontSize.body)
          .fontColor(AppColor.textBody)
        Text(this.shortId(item.deviceId))
          .fontSize(FontSize.dataSm)
          .fontFamily(FontFamily.mono)
          .fontColor(AppColor.textSecondary)
      }
      .alignItems(HorizontalAlign.Start)
      Blank()
      if (isTrusted) {
        Row({ space: Space.sm }) {
          Text('已信任')
            .fontSize(FontSize.bodySm)
            .fontColor(AppColor.onPrimary)
            .backgroundColor(AppColor.primary)
            .borderRadius(Radius.pill)
            .height(36)
            .padding({ left: Space.md, right: Space.md })
          // 解绑：互信坏掉/想重配时重置。解绑后该设备重新出现在「发现的新设备」可再次配对。
          Button('解绑')
            .fontSize(FontSize.bodySm)
            .fontColor(AppColor.textSecondary)
            .backgroundColor(AppColor.surface)
            .border({ width: 1, color: AppColor.textCaption })
            .borderRadius(Radius.pill)
            .height(36)
            .padding({ left: Space.md, right: Space.md })
            .enabled(this.bindingId.length === 0)
            .onClick(() => this.unbind(item))
        }
      } else {
        Row({ space: Space.sm }) {
          Button() {
            Row() {
              if (this.bindingId === item.deviceId) {
                LoadingProgress().width(16).height(16).color(AppColor.primary)
              }
              Text(this.bindingId === item.deviceId ? '认证中' : '配对')
                .fontSize(FontSize.bodySm)
                .fontColor(AppColor.primary)
                .margin({ left: this.bindingId === item.deviceId ? Space.xs : 0 })
            }
          }
          .backgroundColor(AppColor.surface)
          .border({ width: 1, color: AppColor.primary })
          .borderRadius(Radius.pill)
          .height(36)
          .padding({ left: Space.md, right: Space.md })
          .enabled(this.bindingId.length === 0)
          .onClick(() => this.bind(item))
          // 解绑重置：用于"之前绑过但卡住"（不在已信任列表、再配对不弹 PIN）的设备——强制解绑清残留互信，再配对即重新弹 PIN。
          Button('解绑重置')
            .fontSize(FontSize.caption)
            .fontColor(AppColor.textCaption)
            .backgroundColor(AppColor.surface)
            .border({ width: 1, color: AppColor.textCaption })
            .borderRadius(Radius.pill)
            .height(36)
            .padding({ left: Space.sm, right: Space.sm })
            .enabled(this.bindingId.length === 0)
            .onClick(() => this.unbind(item))
        }
      }
    }
    .width('100%')
    .padding({ top: Space.sm, bottom: Space.sm, left: Space.sm, right: Space.sm })
    .borderRadius(Radius.sm)
    .backgroundColor(AppColor.surface)
  }

  // ── 发现 / 信任 ───────────────────────────────────────────────────────────────
  private refreshTrusted(): void {
    this.trusted = this.fleet.listDevices();
    this.log.i(`refreshTrusted → 已信任列表 = ${this.trusted.length} 台`);
  }

  private startDiscover(): void {
    this.stopDiscover();
    this.discoveredMap.clear();
    this.discovered = [];
    this.discovering = true;
    this.fleet.startDiscovering((device: DeviceInfo) => {
      // 去重 + 排除已信任，整体重建 @State（不在回调里 push @State 数组）。
      const trusted: boolean = this.isTrusted(device.deviceId);
      this.log.i(`发现回调: id=${device.deviceId} 已信任=${trusted}${trusted ? '（已信任→不入"新设备"列表）' : ''}`);
      if (device.deviceId.length === 0 || trusted) {
        return;
      }
      this.discoveredMap.set(device.deviceId, device);
      this.discovered = Array.from(this.discoveredMap.values());
    });
    // 发现窗口（与 UI 扫描态同步；底层可持续，UI 仅复位按钮态）。
    setTimeout(() => {
      this.discovering = false;
    }, 5000);
  }

  private stopDiscover(): void {
    this.fleet.stopDiscovering();
    this.discovering = false;
  }

  private bind(item: DeviceInfo): void {
    this.bindingId = item.deviceId;
    this.statusMsg = '认证中：车屏会弹出 PIN 码，请按平板系统提示输入…';
    this.fleet.bindDevice(item.deviceId, (ok: boolean, msg: string) => {
      this.bindingId = '';
      this.log.i(`bind 回调: ok=${ok} msg=${msg} device=${item.deviceId}`);
      if (ok) {
        this.statusMsg = '配对成功，已建立互信。';
        // 从发现列表移到已信任。
        this.discoveredMap.delete(item.deviceId);
        this.discovered = Array.from(this.discoveredMap.values());
        this.refreshTrusted();
      } else {
        this.statusMsg = `配对失败：${msg}`;
      }
    });
  }

  /** 解绑已信任设备：解除互信 → 刷新列表 + 重新发现（该设备会重新出现在「发现的新设备」可再配）。 */
  private unbind(item: DeviceInfo): void {
    this.statusMsg = `解绑 ${this.shortId(item.deviceId)}…`;
    this.fleet.unbindDevice(item.deviceId, (ok: boolean, msg: string) => {
      if (ok) {
        this.statusMsg = '已解绑。点「重新搜索」即可重新发现并配对。';
        this.refreshTrusted();
        this.startDiscover();
      } else {
        this.statusMsg = `解绑失败：${msg}`;
      }
    });
  }

  private isTrusted(deviceId: string): boolean {
    return this.trusted.some((d: DeviceInfo) => d.deviceId === deviceId);
  }

  private shortId(deviceId: string): string {
    if (deviceId.length <= 12) {
      return deviceId;
    }
    return `${deviceId.substring(0, 6)}…${deviceId.substring(deviceId.length - 4)}`;
  }
}
