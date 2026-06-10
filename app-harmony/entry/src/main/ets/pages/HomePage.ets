/**
 * HomePage —— 首页：机器人列表 + 模式选择，取代旧 god-page Index。
 *
 * 修旧 bug：旧 `Index.onPageShow` 每次显示都 `arr.push` 不清空 → 机器人列表累积重复。
 *   这里列表始终由发现结果**整体重建**（RobotTransport.discover 已按 IP 去重并每次回传全量），绝不在生命周期里 push 追加。
 *
 * 连接状态（用户反馈 2026-06-08）：车列表**显式展示每台车的连接/心跳态**——
 *   ⚪已发现(灰·未连接) → 🟡连接中(黄·等待心跳) → 🟢已连接(绿·心跳正常·新鲜度) → 🔴连接丢失(红)。
 *   状态由 `RobotTransport.connStateOf` 派生（保活链路 + 心跳新鲜度），每 600ms 刷新。
 *   交互：点「连接」起**中性保活**(pending+stop，不建图/不运动) → 收到心跳即"已连接" → 点「进入控制」进 ControlPage。
 *   **单连接**：连新车自动断旧车（`RobotTransport.connectExclusive`），避免多车被半连接后各自 3s 急停。
 *
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）。
 */

import { router } from '@kit.ArkUI';
import { ControlMode } from '../model/mission';
import { RobotTransport, DiscoveredRobot, ConnState } from '../service/RobotTransport';
import { RobotCommand, MoveDirection, UdpSendData } from '../model/protocol';
import { AppColor, FontSize, FontFamily, Space, Radius, Elevation, TOUCH_MIN } from '../constants/theme';
import { Log, Logger } from '../utils/log';

const log: Logger = Log.scoped('HomePage');
/** 连接状态 UI 刷新间隔（心跳新鲜度/丢失为时间派生，需定时重算）。 */
const CARD_REFRESH_MS: number = 600;

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

/** 车卡片视图模型（由 DiscoveredRobot + RobotTransport 连接状态派生）。 */
interface CarCardVM {
  ip: string;
  state: ConnState;
  detail: string;
}

@Entry
@Component
struct HomePage {
  @State private selectedMode: ControlMode = ControlMode.astar;
  @State private devices: DiscoveredRobot[] = [];
  @State private cards: CarCardVM[] = [];
  @State private scanning: boolean = false;
  private stopDiscover: (() => void) | null = null;
  private refreshTimer: number = -1;
  private transport: RobotTransport = RobotTransport.getInstance();
  private readonly modes: ModeOption[] = [
    { mode: ControlMode.astar, label: '单机导航' },
    { mode: ControlMode.fullpath, label: '全路径覆盖' },
    { mode: ControlMode.distributed, label: '多机协同' }
  ];

  aboutToAppear(): void {
    this.startScan();
    // 周期刷新连接状态（心跳新鲜度/丢失是时间派生，需定时重算）。
    this.refreshTimer = setInterval(() => this.refreshCards(), CARD_REFRESH_MS);
  }

