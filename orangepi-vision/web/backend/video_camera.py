#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
摄像头持续运行模式 - 服务启动时初始化，一直运行
"""
import os
import sys
import time
import cv2
import numpy as np
import math
import threading
from collections import deque

# 添加 deployment_bundle 到路径
BUNDLE_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'deployment_bundle')
if BUNDLE_DIR not in sys.path:
    sys.path.insert(0, BUNDLE_DIR)

from yolov5_nchw_inference_fixed import ACLContext, YOLOv5Model, postprocess as yolo_post
from simple_baselines_om_infer import OMModel

# 导入数据收集器
try:
    from data_collector import collect_gauge_reading
    DATA_COLLECTOR_AVAILABLE = True
except ImportError:
    print("[WARNING] 数据收集器模块未找到，数据收集功能将被禁用")
    DATA_COLLECTOR_AVAILABLE = False


def _iou_xywh(a, b):
    """两个 [x, y, w, h, ...] 框的 IoU（交并比）。"""
    ax1, ay1, aw, ah = a[0], a[1], a[2], a[3]
    bx1, by1, bw, bh = b[0], b[1], b[2], b[3]
    ax2, ay2, bx2, by2 = ax1 + aw, ay1 + ah, bx1 + bw, by1 + bh
    ix1, iy1 = max(ax1, bx1), max(ay1, by1)
    ix2, iy2 = min(ax2, bx2), min(ay2, by2)
    iw, ih = max(0.0, ix2 - ix1), max(0.0, iy2 - iy1)
    inter = iw * ih
    union = aw * ah + bw * bh - inter
    return inter / union if union > 0 else 0.0


def _dedup_overlapping(dets, iou_thresh=0.5):
    """去掉重叠过高的重复框，避免同一仪表出现多个框。
    按 score 降序贪心：若某框与已保留的任一框 IoU > 阈值，则丢弃它（保留 score 更高者）。
    det 结构 = [x, y, w, h, score, cls]（score 在 index 4）。"""
    if len(dets) <= 1:
        return dets
    order = sorted(range(len(dets)), key=lambda i: dets[i][4], reverse=True)
    keep = []
    for idx in order:
        if all(_iou_xywh(dets[idx], dets[k]) <= iou_thresh for k in keep):
            keep.append(idx)
    return [dets[i] for i in keep]


class CameraProcessor:
    """摄像头处理器 - 服务启动时初始化，持续运行"""
    
    def __init__(self, max_gauges=5):
        self.acl_context = None
        self.yolo_model = None
        self.pose_model = None
        self.cap = None
        self.frame_count = 0
        self.fps = 0.0
        self.fps_start = None
        self.initialized = False
        self.max_gauges = max_gauges  # 最多处理多少个仪表（性能考虑）
        self.paused = False  # 暂停标志（释放 NPU 给 DeepSeek）
        self.last_frame_data = None  # 保存最后一帧，用于暂停时显示
        
        # 摄像头重连相关
        self.reconnect_attempts = 0
        self.max_reconnect_attempts = 5
        self.reconnect_delay = 2.0  # 秒
        
        # 激进优化：跳帧策略
        self.skip_frame_interval = 2  # 每2帧处理1帧（跳过1帧）
        self.frame_skip_counter = 0
        
        # 后台线程相关
        self._thread = None
        self._stop_flag = False
        self._frame_queue = deque(maxlen=2)  # 只保留最新的2帧
        self._lock = threading.Lock()
    
    def init(self, video_source="0"):
        """初始化（服务启动时调用一次）- 只打开摄像头，ACL在后台线程中初始化"""
        if self.initialized:
            print("[WARNING] 已初始化，跳过")
            return True
        
        try:
            # 保存视频源配置
            self.video_source = video_source
            
            # 只打开摄像头（不初始化 ACL，避免线程亲和性问题）
            video_src = int(video_source) if video_source.isdigit() else video_source
            self.cap = cv2.VideoCapture(video_src)
            if not self.cap.isOpened():
                raise RuntimeError(f"无法打开摄像头: {video_source}")
            
            # 摄像头优化设置
            self.cap.set(cv2.CAP_PROP_FPS, 30)  # 尝试设置更高帧率
            # 注意：不设置BUFFERSIZE，使用默认值（之前设为1会导致性能下降）
            
            # 激进优化：降低摄像头分辨率以加快读取
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)   # 降低到640
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)  # 降低到480
            
            actual_fps = self.cap.get(cv2.CAP_PROP_FPS)
            actual_width = self.cap.get(cv2.CAP_PROP_FRAME_WIDTH)
            actual_height = self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT)
            print(f"[SUCCESS] 摄像头打开成功: {video_source} (FPS: {actual_fps}, 分辨率: {actual_width}x{actual_height})")
            
            self.fps_start = time.time()
            self.initialized = True
            self.reconnect_attempts = 0  # 重置重连计数
            
            # 启动后台处理线程（ACL 将在线程内初始化）
            self._stop_flag = False
            self._thread = threading.Thread(target=self._background_loop, daemon=True)
            self._thread.start()
            print("[SUCCESS] 后台处理线程已启动（ACL 将在线程内初始化）")
            
            return True
            
        except Exception as e:
            print(f"[ERROR] 初始化失败: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def pause(self):
        """暂停摄像头推理（释放 NPU 内存给 DeepSeek）"""
        self.paused = True
        print("[PAUSE] [摄像头] 已暂停推理，释放 NPU 资源给 DeepSeek")
        # 等待当前帧处理完成
        time.sleep(0.2)
    
    def resume(self):
        """恢复摄像头推理"""
        self.paused = False
        # 重置FPS计时器，避免暂停时间影响FPS计算
        self.fps_start = time.time()
        self.frame_count = 0
        print("[RESUME] [摄像头] 已恢复推理（FPS 计时器已重置）")
    
    def _background_loop(self):
        """后台线程：持续处理摄像头帧"""
        print("[START] [后台线程] 开始运行")
        
        try:
            # 在后台线程中初始化 ACL 和模型（确保线程亲和性）
            print("[LOADING] [后台线程] 初始化 ACL...")
            self.acl_context = ACLContext(device_id=0)
            self.acl_context.__enter__()  # 手动调用上下文管理器的初始化
            print("[SUCCESS] [后台线程] ACL 初始化成功")
            
            print("[LOADING] [后台线程] 加载 YOLO 模型...")
            yolo_model_path = os.path.join(BUNDLE_DIR, "yolo/yolov5s_gauge_nchw_aipp.om")
            self.yolo_model = YOLOv5Model(yolo_model_path)
            self.yolo_model.load()
            print("[SUCCESS] [后台线程] YOLO 模型加载成功")
            
            print("[LOADING] [后台线程] 加载 Pose 模型...")
            pose_model_path = os.path.join(BUNDLE_DIR, "simple_baselines/simple_baselines_256x192_bs1_fp32.om")
            self.pose_model = OMModel(pose_model_path)
            self.pose_model.load()
            print("[SUCCESS] [后台线程] Pose 模型加载成功")
            
            print("[SUCCESS] [后台线程] 所有模型加载完成，开始处理帧")
            
        except Exception as e:
            print(f"[ERROR] [后台线程] 初始化失败: {e}")
            import traceback
            traceback.print_exc()
            self._stop_flag = True
            return
        
        # 主循环：持续处理帧
        while not self._stop_flag:
            try:
                result = self._process_frame_internal()
                if result is not None:
                    with self._lock:
                        self._frame_queue.append(result)
            except Exception as e:
                print(f"[ERROR] [后台线程] 处理帧失败: {e}")
            # 减小 sleep 时间以提高 FPS（0.01秒 = 10ms）
            time.sleep(0.01)
        
        print("[STOP] [后台线程] 已停止")
    
    def get_latest_frame(self):
        """获取最新的帧（非阻塞）"""
        with self._lock:
            if len(self._frame_queue) > 0:
                return self._frame_queue[-1]
            return None
    
    def process_frame(self):
        """获取最新处理好的帧（用于 WebSocket）"""
        return self.get_latest_frame()
    
    def _process_frame_internal(self):
        """内部方法：处理一帧（在后台线程中调用）"""
        if not self.initialized:
            return None
        
        # 如果暂停，返回最后一帧（带暂停提示）
        if self.paused:
            if self.last_frame_data is not None:
                # 复制最后一帧并添加暂停提示
                result = self.last_frame_data.copy()
                vis_frame = result['vis_frame'].copy()
                
                # 添加半透明遮罩
                overlay = vis_frame.copy()
                cv2.rectangle(overlay, (0, 0), (vis_frame.shape[1], 80), (0, 0, 0), -1)
                cv2.addWeighted(overlay, 0.6, vis_frame, 0.4, 0, vis_frame)
                
                # 绘制暂停提示
                cv2.putText(vis_frame, "PAUSED - DeepSeek Processing...", 
                           (20, 50), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 165, 255), 3)
                
                result['vis_frame'] = vis_frame
                result['inference_time_ms'] = 0.0
                result['yolo_time_ms'] = 0.0
                result['pose_time_ms'] = 0.0
                return result
            return None
        
        # 激进优化：跳帧处理
        self.frame_skip_counter += 1
        if self.frame_skip_counter < self.skip_frame_interval:
            # 跳过这一帧，返回上一帧结果
            if self.last_frame_data is not None:
                return self.last_frame_data
            # 如果没有上一帧，继续处理
        else:
            self.frame_skip_counter = 0  # 重置计数器
        
        # 详细性能监控
        t_frame_start = time.time()
        t_read_start = time.time()
        ret, frame = self.cap.read()
        t_read = (time.time() - t_read_start) * 1000
        if not ret:
            # 摄像头读取失败，尝试重连
            print(f"[WARNING] 摄像头读取失败，尝试重连... (尝试 {self.reconnect_attempts + 1}/{self.max_reconnect_attempts})")
            
            if self.reconnect_attempts < self.max_reconnect_attempts:
                self.reconnect_attempts += 1
                
                # 尝试重新打开摄像头
                if self._reconnect_camera():
                    print(f"[SUCCESS] 摄像头重连成功！")
                    self.reconnect_attempts = 0  # 重置计数
                    # 重新读取一帧
                    ret, frame = self.cap.read()
                    if ret:
                        # 继续处理
                        pass
                    else:
                        return None
                else:
                    # 重连失败，等待后重试
                    time.sleep(self.reconnect_delay)
                    return None
            else:
                # 超过最大重连次数，返回错误帧
                print(f"[ERROR] 摄像头重连失败，已达到最大尝试次数")
                return self._create_error_frame("摄像头连接丢失")
        else:
            # 读取成功，重置重连计数
            if self.reconnect_attempts > 0:
                print(f"[SUCCESS] 摄像头恢复正常")
                self.reconnect_attempts = 0
        
        self.frame_count += 1
        h, w = frame.shape[:2]
        
        # YOLO 推理（详细计时）
        t_yolo_preprocess = time.time()
        yolo_input = self._preprocess_yolo(frame)
        yolo_preprocess_time = (time.time() - t_yolo_preprocess) * 1000
        
        t_yolo_infer = time.time()
        yolo_output = self.yolo_model.infer(yolo_input)
        yolo_infer_time = (time.time() - t_yolo_infer) * 1000
        
        t_yolo_post = time.time()
        detections = yolo_post(yolo_output, (w, h), 0.5, 0.6, 1)
        # 二次去重：postprocess 已做 DIoU-NMS，这里再加一道更激进的 IoU 去重，
        # 避免同一仪表出现多个重叠框——两框重叠过高则只留 score 更高的那个
        detections = _dedup_overlapping(detections, iou_thresh=0.5)
        yolo_post_time = (time.time() - t_yolo_post) * 1000
        yolo_time = yolo_preprocess_time + yolo_infer_time + yolo_post_time
        
        # Pose 和角度计算
        t_pose_start = time.time()
        keypoints_list = []
        gauge_angles = []
        
        # 处理所有检测到的仪表（支持多实例，可配置最大数量）
        for det in detections[:self.max_gauges]:
            x, y, ww, hh, _ = det[:5]
            x1, y1 = max(0, int(x)), max(0, int(y))
            x2, y2 = min(w - 1, int(x + ww)), min(h - 1, int(y + hh))
            
            if x2 > x1 and y2 > y1:
                crop = frame[y1:y2, x1:x2]
                pose_input = self._preprocess_pose(crop)
                pose_output = self.pose_model.infer_one(pose_input.reshape(-1))
                J = int(pose_output.size / (1 * 64 * 48)) if pose_output.size % (64 * 48) == 0 else 17
                heatmap = pose_output.reshape(1, J, 64, 48)
                kpts = self._heatmap_to_keypoints(heatmap, (x1, y1, x2 - x1, y2 - y1))
                keypoints_list.append(kpts)
                
                # 计算仪表读数百分比（基于4个关键点 + CW/CCW 方向判断）
                # 正确映射：0=指针, 1=圆心, 2=最小值, 3=最大值
                if len(kpts) >= 4:
                    import math
                    pointer_x, pointer_y, p_conf = kpts[0]
                    center_x, center_y, c_conf = kpts[1]
                    min_x, min_y, min_conf = kpts[2]
                    max_x, max_y, max_conf = kpts[3]
                    
                    if c_conf > 0.3 and min_conf > 0.2 and max_conf > 0.2 and p_conf > 0.3:
                        # 向量与角度计算（图像坐标系 y 向下）
                        vP_x, vP_y = pointer_x - center_x, pointer_y - center_y
                        vM_x, vM_y = min_x - center_x, min_y - center_y
                        vX_x, vX_y = max_x - center_x, max_y - center_y

                        # 计算角度（使用 -y 转换为数学坐标系）
                        theta_P = math.atan2(-vP_y, vP_x)
                        theta_M = math.atan2(-vM_y, vM_x)
                        theta_X = math.atan2(-vX_y, vX_x)

                        # 归一化到 [0, 2π)
                        def normalize_positive(angle):
                            while angle < 0:
                                angle += 2 * math.pi
                            while angle >= 2 * math.pi:
                                angle -= 2 * math.pi
                            return angle

                        theta_P = normalize_positive(theta_P)
                        theta_M = normalize_positive(theta_M)
                        theta_X = normalize_positive(theta_X)

                        # 方向1：逆时针（CCW）
                        delta_total_ccw = normalize_positive(theta_X - theta_M)
                        delta_ptr_ccw = normalize_positive(theta_P - theta_M)

                        # 方向2：顺时针（CW）
                        delta_total_cw = normalize_positive(theta_M - theta_X)
                        delta_ptr_cw = normalize_positive(theta_M - theta_P)

                        # 计算两个方向的 ratio
                        ratio_ccw = delta_ptr_ccw / delta_total_ccw if delta_total_ccw > 0.01 else 0
                        ratio_cw = delta_ptr_cw / delta_total_cw if delta_total_cw > 0.01 else 0

                        # 固定使用 CW (顺时针) 方向
                        ratio = ratio_cw
                        ratio_clamped = max(0.0, min(1.0, ratio))
                        percentage = ratio_clamped * 100.0
                        
                        gauge_angles.append(float(percentage))
                    else:
                        gauge_angles.append(None)
                else:
                    gauge_angles.append(None)
            else:
                keypoints_list.append([])
                gauge_angles.append(None)
        
        pose_time = (time.time() - t_pose_start) * 1000
        
        # 可视化
        t_vis = time.time()
        vis_frame = self._visualize(frame, detections, keypoints_list)
        vis_time = (time.time() - t_vis) * 1000
        
        # 更新 FPS
        if self.frame_count % 10 == 0:
            elapsed = time.time() - self.fps_start
            self.fps = self.frame_count / elapsed if elapsed > 0 else 0
        
        total_time = (time.time() - t_frame_start) * 1000
        
        # 详细性能日志（每30帧输出一次）
        if self.frame_count % 30 == 0:
            print(f"[性能] 帧#{self.frame_count} | FPS:{self.fps:.1f} | "
                  f"总:{total_time:.1f}ms | 读取:{t_read:.1f}ms | "
                  f"YOLO预处理:{yolo_preprocess_time:.1f}ms | "
                  f"YOLO推理:{yolo_infer_time:.1f}ms | "
                  f"YOLO后处理:{yolo_post_time:.1f}ms | "
                  f"Pose:{pose_time:.1f}ms | 可视化:{vis_time:.1f}ms")
        
        result = {
            "frame_id": int(self.frame_count),
            "timestamp": float(time.time()),
            "vis_frame": vis_frame,
            "detections": [[float(x) for x in det] for det in detections],
            # 关键点列表：与 detections[:max_gauges] 一一对应；每项 [(x,y,conf),...]，
            # 顺序按模型实际输出 0=指针 1=圆心 2=零刻度 3=满刻度（main.py 据此组装契约 keypoints[].name）
            "keypoints_list": keypoints_list,
            "gauge_angles": gauge_angles,
            "fps": float(self.fps),
            "inference_time_ms": float(total_time),
            "yolo_time_ms": float(yolo_time),
            "pose_time_ms": float(pose_time)
        }
        
        # 数据收集：保存仪表读数到数据库
        if DATA_COLLECTOR_AVAILABLE and len(gauge_angles) > 0:
            for idx, angle in enumerate(gauge_angles):
                if angle is not None:
                    # 收集数据
                    # 注意：这里假设所有检测到的仪表都是同一类型
                    # 实际应用中，可以根据检测框的类别ID或位置来确定仪表类型
                    gauge_type = "压力表"  # 默认类型，可以根据实际需求修改
                    
                    # 如果有多个仪表，可以根据位置或其他特征区分
                    if len(gauge_angles) > 1:
                        # 多仪表场景：使用索引作为标识
                        gauge_type = f"压力表_{idx + 1}"
                    
                    # 调用数据收集器
                    try:
                        collect_gauge_reading(
                            gauge_type=gauge_type,
                            reading=angle,
                            unit="%",
                            confidence=1.0,  # 如果YOLO检测有置信度，可以传入
                            frame_id=self.frame_count,
                            fps=self.fps,
                            latency=total_time
                        )
                    except Exception as e:
                        # 数据收集失败不影响主流程
                        if self.frame_count % 100 == 0:  # 每100帧打印一次错误
                            print(f"[WARNING] 数据收集失败: {e}")
        
        # 保存最后一帧数据，用于暂停时显示
        self.last_frame_data = result
        
        return result
    
    def _preprocess_yolo(self, bgr):
        # 优化：使用更快的颜色转换和resize
        rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
        # 使用INTER_NEAREST比INTER_LINEAR快，但质量略低（对检测影响小）
        rgb = cv2.resize(rgb, (640, 640), interpolation=cv2.INTER_NEAREST)
        return np.expand_dims(rgb, 0).astype(np.uint8)
    
    def _preprocess_pose(self, bgr):
        # 优化：使用更快的插值方法
        rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
        rgb = cv2.resize(rgb, (192, 256), interpolation=cv2.INTER_NEAREST)  # LINEAR→NEAREST
        x = rgb.astype(np.float32) / 255.0
        mean = np.array([0.485, 0.456, 0.406], dtype=np.float32)
        std = np.array([0.229, 0.224, 0.225], dtype=np.float32)
        x = (x - mean) / std
        x = np.transpose(x, (2, 0, 1))
        return np.expand_dims(x, 0)
    
    def _heatmap_to_keypoints(self, heatmap, crop_xywh):
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
    
    def _reconnect_camera(self):
        """尝试重新连接摄像头"""
        try:
            # 释放旧的摄像头
            if self.cap is not None:
                self.cap.release()
            
            # 重新打开
            video_src = int(self.video_source) if self.video_source.isdigit() else self.video_source
            self.cap = cv2.VideoCapture(video_src)
            
            if self.cap.isOpened():
                self.cap.set(cv2.CAP_PROP_FPS, 30)
                return True
            return False
        except Exception as e:
            print(f"[WARNING] 重连摄像头时出错: {e}")
            return False
    
    def _create_error_frame(self, message):
        """创建错误提示帧"""
        # 创建黑色背景
        error_frame = np.zeros((480, 640, 3), dtype=np.uint8)
        
        # 添加错误消息
        cv2.putText(error_frame, message, 
                   (50, 240), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 0, 255), 2)
        cv2.putText(error_frame, "Attempting to reconnect...", 
                   (50, 280), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 165, 255), 2)
        
        return {
            "frame_id": int(self.frame_count),
            "timestamp": float(time.time()),
            "vis_frame": error_frame,
            "detections": [],
            "gauge_angles": [],
            "fps": 0.0,
            "inference_time_ms": 0.0,
            "yolo_time_ms": 0.0,
            "pose_time_ms": 0.0
        }
    
    def _visualize(self, frame, detections, keypoints_list):
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
            
            # 绘制关键点和计算读数
            if i < len(keypoints_list) and keypoints_list[i]:
                kpts = keypoints_list[i]
                
                # 工业仪表关键点（正确映射）：0=指针, 1=圆心, 2=最小值, 3=最大值
                # 绘制所有关键点（不同颜色）
                colors = [(0, 0, 255), (255, 255, 0), (0, 255, 255), (255, 0, 255)]  # 红、黄、青、品红
                labels = ['Pointer', 'Center', 'Min', 'Max']
                
                for idx, (kx, ky, ks) in enumerate(kpts[:4]):
                    if ks > 0.1:
                        color = colors[idx] if idx < len(colors) else (255, 255, 255)
                        radius = 6 if idx == 0 else 5  # 指针点大一点
                        cv2.circle(vis, (int(kx), int(ky)), radius, color, -1)
                        # 标注关键点
                        if idx < len(labels):
                            cv2.putText(vis, labels[idx], (int(kx)+8, int(ky)-8),
                                       cv2.FONT_HERSHEY_SIMPLEX, 0.45, color, 1)
                
                # 如果有足够的关键点，计算读数百分比
                if len(kpts) >= 4:
                    pointer_x, pointer_y, p_conf = kpts[0]
                    center_x, center_y, c_conf = kpts[1]
                    min_x, min_y, min_conf = kpts[2]
                    max_x, max_y, max_conf = kpts[3]
                    
                    if c_conf > 0.2 and min_conf > 0.15 and max_conf > 0.15 and p_conf > 0.2:
                        import math
                        
                        # 向量与角度计算（图像坐标系 y 向下）
                        vP_x, vP_y = pointer_x - center_x, pointer_y - center_y
                        vM_x, vM_y = min_x - center_x, min_y - center_y
                        vX_x, vX_y = max_x - center_x, max_y - center_y
                        
                        # 计算角度（使用 -y 转换为数学坐标系）
                        theta_P = math.atan2(-vP_y, vP_x)
                        theta_M = math.atan2(-vM_y, vM_x)
                        theta_X = math.atan2(-vX_y, vX_x)
                        
                        # 计算角度（两种方向）
                        # 方向1：逆时针（CCW）- 数学正向
                        delta_total_ccw = theta_X - theta_M
                        delta_ptr_ccw = theta_P - theta_M
                        
                        # 方向2：顺时针（CW）- 很多仪表盘是这样
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
                        
                        # 固定使用 CW (顺时针) 方向，因为大多数仪表盘是顺时针递增
                        ratio = ratio_cw
                        delta_total = delta_total_cw
                        delta_ptr = delta_ptr_cw
                        direction = "CW"
                        
                        ratio_clamped = max(0.0, min(1.0, ratio))
                        percentage = ratio_clamped * 100.0
                        
                        # 绘制辅助线
                        cv2.line(vis, (int(center_x), int(center_y)), (int(min_x), int(min_y)),
                                (0, 255, 255), 1)  # 最小值线（青色）
                        cv2.line(vis, (int(center_x), int(center_y)), (int(max_x), int(max_y)),
                                (255, 0, 255), 1)  # 最大值线（品红）
                        cv2.line(vis, (int(center_x), int(center_y)), (int(pointer_x), int(pointer_y)),
                                (0, 0, 255), 3)  # 指针线（红色，粗）
                        
                        # 显示读数
                        reading_text = f"Reading: {percentage:.1f}%"
                        cv2.putText(vis, reading_text, (p1[0], min(h-10, p2[1] + 25)),
                                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        
        # 不在视频帧上显示 FPS 和 Frame（改由前端 Metrics 面板显示）
        return vis


# 全局实例（服务启动时创建）
camera = CameraProcessor()

