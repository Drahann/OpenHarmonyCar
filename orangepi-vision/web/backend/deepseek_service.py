"""DeepSeek service wrapper for FastAPI backend with JIT optimization.

JIT优化版本 - 使用MindSpore静态图编译加速推理
参考: guide.txt 第五章 推理JIT优化
"""

from __future__ import annotations

import os
import time
import threading
import numpy as np
from dataclasses import dataclass, field
from typing import List, Dict, Any

# 启用推理时间记录
os.environ['INFERENCE_TIME_RECORD'] = 'True'

try:
    import mindspore
    from mindnlp.transformers import AutoModelForCausalLM, AutoTokenizer, StaticCache
    from mindnlp.core import ops
    from mindnlp.configs import set_pyboost
    _HAS_MINDNLP = True
    
    # 【JIT优化】配置MindSpore为图模式 + O2级别优化
    try:
        mindspore.set_context(
            mode=mindspore.GRAPH_MODE,  # 使用图模式进行静态编译
            device_target="Ascend",
            jit_config={"jit_level": "O2"},  # O2级别JIT优化
        )
        print("[SUCCESS] [JIT优化] MindSpore图模式已启用 (jit_level=O2)")
    except Exception as e:
        print(f"[WARNING] [JIT优化] 配置失败: {e}")
        try:
            mindspore.set_context(mode=mindspore.PYNATIVE_MODE, device_target="Ascend")
            print("[WARNING] [降级] 使用PYNATIVE模式")
        except:
            pass
        
except Exception:
    mindspore = None
    AutoModelForCausalLM = None
    AutoTokenizer = None
    StaticCache = None
    _HAS_MINDNLP = False


MODEL_NAME = "MindSpore-Lab/DeepSeek-R1-Distill-Qwen-1.5B-FP16"
SYSTEM_PROMPT = "你是一个专业的AI助手，擅长工业仪表读数分析与数据处理。请简洁准确地回答问题。重要：思考过程请保持简短（3-5行），重点放在回答内容上。"

# 本地模型路径（相对于当前文件）
LOCAL_MODEL_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__),
    "../.mindnlp/model/MindSpore-Lab/DeepSeek-R1-Distill-Qwen-1.5B-FP16"
))

# JIT优化参数
MAX_NEW_TOKENS = 2048  # 最大生成token数（模型上限）
TEMPERATURE = 0.8      # 温度参数
TOP_P = 0.8            # Top-p采样阈值
REPETITION_PENALTY = 1.2  # 重复惩罚（参考gradio示例）
MAX_CACHE_LEN = 4096   # KV Cache最大长度（适配长文本）


@dataclass
class DeepSeekState:
    status: str = "offline"
    detail: str = "MindNLP not available"
    model_name: str = MODEL_NAME
    conversations: Dict[str, List[tuple[str, str]]] = field(default_factory=dict)
    total_queries: int = 0
    last_query_time: float = 0.0
    avg_query_time: float = 0.0
    jit_compiled: bool = False  # JIT是否已编译


def sample_top_p(probs, p=TOP_P):
    """
    Top-p采样函数（Numpy优化版，昇腾开发板上更快）
    """
    probs_np = probs.asnumpy()
    sorted_indices = np.argsort(-probs_np, axis=-1)
    sorted_probs = np.take_along_axis(probs_np, sorted_indices, axis=-1)
    cumulative_probs = np.cumsum(sorted_probs, axis=-1)
    mask = cumulative_probs - sorted_probs > p
    sorted_probs[mask] = 0.0
    sorted_probs = sorted_probs / np.sum(sorted_probs, axis=-1, keepdims=True)
    
    sorted_probs_tensor = mindspore.Tensor(sorted_probs, dtype=mindspore.float32)
    sorted_indices_tensor = mindspore.Tensor(sorted_indices, dtype=mindspore.int32)
    next_token_idx = ops.multinomial(sorted_probs_tensor, 1)
    next_token = mindspore.ops.gather(sorted_indices_tensor, next_token_idx, axis=1, batch_dims=1)
    return next_token


