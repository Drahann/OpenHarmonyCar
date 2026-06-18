#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
相机帧率模拟：
 - 单次初始化 ACL、YOLO(OM, AIPP NHWC uint8, 640x640) 与 Pose(OM, NCHW FP32 1x3x256x192)
 - 预加载图像到内存，循环多帧（可指定 warmup 与 frames）
 - 统计端到端、YOLO、Pose 的平均/中位/TP95耗时与 FPS
"""
import os
import glob
import time
import argparse
import numpy as np
import cv2

from yolov5_nchw_inference_fixed import ACLContext, YOLOv5Model, postprocess as yolo_post
from simple_baselines_om_infer import OMModel


def preprocess_yolo_aipp(bgr: np.ndarray, size=(640, 640)) -> np.ndarray:
    rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    rgb = cv2.resize(rgb, size, interpolation=cv2.INTER_LINEAR)
    return np.expand_dims(rgb, axis=0).astype(np.uint8)  # NHWC uint8


def preprocess_pose(bgr: np.ndarray, dst_hw=(256, 192)) -> np.ndarray:
    h, w = dst_hw
    rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    rgb = cv2.resize(rgb, (w, h), interpolation=cv2.INTER_LINEAR)
    x = rgb.astype(np.float32) / 255.0
    mean = np.array([0.485, 0.456, 0.406], dtype=np.float32)
    std = np.array([0.229, 0.224, 0.225], dtype=np.float32)
    x = (x - mean) / std
    x = np.transpose(x, (2, 0, 1))
    x = np.expand_dims(x, 0)
    return x


def main():
    ap = argparse.ArgumentParser(description='Camera FPS Simulation (YOLO → Pose)')
    default_root = os.path.dirname(__file__)
    default_yolo = os.path.join(default_root, 'yolo', 'yolov5s_gauge_nchw_aipp.om')
    default_pose = os.path.join(default_root, 'simple_baselines', 'simple_baselines_256x192_bs1_fp32.om')
    ap.add_argument('--yolo_om', default=default_yolo, help='YOLO OM 路径 (AIPP NHWC uint8)')
    ap.add_argument('--pose_om', default=default_pose, help='Pose OM 路径 (NCHW FP32 1x3x256x192)')
    ap.add_argument('--img_dir', required=True, help='模拟相机帧来源目录')
    ap.add_argument('--warmup', type=int, default=5, help='预热帧数')
    ap.add_argument('--frames', type=int, default=100, help='统计帧数')
    args = ap.parse_args()

    assert os.path.exists(args.yolo_om), f'not found: {args.yolo_om}'
    assert os.path.exists(args.pose_om), f'not found: {args.pose_om}'
    assert os.path.isdir(args.img_dir), f'not a dir: {args.img_dir}'

    # 预加载图片到内存
    img_paths = []
    for ext in ('*.jpg','*.jpeg','*.png','*.bmp'):
        img_paths.extend(glob.glob(os.path.join(args.img_dir, ext)))
        img_paths.extend(glob.glob(os.path.join(args.img_dir, ext.upper())))
    img_paths = sorted(img_paths)
    if not img_paths:
        print('No images found.')
        return
    images = []
    for p in img_paths:
        bgr = cv2.imread(p)
        if bgr is not None:
            images.append(bgr)
    if not images:
        print('All images failed to load.')
        return

    with ACLContext():
        yolo = YOLOv5Model(args.yolo_om)
        yolo.load()
        pose = OMModel(args.pose_om)
        pose.load()

        # 统一检查 pose 输入大小
        expect_bytes = 1*3*256*192*4
        import acl as _acl
        if _acl.mdl.get_input_size_by_index(pose.desc, 0) != expect_bytes:
            print('[WARN] Pose 模型输入大小与 1x3x256x192 不符，结果可能异常')

        def run_one(bgr):
            # YOLO
            y_in = preprocess_yolo_aipp(bgr)
            y_out = yolo.infer(y_in)
            h, w = bgr.shape[:2]
            dets = yolo_post(y_out, (w, h), 0.3, 0.6, 1)
            # 仅对首个框进行关键点（模拟相机实时取首个目标）
            if not dets:
                return 0.0, 0.0
            x, y, ww, hh, _ = dets[0][:5]
            x1, y1 = max(0, int(x)), max(0, int(y))
            x2, y2 = min(w-1, int(x+ww)), min(h-1, int(y+hh))
            if x2 <= x1 or y2 <= y1:
                return 0.0, 0.0
            crop = bgr[y1:y2, x1:x2]
            p_in = preprocess_pose(crop)
            _ = pose.infer_one(p_in.reshape(-1))
            return 1.0, 1.0

        # 预热
        idx = 0
        for _ in range(args.warmup):
            _ = run_one(images[idx % len(images)])
            idx += 1

        # 统计
        end2end_ms = []
        yolo_ms = []
        pose_ms = []
        idx = 0
        for _ in range(args.frames):
            bgr = images[idx % len(images)]
            idx += 1
            t0 = time.time()
            # 分段计时
            t_y0 = time.time()
            y_in = preprocess_yolo_aipp(bgr)
            y_out = yolo.infer(y_in)
            t_y1 = time.time()
            h, w = bgr.shape[:2]
            dets = yolo_post(y_out, (w, h), 0.3, 0.6, 1)
            if dets:
                x, y, ww, hh, _ = dets[0][:5]
                x1, y1 = max(0, int(x)), max(0, int(y))
                x2, y2 = min(w-1, int(x+ww)), min(h-1, int(y+hh))
                if x2 > x1 and y2 > y1:
                    crop = bgr[y1:y2, x1:x2]
                    p_in = preprocess_pose(crop)
                    t_p0 = time.time()
                    _ = pose.infer_one(p_in.reshape(-1))
                    t_p1 = time.time()
                    pose_ms.append((t_p1 - t_p0) * 1000.0)
            yolo_ms.append((t_y1 - t_y0) * 1000.0)
            t1 = time.time()
            end2end_ms.append((t1 - t0) * 1000.0)

        def pct(vals, q):
            arr = np.array(vals, dtype=np.float64)
            return float(np.percentile(arr, q)) if len(arr) else 0.0

        avg = np.mean(end2end_ms)
        fps = 1000.0 / avg if avg > 0 else 0.0
        print(f"Frames: {len(end2end_ms)}, End2End avg: {avg:.2f} ms, FPS: {fps:.2f}")
        print(f"End2End p50: {pct(end2end_ms,50):.2f} ms, p95: {pct(end2end_ms,95):.2f} ms")
        if yolo_ms:
            print(f"YOLO   avg: {np.mean(yolo_ms):.2f} ms, p50: {pct(yolo_ms,50):.2f} ms, p95: {pct(yolo_ms,95):.2f} ms")
        if pose_ms:
            print(f"Pose   avg: {np.mean(pose_ms):.2f} ms, p50: {pct(pose_ms,50):.2f} ms, p95: {pct(pose_ms,95):.2f} ms")


if __name__ == '__main__':
    main()







