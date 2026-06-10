/**
 * 统一主题 token —— 融合 style/ 五套设计（融合决策见 docs/ui-design.md）。
 * 取向：F&F 极简(new/) 为骨架基底；Google Maps 移动端为地图/控制屏范式；Area 墨绿为品牌主色；
 *       Google/Colin 高饱和色为语义状态色；iOS(plants) 圆角卡片为组件质感；等宽字体承载遥测数字。
 *
 * ⚠️ 未经 DevEco 验证（本机无 hvigor）。纯 token 常量，UI 组件统一从此导入（单一来源）。
 * ⚠️ 自定义字体（Instrument Sans / Satoshi / Roboto Mono 等）需在 EntryAbility 用 font.registerFont
 *    注册后方可用；未注册前 sans 走系统默认、mono 走系统等宽回退（见 FontFamily）。
 */

/** 配色（hex 字符串；ArkUI 的 fontColor/backgroundColor/border 直接接受）。 */
export class AppColor {
  // —— 品牌 / 主色（Area 墨绿系）——
  static readonly primary: string = '#485c11';        // 主按钮 / 主操作 / 选中
  static readonly primaryPressed: string = '#3a4a0e'; // 按下态
  static readonly primarySoft: string = '#dfecc6';    // 主色浅底 / 标签底（Area accent-2）
  static readonly onPrimary: string = '#ffffff';      // 主色之上的文字

  // —— 中性 / 表面（F&F 极简 + Area 页底）——
  static readonly pageBg: string = '#edf4f1';         // 整页淡绿底（要更中性可改 '#f6f7f8'）
  static readonly surface: string = '#ffffff';        // 卡片 / 浮层
  static readonly surfaceMuted: string = '#ececec';   // 次级表面（F&F bg-muted）
  static readonly divider: string = '#e9e9e9';        // 分割线
  static readonly border: string = '#dfe4ea';         // 描边（Maps line）

  // —— 文字 ——
  static readonly textTitle: string = '#000000';
  static readonly textBody: string = '#1f2329';
  static readonly textSecondary: string = '#6f7787';  // 次要 / 说明（Maps muted）
  static readonly textCaption: string = '#929292';    // 注释 / 占位

  // —— 语义 / 状态色（Google + Colin）——
  static readonly info: string = '#1a73e8';           // 信息 / 链接（Google blue）
  static readonly success: string = '#34a853';        // 成功 / 在线（Google green）
  static readonly warning: string = '#fbbc04';        // 警告 / 暂停（Google yellow）
  static readonly danger: string = '#ea4335';         // 错误 / 急停（Google red）

  // —— 机器人状态（对应 model/mission.ets RobotStatus）——
  static readonly robotOnline: string = '#34a853';    // online / idle
  static readonly robotCovering: string = '#1a73e8';  // covering / scanning（运行中）
  static readonly robotPaused: string = '#fbbc04';    // 协同避让暂停（multi-robot-collab §双车协同避障）
  static readonly robotError: string = '#ea4335';     // error
  static readonly robotOffline: string = '#929292';   // offline

  // —— 地图渲染（与 constants/ui.ets 的 MAP_*_COLOR 对齐，给语义别名）——
  static readonly mapFree: string = '#ffffff';        // 空旷
  static readonly mapWall: string = '#202124';        // 障碍（比纯黑略柔，Maps 风）
  static readonly mapTarget: string = '#485c11';      // 目标点（主色）
  static readonly mapPin: string = '#ea4335';         // 机器人位姿 pin（Maps red teardrop）
}

/**
 * 地图配色 —— MapCanvas 专用「干净建筑图纸」风（贴合品牌墨绿 #485c11 + 浅底，与 AppColor 同源）。
 * 取向：浅色楼层平面图——浅冷灰未知区 + 白色已扫图纸 + **墨绿平滑矢量墙体**（等高线软填充 + 描边，
 *       非栅格方块）+ 克制语义标记（Maps 泪滴目标 pin、带朝向的机器人圆点，软投影非辉光）。
 * 说明：第一版深色霓虹「指挥中心 HUD」经用户否决（不贴品牌），改为本浅色图纸方案，见 docs/map-ui-redesign.md。
 * ⚠️ 纯 token 常量。未经 DevEco 验证。
 */
