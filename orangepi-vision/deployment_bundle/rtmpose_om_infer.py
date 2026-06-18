#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
RTMPose OM Inference on Ascend NPU
===================================
Replaces Simple Baselines with RTMPose-Tiny for gauge keypoint detection.
Input : [1, 3, 256, 192] NCHW float32
Output: SimCC predictions → 4 keypoint coordinates

RTMPose uses SimCC (Simple Coordinate Classification) instead of heatmaps:
  - simcc_x: [1, 4, 384]  (x-axis classification, 192*2=384 bins)
  - simcc_y: [1, 4, 512]  (y-axis classification, 256*2=512 bins)
  - Keypoint = (argmax(simcc_x)/2, argmax(simcc_y)/2) in input image space

Compared to Simple Baselines heatmap approach:
  - More accurate sub-pixel localization
  - Smaller model (~5MB vs 66MB)
  - Faster inference (~2-3ms vs 8ms)
"""
import os
import sys
import time
import numpy as np
import cv2

# Reuse the ACL context from existing deployment
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


class RTMPoseOMModel:
    """RTMPose OM model wrapper for Ascend NPU inference."""

    # Image normalization (same as mmpose default)
    MEAN = np.array([123.675, 116.28, 103.53], dtype=np.float32)
    STD = np.array([58.395, 57.12, 57.375], dtype=np.float32)
    INPUT_SIZE = (192, 256)  # (W, H)
    SIMCC_SPLIT_RATIO = 2.0

    JOINT_NAMES = ["center", "pointer_tip", "zero_mark", "full_mark"]
    JOINT_COLORS = [
        (0, 0, 255),    # red - center
        (0, 255, 0),    # green - pointer tip
        (255, 0, 0),    # blue - zero mark
        (0, 255, 255),  # yellow - full mark
    ]

    def __init__(self, om_path, num_joints=4):
        self.om_path = om_path
        self.num_joints = num_joints
        self.model = None
        # SimCC output dimensions
        self.simcc_x_dim = int(self.INPUT_SIZE[0] * self.SIMCC_SPLIT_RATIO)  # 384
        self.simcc_y_dim = int(self.INPUT_SIZE[1] * self.SIMCC_SPLIT_RATIO)  # 512

    def load(self):
        """Load OM model via ACL."""
        # Import here to avoid ACL dependency during development
        try:
            # Try to use the existing OMModel wrapper
            parent_dir = os.path.dirname(os.path.abspath(__file__))
            bundle_dir = os.path.join(parent_dir, '..', '..', '..', '..',
                                      'simple_baselines', 'deployment_bundle')
            if os.path.isdir(bundle_dir):
                sys.path.insert(0, os.path.abspath(bundle_dir))
            from simple_baselines_om_infer import OMModel
            self.model = OMModel(self.om_path)
            self.model.load()
        except ImportError:
            raise ImportError(
                "Cannot import OMModel. Make sure simple_baselines_om_infer.py "
                "is in the Python path, or the ACL libraries are available."
            )

    def preprocess(self, img_bgr, bbox_xywh=None):
        """Preprocess image for RTMPose.

        Args:
            img_bgr: BGR image (numpy array)
            bbox_xywh: Optional (x, y, w, h) bounding box to crop

        Returns:
            nchw: [1, 3, 256, 192] float32 input tensor
            meta: dict with center, scale for coordinate mapping
        """
        h, w = img_bgr.shape[:2]

        if bbox_xywh is not None:
            bx, by, bw, bh = bbox_xywh
            bx, by = max(0, int(bx)), max(0, int(by))
            bx2 = min(w, int(bx + int(bw)))
            by2 = min(h, int(by + int(bh)))
            crop = img_bgr[by:by2, bx:bx2].copy()
            meta = {
                'bbox': (bx, by, bx2 - bx, by2 - by),
                'center': np.array([bx + bw / 2.0, by + bh / 2.0]),
                'scale': np.array([float(bw), float(bh)]),
            }
        else:
            crop = img_bgr.copy()
            meta = {
                'bbox': (0, 0, w, h),
                'center': np.array([w / 2.0, h / 2.0]),
                'scale': np.array([float(w), float(h)]),
            }

        # Resize to model input
        resized = cv2.resize(crop, (self.INPUT_SIZE[0], self.INPUT_SIZE[1]),
                             interpolation=cv2.INTER_LINEAR)

        # BGR -> RGB, normalize (mmpose default: subtract mean, divide std)
        rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB).astype(np.float32)
        normalized = (rgb - self.MEAN) / self.STD

        # HWC -> CHW -> NCHW
        chw = np.transpose(normalized, (2, 0, 1))
        nchw = np.expand_dims(chw, axis=0).astype(np.float32)

        return nchw, meta

    def postprocess(self, output_flat, meta):
        """Decode SimCC output to keypoint coordinates.

        RTMPose output: flattened [simcc_x(4*384) + simcc_y(4*512)]
        or alternatively [simcc_x(4*384), simcc_y(4*512)] as two outputs.

        Args:
            output_flat: flat numpy array from OM inference
            meta: preprocessing metadata with bbox info

        Returns:
            keypoints: [num_joints, 3] array of (x, y, confidence) in original image coords
        """
        total_size = output_flat.size
        simcc_x_size = self.num_joints * self.simcc_x_dim  # 4 * 384 = 1536
        simcc_y_size = self.num_joints * self.simcc_y_dim  # 4 * 512 = 2048

        if total_size == simcc_x_size + simcc_y_size:
            # Single concatenated output
            simcc_x = output_flat[:simcc_x_size].reshape(self.num_joints, self.simcc_x_dim)
            simcc_y = output_flat[simcc_x_size:].reshape(self.num_joints, self.simcc_y_dim)
        elif total_size == simcc_x_size:
            # Two separate outputs - this handles just simcc_x
            raise ValueError("Expected concatenated simcc_x + simcc_y output")
        else:
            # Try to detect heatmap output (fallback for Simple Baselines compatibility)
            hm_size = self.num_joints * 64 * 48
            if total_size == hm_size:
                return self._postprocess_heatmap(output_flat, meta)
            raise ValueError(
                "Unexpected output size: {}. Expected {} (SimCC) or {} (heatmap)".format(
                    total_size, simcc_x_size + simcc_y_size, hm_size))

        bbox_x, bbox_y, bbox_w, bbox_h = meta['bbox']
        keypoints = np.zeros((self.num_joints, 3), dtype=np.float32)

        for j in range(self.num_joints):
            # Softmax and argmax for x
            sx = simcc_x[j]
            sx_exp = np.exp(sx - sx.max())
            sx_prob = sx_exp / sx_exp.sum()
            px = int(np.argmax(sx_prob))
            conf_x = float(sx_prob[px])

            # Softmax and argmax for y
            sy = simcc_y[j]
            sy_exp = np.exp(sy - sy.max())
            sy_prob = sy_exp / sy_exp.sum()
            py = int(np.argmax(sy_prob))
            conf_y = float(sy_prob[py])

            # Convert SimCC bin index to input image pixel coordinate
            x_in_input = px / self.SIMCC_SPLIT_RATIO
            y_in_input = py / self.SIMCC_SPLIT_RATIO

            # Map back to original image coordinates
            x_orig = bbox_x + (x_in_input / self.INPUT_SIZE[0]) * bbox_w
            y_orig = bbox_y + (y_in_input / self.INPUT_SIZE[1]) * bbox_h

            confidence = min(conf_x, conf_y)
            keypoints[j] = [x_orig, y_orig, confidence]

        return keypoints

    def _postprocess_heatmap(self, output_flat, meta):
        """Fallback: decode heatmap output (for Simple Baselines OM model)."""
        heatmaps = output_flat.reshape(1, self.num_joints, 64, 48)
        bbox_x, bbox_y, bbox_w, bbox_h = meta['bbox']
        keypoints = np.zeros((self.num_joints, 3), dtype=np.float32)

        for j in range(self.num_joints):
            hm = heatmaps[0, j]
            idx = int(np.argmax(hm))
            py, px = divmod(idx, 48)
            score = float(hm[py, px])

            # Sub-pixel refinement
            if 0 < px < 47 and 0 < py < 63:
                dx = 0.25 * (float(hm[py, px + 1]) - float(hm[py, px - 1]))
                dy = 0.25 * (float(hm[py + 1, px]) - float(hm[py - 1, px]))
                px_r, py_r = px + dx, py + dy
            else:
                px_r, py_r = float(px), float(py)

            x = bbox_x + (px_r / 48) * bbox_w
            y = bbox_y + (py_r / 64) * bbox_h
            keypoints[j] = [x, y, score]

        return keypoints

    def infer(self, img_bgr, bbox_xywh=None):
        """Full inference pipeline: preprocess -> infer -> postprocess.

        Args:
            img_bgr: BGR image
            bbox_xywh: Optional gauge bounding box

        Returns:
            keypoints: [num_joints, 3] array of (x, y, confidence)
            inference_time_ms: inference time in milliseconds
        """
        nchw, meta = self.preprocess(img_bgr, bbox_xywh)

        t0 = time.time()
        flat_input = nchw.reshape(-1).astype(np.float32)
        flat_output = self.model.infer_one(flat_input)
        t1 = time.time()

        keypoints = self.postprocess(flat_output, meta)
        return keypoints, (t1 - t0) * 1000


def compute_gauge_reading(keypoints, full_scale_value=1.6):
    """Compute gauge reading from 4 keypoints using CW angle calculation.

    Args:
        keypoints: [4, 3] array — center, pointer_tip, zero_mark, full_mark
        full_scale_value: the value at full_mark (e.g. 1.6 MPa)

    Returns:
        reading_fraction: fraction of full scale [0, 1]
        actual_value: reading * full_scale_value
    """
    center = keypoints[0, :2]
    pointer = keypoints[1, :2]
    zero = keypoints[2, :2]
    full = keypoints[3, :2]

    def angle_from_center(pt):
        dx = pt[0] - center[0]
        dy = -(pt[1] - center[1])
        return np.degrees(np.arctan2(dy, dx))

    a_pointer = angle_from_center(pointer)
    a_zero = angle_from_center(zero)
    a_full = angle_from_center(full)

    def cw_from_zero(a):
        diff = a_zero - a
        while diff < 0:
            diff += 360
        while diff >= 360:
            diff -= 360
        return diff

    ptr_cw = cw_from_zero(a_pointer)
    full_cw = cw_from_zero(a_full)

    reading = ptr_cw / full_cw if full_cw > 1.0 else 0.0
    reading = max(0.0, min(1.5, reading))

    return reading, reading * full_scale_value


def draw_gauge_keypoints(img_bgr, keypoints, bbox_xywh=None, reading_text=None):
    """Visualize keypoints and gauge reading on image."""
    vis = img_bgr.copy()

    if bbox_xywh is not None:
        bx, by, bw, bh = bbox_xywh
        cv2.rectangle(vis, (int(bx), int(by)), (int(bx + bw), int(by + bh)),
                      (0, 255, 255), 2)

    names = RTMPoseOMModel.JOINT_NAMES
    colors = RTMPoseOMModel.JOINT_COLORS

    for j in range(min(keypoints.shape[0], 4)):
        x, y, conf = keypoints[j]
        if conf < 0.05:
            continue
        color = colors[j]
        name = names[j]
        cx, cy = int(x), int(y)
        cv2.circle(vis, (cx, cy), 5, color, -1, cv2.LINE_AA)
        cv2.circle(vis, (cx, cy), 7, (255, 255, 255), 1, cv2.LINE_AA)
        cv2.putText(vis, "{}({:.2f})".format(name, conf),
                    (cx + 8, cy - 5), cv2.FONT_HERSHEY_SIMPLEX,
                    0.4, color, 1, cv2.LINE_AA)

    # Draw skeleton
    if keypoints.shape[0] >= 4:
        c = keypoints[0]
        for j in [1, 2, 3]:
            if c[2] > 0.05 and keypoints[j, 2] > 0.05:
                pt1 = (int(c[0]), int(c[1]))
                pt2 = (int(keypoints[j, 0]), int(keypoints[j, 1]))
                cv2.line(vis, pt1, pt2, colors[j], 1, cv2.LINE_AA)

    if reading_text:
        cv2.putText(vis, reading_text, (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)

    return vis


# ---- CLI for standalone testing ----
if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='RTMPose OM Gauge Inference')
    parser.add_argument('--image', required=True)
    parser.add_argument('--om', required=True, help='OM model path (RTMPose or SimpleBaselines)')
    parser.add_argument('--bbox', default=None, help='x,y,w,h or omit for full image')
    parser.add_argument('--full_scale', type=float, default=1.6, help='Full scale value')
    parser.add_argument('--out', default='rtmpose_result.jpg')
    args = parser.parse_args()

    # Need ACL context
    bundle_dir = os.path.expanduser('~/simple_baselines/deployment_bundle')
    sys.path.insert(0, bundle_dir)
    from yolov5_nchw_inference_fixed import ACLContext

    img = cv2.imread(args.image)
    assert img is not None

    bbox = None
    if args.bbox:
        bbox = tuple(float(x) for x in args.bbox.split(','))

    with ACLContext():
        model = RTMPoseOMModel(args.om)
        model.load()
        kpts, ms = model.infer(img, bbox)

    print("Inference: {:.1f} ms".format(ms))
    for j in range(kpts.shape[0]):
        print("  {}: ({:.1f}, {:.1f}) conf={:.4f}".format(
            RTMPoseOMModel.JOINT_NAMES[j], kpts[j, 0], kpts[j, 1], kpts[j, 2]))

    if all(kpts[j, 2] > 0.05 for j in range(4)):
        frac, val = compute_gauge_reading(kpts, args.full_scale)
        text = "{:.3f} MPa ({:.1f}%)".format(val, frac * 100)
        print("Reading:", text)
    else:
        text = None

    vis = draw_gauge_keypoints(img, kpts, bbox, text)
    cv2.imwrite(args.out, vis)
    print("Saved:", args.out)
