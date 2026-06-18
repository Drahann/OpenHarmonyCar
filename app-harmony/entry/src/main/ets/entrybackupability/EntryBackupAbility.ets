import { BackupExtensionAbility, BundleVersion } from '@kit.CoreFileKit';
import { Log, Logger } from '../utils/log';

const log: Logger = Log.scoped('EntryBackupAbility');

export default class EntryBackupAbility extends BackupExtensionAbility {
  async onBackup(): Promise<void> {
    log.i('onBackup ok');
  }

  async onRestore(bundleVersion: BundleVersion): Promise<void> {
    log.i(`onRestore ok ${JSON.stringify(bundleVersion)}`);
  }
}
