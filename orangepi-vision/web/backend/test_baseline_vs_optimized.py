"""简单的性能对比测试 - 基准版本 vs 优化版本"""
import os
import time

# 启用推理时间记录
os.environ['INFERENCE_TIME_RECORD'] = 'True'

# 测试问题
TEST_QUESTION = "什么是压力表？请简单介绍一下。"

print("="*70)
print("DeepSeek 性能对比测试")
print("="*70)

# ============================================================
# 测试1: 基准版本（无优化）
# ============================================================
print("\n【测试1】基准版本（无任何优化）")
print("-"*70)

from deepseek_baseline import DEEPSEEK_BASELINE

print("\n加载模型...")
load_start = time.time()
DEEPSEEK_BASELINE.ensure_loaded()
load_time = time.time() - load_start
print(f"✓ 模型加载完成: {load_time:.2f}秒\n")

print(f"问题: {TEST_QUESTION}\n")
print("开始生成:")
print("-"*70)

baseline_result = DEEPSEEK_BASELINE.generate(TEST_QUESTION)

print("\n基准版本统计:")
print(f"  - 总耗时: {baseline_result['elapsed_time']:.2f}秒")
print(f"  - 生成tokens: {baseline_result['tokens_generated']}")
print(f"  - 平均: {baseline_result['avg_time_per_token']:.3f}秒/token")
print(f"  - 吞吐量: {baseline_result['tokens_generated']/baseline_result['elapsed_time']:.2f} tokens/s")
print(f"  - 回复长度: {len(baseline_result['reply'])}字符")

# ============================================================
# 测试2: JIT优化版本
# ============================================================
print("\n\n【测试2】JIT优化版本")
print("-"*70)

from deepseek_service import DEEPSEEK

print("\n加载模型...")
load_start = time.time()
DEEPSEEK.ensure_loaded()
load_time = time.time() - load_start
print(f"✓ 模型加载完成: {load_time:.2f}秒")

if not DEEPSEEK.state.jit_compiled:
    print("\n⚠️  首次推理将触发JIT编译（约140秒），请耐心等待...\n")

print(f"问题: {TEST_QUESTION}\n")
print("开始生成（每个token的时间）:")
print("-"*70)

optimized_start = time.time()

for chunk in DEEPSEEK.generate(TEST_QUESTION, []):
    if chunk.get("done"):
        optimized_total = time.time() - optimized_start
        optimized_result = chunk
        break

print("\nJIT优化版本统计:")
print(f"  - 总耗时: {optimized_result['elapsed_time']:.2f}秒")
print(f"  - 生成tokens: {optimized_result['tokens_generated']}")
print(f"  - 平均: {optimized_result['avg_time_per_token']:.3f}秒/token")
print(f"  - 吞吐量: {optimized_result['tokens_generated']/optimized_result['elapsed_time']:.2f} tokens/s")
print(f"  - 回复长度: {len(optimized_result['reply'])}字符")
print(f"  - 首Token: {optimized_result['first_token_time']:.3f}秒")

# ============================================================
# 对比结果
# ============================================================
print("\n\n" + "="*70)
print("性能对比结果")
print("="*70)

speedup_token = baseline_result['avg_time_per_token'] / optimized_result['avg_time_per_token']
speedup_throughput = (optimized_result['tokens_generated']/optimized_result['elapsed_time']) / (baseline_result['tokens_generated']/baseline_result['elapsed_time'])

print(f"\n每Token速度:")
print(f"  基准版本: {baseline_result['avg_time_per_token']:.3f}秒/token")
print(f"  优化版本: {optimized_result['avg_time_per_token']:.3f}秒/token")
print(f"  加速比: {speedup_token:.2f}x")

print(f"\n吞吐量:")
print(f"  基准版本: {baseline_result['tokens_generated']/baseline_result['elapsed_time']:.2f} tokens/s")
print(f"  优化版本: {optimized_result['tokens_generated']/optimized_result['elapsed_time']:.2f} tokens/s")
print(f"  提升: {(speedup_throughput-1)*100:.1f}%")

print(f"\n总耗时:")
print(f"  基准版本: {baseline_result['elapsed_time']:.2f}秒")
print(f"  优化版本: {optimized_result['elapsed_time']:.2f}秒")
print(f"  节省: {baseline_result['elapsed_time'] - optimized_result['elapsed_time']:.2f}秒 ({(1-optimized_result['elapsed_time']/baseline_result['elapsed_time'])*100:.1f}%)")

print("\n" + "="*70)
print("测试完成")
print("="*70)

