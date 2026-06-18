#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
YOLOv5 NCHW格式 工业仪表检测推理代码（修复版）
适配昇腾NPU环境，基于model_1中的正确预处理和后处理逻辑
"""

import os
import sys
import glob
import argparse
import logging
import numpy as np
import cv2
import acl
import time
from typing import List, Dict, Tuple
from PIL import Image

# 可选的 C++ 加速 NMS
_CPP_NMS_AVAILABLE = False
try:
    import cppops  # type: ignore
    _CPP_NMS_AVAILABLE = True
except Exception:
    # 尝试将项目内 cppops 模块加入路径
    try:
        proj_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
        cppops_dir = os.path.join(proj_root, 'cppops')
        if os.path.isdir(cppops_dir):
            sys.path.insert(0, cppops_dir)
            import cppops  # type: ignore
            _CPP_NMS_AVAILABLE = True
        else:
            _CPP_NMS_AVAILABLE = False
    except Exception:
        _CPP_NMS_AVAILABLE = False

# 配置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# 类别信息
CLASS_NAMES = ['gauge']
NUM_CLASSES = 1

def get_interp_method(interp, sizes=()):
    """Get interpolation method"""
    if interp == 9:
        if sizes:
            assert len(sizes) == 4
            oh, ow, nh, nw = sizes
            if nh > oh and nw > ow:
                return 2  # Bicubic
            if nh < oh and nw < ow:
                return 3  # Area
            return 1  # Bilinear
        return 3  # Area
    return interp

def reshape_data(image, image_size):
    """Reshape image like in transforms.py"""
    if not isinstance(image, Image.Image):
        image = Image.fromarray(image)
    ori_w, ori_h = image.size
    ori_image_shape = np.array([ori_w, ori_h], np.int32)
    
    h, w = image_size
    interp = get_interp_method(interp=9, sizes=(ori_h, ori_w, h, w))
    
    # 使用 PIL 的 resize 方法
    if interp == 1:
        image = image.resize((w, h), Image.BILINEAR)
    elif interp == 2:
        image = image.resize((w, h), Image.BICUBIC)
    elif interp == 3:
        image = image.resize((w, h), Image.LANCZOS)
    else:
        image = image.resize((w, h), Image.NEAREST)
    
    image_data = np.array(image)
    if len(image_data.shape) == 2:
        image_data = np.expand_dims(image_data, axis=-1)
        image_data = np.concatenate([image_data, image_data, image_data], axis=-1)
    
    return image_data, ori_image_shape


def preprocess_image_aipp(image_path, input_shape=(640, 640)):
    # AIPP-compatible preprocessing: output NHWC uint8 RGB for OM model with AIPP
    img = cv2.imread(image_path)
    if img is None:
        return None, None, None
    
    original_img = img.copy()
    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    rgb = cv2.resize(rgb, input_shape, interpolation=cv2.INTER_LINEAR)
    # AIPP model expects NHWC uint8 format
    img_batch = np.expand_dims(rgb, axis=0)  # [1, H, W, 3] uint8
    ori_shape = np.array([original_img.shape[1], original_img.shape[0]], np.int32)
    
    return img_batch, original_img, ori_shape


def preprocess_image_correct(image_path, input_shape=(640, 640)):
    """正确的预处理方式，基于model_1中的逻辑"""
    # 读取图像
    img = cv2.imread(image_path)
    if img is None:
        return None, None, None
    
    original_img = img.copy()
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    
    # 使用 PIL 和 reshape_data 函数
    pil_img = Image.fromarray(img)
    img_processed, ori_shape = reshape_data(pil_img, input_shape)
    
    # 转换为 float32，注意这里不除以255，因为模型内部会处理
    img_float = img_processed.astype(np.float32)
    
    # NHWC to NCHW: [H, W, C] -> [C, H, W]
    img_nchw = np.transpose(img_float, (2, 0, 1))
    # 添加 batch 维度: [C, H, W] -> [1, C, H, W]
    img_batch = np.expand_dims(img_nchw, axis=0)
    
    return img_batch, original_img, ori_shape

def diou_nms(dets, thresh=0.5):
    """DIoU NMS"""
    # 优先调用 C++ 实现
    if _CPP_NMS_AVAILABLE:
        try:
            keep_idx = cppops.diou_nms(dets.astype('float32', copy=False), float(thresh))
            return keep_idx.tolist()
        except Exception:
            pass
    x1 = dets[:, 0]
    y1 = dets[:, 1]
    x2 = x1 + dets[:, 2]
    y2 = y1 + dets[:, 3]
    scores = dets[:, 4]
    areas = (x2 - x1 + 1) * (y2 - y1 + 1)
    order = scores.argsort()[::-1]
    keep = []
    while order.size > 0:
        i = order[0]
        keep.append(i)
        xx1 = np.maximum(x1[i], x1[order[1:]])
        yy1 = np.maximum(y1[i], y1[order[1:]])
        xx2 = np.minimum(x2[i], x2[order[1:]])
        yy2 = np.minimum(y2[i], y2[order[1:]])
        w = np.maximum(0.0, xx2 - xx1 + 1)
        h = np.maximum(0.0, yy2 - yy1 + 1)
        inter = w * h
        ovr = inter / (areas[i] + areas[order[1:]] - inter)
        center_x1 = (x1[i] + x2[i]) / 2
        center_x2 = (x1[order[1:]] + x2[order[1:]]) / 2
        center_y1 = (y1[i] + y2[i]) / 2
        center_y2 = (y1[order[1:]] + y2[order[1:]]) / 2
        inter_diag = (center_x2 - center_x1) ** 2 + (center_y2 - center_y1) ** 2
        out_max_x = np.maximum(x2[i], x2[order[1:]])
        out_max_y = np.maximum(y2[i], y2[order[1:]])
        out_min_x = np.minimum(x1[i], x1[order[1:]])
        out_min_y = np.minimum(y1[i], y1[order[1:]])
        outer_diag = (out_max_x - out_min_x) ** 2 + (out_max_y - out_min_y) ** 2
        diou = ovr - inter_diag / outer_diag
        diou = np.clip(diou, -1, 1)
        inds = np.where(diou <= thresh)[0]
        order = order[inds + 1]
    return keep

def postprocess(outputs, ori_wh, score_thresh=0.3, nms_thresh=0.6, num_classes=1):
    """后处理输出，基于model_1中的逻辑，降低目标置信度阈值"""
    ori_w, ori_h = ori_wh
    all_dets = []
    
    for out in outputs:  # each scale: [1, gy, gx, 3, 5+num_classes]
        o = out[0]  # [gy, gx, 3, 5+num_classes]
        o = o.reshape(-1, 5 + num_classes)
        box_xy = o[:, 0:2]
        box_wh = o[:, 2:4]
        conf = o[:, 4:5]
        probs = o[:, 5:]
        
        # multi-label
        confidence = conf * probs
        cls_ids = np.tile(np.arange(num_classes)[None, :], (confidence.shape[0], 1))
        
        # 降低目标置信度阈值，因为模型输出的目标置信度很低
        mask = (conf > 0.001) & (confidence >= score_thresh)
        ys, xs = np.where(mask)
        
        for idx, cls_idx in zip(ys, xs):
            x_c, y_c, w, h = box_xy[idx, 0] * ori_w, box_xy[idx, 1] * ori_h, box_wh[idx, 0] * ori_w, box_wh[idx, 1] * ori_h
            x1, y1 = max(0.0, x_c - w / 2), max(0.0, y_c - h / 2)
            w, h = min(w, ori_w), min(h, ori_h)
            score = float(confidence[idx, cls_idx])
            all_dets.append([x1, y1, w, h, score, int(cls_idx)])
    
    dets_np = np.array(all_dets, dtype=np.float32) if all_dets else np.zeros((0, 6), dtype=np.float32)
    results = []
    
    if dets_np.shape[0] > 0:
        keep = diou_nms(dets_np[:, :5], thresh=nms_thresh)
        for i in keep:
            x, y, w, h, s = dets_np[i, :5]
            c = int(dets_np[i, 5])
            results.append([x, y, w, h, s, c])
    
    return results

class ACLContext:
    """ACL上下文管理"""
    def __init__(self, device_id: int = 0):
        self.device_id = device_id
        self.ctx = None
        self.stream = None
        
    def __enter__(self):
        ret = acl.init()
        assert ret == 0, f"ACL初始化失败: {ret}"
        
        ret = acl.rt.set_device(self.device_id)
        assert ret == 0, f"设置设备失败: {ret}"
        
        self.ctx, ret = acl.rt.create_context(self.device_id)
        assert ret == 0, f"创建上下文失败: {ret}"
        
        self.stream, ret = acl.rt.create_stream()
        assert ret == 0, f"创建流失败: {ret}"
        
        return self
        
    def __exit__(self, exc_type, exc_val, traceback):
        try:
            if self.stream:
                acl.rt.destroy_stream(self.stream)
            if self.ctx:
                acl.rt.destroy_context(self.ctx)
            acl.rt.reset_device(self.device_id)
        finally:
            acl.finalize()

class YOLOv5Model:
    """YOLOv5模型推理类"""
    def __init__(self, model_path: str):
        self.model_path = model_path
        self.model_id = None
        self.desc = None
        
    def load(self):
        """加载模型"""
        self.model_id, ret = acl.mdl.load_from_file(self.model_path)
        assert ret == 0, f"模型加载失败: {ret}"
        
        self.desc = acl.mdl.create_desc()
        ret = acl.mdl.get_desc(self.desc, self.model_id)
        assert ret == 0, f"获取模型描述失败: {ret}"
        
        logger.info(f"模型加载成功: {self.model_path}")
        
    def infer(self, input_data: np.ndarray) -> List[np.ndarray]:
        """
        执行推理
        Args:
            input_data: 输入数据 [1, 3, 640, 640] NCHW格式
        Returns:
            输出列表
        """
        # 获取输入大小
        input_size = acl.mdl.get_input_size_by_index(self.desc, 0)
        
        # 分配设备内存
        input_buf, ret = acl.rt.malloc(input_size, 0)
        assert ret == 0, f"分配输入内存失败: {ret}"
        
        # 拷贝数据到设备
        ret = acl.rt.memcpy(input_buf, input_size, input_data.ctypes.data, 
                           input_data.nbytes, 1)
        assert ret == 0, f"拷贝输入数据失败: {ret}"
        
        # 创建输入数据集
        input_dataset = acl.mdl.create_dataset()
        input_data_buf = acl.create_data_buffer(input_buf, input_size)
        _, ret = acl.mdl.add_dataset_buffer(input_dataset, input_data_buf)
        assert ret == 0, f"添加输入缓冲区失败: {ret}"
        
        # 创建输出数据集
        output_dataset = acl.mdl.create_dataset()
        output_bufs = []
        output_num = acl.mdl.get_num_outputs(self.desc)
        
        for i in range(output_num):
            output_size = acl.mdl.get_output_size_by_index(self.desc, i)
            output_buf, ret = acl.rt.malloc(output_size, 0)
            assert ret == 0, f"分配输出内存失败: {ret}"
            
            output_bufs.append(output_buf)
            output_data_buf = acl.create_data_buffer(output_buf, output_size)
            _, ret = acl.mdl.add_dataset_buffer(output_dataset, output_data_buf)
            assert ret == 0, f"添加输出缓冲区失败: {ret}"
        
        # 执行推理
        ret = acl.mdl.execute(self.model_id, input_dataset, output_dataset)
        assert ret == 0, f"模型推理失败: {ret}"
        
        # 获取输出结果
        outputs = []
        for i, output_buf in enumerate(output_bufs):
            output_size = acl.mdl.get_output_size_by_index(self.desc, i)
            host_output = np.zeros(output_size // 4, dtype=np.float32)
            
            ret = acl.rt.memcpy(host_output.ctypes.data, output_size, 
                               output_buf, output_size, 2)
            assert ret == 0, f"拷贝输出数据失败: {ret}"
            
            # 重塑输出为正确的形状
            if i == 0:  # 20x20
                host_output = host_output.reshape(1, 20, 20, 3, 6)
            elif i == 1:  # 40x40
                host_output = host_output.reshape(1, 40, 40, 3, 6)
            elif i == 2:  # 80x80
                host_output = host_output.reshape(1, 80, 80, 3, 6)
            
            outputs.append(host_output)
        
        # 清理资源
        acl.rt.free(input_buf)
        acl.destroy_data_buffer(input_data_buf)
        acl.mdl.destroy_dataset(input_dataset)
        
        for output_buf in output_bufs:
            acl.rt.free(output_buf)
        acl.mdl.destroy_dataset(output_dataset)
        
        return outputs

def draw_detections(img_bgr, dets, labels):
    """在图像上绘制检测框，基于model_1中的逻辑"""
    im = img_bgr.copy()
    for x, y, w, h, s, c in dets:
        p1 = (int(x), int(y))
        p2 = (int(x + w), int(y + h))
        cv2.rectangle(im, p1, p2, (0, 255, 0), 2)
        name = labels[c] if c < len(labels) else str(c)
        cv2.putText(im, f"{name}:{s:.2f}", (p1[0], max(0, p1[1]-5)), 
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0,255,0), 2)
    return im

def inference_single_image(model: YOLOv5Model, image_path: str, 
                          conf_thres: float = 0.3, nms_thres: float = 0.6) -> Dict:
    """
    对单张图像进行推理
    Args:
        model: YOLOv5模型
        image_path: 图像路径
        conf_thres: 置信度阈值
        nms_thres: NMS阈值
    Returns:
        推理结果
    """
    # 预处理图像
    img_batch, original_img, ori_shape = preprocess_image_aipp(image_path, (640, 640))
    
    if img_batch is None:
        raise ValueError(f"无法读取图像: {image_path}")
    
    # 执行推理
    start_time = time.time()
    outputs = model.infer(img_batch)
    inference_time = time.time() - start_time
    
    # 后处理
    ori_h, ori_w = original_img.shape[:2]
    dets = postprocess(outputs, (ori_w, ori_h), 
                      score_thresh=conf_thres, 
                      nms_thresh=nms_thres, 
                      num_classes=len(CLASS_NAMES))
    
    return {
        'image': original_img,
        'detections': dets,
        'inference_time': inference_time,
        'image_path': image_path
    }

def main():
    parser = argparse.ArgumentParser(description='YOLOv5 NCHW格式 工业仪表检测推理（修复版）')
    default_root = os.path.abspath(os.path.join(os.path.dirname(__file__), 'yolo'))
    default_om = os.path.join(default_root, 'yolov5s_gauge_nchw_aipp.om')
    parser.add_argument('--om_path', type=str, default=default_om,
                       help='OM模型文件路径')
    parser.add_argument('--img_dir', type=str, default='picture',
                       help='图像目录路径')
    parser.add_argument('--conf_thres', type=float, default=0.3,
                       help='置信度阈值')
    parser.add_argument('--nms_thres', type=float, default=0.6,
                       help='NMS阈值')
    parser.add_argument('--output_dir', type=str, default='results_nchw_fixed',
                       help='输出目录')
    
    args = parser.parse_args()
    
    # 检查输入文件
    if not os.path.exists(args.om_path):
        logger.error(f"OM模型文件不存在: {args.om_path}")
        return
    
    if not os.path.exists(args.img_dir):
        logger.error(f"图像目录不存在: {args.img_dir}")
        return
    
    # 创建输出目录
    os.makedirs(args.output_dir, exist_ok=True)
    
    # 查找图像文件
    image_extensions = ['*.jpg', '*.jpeg', '*.png', '*.bmp']
    image_files = []
    for ext in image_extensions:
        image_files.extend(glob.glob(os.path.join(args.img_dir, ext)))
        image_files.extend(glob.glob(os.path.join(args.img_dir, ext.upper())))
    
    if not image_files:
        logger.error(f"在目录 {args.img_dir} 中未找到图像文件")
        return
    
    logger.info(f"找到 {len(image_files)} 张图像")
    
    # 初始化ACL和模型
    with ACLContext():
        model = YOLOv5Model(args.om_path)
        model.load()
        
        total_time = 0
        total_detections = 0
        
        for image_path in image_files:
            try:
                logger.info(f"处理图像: {image_path}")
                
                # 执行推理
                result = inference_single_image(model, image_path, 
                                              args.conf_thres, args.nms_thres)
                
                # 绘制结果
                output_path = os.path.join(args.output_dir, 
                                         f"result_{os.path.basename(image_path)}")
                result_img = draw_detections(result['image'], result['detections'], CLASS_NAMES)
                cv2.imwrite(output_path, result_img)
                
                # 统计信息
                total_time += result['inference_time']
                total_detections += len(result['detections'])
                
                logger.info(f"检测到 {len(result['detections'])} 个目标, "
                           f"推理时间: {result['inference_time']:.3f}s")
                
            except Exception as e:
                logger.error(f"处理图像 {image_path} 时出错: {e}")
        
        # 输出统计信息
        avg_time = total_time / len(image_files)
        logger.info(f"处理完成! 平均推理时间: {avg_time:.3f}s, "
                   f"总检测数量: {total_detections}")

if __name__ == '__main__':
    main()




