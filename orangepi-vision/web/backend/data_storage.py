"""
数据存储模块 - 使用 SQLite 存储仪表读数和分析报告

设计原则：
- 简洁：只实现核心功能
- 灵活：易于扩展和修改
- 鲁棒：包含错误处理
"""

import sqlite3
import json
import os
from datetime import datetime, timedelta
from typing import List, Dict, Optional, Tuple
from contextlib import contextmanager
from pathlib import Path


# 数据库路径
DB_DIR = Path(__file__).parent.parent / "data"
DB_PATH = DB_DIR / "gauge_readings.db"


class DataStorage:
    """数据存储管理类"""
    
    def __init__(self, db_path: str = None):
        """
        初始化数据存储
        
        Args:
            db_path: 数据库路径，默认使用 DB_PATH
        """
        self.db_path = db_path or str(DB_PATH)
        self._ensure_db_dir()
        self._init_database()
    
    def _ensure_db_dir(self):
        """确保数据目录存在"""
        db_dir = Path(self.db_path).parent
        db_dir.mkdir(parents=True, exist_ok=True)
    
    @contextmanager
    def _get_connection(self):
        """获取数据库连接（上下文管理器）"""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row  # 允许通过列名访问
        # 确保文本以 UTF-8 字符串形式返回，避免问号乱码
        conn.text_factory = str
        # 设置编码
        conn.execute("PRAGMA encoding = 'UTF-8'")
        try:
            yield conn
            conn.commit()
        except Exception as e:
            conn.rollback()
            raise e
        finally:
            conn.close()
    
    def _init_database(self):
        """初始化数据库表"""
        with self._get_connection() as conn:
            cursor = conn.cursor()
            
            # 创建仪表读数表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS gauge_readings (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                    gauge_type TEXT NOT NULL,
                    reading REAL NOT NULL,
                    unit TEXT,
                    confidence REAL,
                    frame_id INTEGER,
                    metadata TEXT
                )
            """)
            
            # 创建索引
            cursor.execute("""
                CREATE INDEX IF NOT EXISTS idx_timestamp 
                ON gauge_readings(timestamp)
            """)
            cursor.execute("""
                CREATE INDEX IF NOT EXISTS idx_gauge_type 
                ON gauge_readings(gauge_type)
            """)
            
            # 创建分析报告表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS analysis_reports (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                    period_start DATETIME NOT NULL,
                    period_end DATETIME NOT NULL,
                    data_points INTEGER,
                    report_text TEXT,
                    summary TEXT,
                    statistics TEXT
                )
            """)
            
            cursor.execute("""
                CREATE INDEX IF NOT EXISTS idx_created_at 
                ON analysis_reports(created_at)
            """)
    
    # ========== 仪表读数相关方法 ==========
    
    def add_reading(
        self, 
        gauge_type: str, 
        reading: float, 
        unit: str = None,
        confidence: float = None,
        frame_id: int = None,
        metadata: Dict = None
    ) -> int:
        """
        添加一条仪表读数记录
        
        Args:
            gauge_type: 仪表类型（如"压力表"、"温度表"）
            reading: 读数值
            unit: 单位（如"MPa"、"°C"）
            confidence: 置信度（0-1）
            frame_id: 帧号
            metadata: 额外元数据（字典，将转为JSON）
        
        Returns:
            插入记录的ID
        """
        metadata_json = json.dumps(metadata, ensure_ascii=False) if metadata else None
        
        with self._get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute("""
                INSERT INTO gauge_readings 
                (gauge_type, reading, unit, confidence, frame_id, metadata)
                VALUES (?, ?, ?, ?, ?, ?)
            """, (gauge_type, reading, unit, confidence, frame_id, metadata_json))
            return cursor.lastrowid
    
    def add_readings_batch(self, readings: List[Dict]) -> int:
        """
        批量添加读数记录（性能优化）
        
        Args:
            readings: 读数列表，每个元素是字典
        
        Returns:
            插入的记录数
        """
        with self._get_connection() as conn:
            cursor = conn.cursor()
            
            data = []
            for r in readings:
                metadata_json = json.dumps(r.get('metadata'), ensure_ascii=False) if r.get('metadata') else None
                data.append((
                    r['gauge_type'],
                    r['reading'],
                    r.get('unit'),
                    r.get('confidence'),
                    r.get('frame_id'),
                    metadata_json
                ))
            
            cursor.executemany("""
                INSERT INTO gauge_readings 
                (gauge_type, reading, unit, confidence, frame_id, metadata)
                VALUES (?, ?, ?, ?, ?, ?)
            """, data)
            
            return len(data)
    
    def get_readings(
        self,
        start_time: datetime = None,
        end_time: datetime = None,
        gauge_type: str = None,
        limit: int = 1000
    ) -> List[Dict]:
        """
        查询仪表读数
        
        Args:
            start_time: 开始时间
            end_time: 结束时间
            gauge_type: 仪表类型（筛选）
            limit: 最大返回数量
        
        Returns:
            读数列表
        """
        with self._get_connection() as conn:
            cursor = conn.cursor()
            
            query = "SELECT * FROM gauge_readings WHERE 1=1"
            params = []
            
            if start_time:
                query += " AND timestamp >= ?"
                params.append(start_time.strftime("%Y-%m-%d %H:%M:%S"))
            
            if end_time:
                query += " AND timestamp <= ?"
                params.append(end_time.strftime("%Y-%m-%d %H:%M:%S"))
            
            if gauge_type:
                query += " AND gauge_type = ?"
                params.append(gauge_type)
            
            query += " ORDER BY timestamp DESC LIMIT ?"
            params.append(limit)
            
            cursor.execute(query, params)
            rows = cursor.fetchall()
            
            # 转换为字典列表
            results = []
            for row in rows:
                result = dict(row)
                # 解析 metadata JSON
                if result.get('metadata'):
                    try:
                        result['metadata'] = json.loads(result['metadata'])
                    except:
                        pass
                results.append(result)
            
            return results
    
    def get_statistics(
        self,
        start_time: datetime = None,
        end_time: datetime = None,
        gauge_type: str = None
    ) -> Dict:
        """
        获取统计数据
        
        Args:
            start_time: 开始时间
            end_time: 结束时间
            gauge_type: 仪表类型（可选，不指定则统计所有）
        
        Returns:
            统计数据字典
        """
        with self._get_connection() as conn:
            cursor = conn.cursor()
            
            # 基础查询条件
            where_clause = "WHERE 1=1"
            params = []
            
            if start_time:
                where_clause += " AND timestamp >= ?"
                params.append(start_time.strftime("%Y-%m-%d %H:%M:%S"))
            
            if end_time:
                where_clause += " AND timestamp <= ?"
                params.append(end_time.strftime("%Y-%m-%d %H:%M:%S"))
            
            if gauge_type:
                where_clause += " AND gauge_type = ?"
                params.append(gauge_type)
            
            # 总体统计
            cursor.execute(f"""
                SELECT 
                    COUNT(*) as total_count,
                    COUNT(DISTINCT gauge_type) as gauge_types_count
                FROM gauge_readings
                {where_clause}
            """, params)
            overall = dict(cursor.fetchone())
            
            # 按仪表类型分组统计
            cursor.execute(f"""
                SELECT 
                    gauge_type,
                    COUNT(*) as count,
                    MIN(reading) as min_reading,
                    MAX(reading) as max_reading,
                    AVG(reading) as avg_reading,
                    MIN(timestamp) as first_reading_time,
                    MAX(timestamp) as last_reading_time
                FROM gauge_readings
                {where_clause}
                GROUP BY gauge_type
            """, params)
            
            by_type = {}
            for row in cursor.fetchall():
                row_dict = dict(row)
                gauge_type_name = row_dict.pop('gauge_type')
                by_type[gauge_type_name] = row_dict
            
            return {
                "total_readings": overall['total_count'],
                "gauge_types_count": overall['gauge_types_count'],
                "by_type": by_type,
                "query_period": {
                    "start": start_time.isoformat() if start_time else None,
                    "end": end_time.isoformat() if end_time else None
                }
            }
    
    def delete_old_readings(self, days: int = 30) -> int:
        """
        删除旧数据（数据清理）
        
        Args:
            days: 保留最近N天的数据
        
        Returns:
            删除的记录数
        """
        cutoff_date = datetime.now() - timedelta(days=days)
        
        with self._get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute("""
                DELETE FROM gauge_readings
                WHERE timestamp < ?
            """, (cutoff_date.strftime("%Y-%m-%d %H:%M:%S"),))
            return cursor.rowcount
    
    # ========== 分析报告相关方法 ==========
    
    def save_report(
        self,
        period_start: datetime,
        period_end: datetime,
        report_text: str,
        summary: str = None,
        statistics: Dict = None
    ) -> int:
        """
        保存分析报告
        
        Args:
            period_start: 分析周期开始时间
            period_end: 分析周期结束时间
            report_text: 报告正文（Markdown格式）
            summary: 摘要
            statistics: 统计数据（字典，将转为JSON）
        
        Returns:
            报告ID
        """
        # 计算数据点数量
        with self._get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute("""
                SELECT COUNT(*) as count FROM gauge_readings
                WHERE timestamp >= ? AND timestamp <= ?
            """, (
                period_start.strftime("%Y-%m-%d %H:%M:%S"),
                period_end.strftime("%Y-%m-%d %H:%M:%S")
            ))
            data_points = cursor.fetchone()[0]
        
        statistics_json = json.dumps(statistics, ensure_ascii=False) if statistics else None
        
        with self._get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute("""
                INSERT INTO analysis_reports
                (period_start, period_end, data_points, report_text, summary, statistics)
                VALUES (?, ?, ?, ?, ?, ?)
            """, (
                period_start.strftime("%Y-%m-%d %H:%M:%S"),
                period_end.strftime("%Y-%m-%d %H:%M:%S"),
                data_points,
                report_text,
                summary,
                statistics_json
            ))
            return cursor.lastrowid
    
    def get_reports(self, limit: int = 10, offset: int = 0) -> List[Dict]:
        """
        获取报告列表
        
        Args:
            limit: 返回数量
            offset: 偏移量
        
        Returns:
            报告列表
        """
        with self._get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute("""
                SELECT * FROM analysis_reports
                ORDER BY created_at DESC
                LIMIT ? OFFSET ?
            """, (limit, offset))
            
            rows = cursor.fetchall()
            results = []
            for row in rows:
                result = dict(row)
                # 解析 statistics JSON
                if result.get('statistics'):
                    try:
                        result['statistics'] = json.loads(result['statistics'])
                    except:
                        pass
                results.append(result)
            
            return results
    
    def get_report(self, report_id: int) -> Optional[Dict]:
        """
        获取单个报告详情
        
        Args:
            report_id: 报告ID
        
        Returns:
            报告字典，不存在则返回None
        """
        with self._get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute("""
                SELECT * FROM analysis_reports WHERE id = ?
            """, (report_id,))
            
            row = cursor.fetchone()
            if not row:
                return None
            
            result = dict(row)
            # 解析 statistics JSON
            if result.get('statistics'):
                try:
                    result['statistics'] = json.loads(result['statistics'])
                except:
                    pass
            
            return result
    
    def delete_report(self, report_id: int) -> bool:
        """
        删除报告
        
        Args:
            report_id: 报告ID
        
        Returns:
            是否删除成功
        """
        with self._get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute("""
                DELETE FROM analysis_reports WHERE id = ?
            """, (report_id,))
            return cursor.rowcount > 0
    
    # ========== 工具方法 ==========
    
    def get_database_info(self) -> Dict:
        """获取数据库信息（用于调试）"""
        with self._get_connection() as conn:
            cursor = conn.cursor()
            
            # 读数总数
            cursor.execute("SELECT COUNT(*) FROM gauge_readings")
            readings_count = cursor.fetchone()[0]
            
            # 报告总数
            cursor.execute("SELECT COUNT(*) FROM analysis_reports")
            reports_count = cursor.fetchone()[0]
            
            # 数据库文件大小
            db_size = os.path.getsize(self.db_path) if os.path.exists(self.db_path) else 0
            
            # 最早和最新的记录时间
            cursor.execute("""
                SELECT MIN(timestamp), MAX(timestamp) 
                FROM gauge_readings
            """)
            result = cursor.fetchone()
            first_reading = result[0] if result[0] else None
            last_reading = result[1] if result[1] else None
            
            return {
                "database_path": self.db_path,
                "database_size_mb": round(db_size / 1024 / 1024, 2),
                "total_readings": readings_count,
                "total_reports": reports_count,
                "first_reading_time": first_reading,
                "last_reading_time": last_reading
            }
    
    def delete_readings_by_type(self, gauge_type: str) -> int:
        """
        删除指定类型仪表的所有读数
        
        Args:
            gauge_type: 仪表类型
        
        Returns:
            删除的记录数
        """
        with self._get_connection() as conn:
            cursor = conn.cursor()
            
            # 先查询有多少条
            cursor.execute("SELECT COUNT(*) FROM gauge_readings WHERE gauge_type = ?", (gauge_type,))
            count = cursor.fetchone()[0]
            
            # 删除
            cursor.execute("DELETE FROM gauge_readings WHERE gauge_type = ?", (gauge_type,))
            conn.commit()
            
            print(f"[SUCCESS] 已删除 {gauge_type} 的 {count} 条读数")
            return count
    
    def delete_readings_by_time_range(self, start_time: datetime = None, end_time: datetime = None) -> int:
        """
        删除指定时间范围的读数
        
        Args:
            start_time: 开始时间（None = 从最早开始）
            end_time: 结束时间（None = 到最晚结束）
        
        Returns:
            删除的记录数
        """
        with self._get_connection() as conn:
            cursor = conn.cursor()
            
            if start_time is None and end_time is None:
                # 删除所有数据
                cursor.execute("SELECT COUNT(*) FROM gauge_readings")
                count = cursor.fetchone()[0]
                cursor.execute("DELETE FROM gauge_readings")
            elif start_time is None:
                cursor.execute("SELECT COUNT(*) FROM gauge_readings WHERE timestamp <= ?", (end_time,))
                count = cursor.fetchone()[0]
                cursor.execute("DELETE FROM gauge_readings WHERE timestamp <= ?", (end_time,))
            elif end_time is None:
                cursor.execute("SELECT COUNT(*) FROM gauge_readings WHERE timestamp >= ?", (start_time,))
                count = cursor.fetchone()[0]
                cursor.execute("DELETE FROM gauge_readings WHERE timestamp >= ?", (start_time,))
            else:
                cursor.execute("""
                    SELECT COUNT(*) FROM gauge_readings 
                    WHERE timestamp >= ? AND timestamp <= ?
                """, (start_time, end_time))
                count = cursor.fetchone()[0]
                cursor.execute("""
                    DELETE FROM gauge_readings 
                    WHERE timestamp >= ? AND timestamp <= ?
                """, (start_time, end_time))
            
            conn.commit()
            
            print(f"[SUCCESS] 已删除时间范围内的 {count} 条读数")
            return count
    
    def clear_all_data(self) -> Dict[str, int]:
        """
        清空所有数据（读数和报告）
        
        Returns:
            删除统计：{'readings': 数量, 'reports': 数量}
        """
        with self._get_connection() as conn:
            cursor = conn.cursor()
            
            # 统计数量
            cursor.execute("SELECT COUNT(*) FROM gauge_readings")
            readings_count = cursor.fetchone()[0]
            
            cursor.execute("SELECT COUNT(*) FROM analysis_reports")
            reports_count = cursor.fetchone()[0]
            
            # 删除所有数据
            cursor.execute("DELETE FROM gauge_readings")
            cursor.execute("DELETE FROM analysis_reports")
            
            conn.commit()
            
            print(f"[SUCCESS] 已清空所有数据：{readings_count} 条读数，{reports_count} 份报告")
            return {
                'readings': readings_count,
                'reports': reports_count
            }


# 全局单例
_storage_instance = None


def get_storage() -> DataStorage:
    """获取全局存储实例（单例模式）"""
    global _storage_instance
    if _storage_instance is None:
        _storage_instance = DataStorage()
    return _storage_instance


if __name__ == "__main__":
    # 测试代码
    print("初始化数据存储...")
    storage = DataStorage()
    
    # 添加测试数据
    print("\n添加测试读数...")
    storage.add_reading("压力表", 75.5, "MPa", confidence=0.95, frame_id=1001)
    storage.add_reading("温度表", 22.3, "°C", confidence=0.98, frame_id=1002)
    storage.add_reading("压力表", 76.2, "MPa", confidence=0.96, frame_id=1003)
    
    # 查询统计
    print("\n统计数据:")
    stats = storage.get_statistics()
    print(json.dumps(stats, indent=2, ensure_ascii=False))
    
    # 查询读数
    print("\n最近的读数:")
    readings = storage.get_readings(limit=5)
    for r in readings:
        print(f"  {r['timestamp']} | {r['gauge_type']}: {r['reading']} {r['unit']}")
    
    # 数据库信息
    print("\n数据库信息:")
    info = storage.get_database_info()
    print(json.dumps(info, indent=2, ensure_ascii=False))

