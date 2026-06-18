import { describe, it, expect } from '@ohos/hypium';
import { encodeSend, decodeReceive, UdpSendData, RobotCommand } from '../main/ets/model/protocol';
import { canvasToMap, mapToCanvas, MapTransform, Point } from '../main/ets/model/geometry';
import { MapService, ParsedMap } from '../main/ets/service/MapService';
import {
  Mission, MissionSnapshot, ControlMode, MissionPhase, RobotStatus, RobotRuntime, EndPoint, Assignment, Rect
} from '../main/ets/model/mission';

/** ArrayBuffer -> "b0,b1,..." 便于断言。 */
function bytesString(buf: ArrayBuffer): string {
  return Array.from(new Uint8Array(buf)).join(',');
}

export default function localUnitTest() {
  describe('protocolCodec', () => {
    // 与 contracts/udp-protocol.md 逐字节核对
    it('encode_fixedVector_bigEndian', 0, () => {
      const buf: ArrayBuffer = encodeSend(new UdpSendData(RobotCommand.startRoute, 1, 35, 258, -1));
      // 258=0x0102, -1=0xFFFF（大端），byte7/8=0
      expect(bytesString(buf)).assertEqual('3,1,35,1,2,255,255,0,0');
    });

    it('decode_heartbeat_bigEndian', 0, () => {
      const frame: ArrayBuffer = new Uint8Array([3, 0, 0, 0x00, 0x64, 0xFF, 0x9C, 0x01, 0x68]).buffer;
      const r = decodeReceive(frame);
      expect(r.state).assertEqual(3);
      expect(r.x).assertEqual(100);
      expect(r.y).assertEqual(-100);
      expect(r.r).assertEqual(360);
      expect(r.hasPose()).assertTrue();
    });

    it('roundTrip_decode_of_encode', 0, () => {
      const samples: UdpSendData[] = [
        new UdpSendData(RobotCommand.beforeStart, 0, 0, 0, 0),
        new UdpSendData(RobotCommand.startRoute, 0, 0, 1234, -5678),
        new UdpSendData(RobotCommand.distributed, 0, 0, 32767, -32768)
      ];
      samples.forEach((s: UdpSendData) => {
        const d = decodeReceive(encodeSend(s));
        expect(d.state).assertEqual(s.state);
        expect(d.x).assertEqual(s.endX);
        expect(d.y).assertEqual(s.endY);
        expect(d.r).assertEqual(0);
      });
    });
  });

  describe('geometryTransform', () => {
    // 包围盒 startX=0,endY=5,gridSize=100,txtAverSize=5（见下方 mapParse 用例）
    const t: MapTransform = new MapTransform(0, 1, 4, 5, 100, 5, 5);
    const half: number = 250;

    it('mapToCanvas_then_canvasToMap_isIdentity', 0, () => {
      const points: Point[] = [{ x: 2, y: 2 }, { x: 1, y: 4 }, { x: 3, y: 1 }];
      points.forEach((p: Point) => {
        const back: Point = canvasToMap(mapToCanvas(p, t, half), t, half);
        expect(back.x).assertEqual(p.x);
        expect(back.y).assertEqual(p.y);
      });
    });
  });

  describe('mapParse', () => {
    // 5x5：四周墙 + 中心一点
    const text: string = '5 5\n11111\n10001\n10101\n10001\n11111\n';

    it('parseMap_boundingBox_and_sizes', 0, () => {
      const m: ParsedMap = MapService.parseMap(text, 500, 500);
      expect(m.rows).assertEqual(5);
      expect(m.cols).assertEqual(5);
      expect(m.startX).assertEqual(0);
      expect(m.startY).assertEqual(1);
      expect(m.endX).assertEqual(4);
      expect(m.endY).assertEqual(5);
      expect(m.squareSize).assertEqual(5);
      expect(m.gridSize).assertEqual(100); // 500/5
      expect(m.txtAverSize).assertEqual(5);
    });

    it('parseMap_header_takesLastTwoInts', 0, () => {
      // 兼容旧 4-token 首行：末两个为行列数
      const m: ParsedMap = MapService.parseMap('0 0 5 5\n11111\n10001\n10101\n10001\n11111\n', 500, 500);
      expect(m.rows).assertEqual(5);
      expect(m.cols).assertEqual(5);
    });
  });

  describe('missionSnapshot', () => {
    // FleetMission 黑板快照往返：toSnapshot → applySnapshot 应逐字段还原（含协同字段）
    it('toSnapshot_applySnapshot_roundTrip', 0, () => {
      const m: Mission = new Mission(ControlMode.distributed);
      m.phase = MissionPhase.covering;
      m.mapReady = true;
      m.frame = { originX: 1, originY: 2, resolution: 0.05, headingZero: 90 };
      m.map = { ref: 'master-1', text: '5 5\n11111' };
      m.area = { corner1: { x: 0, y: 0 }, corner2: { x: 10, y: 10 } };
      m.endPoints = [new EndPoint(3, 4), new EndPoint(null, null)];
      m.assignments = [new Assignment(2, { x: 1, y: 1 }, { x: 5, y: 5 }, 1)];
      const r0: RobotRuntime = new RobotRuntime(1, '172.168.11.99', 'net-1');
      r0.online = true;
      r0.x = 7;
      r0.y = 8;
      r0.r = 45;
      r0.command = 105;
      r0.progress = 0.5;
      r0.status = RobotStatus.covering;
      m.robots = [r0];

      // 经 JSON 串行化（与软总线同步路径一致）再还原
      const m2: Mission = new Mission();
      m2.applySnapshot(JSON.parse(JSON.stringify(m.toSnapshot())) as MissionSnapshot);

      expect(m2.mode).assertEqual(ControlMode.distributed);
      expect(m2.phase).assertEqual(MissionPhase.covering);
      expect(m2.mapReady).assertTrue();
      expect(m2.frame.resolution).assertEqual(0.05);
      expect(m2.frame.headingZero).assertEqual(90);
      expect(m2.map.ref).assertEqual('master-1');

      const area: Rect | null = m2.area;
      expect(area !== null).assertTrue();
      if (area !== null) {
        expect(area.corner1.x).assertEqual(0);
        expect(area.corner2.x).assertEqual(10);
      }

      expect(m2.endPoints.length).assertEqual(2);
      expect(m2.endPoints[0].x).assertEqual(3);
      expect(m2.assignments.length).assertEqual(1);
      expect(m2.assignments[0].carId).assertEqual(2);
      expect(m2.assignments[0].corner2.y).assertEqual(5);
      expect(m2.assignments[0].robotId).assertEqual(1);
      expect(m2.robots.length).assertEqual(1);
      expect(m2.robots[0].networkId).assertEqual('net-1');
      expect(m2.robots[0].progress).assertEqual(0.5);
      expect(m2.robots[0].status).assertEqual(RobotStatus.covering);
    });
  });
}
