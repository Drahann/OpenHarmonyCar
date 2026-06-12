"""
数据分析模块 - 生成仪表读数分析报告

设计原则：
- 灵活：提示词模板易于修改
- 简洁：核心逻辑清晰
- 可扩展：预留扩展点
"""

import numpy as np
from datetime import datetime, timedelta
from typing import Dict, List, Optional
from data_storage import get_storage


# DeepSeek 分析提示词模板
ANALYSIS_PROMPT_TEMPLATE = """你是一个专业的工业仪表数据分析专家。

重要：请直接给出分析结果，思考过程尽量简短（3-5行），重点放在分析报告的内容上。

## 数据概况

**时间范围**: {period_start} 至 {period_end} (共 {duration_hours:.1f} 小时)
**总读数次数**: {total_readings} 次
**仪表类型**: {gauge_types_list}

## 详细统计

{detailed_stats}

## 要求

请生成一份简洁专业的分析报告，包含：
1. **数据概览**: 总体情况描述
2. **趋势分析**: 各仪表读数的变化趋势
3. **异常识别**: 是否存在异常读数
4. **建议**: 基于数据给出的运维建议

请使用 Markdown 格式输出，简洁明了。不需要冗长的思考过程，直接给出结论。
"""


class DataAnalyzer:
    """数据分析器"""
    
    def __init__(self):
        self.storage = get_storage()
    
    def generate_statistics(
        self,
        start_time: datetime,
        end_time: datetime
    ) -> Dict:
        """
        生成统计数据（不调用DeepSeek，仅计算统计值）
        
        Args:
            start_time: 开始时间
            end_time: 结束时间
        
        Returns:
            统计数据字典
        """
        # 获取基础统计
        stats = self.storage.get_statistics(start_time, end_time)
        
        # 计算持续时间
        duration = end_time - start_time
        duration_hours = duration.total_seconds() / 3600
        
        # 增强统计信息
        result = {
            "period": {
                "start": start_time.isoformat(),
                "end": end_time.isoformat(),
                "duration_hours": duration_hours
            },
            "total_readings": stats["total_readings"],
            "gauge_types_count": stats["gauge_types_count"],
            "by_type": {}
        }
        
        # 为每种仪表类型计算更多统计指标
        for gauge_type, type_stats in stats["by_type"].items():
            # 获取该类型的所有读数
            readings_data = self.storage.get_readings(
                start_time=start_time,
                end_time=end_time,
                gauge_type=gauge_type,
                limit=10000  # 足够大的限制
            )
            
            if readings_data:
                values = [r['reading'] for r in readings_data]
                
                # 计算标准差
                std_dev = np.std(values) if len(values) > 1 else 0
                
                # 简单的趋势判断
                if len(values) >= 2:
                    # 比较前半段和后半段的平均值
                    mid = len(values) // 2
                    first_half_avg = np.mean(values[:mid])
                    second_half_avg = np.mean(values[mid:])
                    
                    if second_half_avg > first_half_avg * 1.05:
                        trend = "上升"
                    elif second_half_avg < first_half_avg * 0.95:
                        trend = "下降"
                    else:
                        trend = "稳定"
                else:
                    trend = "数据不足"
                
                result["by_type"][gauge_type] = {
                    "count": type_stats['count'],
                    "min": type_stats['min_reading'],
                    "max": type_stats['max_reading'],
                    "avg": round(type_stats['avg_reading'], 2),
                    "std": round(std_dev, 2),
                    "trend": trend,
                    "first_time": type_stats['first_reading_time'],
                    "last_time": type_stats['last_reading_time']
                }
        
        return result
    
    def build_analysis_prompt(
        self,
        start_time: datetime,
        end_time: datetime
    ) -> str:
        """
        构建分析提示词
        
        Args:
            start_time: 开始时间
            end_time: 结束时间
        
        Returns:
            完整的提示词字符串
        """
        # 生成统计数据
        stats = self.generate_statistics(start_time, end_time)
        
        # 格式化时间
        period_start = start_time.strftime("%Y-%m-%d %H:%M:%S")
        period_end = end_time.strftime("%Y-%m-%d %H:%M:%S")
        duration_hours = stats["period"]["duration_hours"]
        
        # 仪表类型列表
        gauge_types_list = "、".join(stats["by_type"].keys()) if stats["by_type"] else "无"
        
        # 详细统计（格式化为文本）
        detailed_stats_lines = []
        for gauge_type, type_stats in stats["by_type"].items():
            detailed_stats_lines.append(f"""
### {gauge_type}
- 读数次数: {type_stats['count']} 次
- 数值范围: {type_stats['min']:.2f} ~ {type_stats['max']:.2f}
- 平均值: {type_stats['avg']:.2f}
- 标准差: {type_stats['std']:.2f}
- 趋势: {type_stats['trend']}
""")
        
        detailed_stats = "\n".join(detailed_stats_lines) if detailed_stats_lines else "无数据"
        
        # 填充模板
        prompt = ANALYSIS_PROMPT_TEMPLATE.format(
            period_start=period_start,
            period_end=period_end,
            duration_hours=duration_hours,
            total_readings=stats["total_readings"],
            gauge_types_list=gauge_types_list,
            detailed_stats=detailed_stats
        )
        
        return prompt, stats
    
    async def generate_report_with_deepseek(
        self,
        start_time: datetime,
        end_time: datetime,
        deepseek_service  # DeepSeekService 实例
    ) -> Dict:
        """
        使用 DeepSeek 生成分析报告
        
        Args:
            start_time: 开始时间
            end_time: 结束时间
            deepseek_service: DeepSeek 服务实例
        
        Returns:
            包含报告文本和统计数据的字典
        """
        # 构建提示词
        prompt, statistics = self.build_analysis_prompt(start_time, end_time)
        
        # 调用 DeepSeek 生成报告
        print(f"[REPORT] 正在生成分析报告...")
        print(f"[REPORT] 数据范围: {start_time} - {end_time}")
        print(f"[REPORT] 数据点数: {statistics['total_readings']}")
        
        # 收集DeepSeek的流式输出
        report_text = ""
        history = []  # 空历史
        
        try:
            for chunk in deepseek_service.generate(prompt, history):
                if chunk.get("reply"):
                    report_text = chunk["reply"]
                
                if chunk.get("done"):
                    break
            
            # 生成摘要（提取报告的第一段或前100字）
            summary_lines = report_text.split('\n\n')
            summary = summary_lines[0] if summary_lines else report_text[:100]
            
            # 保存到数据库
            report_id = self.storage.save_report(
                period_start=start_time,
                period_end=end_time,
                report_text=report_text,
                summary=summary,
                statistics=statistics
            )
            
            print(f"[SUCCESS] 报告生成完成，ID: {report_id}")
            
            return {
                "report_id": report_id,
                "report_text": report_text,
                "summary": summary,
                "statistics": statistics,
                "period_start": start_time.isoformat(),
                "period_end": end_time.isoformat()
            }
            
        except Exception as e:
            print(f"[ERROR] 报告生成失败: {e}")
            import traceback
            traceback.print_exc()
            raise
    
    def get_default_time_range(self, hours: int = 24) -> tuple[datetime, datetime]:
        """
        获取默认时间范围
        
        Args:
            hours: 往前推多少小时
        
        Returns:
            (start_time, end_time)
        """
        end_time = datetime.now()
        start_time = end_time - timedelta(hours=hours)
        return start_time, end_time


# 全局单例
_analyzer_instance = None


def get_analyzer() -> DataAnalyzer:
    """获取全局分析器实例"""
    global _analyzer_instance
    if _analyzer_instance is None:
        _analyzer_instance = DataAnalyzer()
    return _analyzer_instance


if __name__ == "__main__":
    # 测试代码
    import json
    
    analyzer = DataAnalyzer()
    
    # 测试统计生成
    print("生成统计数据...")
    start_time, end_time = analyzer.get_default_time_range(hours=24)
    
    stats = analyzer.generate_statistics(start_time, end_time)
    print(json.dumps(stats, indent=2, ensure_ascii=False))
    
    # 测试提示词生成
    print("\n生成分析提示词...")
    prompt, _ = analyzer.build_analysis_prompt(start_time, end_time)
    print(prompt)

