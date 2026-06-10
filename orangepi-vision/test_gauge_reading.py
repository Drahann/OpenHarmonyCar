#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试仪表读数计算 - 单张图片详细分析
"""
import os
import sys
import cv2
import numpy as np
import math

# 添加路径
BUNDLE_DIR = os.path.join(os.path.dirname(__file__), 'deployment_bundle')
sys.path.insert(0, BUNDLE_DIR)

from yolov5_nchw_inference_fixed import ACLContext, YOLOv5Model, postprocess as yolo_post
from simple_baselines_om_infer import OMModel


def preprocess_yolo(bgr):
    """YOLO 预处理"""
    rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    rgb = cv2.resize(rgb, (640, 640), interpolation=cv2.INTER_LINEAR)
    return np.expand_dims(rgb, 0).astype(np.uint8)


def preprocess_pose(bgr):
    """Pose 预处理"""
    rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    rgb = cv2.resize(rgb, (192, 256), interpolation=cv2.INTER_LINEAR)
    x = rgb.astype(np.float32) / 255.0
    mean = np.array([0.485, 0.456, 0.406], dtype=np.float32)
    std = np.array([0.229, 0.224, 0.225], dtype=np.float32)
    x = (x - mean) / std
    x = np.transpose(x, (2, 0, 1))
    return np.expand_dims(x, 0)


def heatmap_to_keypoints(heatmap, crop_xywh):
    """热图转关键点"""
    _, J, H, W = heatmap.shape
    crop_x, crop_y, crop_w, crop_h = crop_xywh
    sx, sy = crop_w / 192.0, crop_h / 256.0
    kpts = []
    for j in range(J):
        hm = heatmap[0, j]
        idx = int(np.argmax(hm))
        py, px = divmod(idx, W)
        score = float(hm[py, px])
        x = crop_x + (px + 0.5) * (192.0 / W) * sx
        y = crop_y + (py + 0.5) * (256.0 / H) * sy
        kpts.append((float(x), float(y), score))
    return kpts


def normalize_angle(delta):
    """归一化角度差到 (-π, π]"""
    while delta <= -math.pi:
        delta += 2 * math.pi
    while delta > math.pi:
        delta -= 2 * math.pi
    return delta


def calculate_gauge_reading(kpts, frame_shape):
    """计算仪表读数"""
    print("\n" + "="*80)
    print("📊 仪表读数计算详细过程")
    print("="*80)
    
    if len(kpts) < 4:
        print(f"❌ 关键点数量不足: {len(kpts)} < 4")
        return None
    
    # 关键点映射：0=指针, 1=圆心, 2=最小值, 3=最大值
    pointer_x, pointer_y, p_conf = kpts[0]
    center_x, center_y, c_conf = kpts[1]
    min_x, min_y, min_conf = kpts[2]
    max_x, max_y, max_conf = kpts[3]
    
    print(f"\n1️⃣ 关键点坐标和置信度:")
    print(f"   Pointer (红色): ({pointer_x:.1f}, {pointer_y:.1f}) - 置信度: {p_conf:.3f}")
    print(f"   Center  (黄色): ({center_x:.1f}, {center_y:.1f}) - 置信度: {c_conf:.3f}")
    print(f"   Min     (青色): ({min_x:.1f}, {min_y:.1f}) - 置信度: {min_conf:.3f}")
    print(f"   Max     (品红): ({max_x:.1f}, {max_y:.1f}) - 置信度: {max_conf:.3f}")
    
    # 检查置信度
    if c_conf < 0.2 or min_conf < 0.15 or max_conf < 0.15 or p_conf < 0.2:
        print(f"\n❌ 置信度过低，跳过计算")
        return None
    
    # 计算向量
    print(f"\n2️⃣ 计算向量（从圆心出发）:")
    vP_x, vP_y = pointer_x - center_x, pointer_y - center_y
    vM_x, vM_y = min_x - center_x, min_y - center_y
    vX_x, vX_y = max_x - center_x, max_y - center_y
    
    print(f"   vPointer = ({vP_x:.1f}, {vP_y:.1f})")
    print(f"   vMin     = ({vM_x:.1f}, {vM_y:.1f})")
    print(f"   vMax     = ({vX_x:.1f}, {vX_y:.1f})")
    
    # 计算角度（图像坐标系 y 向下，使用 -y 转换为数学坐标系）
    print(f"\n3️⃣ 计算角度（使用 atan2(-y, x) 转换为数学坐标系）:")
    theta_P = math.atan2(-vP_y, vP_x)
    theta_M = math.atan2(-vM_y, vM_x)
    theta_X = math.atan2(-vX_y, vX_x)
    
    print(f"   θPointer = atan2({-vP_y:.1f}, {vP_x:.1f}) = {math.degrees(theta_P):7.2f}°")
    print(f"   θMin     = atan2({-vM_y:.1f}, {vM_x:.1f}) = {math.degrees(theta_M):7.2f}°")
    print(f"   θMax     = atan2({-vX_y:.1f}, {vX_x:.1f}) = {math.degrees(theta_X):7.2f}°")
    
    # 计算角度（两种方向）
    print(f"\n4️⃣ 计算角度差（两个方向）:")
    
    # 方向1：逆时针（CCW）
    delta_total_ccw = theta_X - theta_M
    delta_ptr_ccw = theta_P - theta_M
    
    # 方向2：顺时针（CW）
    delta_total_cw = theta_M - theta_X
    delta_ptr_cw = theta_M - theta_P
    
    # 归一化到 [0, 2π)
    def normalize_positive(angle):
        while angle < 0:
            angle += 2 * math.pi
        while angle >= 2 * math.pi:
            angle -= 2 * math.pi
        return angle
    
    delta_total_ccw = normalize_positive(delta_total_ccw)
    delta_ptr_ccw = normalize_positive(delta_ptr_ccw)
    delta_total_cw = normalize_positive(delta_total_cw)
    delta_ptr_cw = normalize_positive(delta_ptr_cw)
    
    # 计算两个方向的 ratio
    ratio_ccw = delta_ptr_ccw / delta_total_ccw if delta_total_ccw > 0.01 else 0
    ratio_cw = delta_ptr_cw / delta_total_cw if delta_total_cw > 0.01 else 0
    
    print(f"   CCW (逆时针): Δtotal={math.degrees(delta_total_ccw):.1f}° Δptr={math.degrees(delta_ptr_ccw):.1f}° ratio={ratio_ccw:.4f}")
    print(f"   CW  (顺时针): Δtotal={math.degrees(delta_total_cw):.1f}° Δptr={math.degrees(delta_ptr_cw):.1f}° ratio={ratio_cw:.4f}")
    
    # 固定使用 CW (顺时针) 方向，因为大多数仪表盘是顺时针递增
    ratio = ratio_cw
    delta_total = delta_total_cw
    delta_ptr = delta_ptr_cw
    direction = "CW (顺时针)"
    print(f"   → 固定使用 CW 方向")
    
    ratio_clamped = max(0.0, min(1.0, ratio))
    
    print(f"\n5️⃣ 最终计算:")
    print(f"   选择方向: {direction}")
    print(f"   ratio = Δptr / Δtotal = {math.degrees(delta_ptr):.2f}° / {math.degrees(delta_total):.2f}° = {ratio:.4f}")
    print(f"   ratio_clamped = {ratio_clamped:.4f} (限制在 [0, 1])")
    
    # 百分比
    percentage = ratio_clamped * 100.0
    print(f"\n6️⃣ 最终读数:")
    print(f"   percentage = ratio_clamped × 100% = {percentage:.2f}%")
    print("="*80 + "\n")
    
    return {
        'percentage': percentage,
        'ratio': ratio_clamped,
        'theta_P': theta_P,
        'theta_M': theta_M,
        'theta_X': theta_X,
        'delta_total': delta_total,
        'delta_ptr': delta_ptr
    }


def visualize_result(frame, detections, keypoints_list, result):
    """可视化结果"""
    vis = frame.copy()
    h, w = vis.shape[:2]
    
    for i, det in enumerate(detections):
        if len(det) < 5:
            continue
        x, y, ww, hh, score = det[:5]
        p1, p2 = (int(x), int(y)), (int(x + ww), int(y + hh))
        
        # 绘制检测框
        cv2.rectangle(vis, p1, p2, (0, 255, 0), 2)
        cv2.putText(vis, f"Gauge: {score:.2f}", (p1[0], max(0, p1[1] - 5)),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        
        # 绘制关键点
        if i < len(keypoints_list) and keypoints_list[i]:
            kpts = keypoints_list[i]
            colors = [(0, 0, 255), (255, 255, 0), (0, 255, 255), (255, 0, 255)]
            labels = ['Pointer', 'Center', 'Min', 'Max']
            
            for idx, (kx, ky, ks) in enumerate(kpts[:4]):
                if ks > 0.1:
                    color = colors[idx] if idx < len(colors) else (255, 255, 255)
                    radius = 8 if idx == 0 else 6
                    cv2.circle(vis, (int(kx), int(ky)), radius, color, -1)
                    if idx < len(labels):
                        cv2.putText(vis, f"{labels[idx]}({ks:.2f})", (int(kx)+10, int(ky)-10),
                                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
            
            # 绘制辅助线
            if len(kpts) >= 4:
                center = (int(kpts[1][0]), int(kpts[1][1]))
                min_pt = (int(kpts[2][0]), int(kpts[2][1]))
                max_pt = (int(kpts[3][0]), int(kpts[3][1]))
                ptr_pt = (int(kpts[0][0]), int(kpts[0][1]))
                
                cv2.line(vis, center, min_pt, (0, 255, 255), 2)
                cv2.line(vis, center, max_pt, (255, 0, 255), 2)
                cv2.line(vis, center, ptr_pt, (0, 0, 255), 3)
                
                # 显示读数
                if result:
                    cv2.putText(vis, f"Reading: {result['percentage']:.1f}%", 
                               (p1[0], min(h-10, p2[1] + 30)),
                               cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
    
    return vis


def main():
    # 图片路径
    import sys
    if len(sys.argv) > 1:
        img_path = sys.argv[1]
    else:
        img_path = "/home/HwHiAiUser/simple_baselines/real_image/test_image_real.jpg"
    
    # 模型路径
    yolo_om = "/home/HwHiAiUser/simple_baselines/deployment_bundle/yolo/yolov5s_gauge_nchw_aipp.om"
    pose_om = "/home/HwHiAiUser/simple_baselines/deployment_bundle/simple_baselines/simple_baselines_256x192_bs1_fp32.om"
    
    print("📷 加载图片...")
    frame = cv2.imread(img_path)
    if frame is None:
        print(f"❌ 无法读取图片: {img_path}")
        return
    
    h, w = frame.shape[:2]
    print(f"✅ 图片大小: {w} x {h}\n")
    
    print("🚀 初始化 ACL 和模型...")
    with ACLContext():
        # 加载 YOLO
        yolo_model = YOLOv5Model(yolo_om)
        yolo_model.load()
        print("✅ YOLO 模型加载完成")
        
        # 加载 Pose
        pose_model = OMModel(pose_om)
        pose_model.load()
        print("✅ Pose 模型加载完成\n")
        
        # YOLO 推理
        print("="*80)
        print("🔍 YOLO 检测")
        print("="*80)
        yolo_input = preprocess_yolo(frame)
        print(f"输入形状: {yolo_input.shape}")
        yolo_output = yolo_model.infer(yolo_input)
        detections = yolo_post(yolo_output, (w, h), 0.3, 0.6, 1)
        
        print(f"\n检测结果: {len(detections)} 个目标")
        for i, det in enumerate(detections):
            x, y, ww, hh, score = det[:5]
            print(f"  [{i}] bbox: ({x:.1f}, {y:.1f}, {ww:.1f}, {hh:.1f}) - 置信度: {score:.3f}")
        
        # Pose 推理
        print("\n" + "="*80)
        print("🦴 Pose 估计")
        print("="*80)
        
        keypoints_list = []
        for i, det in enumerate(detections[:1]):
            x, y, ww, hh, _ = det[:5]
            x1, y1 = max(0, int(x)), max(0, int(y))
            x2, y2 = min(w - 1, int(x + ww)), min(h - 1, int(y + hh))
            
            print(f"\n目标 [{i}] 裁剪区域: ({x1}, {y1}) -> ({x2}, {y2})")
            
            if x2 > x1 and y2 > y1:
                crop = frame[y1:y2, x1:x2]
                print(f"裁剪大小: {crop.shape}")
                
                pose_input = preprocess_pose(crop)
                print(f"Pose 输入形状: {pose_input.shape}")
                
                pose_output = pose_model.infer_one(pose_input.reshape(-1))
                J = int(pose_output.size / (1 * 64 * 48)) if pose_output.size % (64 * 48) == 0 else 17
                heatmap = pose_output.reshape(1, J, 64, 48)
                print(f"热图形状: {heatmap.shape}")
                
                kpts = heatmap_to_keypoints(heatmap, (x1, y1, x2 - x1, y2 - y1))
                keypoints_list.append(kpts)
                
                print(f"\n关键点数量: {len(kpts)}")
                print(f"关键点详情 (x, y, confidence):")
                for j, (kx, ky, ks) in enumerate(kpts):
                    print(f"  [{j:2d}] ({kx:7.2f}, {ky:7.2f}) - conf: {ks:.3f}")
        
        # 计算读数
        result = None
        if keypoints_list and len(keypoints_list[0]) >= 4:
            result = calculate_gauge_reading(keypoints_list[0], frame.shape)
        
        # 可视化
        print("🎨 生成可视化结果...")
        vis_frame = visualize_result(frame, detections, keypoints_list, result)
        
        # 保存（根据输入文件名）
        base_name = os.path.splitext(os.path.basename(img_path))[0]
        output_path = f"/home/HwHiAiUser/simple_baselines/test_result_{base_name}.jpg"
        cv2.imwrite(output_path, vis_frame)
        print(f"✅ 结果已保存: {output_path}")


if __name__ == "__main__":
    main()

