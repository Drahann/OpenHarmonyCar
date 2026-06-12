"""
数据收集模块 - 自动收集仪表读数并存储

使用说明：
1. 在检测到仪表读数时调用 collect_reading()
2. 支持批量收集和定时保存
3. 自动处理错误，不影响主流程
"""

import time
from typing import Dict, List, Optional
from datetime import datetime
from data_storage import get_storage


class DataCollector:
    """数据收集器"""
    
    def __init__(self, batch_size: int = 10, auto_save_interval: int = 60):
        """
        初始化数据收集器
        
        Args:
            batch_size: 批量保存的阈值（条数）
            auto_save_interval: 自动保存时间间隔（秒）
        """
        self.storage = get_storage()
        self.batch_size = batch_size
        self.auto_save_interval = auto_save_interval
        
        self.buffer = []  # 缓冲区
        self.last_save_time = time.time()
        
        self.total_collected = 0
        self.total_saved = 0
        self.enabled = True  # 是否启用数据收集
    
    def collect_reading(
        self,
        gauge_type: str,
        reading: float,
        unit: str = None,
        confidence: float = None,
        frame_id: int = None,
        metadata: Dict = None
    ) -> bool:
        """
        收集一条仪表读数（添加到缓冲区）
        
        Args:
            gauge_type: 仪表类型
            reading: 读数值
            unit: 单位
            confidence: 置信度
            frame_id: 帧号
            metadata: 元数据
        
        Returns:
            是否成功
        """
        if not self.enabled:
            return False
        
        try:
            # 添加到缓冲区
            record = {
                "gauge_type": gauge_type,
                "reading": reading,
                "unit": unit,
                "confidence": confidence,
                "frame_id": frame_id,
                "metadata": metadata
            }
            
            self.buffer.append(record)
            self.total_collected += 1
            
            # 检查是否需要保存
            current_time = time.time()
            should_save = (
                len(self.buffer) >= self.batch_size or  # 达到批量阈值
                current_time - self.last_save_time >= self.auto_save_interval  # 达到时间间隔
            )
            
            if should_save:
                self.flush()
            
            return True
            
        except Exception as e:
            print(f"[WARNING] 数据收集失败: {e}")
            return False
    
    def flush(self) -> int:
        """
        将缓冲区的数据保存到数据库
        
        Returns:
            保存的记录数
        """
        if not self.buffer:
            return 0
        
        try:
            count = self.storage.add_readings_batch(self.buffer)
            self.total_saved += count
            self.buffer.clear()
            self.last_save_time = time.time()
            
            # print(f"[INFO] 已保存 {count} 条读数（累计: {self.total_saved}）")
            return count
            
        except Exception as e:
            print(f"[ERROR] 保存数据失败: {e}")
            return 0
    
    def get_stats(self) -> Dict:
        """获取收集器统计信息"""
        return {
            "enabled": self.enabled,
            "total_collected": self.total_collected,
            "total_saved": self.total_saved,
            "buffer_size": len(self.buffer),
            "last_save_time": datetime.fromtimestamp(self.last_save_time).isoformat()
        }
    
    def enable(self):
        """启用数据收集"""
        self.enabled = True
        print("[INFO] 数据收集已启用")
    
    def disable(self):
        """禁用数据收集"""
        self.enabled = False
        # 保存缓冲区剩余数据
        if self.buffer:
            self.flush()
        print("[INFO] 数据收集已禁用")


# 全局单例
_collector_instance = None


def get_collector() -> DataCollector:
    """获取全局收集器实例"""
    global _collector_instance
    if _collector_instance is None:
        _collector_instance = DataCollector(
            batch_size=10,  # 每10条保存一次
            auto_save_interval=30  # 或每30秒保存一次
        )
    return _collector_instance


# 便捷函数
def collect_gauge_reading(
    gauge_type: str,
    reading: float,
    unit: str = None,
    confidence: float = None,
    frame_id: int = None,
    **kwargs
) -> bool:
    """
    便捷函数：收集仪表读数
    
    这个函数可以直接在 video_camera.py 中调用
    """
    collector = get_collector()
    return collector.collect_reading(
        gauge_type=gauge_type,
        reading=reading,
        unit=unit,
        confidence=confidence,
        frame_id=frame_id,
        metadata=kwargs if kwargs else None
    )


if __name__ == "__main__":
    # 测试代码
    print("测试数据收集器...")
    
    collector = DataCollector(batch_size=3, auto_save_interval=10)
    
    # 添加一些测试数据
    for i in range(5):
        collector.collect_reading(
            gauge_type="压力表",
            reading=75.0 + i * 0.5,
            unit="MPa",
            confidence=0.95,
            frame_id=1000 + i
        )
        print(f"  已收集 {i+1} 条读数，缓冲区大小: {len(collector.buffer)}")
    
    # 强制保存
    collector.flush()
    
    # 查看统计
    print("\n收集器统计:")
    import json
    print(json.dumps(collector.get_stats(), indent=2, ensure_ascii=False))

