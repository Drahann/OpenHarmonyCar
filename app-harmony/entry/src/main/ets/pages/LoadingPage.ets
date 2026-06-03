/**
 * 占位首页 —— 功能内核重构阶段仅用于让工程可构建。
 * UI 阶段将替换为真正的 HomePage（机器人列表 + 算法选择），并路由到参数化 ControlPage。
 */
@Entry
@Component
struct LoadingPage {
  build() {
    Column() {
      Text('巡检机器人')
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
      Text('功能内核重构中 · UI 待接入')
        .fontSize(14)
        .fontColor('#888888')
        .margin({ top: 8 })
    }
    .width('100%')
    .height('100%')
    .justifyContent(FlexAlign.Center)
  }
}
