#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
YOLOv5 AIPP OM 批量推理（NHWC uint8 640x640），不修改原文件。
输入: --om_path --img_dir --output_dir
输出: 在输出目录保存画框结果
"""
import os
import glob
import argparse
import logging
import numpy as np
import cv2

from yolov5_nchw_inference_fixed import ACLContext, YOLOv5Model, postprocess

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


def preprocess_aipp_batch(image_path: str, size=(640, 640)):
    bgr = cv2.imread(image_path)
    if bgr is None:
        raise FileNotFoundError(image_path)
    rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    rgb = cv2.resize(rgb, size, interpolation=cv2.INTER_LINEAR)
    nhwc_u8 = np.expand_dims(rgb, axis=0)  # NHWC uint8
    return bgr, nhwc_u8


def draw_dets(bgr: np.ndarray, dets):
    vis = bgr.copy()
    for d in dets:
        if len(d) >= 5:
            x, y, w, h, s = d[:5]
            p1 = (int(x), int(y))
            p2 = (int(x + w), int(y + h))
            cv2.rectangle(vis, p1, p2, (0, 255, 0), 2)
            cv2.putText(vis, f"obj:{s:.2f}", (p1[0], max(0, p1[1]-5)), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0,255,0), 2)
    return vis


def main():
    parser = argparse.ArgumentParser(description='YOLOv5 AIPP OM 批量推理 (NHWC uint8)')
    default_root = os.path.abspath(os.path.join(os.path.dirname(__file__), 'yolo'))
    default_om = os.path.join(default_root, 'yolov5s_gauge_nchw_aipp.om')
    parser.add_argument('--om_path', default=default_om)
    parser.add_argument('--img_dir', required=True)
    parser.add_argument('--output_dir', required=True)
    parser.add_argument('--conf_thres', type=float, default=0.3)
    parser.add_argument('--nms_thres', type=float, default=0.6)
    args = parser.parse_args()

    assert os.path.exists(args.om_path), f"not found: {args.om_path}"
    assert os.path.exists(args.img_dir), f"not found: {args.img_dir}"
    os.makedirs(args.output_dir, exist_ok=True)

    images = []
    for ext in ('*.jpg','*.jpeg','*.png','*.bmp'):
        images.extend(glob.glob(os.path.join(args.img_dir, ext)))
        images.extend(glob.glob(os.path.join(args.img_dir, ext.upper())))
    images = sorted(images)
    logger.info(f"找到 {len(images)} 张图像")
    if not images:
        return

    with ACLContext():
        model = YOLOv5Model(args.om_path)
        model.load()
        total_dets = 0
        for img_path in images:
            try:
                bgr, nhwc = preprocess_aipp_batch(img_path, (640, 640))
                outs = model.infer(nhwc)
                h, w = bgr.shape[:2]
                dets = postprocess(outs, (w, h), args.conf_thres, args.nms_thres, 1)
                total_dets += len(dets)
                vis = draw_dets(bgr, dets)
                out_path = os.path.join(args.output_dir, os.path.basename(img_path))
                cv2.imwrite(out_path, vis)
                logger.info(f"{os.path.basename(img_path)} -> dets={len(dets)}")
            except Exception as e:
                logger.error(f"{img_path} 失败: {e}")
        logger.info(f"完成，累计检测框: {total_dets}")


if __name__ == '__main__':
    main()







