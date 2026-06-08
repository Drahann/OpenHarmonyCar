import { BusinessError } from '@kit.BasicServicesKit';
import { ComponentContent, promptAction } from '@kit.ArkUI';
import { UIContext } from '@ohos.arkui.UIContext';
import { Log, Logger } from './log';

/**
 * 自定义弹窗助手（保留自旧 utils/componentUtils.ets，UI 阶段复用）。
 */
const log: Logger = Log.scoped('Dialog');

export class PromptActionClass {
  static ctx: UIContext;
  static contentNode: ComponentContent<Object>;
  static options: promptAction.BaseDialogOptions;

  static setContext(context: UIContext): void {
    PromptActionClass.ctx = context;
  }

  static setContentNode(node: ComponentContent<Object>): void {
    PromptActionClass.contentNode = node;
  }

  static setOptions(options: promptAction.BaseDialogOptions): void {
    PromptActionClass.options = options;
  }

  static openDialog(): void {
    if (PromptActionClass.contentNode !== null) {
      PromptActionClass.ctx.getPromptAction().openCustomDialog(PromptActionClass.contentNode, PromptActionClass.options)
        .then(() => {
          log.i('OpenCustomDialog complete.');
        })
        .catch((error: BusinessError) => {
          log.e(`OpenCustomDialog error code=${error.code}, message=${error.message}`);
        });
    }
  }

  static closeDialog(): void {
    if (PromptActionClass.contentNode !== null) {
      PromptActionClass.ctx.getPromptAction().closeCustomDialog(PromptActionClass.contentNode)
        .then(() => {
          log.i('CloseCustomDialog complete.');
        })
        .catch((error: BusinessError) => {
          log.e(`CloseCustomDialog error code=${error.code}, message=${error.message}`);
        });
    }
  }

  static updateDialog(options: promptAction.BaseDialogOptions): void {
    if (PromptActionClass.contentNode !== null) {
      PromptActionClass.ctx.getPromptAction().updateCustomDialog(PromptActionClass.contentNode, options)
        .then(() => {
          log.i('UpdateCustomDialog complete.');
        })
        .catch((error: BusinessError) => {
          log.e(`UpdateCustomDialog error code=${error.code}, message=${error.message}`);
        });
    }
  }
}