# 【JIT核心】使用@mindspore.jit装饰器优化decoder函数
@mindspore.jit(jit_config=mindspore.JitConfig(jit_syntax_level='STRICT'))
def get_decode_one_token_logits(model, cur_token, input_pos, cache_position, past_key_values):
    """单个token的解码函数（JIT编译优化）"""
    logits = model(
        cur_token,
        position_ids=input_pos,
        cache_position=cache_position,
        past_key_values=past_key_values,
        return_dict=False,
        use_cache=True
    )[0]
    return logits


def decode_one_token(model, cur_token, input_pos, cache_position, past_key_values, 
                     temperature=TEMPERATURE, top_p=TOP_P, repetition_penalty=1.0, generated_ids=None):
    """单个token解码（含采样和重复惩罚）"""
    logits = get_decode_one_token_logits(model, cur_token, input_pos, cache_position, past_key_values)
    
    # 应用重复惩罚（参考HuggingFace和gradio示例的实现）
    if repetition_penalty != 1.0 and generated_ids is not None and len(generated_ids) > 0:
        # 只处理最后一层的logits用于采样
        logits_for_sampling = logits[:, -1].asnumpy()  # shape: [batch_size, vocab_size]
        
        # 对已生成的token应用惩罚
        for token_id in set(generated_ids):  # 使用set去重，提高效率
            if 0 <= token_id < logits_for_sampling.shape[-1]:
                # HuggingFace标准做法：score > 0 除以penalty，score < 0 乘以penalty
                if logits_for_sampling[0, token_id] > 0:
                    logits_for_sampling[0, token_id] /= repetition_penalty
                else:
                    logits_for_sampling[0, token_id] *= repetition_penalty
        
        # 转回tensor用于后续采样
        logits_sampling = mindspore.Tensor(logits_for_sampling, dtype=logits.dtype)
    else:
        logits_sampling = logits[:, -1]
    
    # 采样
    if temperature > 0:
        probs = mindspore.mint.softmax(logits_sampling / temperature, dim=-1)
        new_token = sample_top_p(probs, top_p)
    else:
        new_token = mindspore.mint.argmax(logits_sampling, dim=-1)[:, None]
    
    return new_token


