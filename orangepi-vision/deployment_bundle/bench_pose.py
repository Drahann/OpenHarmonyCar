#!/usr/bin/env python3
"""Simple Baselines OM performance benchmark."""
import sys, time, numpy as np
sys.path.insert(0, '.')
from simple_baselines_om_infer import OMModel
from yolov5_nchw_inference_fixed import ACLContext

ctx = ACLContext()
ctx.__enter__()

pose = OMModel('./simple_baselines/simple_baselines_256x192_bs1_fp32.om')
pose.load()

dummy = np.random.randn(1, 3, 256, 192).astype(np.float32)

# Warmup
for _ in range(5):
    pose.infer_one(dummy.reshape(-1))

# Benchmark
N = 50
times = []
for i in range(N):
    t0 = time.time()
    out = pose.infer_one(dummy.reshape(-1))
    t1 = time.time()
    times.append((t1 - t0) * 1000)

times = np.array(times)
print()
print("Simple Baselines OM Benchmark ({} iters)".format(N))
print("  Mean  : {:.2f} ms".format(times.mean()))
print("  Median: {:.2f} ms".format(np.median(times)))
print("  Std   : {:.2f} ms".format(times.std()))
print("  Min   : {:.2f} ms".format(times.min()))
print("  Max   : {:.2f} ms".format(times.max()))
print("  P95   : {:.2f} ms".format(np.percentile(times, 95)))
print("  P99   : {:.2f} ms".format(np.percentile(times, 99)))
print("  FPS   : {:.1f}".format(1000.0 / times.mean()))

ctx.__exit__(None, None, None)