export class MapTheme {
  // —— 背景（未知 / 图纸外）：极浅冷灰，与品牌淡绿页底协调；轻微渐变出景深 ——
  static readonly bg: string = '#e9eef1';
  static readonly bgEdge: string = '#dde5ea';

  // —— 已扫地面（白色图纸）——
  static readonly floor: string = '#ffffff';
  static readonly floorEdge: string = '#e2e9da';          // 图纸描边（极浅绿）
  static readonly grid: string = 'rgba(72,92,17,0.055)';  // 图纸上的极淡比例网格（品牌绿调）

  // —— 墙体（品牌墨绿；平滑等高线 = 软填充 + 描边）——
  static readonly wallFill: string = 'rgba(72,92,17,0.15)';
  static readonly wallStroke: string = '#485c11';

  // —— 机器人状态（清爽语义色，非霓虹；对应 model/mission.ets RobotStatus）——
  static readonly robotOnline: string = '#2e7d32';   // online / idle
  static readonly robotCovering: string = '#1a73e8'; // covering / scanning
  static readonly robotError: string = '#ea4335';    // error
  static readonly robotOffline: string = '#9aa3ad';  // offline
  static readonly robotRing: string = '#ffffff';     // 车体白环
  static readonly robotLabel: string = '#ffffff';    // 车号文字

  // —— 目标点（Maps 泪滴 pin，品牌红 + 白心）——
  static readonly target: string = '#ea4335';
  static readonly targetInner: string = '#ffffff';

  // —— 覆盖子区域（品牌墨绿：软填充 + 实线边 + 角标）——
  static readonly region: string = '#485c11';

  // —— 标记投影（抬起感，非辉光）——
  static readonly markerShadow: string = 'rgba(33,42,12,0.30)';
}

/** 字号（fp；平板数据型尺度，避免 Colin 200px 巨字）。 */
export class FontSize {
  static readonly display: number = 32;
  static readonly h1: number = 24;
  static readonly h2: number = 20;
  static readonly h3: number = 18;
  static readonly body: number = 16;
  static readonly bodySm: number = 15;
  static readonly caption: number = 12;
  static readonly label: number = 11;
  // 遥测数字（等宽：坐标 / IP / 进度 / 仪表读数）
  static readonly dataLg: number = 28;
  static readonly dataMd: number = 18;
  static readonly dataSm: number = 14;
}

/** 字体族。⚠️ 自定义字体需先 font.registerFont；未注册走系统回退。 */
export class FontFamily {
  static readonly sans: string = 'HarmonyOS Sans';            // 正文/标题（留位以便换 Instrument Sans）
  static readonly mono: string = 'HarmonyOS Sans Condensed';  // 遥测数字回退（注册 Roboto Mono 后替换）
}

/** 间距（vp）—— Area 8 点网格。 */
export class Space {
  static readonly xs: number = 4;
  static readonly sm: number = 8;
  static readonly md: number = 16;
  static readonly lg: number = 24;
  static readonly xl: number = 32;
  static readonly xxl: number = 48;
}

/** 圆角（vp）—— iOS/Maps 圆润卡片 + 胶囊。 */
export class Radius {
  static readonly sm: number = 10;
  static readonly md: number = 16;    // 卡片 / 底部操作卡（Maps route-card）
  static readonly lg: number = 20;
  static readonly pill: number = 999; // 胶囊按钮 / chip / 搜索条
}

/** 阴影参数（喂给 ArkUI ShadowOptions: { radius, color, offsetX, offsetY }）。 */
export class Elevation {
  static readonly cardRadius: number = 14;
  static readonly cardColor: string = 'rgba(60,64,67,0.22)';  // Maps 卡片阴影
  static readonly cardOffsetY: number = 4;
  static readonly softRadius: number = 8;
  static readonly softColor: string = 'rgba(0,0,0,0.10)';
  static readonly softOffsetY: number = 2;
}

/** 触控目标最小尺寸（vp）—— 平板手指操作。 */
export const TOUCH_MIN: number = 44;
