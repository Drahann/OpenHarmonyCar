/**
 * 调试开关（logging-plan L1）—— 统一来源，App 与车载 agent 共用。
 * ⚠️ release 应收紧：LOG_MIN_LEVEL 调到 warn、所有 DEBUG_* 关。详见 docs/logging-plan.md。
 */

/** hilog DOMAIN（DevEco HiLog 窗口 / `hdc hilog` 按此 + TAG 过滤）。 */
export const LOG_DOMAIN: number = 0xD002;

/** 最低输出级别：0=debug 1=info 2=warn 3=error；低于此不输出。release 建议 2。 */
export const LOG_MIN_LEVEL: number = 1;

/**
 * 协议线缆**详尽** trace：开则额外打印**保活重发**（去重）与 **RX 心跳**（节流约 1/s）的 hex+解码帧。
 * ⚠️ **显式命令 TX 始终打印**（info 级，无需开此开关）——本开关只是再叠加 RX/保活这些热路径帧。release 必关。
 */
export const DEBUG_WIRE: boolean = false;

/** 黑板 / 软总线 trace（logging-plan L2 用，FleetMissionService 接入）。 */
export const DEBUG_BLACKBOARD: boolean = false;
