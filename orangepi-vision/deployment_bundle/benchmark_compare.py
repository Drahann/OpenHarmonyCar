#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Benchmark: RTMPose vs Simple Baselines on Ascend NPU
=====================================================
Compares inference speed, model size, and keypoint quality.

Usage (on Orange Pi):
  python3 benchmark_compare.py \
    --simple_baselines_om /path/to/simple_baselines_256x192_bs1_fp32.om \
    --rtmpose_om /path/to/rtmpose_t_gauge_4kp_256x192.om \
    --image /path/to/test_image.jpg \
    --yolo_om /path/to/yolov5s_gauge_nchw_aipp.om
"""
import os
import sys
import time
import numpy as np
import cv2

sys.path.insert(0, os.path.expanduser('~/simple_baselines/deployment_bundle'))
from simple_baselines_om_infer import OMModel
from yolov5_nchw_inference_fixed import ACLContext, YOLOv5Model, postprocess
from rtmpose_om_infer import RTMPoseOMModel, compute_gauge_reading


def benchmark_model(model_obj, input_data, n_warmup=5, n_iter=50):
    """Run inference benchmark."""
    flat = input_data.reshape(-1).astype(np.float32)

    for _ in range(n_warmup):
        model_obj.infer_one(flat)

    times = []
    for _ in range(n_iter):
        t0 = time.time()
        out = model_obj.infer_one(flat)
        t1 = time.time()
        times.append((t1 - t0) * 1000)

    times = np.array(times)
    return {
        'mean_ms': times.mean(),
        'median_ms': np.median(times),
        'p95_ms': np.percentile(times, 95),
        'min_ms': times.min(),
        'max_ms': times.max(),
        'fps': 1000.0 / times.mean(),
        'output': out,
    }


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--simple_baselines_om', required=True)
    parser.add_argument('--rtmpose_om', default=None,
                        help='RTMPose OM (skip if not yet trained)')
    parser.add_argument('--image', required=True)
    parser.add_argument('--yolo_om', default=None)
    parser.add_argument('--n_iter', type=int, default=50)
    args = parser.parse_args()

    img = cv2.imread(args.image)
    assert img is not None
    h, w = img.shape[:2]

    with ACLContext():
        # Detect gauge bbox with YOLO
        bbox = None
        if args.yolo_om and os.path.exists(args.yolo_om):
            yolo = YOLOv5Model(args.yolo_om)
            yolo.load()
            rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            rgb = cv2.resize(rgb, (640, 640))
            nhwc = np.expand_dims(rgb, axis=0)
            yolo_outs = yolo.infer(nhwc)
            dets = postprocess(yolo_outs, (w, h), score_thresh=0.3, nms_thresh=0.6, num_classes=1)
            if dets:
                bbox = tuple(dets[0][:4])
                print("YOLO detected gauge: bbox={}".format(bbox))

        # Prepare input
        dummy_nchw = np.random.randn(1, 3, 256, 192).astype(np.float32)

        print("\n" + "=" * 60)
        print("  MODEL BENCHMARK ({} iterations)".format(args.n_iter))
        print("=" * 60)

        # Simple Baselines
        print("\n--- Simple Baselines (ResNet50 + Deconv) ---")
        sb_model = OMModel(args.simple_baselines_om)
        sb_model.load()
        sb_size = os.path.getsize(args.simple_baselines_om) / (1024 * 1024)
        sb_result = benchmark_model(sb_model, dummy_nchw, n_iter=args.n_iter)
        print("  Model size : {:.1f} MB".format(sb_size))
        print("  Mean       : {:.2f} ms".format(sb_result['mean_ms']))
        print("  Median     : {:.2f} ms".format(sb_result['median_ms']))
        print("  P95        : {:.2f} ms".format(sb_result['p95_ms']))
        print("  FPS        : {:.1f}".format(sb_result['fps']))

        # Decode Simple Baselines output
        sb_wrapper = RTMPoseOMModel(args.simple_baselines_om, num_joints=4)
        sb_wrapper.model = sb_model
        if bbox:
            sb_kpts, _ = sb_wrapper.infer(img, bbox)
        else:
            sb_kpts, _ = sb_wrapper.infer(img)
        print("  Keypoints:")
        for j in range(sb_kpts.shape[0]):
            print("    {}: ({:.1f}, {:.1f}) conf={:.4f}".format(
                RTMPoseOMModel.JOINT_NAMES[j],
                sb_kpts[j, 0], sb_kpts[j, 1], sb_kpts[j, 2]))

        if all(sb_kpts[j, 2] > 0.05 for j in range(4)):
            frac, val = compute_gauge_reading(sb_kpts)
            print("  Reading: {:.3f} MPa ({:.1f}%)".format(val, frac * 100))

        # RTMPose (if available)
        if args.rtmpose_om and os.path.exists(args.rtmpose_om):
            print("\n--- RTMPose-Tiny (CSPNeXt + SimCC) ---")
            rt_model = OMModel(args.rtmpose_om)
            rt_model.load()
            rt_size = os.path.getsize(args.rtmpose_om) / (1024 * 1024)
            rt_result = benchmark_model(rt_model, dummy_nchw, n_iter=args.n_iter)
            print("  Model size : {:.1f} MB".format(rt_size))
            print("  Mean       : {:.2f} ms".format(rt_result['mean_ms']))
            print("  Median     : {:.2f} ms".format(rt_result['median_ms']))
            print("  P95        : {:.2f} ms".format(rt_result['p95_ms']))
            print("  FPS        : {:.1f}".format(rt_result['fps']))

            # Comparison
            print("\n--- COMPARISON ---")
            print("  Speed: {:.1f}x faster".format(
                sb_result['mean_ms'] / rt_result['mean_ms']))
            print("  Size:  {:.1f}x smaller".format(sb_size / rt_size))
        else:
            print("\n[INFO] RTMPose OM not provided. Run training + export first.")
            print("  See: ../rtmpose/train.sh")

    print("\n" + "=" * 60)


if __name__ == '__main__':
    main()
