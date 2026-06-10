#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Realtime-like inference using a video file as source (camera simulation).
 - Reads frames from file or RTSP/USB source
 - Runs YOLO (AIPP NHWC 640x640) + Simple-Baselines Pose
 - Overlays boxes and keypoints
 - Shows window and optionally saves an MP4

Examples:
python3 webcam_realtime.py --video /home/HwHiAiUser/simple_baselines/videos/man1.mov \
  --yolo_om /home/HwHiAiUser/simple_baselines/deployment_key_files/yolov5s_gauge_nchw_aipp.om \
  --pose_om /home/HwHiAiUser/simple_baselines/models/simple_baselines_256x192_bs1_fp32.om \
  --save /home/HwHiAiUser/simple_baselines/infer/man1_realtime.mp4
"""
import os
import time
import argparse
from typing import Tuple, List

import cv2
import numpy as np

from yolov5_nchw_inference_fixed import ACLContext, YOLOv5Model, postprocess as yolo_post
from simple_baselines_om_infer import OMModel


def preprocess_yolo_aipp(bgr, size=(640, 640)):
    rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    rgb = cv2.resize(rgb, size, interpolation=cv2.INTER_LINEAR)
    return np.expand_dims(rgb, 0).astype(np.uint8)


def preprocess_pose_input(bgr, dst_hw=(256, 192)):
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


def heatmap_to_keypoints(heatmap: np.ndarray, crop_xywh, dst_hw=(256, 192)):
    _, J, H, W = heatmap.shape
    crop_x, crop_y, crop_w, crop_h = crop_xywh
    sx = crop_w / float(dst_hw[1])
    sy = crop_h / float(dst_hw[0])
    kpts = []
    for j in range(J):
        hm = heatmap[0, j]
        idx = int(np.argmax(hm))
        py, px = divmod(idx, W)
        score = float(hm[py, px])
        x_on_crop = (px + 0.5) * (dst_hw[1] / W)
        y_on_crop = (py + 0.5) * (dst_hw[0] / H)
        x = crop_x + x_on_crop * sx
        y = crop_y + y_on_crop * sy
        kpts.append((float(x), float(y), score))
    return kpts


def draw_dets_and_kpts(im, dets: List[List[float]], kpts_list: List[List[Tuple[float, float, float]]] = None):
    out = im.copy()
    for i, d in enumerate(dets):
        if len(d) < 5:
            continue
        x, y, w, h, s = d[:5]
        p1 = (int(x), int(y))
        p2 = (int(x + w), int(y + h))
        cv2.rectangle(out, p1, p2, (0, 255, 0), 2)
        cv2.putText(out, f"obj:{s:.2f}", (p1[0], max(0, p1[1]-5)), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0,255,0), 2)
        if kpts_list and i < len(kpts_list):
            for (kx, ky, ks) in kpts_list[i]:
                cv2.circle(out, (int(kx), int(ky)), 3, (0, 0, 255), -1)
    return out


def main():
    ap = argparse.ArgumentParser(description='Realtime-like YOLO+Pose on video source')
    default_root = os.path.dirname(__file__)
    default_yolo = os.path.join(default_root, 'yolo', 'yolov5s_gauge_nchw_aipp.om')
    default_pose = os.path.join(default_root, 'simple_baselines', 'simple_baselines_256x192_bs1_fp32.om')
    ap.add_argument('--video', required=True, help='Video path or stream URL')
    ap.add_argument('--yolo_om', default=default_yolo, help='YOLO OM path (AIPP NHWC)')
    ap.add_argument('--pose_om', default=default_pose, help='Simple-Baselines OM path (1x3x256x192 FP32)')
    ap.add_argument('--conf_thres', type=float, default=0.3)
    ap.add_argument('--nms_thres', type=float, default=0.6)
    ap.add_argument('--display', action='store_true', help='Show window during run')
    ap.add_argument('--save', default='', help='Output MP4 path (optional)')
    ap.add_argument('--target_fps', type=float, default=15.0, help='Sleep to simulate realtime FPS')
    args = ap.parse_args()

    cap = cv2.VideoCapture(args.video)
    if not cap.isOpened():
        print(f"Failed to open video: {args.video}")
        return

    writer = None
    if args.save:
        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        writer = cv2.VideoWriter(args.save, fourcc, args.target_fps, (width, height))

    with ACLContext():
        yolo = YOLOv5Model(args.yolo_om)
        yolo.load()
        pose = OMModel(args.pose_om)
        pose.load()

        while True:
            ret, frame = cap.read()
            if not ret:
                break
            t0 = time.time()
            nhwc = preprocess_yolo_aipp(frame)
            outs = yolo.infer(nhwc)
            h, w = frame.shape[:2]
            dets = yolo_post(outs, (w, h), args.conf_thres, args.nms_thres, 1)

            kpts_list = []
            for d in dets[:1]:  # top-1 for speed
                x, y, ww, hh, _ = d[:5]
                x1, y1 = max(0, int(x)), max(0, int(y))
                x2, y2 = min(w - 1, int(x + ww)), min(h - 1, int(y + hh))
                if x2 > x1 and y2 > y1:
                    crop = frame[y1:y2, x1:x2]
                    pose_in = preprocess_pose_input(crop)
                    out_flat = pose.infer_one(pose_in.reshape(-1))
                    J = int(out_flat.size / (1 * 64 * 48)) if out_flat.size % (64 * 48) == 0 else 17
                    heat = out_flat.reshape(1, J, 64, 48)
                    kpts = heatmap_to_keypoints(heat, (x1, y1, x2 - x1, y2 - y1))
                    kpts_list.append(kpts)
                else:
                    kpts_list.append([])

            vis = draw_dets_and_kpts(frame, dets, kpts_list)

            if writer is not None:
                writer.write(vis)
            if args.display:
                cv2.imshow('realtime', vis)
                if cv2.waitKey(1) & 0xFF == 27:
                    break

            # FPS pacing
            dt = time.time() - t0
            wait = max(0.0, (1.0 / args.target_fps) - dt)
            if wait > 0:
                time.sleep(wait)

    cap.release()
    if writer is not None:
        writer.release()
    if args.display:
        cv2.destroyAllWindows()


if __name__ == '__main__':
    main()