  aboutToDisappear(): void {
    this.endScan();
    if (this.refreshTimer !== -1) {
      clearInterval(this.refreshTimer);
      this.refreshTimer = -1;
    }
    // ⚠️ 不 stopHeartbeat：已连接的车保活继续，供 ControlPage 接管，避免导航间隙 3s 急停。
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
        // 仪表识别视频入口（形态A：全屏视觉页）——独立 WS 连香橙派，与导航链路解耦，见 docs/vision-integration-plan.md
        Button('识别视频')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.primary)
          .backgroundColor(AppColor.surface)
          .border({ width: 1, color: AppColor.primary })
          .borderRadius(Radius.pill)
          .height(TOUCH_MIN)
          .padding({ left: Space.md, right: Space.md })
          .margin({ right: Space.sm })
          .onClick(() => {
            router.pushUrl({ url: 'pages/VisionPage' }).catch((e: Error) => {
              log.e(`路由 VisionPage 失败：${e.message}`);
            });
          })
        // 设备互信（软总线配对）入口——多机协同前一次性配对，见 DeviceTrustPage / docs/distributed-trust.md
        Button('设备互信')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.primary)
          .backgroundColor(AppColor.surface)
          .border({ width: 1, color: AppColor.primary })
          .borderRadius(Radius.pill)
          .height(TOUCH_MIN)
          .padding({ left: Space.md, right: Space.md })
          .margin({ right: Space.sm })
          .onClick(() => {
            router.pushUrl({ url: 'pages/DeviceTrustPage' }).catch((e: Error) => {
              log.e(`路由 DeviceTrustPage 失败：${e.message}`);
            });
          })
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
      if (this.cards.length === 0) {
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
          ForEach(this.cards, (vm: CarCardVM) => {
            ListItem() {
              this.RobotCard(vm)
            }
          }, (vm: CarCardVM) => vm.ip)
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
  RobotCard(vm: CarCardVM) {
    Row() {
      Circle({ width: 12, height: 12 })
        .fill(this.dotColor(vm.state))
        .margin({ right: Space.md })
      Column() {
        Text('巡检车')
          .fontSize(FontSize.h3)
          .fontColor(AppColor.textBody)
        Text(vm.ip)
          .fontSize(FontSize.dataSm)
          .fontFamily(FontFamily.mono)
          .fontColor(AppColor.textSecondary)
        Text(vm.detail)
          .fontSize(FontSize.caption)
          .fontColor(vm.state === ConnState.lost ? AppColor.danger : AppColor.textSecondary)
          .margin({ top: 2 })
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)
      // 动作：已连接→进入控制；连接中→spinner；已发现/丢失→连接
      if (vm.state === ConnState.connected) {
        Button('进入控制 〉')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.onPrimary)
          .backgroundColor(AppColor.primary)
          .borderRadius(Radius.pill)
          .height(36)
          .padding({ left: Space.md, right: Space.md })
          .onClick(() => this.enterControl(vm.ip))
      } else if (vm.state === ConnState.connecting) {
        Row() {
          LoadingProgress().width(18).height(18).color(AppColor.warning)
          Text('连接中')
            .fontSize(FontSize.bodySm)
            .fontColor(AppColor.textSecondary)
            .margin({ left: Space.xs })
        }
      } else {
        Button('连接')
          .fontSize(FontSize.bodySm)
          .fontColor(AppColor.primary)
          .backgroundColor(AppColor.surface)
          .border({ width: 1, color: AppColor.primary })
          .borderRadius(Radius.pill)
          .height(36)
          .padding({ left: Space.md, right: Space.md })
          .onClick(() => this.connect(vm.ip))
      }
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
  }

  // ── 连接状态派生 ──────────────────────────────────────────────────────────────
  private refreshCards(): void {
    const list: CarCardVM[] = [];
    for (const d of this.devices) {
      const st: ConnState = this.transport.connStateOf(d.ip);
      list.push({ ip: d.ip, state: st, detail: this.detailOf(st, d.ip) });
    }
    this.cards = list;
  }

  private detailOf(state: ConnState, ip: string): string {
    switch (state) {
      case ConnState.connected:
        return `已连接 · 心跳正常 · ${this.freshLabel(ip)}`;
      case ConnState.connecting:
        return '连接中…（等待心跳）';
      case ConnState.lost:
        return `连接丢失 · 上次心跳 ${this.freshLabel(ip)}`;
      default:
        return '已发现 · 未连接';
    }
  }

  private freshLabel(ip: string): string {
    const ms: number = this.transport.msSinceLastSeen(ip);
    if (ms < 0) {
      return '—';
    }
    if (ms < 1500) {
      return '刚刚';
    }
    return `${Math.round(ms / 1000)} 秒前`;
  }

  private dotColor(state: ConnState): string {
    switch (state) {
      case ConnState.connected:
        return AppColor.success;
      case ConnState.connecting:
        return AppColor.warning;
      case ConnState.lost:
        return AppColor.danger;
      default:
        return AppColor.textCaption;
    }
  }

  // ── 连接 ────────────────────────────────────────────────────────────────────
  /** 点「连接」：独占起中性保活(pending+stop，不建图/不运动)，收到心跳即转"已连接"。连新车自动断旧车。 */
  private connect(ip: string): void {
    const neutral: UdpSendData = new UdpSendData(RobotCommand.pending, MoveDirection.stop, 0);
    this.transport.connectExclusive(ip, neutral);
    this.refreshCards(); // 立即反映"连接中"
  }

  // ── 发现 ────────────────────────────────────────────────────────────────────
  private startScan(): void {
    this.endScan();
    this.scanning = true;
    this.stopDiscover = this.transport.discover((robots: DiscoveredRobot[]) => {
      // 整体替换（去重已在 discover 内完成），绝不 push 追加 —— 修旧累积 bug。
      this.devices = robots;
      this.refreshCards();
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
      log.e(`路由 ControlPage 失败：${e.message}`);
    });
  }
}
