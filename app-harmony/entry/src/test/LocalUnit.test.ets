import { describe, it, expect } from '@ohos/hypium';
import { encodeSend, decodeReceive, UdpSendData, RobotCommand } from '../main/ets/model/protocol';
import { canvasToMap, mapToCanvas, MapTransform, Point } from '../main/ets/model/geometry';
import { MapService, ParsedMap } from '../main/ets/service/MapService';

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
}