class DeepSeekService:
    """JIT优化版 DeepSeek 服务"""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._inference_lock = threading.Lock()
        self._model = None
        self._tokenizer = None
        self._state = DeepSeekState()

    @property
    def available(self) -> bool:
        return _HAS_MINDNLP

    @property
    def state(self) -> DeepSeekState:
        return self._state
    
    def is_busy(self) -> bool:
        return self._inference_lock.locked()

    def ensure_loaded(self) -> None:
        """延迟加载模型"""
        if not self.available:
            raise RuntimeError("MindNLP not available")

        if self._model is not None:
            return

        with self._lock:
            if self._model is not None:
                return

            self._state.status = "loading"
            self._state.detail = "Loading DeepSeek model..."
            
            print("[LOADING] [JIT] 加载 DeepSeek 模型...")
            load_start = time.time()

            # 策略：优先使用本地路径（完全离线），不存在则从镜像下载
            if os.path.exists(LOCAL_MODEL_PATH):
                # 本地模型存在，直接加载（完全离线，无网络请求）
                print(f"   - 检测到本地模型路径: {LOCAL_MODEL_PATH}")
                print(f"   - 从本地路径加载（完全离线模式）...")
                
                self._tokenizer = AutoTokenizer.from_pretrained(
                    LOCAL_MODEL_PATH,  # 使用本地路径而非 model_id
                    ms_dtype=mindspore.float16,
                )
                
                self._model = AutoModelForCausalLM.from_pretrained(
                    LOCAL_MODEL_PATH,  # 使用本地路径而非 model_id
                    ms_dtype=mindspore.float16,
                    low_cpu_mem_usage=True,
                )
                print("   [OK] 已从本地路径加载（完全离线，0 网络请求）")
                
            else:
                # 本地模型不存在，从镜像下载
                print(f"   - 本地路径不存在: {LOCAL_MODEL_PATH}")
                print(f"   - 正在从 Modelers 镜像下载模型...")
                
                self._tokenizer = AutoTokenizer.from_pretrained(
                    MODEL_NAME,
                    mirror="modelers",
                    ms_dtype=mindspore.float16,
                )
                
                self._model = AutoModelForCausalLM.from_pretrained(
                    MODEL_NAME,
                    ms_dtype=mindspore.float16,
                    low_cpu_mem_usage=True,
                    mirror="modelers",
                )
                print("   [OK] 模型已下载并缓存到本地")
                print(f"   [INFO] 下次启动将从本地路径加载: {LOCAL_MODEL_PATH}")
            
            # 【JIT优化】全图静态图化
            try:
                self._model.jit()
                print("[SUCCESS] [JIT] 模型已静态图化 (model.jit())")
            except Exception as e:
                print(f"[WARNING] [JIT] 静态图化失败: {e}")
            
            # 设置为评估模式
            try:
                self._model.set_train(False)
                print("[SUCCESS] [JIT] 模型已设置为评估模式")
            except:
                pass
            
            # 禁用PyBoost（兼容性）
            try:
                set_pyboost(False)
            except:
                pass
            
            load_time = time.time() - load_start
            self._state.status = "ready"
            self._state.detail = "Model loaded"
            
            print(f"[SUCCESS] [JIT] DeepSeek 模型加载完成 (耗时: {load_time:.1f}秒)")
            print(f"[WARNING] [JIT] 首次推理将触发静态图编译（约140秒），请耐心等待...")

    def generate(self, message: str, history: List[Dict[str, str]]):
        """生成回复（JIT优化版，支持流式输出）- 返回生成器"""
        # 并发控制
        if not self._inference_lock.acquire(blocking=False):
            print("[WARNING] [JIT] 检测到并发推理请求，拒绝")
            yield {
                "reply": "系统正忙，请等待当前推理完成后再试。",
                "history": history,
                "elapsed_time": 0.0,
                "error": "concurrent_request",
                "done": True,
            }
            return
        
        try:
            self.ensure_loaded()
            # 流式生成
            yield from self._generate_jit_stream(message, history)
        finally:
            self._inference_lock.release()

    def _generate_jit_stream(self, message: str, history: List[Dict[str, str]]):
        """JIT优化的流式生成实现"""
        print(f"\n{'='*60}")
        print(f"[QUERY] [JIT] 问题: {message}")
        print(f"[START] [JIT] 开始流式生成... (时间: {time.strftime('%H:%M:%S')})")
        
        start_time = time.time()
        
        # 构建对话历史
        messages = [{'role': 'system', 'content': SYSTEM_PROMPT}]
        for item in history:
            messages.append({'role': 'user', 'content': item.get('user', '')})
            messages.append({'role': 'assistant', 'content': item.get('assistant', '')})
        messages.append({'role': 'user', 'content': message})
        
        # Tokenize
        inputs = self._tokenizer.apply_chat_template(
            messages,
            add_generation_prompt=True,
            return_tensors="ms",
            tokenize=True
        )
        
        batch_size, seq_length = inputs.shape
        
        # 【JIT优化】创建StaticCache
        past_key_values = StaticCache(
            config=self._model.config,
            max_batch_size=batch_size,
            max_cache_len=MAX_CACHE_LEN,
            dtype=self._model.dtype
        )
        
        cache_position = ops.arange(seq_length)
        generated_ids = ops.zeros(
            batch_size, seq_length + MAX_NEW_TOKENS + 1, dtype=mindspore.int32
        )
        generated_ids[:, cache_position] = inputs.to(mindspore.int32)
        
        # 初始前向传播
        logits = self._model(
            inputs, cache_position=cache_position, past_key_values=past_key_values,
            return_dict=False, use_cache=True
        )[0]
        
        # 生成第一个token
        first_token_start = time.time()
        if TEMPERATURE > 0:
            probs = mindspore.mint.softmax(logits[:, -1] / TEMPERATURE, dim=-1)
            next_token = sample_top_p(probs, TOP_P)
        else:
            next_token = mindspore.mint.argmax(logits[:, -1], dim=-1)[:, None]
        
        generated_ids[:, seq_length] = next_token[:, 0]
        first_token_time = time.time() - first_token_start
        
        if not self._state.jit_compiled:
            print(f"   - [JIT] 首Token编译+推理: {first_token_time:.2f}秒")
            self._state.jit_compiled = True
        else:
            print(f"   - [JIT] 首Token: {first_token_time:.2f}秒")
        
        # 累积生成的 token IDs
        all_tokens = [next_token.int()[0].item()]
        
        # 解码并yield第一个token
        reply = self._tokenizer.decode(all_tokens, skip_special_tokens=True, clean_up_tokenization_spaces=False)
        
        # 【流式输出】yield第一个token
        yield {
            "token": reply,
            "reply": reply,
            "done": False,
        }
        
        # 自回归生成循环
        cache_position = mindspore.tensor([seq_length + 1])
        token_times = [first_token_time]
        actual_tokens = 1
        
        for step in range(1, MAX_NEW_TOKENS):
            t_start = time.time()
            next_token = decode_one_token(
                self._model, next_token, None, cache_position, past_key_values,
                temperature=TEMPERATURE, top_p=TOP_P,
                repetition_penalty=REPETITION_PENALTY,  # 添加重复惩罚
                generated_ids=all_tokens  # 传入已生成的token列表
            )
            generated_ids[:, cache_position] = next_token.int()
            cache_position += 1
            t_elapsed = time.time() - t_start
            token_times.append(t_elapsed)
            
            # 检查是否结束
            if self._tokenizer.eos_token_id in next_token.int()[0]:
                break
            
            # 累积token ID
            all_tokens.append(next_token.int()[0].item())
            actual_tokens += 1
            
            # 解码所有累积的tokens（这样可以正确处理多字节字符）
            reply = self._tokenizer.decode(all_tokens, skip_special_tokens=True, clean_up_tokenization_spaces=False)
            
            # 【流式输出】yield累积的完整回复
            yield {
                "token": "",  # 不再单独返回token
                "reply": reply,
                "done": False,
            }
        
        elapsed_time = time.time() - start_time
        
        # 统计
        avg_per_token = sum(token_times) / len(token_times) if token_times else 0
        min_per_token = min(token_times) if token_times else 0
        max_per_token = max(token_times) if token_times else 0
        
        # 输出每个token的推理时间（类似基准版本）
        print(f"..average inference time is: {avg_per_token}")
        print(f"inference time record: {token_times}")
        
        print(f"\n[SUCCESS] [JIT] 生成完成!")
        print(f"   - 总耗时: {elapsed_time:.2f}秒")
        print(f"   - 回复长度: {len(reply)}字符")
        print(f"   - 实际生成tokens: {actual_tokens}")
        print(f"   - 平均每token: {avg_per_token:.3f}秒 ({1.0/avg_per_token:.1f} tokens/s)" if avg_per_token > 0 else "")
        print(f"   - 最快token: {min_per_token:.3f}秒")
        print(f"   - 最慢token: {max_per_token:.3f}秒")
        
        # 更新统计
        self._state.total_queries += 1
        self._state.last_query_time = elapsed_time
        if self._state.total_queries == 1:
            self._state.avg_query_time = elapsed_time
        else:
            self._state.avg_query_time = (
                self._state.avg_query_time * (self._state.total_queries - 1) + elapsed_time
            ) / self._state.total_queries
        
        # 更新历史
        new_history = history.copy()
        new_history.append({"user": message, "assistant": reply})
        
        # 【流式输出】最后yield完整结果
        yield {
            "token": "",
            "reply": reply,
            "history": new_history,
            "elapsed_time": elapsed_time,
            "tokens_generated": actual_tokens,
            "avg_time_per_token": avg_per_token,
            "first_token_time": first_token_time,
            "done": True,
        }


# 全局单例
DEEPSEEK = DeepSeekService()
