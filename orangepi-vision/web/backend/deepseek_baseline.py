"""DeepSeek 基准版本 - 无任何优化"""
import os
import time
import threading

# 启用推理时间记录
os.environ['INFERENCE_TIME_RECORD'] = 'True'

try:
    import mindspore
    from mindnlp.transformers import AutoModelForCausalLM, AutoTokenizer
    _HAS_MINDNLP = True
    
    # 基准配置：PYNATIVE模式（动态图，无优化）
    mindspore.set_context(mode=mindspore.PYNATIVE_MODE, device_target="Ascend")
    print("[BASELINE] PYNATIVE模式（无优化）")
except:
    mindspore = None
    AutoModelForCausalLM = None
    AutoTokenizer = None
    _HAS_MINDNLP = False


MODEL_NAME = "MindSpore-Lab/DeepSeek-R1-Distill-Qwen-1.5B-FP16"
LOCAL_MODEL_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__),
    "../.mindnlp/model/MindSpore-Lab/DeepSeek-R1-Distill-Qwen-1.5B-FP16"
))


class DeepSeekBaseline:
    def __init__(self):
        self._model = None
        self._tokenizer = None
        self._lock = threading.Lock()

    def ensure_loaded(self):
        if self._model is not None:
            return
        
        with self._lock:
            if self._model is not None:
                return
            
            if os.path.exists(LOCAL_MODEL_PATH):
                print(f"从本地加载: {LOCAL_MODEL_PATH}")
                self._tokenizer = AutoTokenizer.from_pretrained(LOCAL_MODEL_PATH, ms_dtype=mindspore.float16)
                self._model = AutoModelForCausalLM.from_pretrained(LOCAL_MODEL_PATH, ms_dtype=mindspore.float16)
            else:
                print(f"从镜像下载: {MODEL_NAME}")
                self._tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME, mirror="modelers", ms_dtype=mindspore.float16)
                self._model = AutoModelForCausalLM.from_pretrained(MODEL_NAME, mirror="modelers", ms_dtype=mindspore.float16)
            
            self._model.set_train(False)
    
    def generate(self, question: str):
        self.ensure_loaded()
        
        messages = [
            {'role': 'system', 'content': '你是专业的AI助手。'},
            {'role': 'user', 'content': question}
        ]
        
        inputs = self._tokenizer.apply_chat_template(
            messages, add_generation_prompt=True, return_tensors="ms", tokenize=True
        )
        
        print("\n[生成开始]")
        start_time = time.time()
        
        outputs = self._model.generate(
            inputs,
            max_new_tokens=512,
            do_sample=True,
            temperature=0.8,
            top_p=0.8,
            repetition_penalty=1.2,
            eos_token_id=self._tokenizer.eos_token_id,
        )
        
        elapsed = time.time() - start_time
        
        generated_tokens = outputs[0][inputs.shape[1]:]
        reply = self._tokenizer.decode(generated_tokens, skip_special_tokens=True)
        
        token_count = len(generated_tokens)
        avg_per_token = elapsed / token_count if token_count > 0 else 0
        
        print(f"\n[生成完成]")
        print(f"总耗时: {elapsed:.2f}秒")
        print(f"生成tokens: {token_count}")
        print(f"平均: {avg_per_token:.3f}秒/token ({1.0/avg_per_token:.2f} tokens/s)")
        
        return {
            'reply': reply,
            'elapsed_time': elapsed,
            'tokens_generated': token_count,
            'avg_time_per_token': avg_per_token,
        }


DEEPSEEK_BASELINE = DeepSeekBaseline()


