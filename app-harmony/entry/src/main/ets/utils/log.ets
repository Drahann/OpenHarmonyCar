/**
 * 统一日志（hilog 薄封装）—— 取代散落的 console.* 与裸 hilog 调用（logging-plan L1）。
 * **App 与车载 agent 共用**（属共享层，见 docs/car-agent-plan.md §2、docs/logging-plan.md）。
 *
 * - 固定 DOMAIN（constants/debug.ets）+ 每模块 TAG，便于 DevEco HiLog / `hdc hilog` 按 TAG 过滤。
 * - 级别门限 LOG_MIN_LEVEL：低于此不输出；verbose / 线缆 trace 由 DEBUG_* 开关另控（默认关）。
 * - 不引第三方库。⚠️ 未经 DevEco 验证（本机无 hvigor）。
 *
 * 用法：`private log: Logger = Log.scoped('RobotTransport'); this.log.i('bound');`
 */

import { hilog } from '@kit.PerformanceAnalysisKit';
import { LOG_DOMAIN, LOG_MIN_LEVEL } from '../constants/debug';

export enum LogLevel {
  debug = 0,
  info = 1,
  warn = 2,
  error = 3
}

/** 一个带固定 TAG 的记录器。 */
export class Logger {
  private readonly tag: string;

  constructor(tag: string) {
    this.tag = tag;
  }

  d(msg: string): void {
    this.out(LogLevel.debug, msg);
  }

  i(msg: string): void {
    this.out(LogLevel.info, msg);
  }

  w(msg: string): void {
    this.out(LogLevel.warn, msg);
  }

  e(msg: string): void {
    this.out(LogLevel.error, msg);
  }

  private out(level: LogLevel, msg: string): void {
    if (level < LOG_MIN_LEVEL) {
      return;
    }
    switch (level) {
      case LogLevel.debug:
        hilog.debug(LOG_DOMAIN, this.tag, '%{public}s', msg);
        break;
      case LogLevel.info:
        hilog.info(LOG_DOMAIN, this.tag, '%{public}s', msg);
        break;
      case LogLevel.warn:
        hilog.warn(LOG_DOMAIN, this.tag, '%{public}s', msg);
        break;
      default:
        hilog.error(LOG_DOMAIN, this.tag, '%{public}s', msg);
        break;
    }
  }
}

export class Log {
  /** 取一个带 TAG 的记录器。 */
  static scoped(tag: string): Logger {
    return new Logger(tag);
  }
}
