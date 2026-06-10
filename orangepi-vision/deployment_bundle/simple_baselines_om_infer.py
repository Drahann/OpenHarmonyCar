#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import sys
import glob
import argparse
import logging
import numpy as np
import acl


logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class ACLContext:
    def __init__(self, device_id: int = 0):
        self.device_id = device_id
        self.ctx = None
        self.stream = None

    def __enter__(self):
        ret = acl.init()
        assert ret == 0, f"ACL init failed: {ret}"
        ret = acl.rt.set_device(self.device_id)
        assert ret == 0, f"set device failed: {ret}"
        self.ctx, ret = acl.rt.create_context(self.device_id)
        assert ret == 0, f"create context failed: {ret}"
        self.stream, ret = acl.rt.create_stream()
        assert ret == 0, f"create stream failed: {ret}"
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


class OMModel:
    def __init__(self, om_path: str):
        self.om_path = om_path
        self.model_id = None
        self.desc = None

    def load(self):
        self.model_id, ret = acl.mdl.load_from_file(self.om_path)
        assert ret == 0, f"load model failed: {ret}"
        self.desc = acl.mdl.create_desc()
        ret = acl.mdl.get_desc(self.desc, self.model_id)
        assert ret == 0, f"get model desc failed: {ret}"
        logger.info(f"模型加载成功: {self.om_path}")

    def infer_one(self, input_array: np.ndarray) -> np.ndarray:
        # 输入检查
        assert input_array.flags['C_CONTIGUOUS'], "input_array must be C contiguous"
        input_bytes = input_array.nbytes
        expected_size = acl.mdl.get_input_size_by_index(self.desc, 0)
        assert input_bytes == expected_size, f"input bytes {input_bytes} != model expected {expected_size}"

        # 分配输入设备内存并拷贝
        input_buf, ret = acl.rt.malloc(expected_size, 0)
        assert ret == 0, f"malloc input failed: {ret}"
        ret = acl.rt.memcpy(input_buf, expected_size, input_array.ctypes.data, input_bytes, 1)
        assert ret == 0, f"memcpy input failed: {ret}"

        # 构建输入数据集
        input_dataset = acl.mdl.create_dataset()
        input_data_buf = acl.create_data_buffer(input_buf, expected_size)
        _, ret = acl.mdl.add_dataset_buffer(input_dataset, input_data_buf)
        assert ret == 0, f"add input buffer failed: {ret}"

        # 构建输出数据集（本模型仅一个输出）
        output_dataset = acl.mdl.create_dataset()
        output_size = acl.mdl.get_output_size_by_index(self.desc, 0)
        output_buf, ret = acl.rt.malloc(output_size, 0)
        assert ret == 0, f"malloc output failed: {ret}"
        output_data_buf = acl.create_data_buffer(output_buf, output_size)
        _, ret = acl.mdl.add_dataset_buffer(output_dataset, output_data_buf)
        assert ret == 0, f"add output buffer failed: {ret}"

        # 执行
        ret = acl.mdl.execute(self.model_id, input_dataset, output_dataset)
        assert ret == 0, f"execute failed: {ret}"

        # 拷贝输出到主机
        host_out = np.empty(output_size // 4, dtype=np.float32)
        ret = acl.rt.memcpy(host_out.ctypes.data, output_size, output_buf, output_size, 2)
        assert ret == 0, f"memcpy output failed: {ret}"

        # 资源释放
        acl.rt.free(input_buf)
        acl.destroy_data_buffer(input_data_buf)
        acl.mdl.destroy_dataset(input_dataset)

        acl.rt.free(output_buf)
        acl.destroy_data_buffer(output_data_buf)
        acl.mdl.destroy_dataset(output_dataset)

        return host_out


def main():
    parser = argparse.ArgumentParser(description='Simple-Baselines OM 推理（读取 bin，输出 bin）')
    default_root = os.path.abspath(os.path.join(os.path.dirname(__file__), 'simple_baselines'))
    default_om = os.path.join(default_root, 'simple_baselines_256x192_bs1_fp32.om')
    parser.add_argument('--om_path', type=str, default=default_om, help='OM 模型路径')
    parser.add_argument('--input_dir', type=str, required=True, help='输入 bin 目录（NCHW float32）')
    parser.add_argument('--output_dir', type=str, required=True, help='输出结果目录')
    parser.add_argument('--device_id', type=int, default=0, help='设备 ID')
    parser.add_argument('--input_shape', type=str, default='1,3,256,192', help='输入形状，默认 1,3,256,192')
    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)

    # 收集输入 bin
    bins = sorted(glob.glob(os.path.join(args.input_dir, '*.bin')))
    if not bins:
        logger.error(f"未在 {args.input_dir} 找到 bin 输入")
        sys.exit(1)

    n, c, h, w = [int(x) for x in args.input_shape.split(',')]
    elem_num = n * c * h * w
    byte_size = elem_num * 4

    with ACLContext(args.device_id):
        model = OMModel(args.om_path)
        model.load()

        # 校验模型输入大小
        model_in_size = acl.mdl.get_input_size_by_index(model.desc, 0)
        if model_in_size != byte_size:
            logger.error(f"输入大小不匹配: from_arg={byte_size}, model={model_in_size}")
            sys.exit(1)

        for fp in bins:
            base = os.path.basename(fp)
            # 输出文件名需与 postprocess.py 兼容，如 sp_bs1_0.bin -> sp_bs1_0_0.bin
            # 实际 C++ 写法为在原名后追加 _0.bin
            if base.endswith('.bin'):
                out_name = base[:-4] + '_0.bin'
            else:
                out_name = base + '_0.bin'
            out_fp = os.path.join(args.output_dir, out_name)

            data = np.fromfile(fp, dtype=np.float32)
            if data.nbytes != byte_size:
                logger.error(f"输入 {base} 大小不符: {data.nbytes} != {byte_size}")
                sys.exit(1)

            # 直接按 NCHW float32 原样送入
            host_out = model.infer_one(data)
            host_out.tofile(out_fp)
            logger.info(f"完成: {base} -> {out_name}")

    logger.info("全部输入推理完成。")


if __name__ == '__main__':
    main()


