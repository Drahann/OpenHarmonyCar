/**
 * AgentStatusPage —— 车载 agent 运行时的极简状态页。
 *
 * 仅因当前用 UIAbility 外壳托管 AgentCore（headless ServiceExtension 系统 API 第三方编不过的过渡形态），
 * UIAbility 需要一张窗口内容页。日常运行不依赖它；真正的桥接逻辑在后台的 AgentCore。
 */
@Entry
@Component
struct AgentStatusPage {
  build() {
    Column({ space: 12 }) {
      Text('车载 Agent 运行中')
        .fontSize(20)
        .fontWeight(FontWeight.Bold)
      Text('FleetMission 黑板 ↔ 本机 UDP 桥（127.0.0.1:5001）')
        .fontSize(13)
        .fontColor('#666666')
      Text('过渡形态：UIAbility 外壳；配网认证请走「配对」入口')
        .fontSize(12)
        .fontColor('#999999')
        .textAlign(TextAlign.Center)
    }
    .width('100%')
    .height('100%')
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .padding(24)
  }
}
