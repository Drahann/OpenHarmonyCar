/**
 * 与紫派对接的协议/格式常量 —— 唯一来源，与 contracts/ 双向对齐。
 *
 * - UDP 控制/心跳：contracts/udp-protocol.md
 * - 地图文本格式：contracts/map-format.md
 *
 * ⚠️ 改这里的任何值都属于"协议变更"，必须同步 contracts/ 并通知紫派侧。
 */

// ── 传输层（contracts/udp-protocol.md §传输层）──────────────────────────────
/** App 收发同端口 */
export const UDP_PORT: number = 5001;
/** 固定包长 9 字节 */
export const UDP_PACKET_BYTES: number = 9;
/** 默认目标 IP（可在界面修改） */
export const DEFAULT_TARGET_IP: string = '172.168.11.99';
/** DataView 多字节整型用大端（网络序）。littleEndian = false。 */
export const BIG_ENDIAN: boolean = false;

// ── 9 字节布局的字节偏移 ────────────────────────────────────────────────────
export const OFFSET_STATE: number = 0;    // 命令码 / 心跳状态字节
export const OFFSET_RUNSTATE: number = 1;  // 运动方向（发送）
export const OFFSET_SPEED: number = 2;     // 速度（发送）
export const OFFSET_X: number = 3;         // int16 BE：endX（发送）/ x（接收）
export const OFFSET_Y: number = 5;         // int16 BE：endY（发送）/ y（接收）
export const OFFSET_R: number = 7;         // int16 BE：r 朝向（仅接收）

// ── 连接与保活（contracts/udp-protocol.md §连接与保活）─────────────────────
/** App keepalive 间隔：至少每 1s 发一次指令（哪怕空指令）。注：紫派实际每 500ms 回心跳（udp.c:121）。 */
export const HEARTBEAT_INTERVAL_MS: number = 1000;
/** 紫派心跳实际间隔（仅参考，对账 udp2lcm 得）。 */
export const PURPLEPI_HEARTBEAT_INTERVAL_MS: number = 500;
/** 紫派若 3s 未收到任何指令立即令轮控急停。⚠️ 两端必须一致。 */
export const FAILSAFE_TIMEOUT_MS: number = 3000;
/** 接收帧首字节为 3 表示本帧带当前坐标（设计文档 3.2.3.1）。 */
export const HEARTBEAT_STATE_HAS_POSE: number = 3;
/** 建连等待回包的超时。 */
export const CONNECT_TIMEOUT_MS: number = 3000;

// ── 地图 HTTP（contracts/map-format.md §文件与传输）─────────────────────────
/** 紫派轻量 HTTP 服务端口 */
export const MAP_HTTP_PORT: number = 8000;
/**
 * 地图文件名。紫派 `chdir("/data/test")` + `python -m http.server` → web 根 = /data/test，
 * URL 不带 /data/test 前缀（即 `http://<紫派IP>:8000/defultMap.txt`）。
 * ✅ 成员A确认（接口功能与对接问题说明.md §六.1 + `Navi/main.cpp:517/642` 实际保存 `defultMap.txt`）：
 *   确切名 = `defultMap.txt`（单 .txt）。`defultMap.txt.txt` 是历史残留，紫派启动即删（`main.cpp:1094-1096`），勿用。
 */
export const MAP_FILE_NAME: string = 'defultMap.txt';
/** 栅格值：空旷可通行 */
export const MAP_GRID_FREE: string = '0';
/** 栅格值：障碍物 */
export const MAP_GRID_WALL: string = '1';

/** 线上整数坐标换算：1 米 = 20 单位（1 单位 = 5cm = 1 个地图格）；r 用度。✅ 对账 udp2lcm.c。 */
export const COORD_UNITS_PER_METER: number = 20;

/** 旧实现里的测试地址（"服务器"归属待定，见 contracts/server-api.md）。 */
export const MAP_LEGACY_TEST_URL: string = 'http://116.63.15.79/defultMap.txt';

/** 由机器人 IP 拼出地图拉取 URL（紫派 :8000，web 根=/data/test）。 */
export function mapUrlFor(ip: string): string {
  return `http://${ip}:${MAP_HTTP_PORT}/${MAP_FILE_NAME}`;
}

/**
 * 软总线协同会话 ID（FleetMission 黑板，distributed 模式用）。平板与各车 agent 用同一 ID 入会才能同步。
 * ⚠️ **平板与 car-agent 必须同值**：car-agent 的 `constants/protocol.ets` 是本文件同源副本（改一处改两处），
 *    其 `AgentServiceAbility` 从该常量 import（不再各自硬编码，杜绝值漂移）。
 */
export const FLEET_SESSION_ID: string = 'OpenHarmonyCarFleetV1';
