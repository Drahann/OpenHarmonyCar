import { BusinessError } from '@kit.BasicServicesKit';
import { ComponentContent, promptAction } from '@kit.ArkUI';
import { UIContext } from '@ohos.arkui.UIContext';

/**
 * 自定义弹窗助手（保留自旧 utils/componentUtils.ets，UI 阶段复用）。
 */
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
          console.info('OpenCustomDialog complete.');
        })
        .catch((error: BusinessError) => {
          console.error(`OpenCustomDialog error code=${error.code}, message=${error.message}`);
        });
    }
  }

  static closeDialog(): void {
    if (PromptActionClass.contentNode !== null) {
      PromptActionClass.ctx.getPromptAction().closeCustomDialog(PromptActionClass.contentNode)
        .then(() => {
          console.info('CloseCustomDialog complete.');
        })
        .catch((error: BusinessError) => {
          console.error(`CloseCustomDialog error code=${error.code}, message=${error.message}`);
        });
    }
  }

  static updateDialog(options: promptAction.BaseDialogOptions): void {
    if (PromptActionClass.contentNode !== null) {
      PromptActionClass.ctx.getPromptAction().updateCustomDialog(PromptActionClass.contentNode, options)
        .then(() => {
          console.info('UpdateCustomDialog complete.');
        })
        .catch((error: BusinessError) => {
          console.error(`UpdateCustomDialog error code=${error.code}, message=${error.message}`);
        });
    }
  }
}
