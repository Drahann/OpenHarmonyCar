#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simple Baselines OM Model - Standalone Debug Script
====================================================
Tests the pose estimation model independently:
1. Load test image
2. (Optional) Run YOLO to get gauge bbox, or use full image
3. Preprocess -> OM inference -> heatmap analysis
4. Detailed per-joint diagnostics
5. Visualization with heatmap overlays
"""
import os
import sys
import time
import numpy as np
import cv2

# ACL imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from simple_baselines_om_infer import OMModel

# ---- Constants ----
JOINT_NAMES = ["center", "pointer_tip", "zero_mark", "full_mark"]
JOINT_COLORS = [
    (0, 0, 255),    # red - center
    (0, 255, 0),    # green - pointer tip
    (255, 0, 0),    # blue - zero mark
    (0, 255, 255),  # yellow - full mark
]
IMAGE_SIZE = (192, 256)   # (W, H) model input
HEATMAP_SIZE = (48, 64)   # (W, H) heatmap output
IMAGENET_MEAN = np.array([0.485, 0.456, 0.406], dtype=np.float32)
IMAGENET_STD  = np.array([0.229, 0.224, 0.225], dtype=np.float32)


def preprocess_for_pose(img_bgr, bbox_xywh=None):
    """Preprocess image for Simple Baselines model.

    If bbox_xywh is given, crop that region first.
    Returns: (input_nchw, center, scale, crop_bgr)
    """
    h, w = img_bgr.shape[:2]

    if bbox_xywh is not None:
        bx, by, bw, bh = bbox_xywh
        bx, by = max(0, int(bx)), max(0, int(by))
        bw_int, bh_int = int(bw), int(bh)
        bx2 = min(w, bx + bw_int)
        by2 = min(h, by + bh_int)
        crop = img_bgr[by:by2, bx:bx2].copy()
        center = np.array([bx + bw/2.0, by + bh/2.0], dtype=np.float32)
        scale_w, scale_h = float(bw), float(bh)
    else:
        crop = img_bgr.copy()
        center = np.array([w/2.0, h/2.0], dtype=np.float32)
        scale_w, scale_h = float(w), float(h)

    # Resize crop to model input size
    resized = cv2.resize(crop, (IMAGE_SIZE[0], IMAGE_SIZE[1]), interpolation=cv2.INTER_LINEAR)

    # BGR -> RGB, normalize
    rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB).astype(np.float32) / 255.0
    normalized = (rgb - IMAGENET_MEAN) / IMAGENET_STD

    # HWC -> CHW -> NCHW
    chw = np.transpose(normalized, (2, 0, 1))
    nchw = np.expand_dims(chw, axis=0).astype(np.float32)

    scale = np.array([scale_w, scale_h], dtype=np.float32)
    return nchw, center, scale, crop


def heatmap_to_keypoints_simple(heatmaps, bbox_xywh=None, img_shape=None):
    """Convert heatmaps [1, J, H, W] to keypoint coordinates.

    Returns: keypoints [J, 3] with (x, y, confidence)
    """
    _, J, H, W = heatmaps.shape
    keypoints = np.zeros((J, 3), dtype=np.float32)

    for j in range(J):
        hm = heatmaps[0, j]
        idx = int(np.argmax(hm))
        py, px = divmod(idx, W)
        score = float(hm[py, px])

        # Sub-pixel refinement (shift by gradient of neighboring pixels)
        if 0 < px < W-1 and 0 < py < H-1:
            dx = 0.25 * (float(hm[py, px+1]) - float(hm[py, px-1]))
            dy = 0.25 * (float(hm[py+1, px]) - float(hm[py-1, px]))
            px_refined = px + dx
            py_refined = py + dy
        else:
            px_refined = float(px)
            py_refined = float(py)

        if bbox_xywh is not None:
            bx, by, bw, bh = bbox_xywh
            x = bx + (px_refined / W) * bw
            y = by + (py_refined / H) * bh
        elif img_shape is not None:
            ih, iw = img_shape[:2]
            x = (px_refined / W) * iw
            y = (py_refined / H) * ih
        else:
            x = px_refined * (IMAGE_SIZE[0] / W)
            y = py_refined * (IMAGE_SIZE[1] / H)

        keypoints[j] = [x, y, score]

    return keypoints


def analyze_heatmaps(heatmaps):
    """Print detailed heatmap analysis."""
    _, J, H, W = heatmaps.shape
    print("\n" + "="*60)
    print("  HEATMAP ANALYSIS")
    print("  Shape: {}  (batch, joints, H, W)".format(heatmaps.shape))
    print("="*60)

    for j in range(J):
        hm = heatmaps[0, j]
        idx = int(np.argmax(hm))
        py, px = divmod(idx, W)
        peak = float(hm[py, px])

        name = JOINT_NAMES[j] if j < len(JOINT_NAMES) else "joint_{}".format(j)
        print("\n  Joint {}: {}".format(j, name))
        print("    Peak position : ({}, {}) on {}x{} grid".format(px, py, W, H))
        print("    Peak value    : {:.6f}".format(peak))
        print("    Mean          : {:.6f}".format(float(hm.mean())))
        print("    Std           : {:.6f}".format(float(hm.std())))
        print("    Min           : {:.6f}".format(float(hm.min())))
        print("    Max           : {:.6f}".format(float(hm.max())))
        print("    >0.5 pixels   : {}".format(int((hm > 0.5).sum())))
        print("    >0.1 pixels   : {}".format(int((hm > 0.1).sum())))

        # Check if heatmap is meaningful
        snr = peak / (float(hm.std()) + 1e-8)
        if peak > 0.3 and snr > 5:
            quality = "GOOD"
        elif peak > 0.1:
            quality = "WEAK"
        else:
            quality = "BAD"
        print("    SNR (peak/std): {:.2f}  [{}]".format(snr, quality))

    print("\n" + "="*60 + "\n")


def save_heatmap_visualization(heatmaps, crop_bgr, out_path):
    """Save heatmap overlay visualization."""
    _, J, H, W = heatmaps.shape

    # Create a grid: original + each joint heatmap + combined
    cols = J + 2
    cell_w, cell_h = 200, 266
    canvas = np.zeros((cell_h, cell_w * cols, 3), dtype=np.uint8)

    # Original crop
    resized_crop = cv2.resize(crop_bgr, (cell_w, cell_h))
    canvas[:, :cell_w] = resized_crop
    cv2.putText(canvas, "Original", (5, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

    # Each joint heatmap
    combined_heat = np.zeros((H, W), dtype=np.float32)
    for j in range(J):
        hm = heatmaps[0, j]
        combined_heat = np.maximum(combined_heat, hm)

        hm_norm = (hm - hm.min()) / (hm.max() - hm.min() + 1e-8)
        hm_u8 = (hm_norm * 255).astype(np.uint8)
        hm_color = cv2.applyColorMap(hm_u8, cv2.COLORMAP_JET)
        hm_resized = cv2.resize(hm_color, (cell_w, cell_h))

        crop_resized = cv2.resize(crop_bgr, (cell_w, cell_h))
        overlay = cv2.addWeighted(crop_resized, 0.4, hm_resized, 0.6, 0)

        col = j + 1
        canvas[:, col*cell_w:(col+1)*cell_w] = overlay
        name = JOINT_NAMES[j] if j < len(JOINT_NAMES) else "J{}".format(j)
        peak = float(hm.max())
        cv2.putText(canvas, "{}:{:.2f}".format(name, peak), (col*cell_w+5, 20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)

    # Combined
    comb_norm = (combined_heat - combined_heat.min()) / (combined_heat.max() - combined_heat.min() + 1e-8)
    comb_u8 = (comb_norm * 255).astype(np.uint8)
    comb_color = cv2.applyColorMap(comb_u8, cv2.COLORMAP_JET)
    comb_resized = cv2.resize(comb_color, (cell_w, cell_h))
    crop_resized = cv2.resize(crop_bgr, (cell_w, cell_h))
    overlay = cv2.addWeighted(crop_resized, 0.4, comb_resized, 0.6, 0)
    col = J + 1
    canvas[:, col*cell_w:(col+1)*cell_w] = overlay
    cv2.putText(canvas, "Combined", (col*cell_w+5, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

    cv2.imwrite(out_path, canvas)
    print("  Heatmap visualization saved: {}".format(out_path))


def draw_keypoints_on_image(img_bgr, keypoints, bbox_xywh=None):
    """Draw keypoints (and optional bbox) on image."""
    vis = img_bgr.copy()

    if bbox_xywh is not None:
        bx, by, bw, bh = bbox_xywh
        cv2.rectangle(vis, (int(bx), int(by)), (int(bx+bw), int(by+bh)), (0, 255, 255), 2)

    for j in range(keypoints.shape[0]):
        x, y, conf = keypoints[j]
        if conf < 0.05:
            continue
        color = JOINT_COLORS[j] if j < len(JOINT_COLORS) else (128, 128, 128)
        name = JOINT_NAMES[j] if j < len(JOINT_NAMES) else "J{}".format(j)
        cx, cy = int(x), int(y)
        cv2.circle(vis, (cx, cy), 5, color, -1, cv2.LINE_AA)
        cv2.circle(vis, (cx, cy), 7, (255, 255, 255), 1, cv2.LINE_AA)
        label = "{}({:.2f})".format(name, conf)
        cv2.putText(vis, label, (cx+8, cy-5), cv2.FONT_HERSHEY_SIMPLEX, 0.45, color, 1, cv2.LINE_AA)

    # Draw connections: center -> pointer_tip, center -> zero, center -> full
    if keypoints.shape[0] >= 4:
        center_pt = keypoints[0]
        for j in [1, 2, 3]:
            if center_pt[2] > 0.05 and keypoints[j, 2] > 0.05:
                pt1 = (int(center_pt[0]), int(center_pt[1]))
                pt2 = (int(keypoints[j, 0]), int(keypoints[j, 1]))
                color = JOINT_COLORS[j]
                cv2.line(vis, pt1, pt2, color, 1, cv2.LINE_AA)

    return vis


def compute_gauge_reading(keypoints, full_scale_value=1.6):
    """Compute gauge reading from 4 keypoints.

    Keypoints: center(0), pointer_tip(1), zero_mark(2), full_mark(3)
    Gauge needles sweep CLOCKWISE from zero to full.
    In image coordinates (y-down), CW = mathematically decreasing angle.

    Returns: reading as fraction of full scale [0, 1]
    """
    center = keypoints[0, :2]
    pointer = keypoints[1, :2]
    zero = keypoints[2, :2]
    full = keypoints[3, :2]

    # Compute math-convention angles (CCW from +x, y-inverted for image coords)
    def angle_from_center(pt):
        dx = pt[0] - center[0]
        dy = -(pt[1] - center[1])  # invert y for math convention
        return np.degrees(np.arctan2(dy, dx))

    a_pointer = angle_from_center(pointer)
    a_zero = angle_from_center(zero)
    a_full = angle_from_center(full)

    # CW angle from zero mark (gauges sweep clockwise)
    def cw_from_zero(a):
        diff = a_zero - a  # CW = decreasing math angle
        while diff < 0:
            diff += 360
        while diff >= 360:
            diff -= 360
        return diff

    ptr_cw = cw_from_zero(a_pointer)
    full_cw = cw_from_zero(a_full)

    reading = ptr_cw / full_cw if full_cw > 1.0 else 0.0
    reading = max(0.0, min(1.5, reading))  # allow slight overshoot for debug
    actual_value = reading * full_scale_value

    print("\n  --- Gauge Reading Calculation ---")
    print("  Center     : ({:.1f}, {:.1f})".format(center[0], center[1]))
    print("  Pointer tip: ({:.1f}, {:.1f})".format(pointer[0], pointer[1]))
    print("  Zero mark  : ({:.1f}, {:.1f})".format(zero[0], zero[1]))
    print("  Full mark  : ({:.1f}, {:.1f})".format(full[0], full[1]))
    print("  Math angles: pointer={:.1f}, zero={:.1f}, full={:.1f} deg".format(a_pointer, a_zero, a_full))
    print("  CW from zero: pointer={:.1f}, full={:.1f} deg".format(ptr_cw, full_cw))
    print("  Angular span (zero->full CW): {:.1f} deg".format(full_cw))
    print("  Reading: {:.4f} ({:.1f}% FS)".format(reading, reading*100))
    print("  Value  : {:.3f} MPa (assuming {:.1f} MPa FS)".format(actual_value, full_scale_value))

    return reading


def run_yolo_detection(image_path, yolo_om_path):
    """Run YOLO to detect gauge bounding boxes."""
    from yolov5_nchw_inference_fixed import YOLOv5Model, postprocess

    img = cv2.imread(image_path)
    h, w = img.shape[:2]

    # Preprocess for AIPP model (NHWC uint8)
    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    rgb = cv2.resize(rgb, (640, 640), interpolation=cv2.INTER_LINEAR)
    nhwc = np.expand_dims(rgb, axis=0)

    yolo = YOLOv5Model(yolo_om_path)
    yolo.load()
    yolo_outs = yolo.infer(nhwc)
    dets = postprocess(yolo_outs, (w, h), score_thresh=0.3, nms_thresh=0.6, num_classes=1)

    print("\n  YOLO detected {} gauge(s)".format(len(dets)))
    for i, det in enumerate(dets):
        x, y, ww, hh, s = det[:5]
        print("    [{}] bbox=({:.0f},{:.0f},{:.0f},{:.0f}) conf={:.3f}".format(i, x, y, ww, hh, s))

    return dets


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Simple Baselines OM Debug")
    parser.add_argument("--image", required=True, help="Test image path")
    parser.add_argument("--pose_om", default=None, help="Pose OM model path")
    parser.add_argument("--yolo_om", default=None, help="YOLO OM model path (optional)")
    parser.add_argument("--bbox", default=None, help="Manual bbox x,y,w,h (skip YOLO)")
    parser.add_argument("--out_dir", default="./debug_pose_output", help="Output directory")
    args = parser.parse_args()

    # Defaults
    base_dir = os.path.dirname(os.path.abspath(__file__))
    if args.pose_om is None:
        args.pose_om = os.path.join(base_dir, "simple_baselines", "simple_baselines_256x192_bs1_fp32.om")
    if args.yolo_om is None:
        args.yolo_om = os.path.join(base_dir, "yolo", "yolov5s_gauge_nchw_aipp.om")

    assert os.path.exists(args.image), "Image not found: {}".format(args.image)
    assert os.path.exists(args.pose_om), "Pose OM not found: {}".format(args.pose_om)

    os.makedirs(args.out_dir, exist_ok=True)

    img = cv2.imread(args.image)
    assert img is not None, "Failed to read: {}".format(args.image)
    h, w = img.shape[:2]
    print("\n  Input image: {}".format(args.image))
    print("  Image size : {} x {}".format(w, h))

    # Initialize ACL context (required for all OM model operations)
    from yolov5_nchw_inference_fixed import ACLContext, YOLOv5Model, postprocess
    acl_ctx = ACLContext()
    acl_ctx.__enter__()
    print("  ACL context initialized.")

    # Determine bounding boxes
    bboxes = []
    if args.bbox:
        parts = [float(x) for x in args.bbox.split(",")]
        bboxes = [tuple(parts)]
        print("  Using manual bbox: {}".format(bboxes[0]))
    elif os.path.exists(args.yolo_om):
        print("\n  Running YOLO detection...")
        dets = run_yolo_detection(args.image, args.yolo_om)
        for det in dets:
            bboxes.append(tuple(det[:4]))
        if not bboxes:
            print("  No detections! Using full image as fallback.")
            bboxes = [(0, 0, w, h)]
    else:
        print("  No YOLO model found, using full image.")
        bboxes = [(0, 0, w, h)]

    # Load pose model
    print("\n  Loading pose model: {}".format(args.pose_om))
    pose_model = OMModel(args.pose_om)
    pose_model.load()
    print("  Model loaded successfully.")

    # Process each detected gauge
    for idx, bbox in enumerate(bboxes):
        print("\n" + "#"*60)
        print("  Processing gauge #{}  bbox={}".format(idx, bbox))
        print("#"*60)

        bx, by, bw, bh = bbox
        bx1, by1 = max(0, int(bx)), max(0, int(by))
        bx2 = min(w, int(bx + bw))
        by2 = min(h, int(by + bh))
        crop = img[by1:by2, bx1:bx2].copy()

        # Preprocess
        nchw, center, scale, crop_vis = preprocess_for_pose(img, bbox_xywh=bbox)
        print("  Input tensor shape: {}".format(nchw.shape))
        print("  Input tensor dtype: {}".format(nchw.dtype))
        print("  Input bytes: {}".format(nchw.nbytes))

        # Inference
        t0 = time.time()
        flat_input = nchw.reshape(-1).astype(np.float32)
        flat_output = pose_model.infer_one(flat_input)
        t1 = time.time()

        # Reshape output
        num_joints = flat_output.size // (64 * 48)
        heatmaps = flat_output.reshape(1, num_joints, 64, 48)

        print("  Inference time: {:.1f} ms".format((t1-t0)*1000))
        print("  Output shape  : {}".format(heatmaps.shape))
        print("  Num joints    : {}".format(num_joints))

        # Analyze heatmaps
        analyze_heatmaps(heatmaps)

        # Get keypoints
        kpts = heatmap_to_keypoints_simple(heatmaps, bbox_xywh=bbox, img_shape=img.shape)
        print("  Keypoint coordinates (on original image):")
        for j in range(kpts.shape[0]):
            name = JOINT_NAMES[j] if j < len(JOINT_NAMES) else "joint_{}".format(j)
            print("    {:15s}: ({:.1f}, {:.1f})  conf={:.4f}".format(name, kpts[j,0], kpts[j,1], kpts[j,2]))

        # Compute gauge reading
        if num_joints >= 4 and all(kpts[j, 2] > 0.05 for j in range(4)):
            reading = compute_gauge_reading(kpts)
        else:
            print("\n  [WARN] Not all keypoints detected with sufficient confidence, skip reading.")

        # Save visualizations
        vis = draw_keypoints_on_image(img, kpts, bbox_xywh=bbox)
        out_kp = os.path.join(args.out_dir, "gauge_{}_keypoints.jpg".format(idx))
        cv2.imwrite(out_kp, vis)
        print("\n  Keypoint vis saved: {}".format(out_kp))

        # Heatmap overlay
        out_hm = os.path.join(args.out_dir, "gauge_{}_heatmaps.jpg".format(idx))
        save_heatmap_visualization(heatmaps, crop, out_hm)

        # Individual heatmaps
        for j in range(num_joints):
            hm = heatmaps[0, j]
            hm_norm = ((hm - hm.min()) / (hm.max() - hm.min() + 1e-8) * 255).astype(np.uint8)
            jname = JOINT_NAMES[j] if j < len(JOINT_NAMES) else str(j)
            out_j = os.path.join(args.out_dir, "gauge_{}_hm_j{}_{}.png".format(idx, j, jname))
            cv2.imwrite(out_j, hm_norm)

    # Cleanup ACL context
    acl_ctx.__exit__(None, None, None)

    print("\n  All done! Results in: {}".format(args.out_dir))
    print("  Total gauges processed: {}".format(len(bboxes)))


if __name__ == "__main__":
    main()
