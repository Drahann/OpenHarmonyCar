#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""多仪表读数 (conf=0.10 + 误检框过滤 + 逐表读数)。复用 test_gauge_reading 的函数。"""
import os, sys, cv2, numpy as np
sys.path.insert(0, '/home/HwHiAiUser/simple_baselines')
sys.path.insert(0, '/home/HwHiAiUser/simple_baselines/deployment_bundle')
import test_gauge_reading as T
from yolov5_nchw_inference_fixed import ACLContext, YOLOv5Model, postprocess as yolo_post
from simple_baselines_om_infer import OMModel

CONF_THRESH = 0.10
NMS_THRESH = 0.6
YOLO_OM = '/home/HwHiAiUser/simple_baselines/deployment_bundle/yolo/yolov5s_gauge_nchw_aipp.om'
POSE_OM = '/home/HwHiAiUser/simple_baselines/deployment_bundle/simple_baselines/simple_baselines_256x192_bs1_fp32.om'


def filter_detections(dets, frame_wh):
    """剔除明显误检框：长宽比偏离仪表(近正方形)的、跨多表的大框。"""
    W, H = frame_wh
    frame_area = float(W * H)
    kept = []
    for det in dets:
        x, y, w, h = det[:4]
        if w <= 1 or h <= 1:
            continue
        ar = w / h
        if not (0.6 <= ar <= 1.7):
            continue
        if (w * h) > 0.85 * frame_area:
            continue
        kept.append(det)
    return kept


def main():
    img_path = sys.argv[1] if len(sys.argv) > 1 else '/home/HwHiAiUser/simple_baselines/samples/1.png'
    frame = cv2.imread(img_path)
    if frame is None:
        print('❌ 无法读取图片: %s' % img_path); return
    h, w = frame.shape[:2]
    print('📷 图片: %s (%dx%d)' % (img_path, w, h))
    with ACLContext():
        ym = YOLOv5Model(YOLO_OM); ym.load()
        pm = OMModel(POSE_OM); pm.load()
        yout = ym.infer(T.preprocess_yolo(frame))
        raw = yolo_post(yout, (w, h), CONF_THRESH, NMS_THRESH, 1)
        dets = filter_detections(raw, (w, h))
        print('🔍 YOLO: 原始 %d → 过滤后 %d 个仪表 (conf>=%.2f)' % (len(raw), len(dets), CONF_THRESH))
        klist, results = [], []
        for det in dets:
            x, y, ww, hh, score = det[:5]
            x1, y1 = max(0, int(x)), max(0, int(y))
            x2, y2 = min(w - 1, int(x + ww)), min(h - 1, int(y + hh))
            if not (x2 > x1 and y2 > y1):
                klist.append(None); results.append(None); continue
            crop = frame[y1:y2, x1:x2]
            pout = pm.infer_one(T.preprocess_pose(crop).reshape(-1))
            J = int(pout.size / (1 * 64 * 48)) if pout.size % (64 * 48) == 0 else 17
            hm = pout.reshape(1, J, 64, 48)
            kpts = T.heatmap_to_keypoints(hm, (x1, y1, x2 - x1, y2 - y1))
            klist.append(kpts)
            res = T.calculate_gauge_reading(kpts, frame.shape) if len(kpts) >= 4 else None
            results.append(res)
        print('')
        print('=' * 70)
        print('📋 各仪表读数汇总 (共 %d 块)' % len(dets))
        print('=' * 70)
        for i, (det, res) in enumerate(zip(dets, results)):
            x, y, ww, hh, score = det[:5]
            pct = ('%.1f%%' % res['percentage']) if res else 'N/A'
            print('  仪表[%d] bbox=(%d,%d,%d,%d) score=%.3f → 读数 %s' % (i, int(x), int(y), int(ww), int(hh), score, pct))
        vis = frame.copy()
        for det, kpts, res in zip(dets, klist, results):
            vis = T.visualize_result(vis, [det], [kpts], res)
        base = os.path.splitext(os.path.basename(img_path))[0]
        outp = '/home/HwHiAiUser/simple_baselines/test_result_%s_multi.jpg' % base
        cv2.imwrite(outp, vis)
        print('✅ 结果已保存: %s' % outp)


if __name__ == '__main__':
    main()
