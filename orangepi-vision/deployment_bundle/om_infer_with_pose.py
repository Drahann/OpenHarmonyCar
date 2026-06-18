#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
不修改原文件的前提下：
先用 YOLO OM 做检测拿到边界框，再用 simple_baselines OM 在框内做关键点推理并可视化。
"""
import os
import argparse
import numpy as np
import cv2
from typing import List

from yolov5_nchw_inference_fixed import ACLContext, YOLOv5Model, postprocess
from simple_baselines_om_infer import OMModel


def preprocess_image_aipp_batched(image_path: str, size=(640, 640)):
    img = cv2.imread(image_path)
    if img is None:
        raise FileNotFoundError(image_path)
    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    rgb = cv2.resize(rgb, size, interpolation=cv2.INTER_LINEAR)
    nhwc_u8 = np.expand_dims(rgb, axis=0)  # NHWC uint8
    return img, nhwc_u8


def draw_detections(image: np.ndarray, detections: List[List[float]], title: str = ""):
    vis = image.copy()
    if title:
        cv2.putText(vis, title, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
    for i, det in enumerate(detections):
        if len(det) >= 5:
            x, y, w, h, score = det[:5]
            x1, y1 = int(x), int(y)
            x2, y2 = int(x + w), int(y + h)
            cv2.rectangle(vis, (x1, y1), (x2, y2), (0, 255, 0), 2)
            cv2.putText(vis, f"obj:{score:.2f}", (x1, max(0, y1 - 5)), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
    return vis


def pose_preprocess_bgr(crop_bgr: np.ndarray, dst_hw=(256, 192)) -> np.ndarray:
    h, w = dst_hw
    img = cv2.cvtColor(crop_bgr, cv2.COLOR_BGR2RGB)
    img = cv2.resize(img, (w, h), interpolation=cv2.INTER_LINEAR)
    img = img.astype(np.float32) / 255.0
    mean = np.array([0.485, 0.456, 0.406], dtype=np.float32)
    std = np.array([0.229, 0.224, 0.225], dtype=np.float32)
    img = (img - mean) / std
    img = np.transpose(img, (2, 0, 1))  # CHW
    img = np.expand_dims(img, axis=0)   # NCHW
    return img


def heatmap_to_keypoints(heatmap: np.ndarray, crop_xywh, dst_hw=(256, 192)):
    _, J, H, W = heatmap.shape
    keypoints = []
    crop_x, crop_y, crop_w, crop_h = crop_xywh
    sx = crop_w / float(dst_hw[1])  # width scale
    sy = crop_h / float(dst_hw[0])  # height scale
    for j in range(J):
        hm = heatmap[0, j]
        idx = int(np.argmax(hm))
        py, px = divmod(idx, W)
        score = float(hm[py, px])
        x_on_crop = (px + 0.5) * (dst_hw[1] / W)
        y_on_crop = (py + 0.5) * (dst_hw[0] / H)
        x = crop_x + x_on_crop * sx
        y = crop_y + y_on_crop * sy
        keypoints.append((float(x), float(y), score))
    return keypoints


def main():
    parser = argparse.ArgumentParser(description='YOLO OM 检测 + SimpleBaselines OM 关键点')
    default_root = os.path.dirname(__file__)
    default_yolo = os.path.join(default_root, 'yolo', 'yolov5s_gauge_nchw_aipp.om')
    default_pose = os.path.join(default_root, 'simple_baselines', 'simple_baselines_256x192_bs1_fp32.om')
    parser.add_argument('--yolo_om', default=default_yolo, help='YOLO OM 路径 (AIPP NHWC uint8)')
    parser.add_argument('--pose_om', default=default_pose, help='SimpleBaselines OM 路径 (NCHW float32 1x3x256x192)')
    parser.add_argument('--image', required=True, help='输入图像路径')
    parser.add_argument('--out', required=True, help='输出图像路径')
    args = parser.parse_args()

    assert os.path.exists(args.yolo_om), f"not found: {args.yolo_om}"
    assert os.path.exists(args.pose_om), f"not found: {args.pose_om}"
    assert os.path.exists(args.image), f"not found: {args.image}"

    bgr, yolo_in = preprocess_image_aipp_batched(args.image, (640, 640))

    with ACLContext():
        # YOLO 推理
        yolo = YOLOv5Model(args.yolo_om)
        yolo.load()
        yolo_outs = yolo.infer(yolo_in)
        h, w = bgr.shape[:2]
        dets = postprocess(yolo_outs, (w, h), score_thresh=0.3, nms_thresh=0.6, num_classes=1)

        vis = draw_detections(bgr, dets, title=f"YOLO det {len(dets)}")

        # Pose 推理
        pose = OMModel(args.pose_om)
        pose.load()
        for det in dets:
            x, y, ww, hh, s = det[:5]
            x1, y1 = max(0, int(x)), max(0, int(y))
            x2, y2 = min(w - 1, int(x + ww)), min(h - 1, int(y + hh))
            if x2 <= x1 or y2 <= y1:
                continue
            crop = bgr[y1:y2, x1:x2]
            pose_in = pose_preprocess_bgr(crop, (256, 192))
            out_flat = pose.infer_one(pose_in.reshape(-1))
            J = int(out_flat.size / (1 * 64 * 48))
            heat = out_flat.reshape(1, J, 64, 48)
            kpts = heatmap_to_keypoints(heat, (x1, y1, x2 - x1, y2 - y1))
            for (kx, ky, ks) in kpts:
                cv2.circle(vis, (int(kx), int(ky)), 3, (0, 0, 255), -1)

        os.makedirs(os.path.dirname(args.out), exist_ok=True)
        cv2.imwrite(args.out, vis)
        print(f"saved: {args.out}")


if __name__ == '__main__':
    main()







