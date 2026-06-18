#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
仪表配置管理模块
用于存储和管理每个仪表的单位、量程、阈值等配置
"""

import sqlite3
import json
from datetime import datetime
from typing import Dict, List, Optional
import os


class GaugeConfigManager:
    """仪表配置管理器"""
    
    def __init__(self, db_path: str = None):
        if db_path is None:
            # 默认使用 data 目录下的数据库
            db_dir = os.path.join(os.path.dirname(__file__), '../data')
            os.makedirs(db_dir, exist_ok=True)
            db_path = os.path.join(db_dir, 'gauge_readings.db')
        
        self.db_path = db_path
        self._init_database()
    
    def _init_database(self):
        """初始化数据库表"""
        with sqlite3.connect(self.db_path) as conn:
            conn.text_factory = str
            cursor = conn.cursor()
            
            # 创建仪表配置表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS gauge_configs (
                    gauge_type TEXT PRIMARY KEY,
                    unit TEXT DEFAULT '%',
                    min_range REAL DEFAULT 0.0,
                    max_range REAL DEFAULT 100.0,
                    low_threshold REAL DEFAULT 0.0,
                    high_threshold REAL DEFAULT 100.0,
                    display_name TEXT,
                    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
                )
            """)
            
            conn.commit()
            print("[SUCCESS] 仪表配置表初始化完成")
    
    def get_config(self, gauge_type: str) -> Optional[Dict]:
        """
        获取指定仪表的配置
        
        Args:
            gauge_type: 仪表类型（如 "压力表"）
        
        Returns:
            配置字典，如果不存在则返回默认配置
        """
        with sqlite3.connect(self.db_path) as conn:
            conn.row_factory = sqlite3.Row
            conn.text_factory = str
            cursor = conn.cursor()
            
            cursor.execute("""
                SELECT * FROM gauge_configs WHERE gauge_type = ?
            """, (gauge_type,))
            
            row = cursor.fetchone()
            
            if row:
                return dict(row)
            else:
                # 返回默认配置
                return {
                    'gauge_type': gauge_type,
                    'unit': '%',
                    'min_range': 0.0,
                    'max_range': 100.0,
                    'low_threshold': 0.0,
                    'high_threshold': 100.0,
                    'display_name': gauge_type,
                    'updated_at': None
                }
    
    def get_all_configs(self) -> Dict[str, Dict]:
        """
        获取所有仪表的配置
        
        Returns:
            字典，key 为 gauge_type，value 为配置
        """
        with sqlite3.connect(self.db_path) as conn:
            conn.row_factory = sqlite3.Row
            conn.text_factory = str
            cursor = conn.cursor()
            
            cursor.execute("SELECT * FROM gauge_configs")
            
            configs = {}
            for row in cursor.fetchall():
                config = dict(row)
                configs[config['gauge_type']] = config
            
            return configs
    
    def save_config(self, gauge_type: str, config: Dict) -> bool:
        """
        保存仪表配置
        
        Args:
            gauge_type: 仪表类型
            config: 配置字典，包含 unit, min_range, max_range, low_threshold, high_threshold, display_name
        
        Returns:
            是否成功
        """
        try:
            with sqlite3.connect(self.db_path) as conn:
                conn.text_factory = str
                cursor = conn.cursor()
                
                cursor.execute("""
                    INSERT OR REPLACE INTO gauge_configs 
                    (gauge_type, unit, min_range, max_range, low_threshold, high_threshold, display_name, updated_at)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """, (
                    gauge_type,
                    config.get('unit', '%'),
                    float(config.get('min_range', 0.0)),
                    float(config.get('max_range', 100.0)),
                    float(config.get('low_threshold', 0.0)),
                    float(config.get('high_threshold', 100.0)),
                    config.get('display_name', gauge_type),
                    datetime.now().isoformat()
                ))
                
                conn.commit()
                print(f"[SUCCESS] 已保存仪表配置: {gauge_type}")
                return True
                
        except Exception as e:
            print(f"[ERROR] 保存仪表配置失败: {e}")
            return False
    
    def delete_config(self, gauge_type: str) -> bool:
        """删除仪表配置"""
        try:
            with sqlite3.connect(self.db_path) as conn:
                conn.text_factory = str
                cursor = conn.cursor()
                cursor.execute("DELETE FROM gauge_configs WHERE gauge_type = ?", (gauge_type,))
                conn.commit()
                return cursor.rowcount > 0
        except Exception as e:
            print(f"[ERROR] 删除仪表配置失败: {e}")
            return False
    
    def convert_reading(self, gauge_type: str, percentage: float) -> Dict:
        """
        将百分比读数转换为实际值
        
        Args:
            gauge_type: 仪表类型
            percentage: 百分比值（0-100）
        
        Returns:
            包含 value, unit, is_alarm 的字典
        """
        config = self.get_config(gauge_type)
        
        # 转换为实际值
        min_range = config['min_range']
        max_range = config['max_range']
        
        # percentage 是 0-100，转换为 min_range ~ max_range
        actual_value = min_range + (percentage / 100.0) * (max_range - min_range)
        
        # 判断是否报警
        low_threshold = config['low_threshold']
        high_threshold = config['high_threshold']
        is_alarm = actual_value < low_threshold or actual_value > high_threshold
        
        return {
            'value': actual_value,
            'unit': config['unit'],
            'is_alarm': is_alarm,
            'display_name': config['display_name'],
            'percentage': percentage
        }


# 全局单例
_gauge_config_manager = None

def get_gauge_config_manager() -> GaugeConfigManager:
    """获取仪表配置管理器单例"""
    global _gauge_config_manager
    if _gauge_config_manager is None:
        _gauge_config_manager = GaugeConfigManager()
    return _gauge_config_manager


# 测试代码
if __name__ == "__main__":
    manager = GaugeConfigManager()
    
    # 测试保存配置
    print("\n=== 测试保存配置 ===")
    manager.save_config("压力表", {
        'unit': 'MPa',
        'min_range': 0.0,
        'max_range': 10.0,
        'low_threshold': 2.0,
        'high_threshold': 8.0,
        'display_name': '主压力表'
    })
    
    # 测试读取配置
    print("\n=== 测试读取配置 ===")
    config = manager.get_config("压力表")
    print(f"配置: {config}")
    
    # 测试转换读数
    print("\n=== 测试转换读数 ===")
    result = manager.convert_reading("压力表", 50.0)  # 50% → 5.0 MPa
    print(f"50% → {result['value']:.2f} {result['unit']}, 报警: {result['is_alarm']}")
    
    result = manager.convert_reading("压力表", 90.0)  # 90% → 9.0 MPa (超过阈值)
    print(f"90% → {result['value']:.2f} {result['unit']}, 报警: {result['is_alarm']}")
    
    # 测试获取所有配置
    print("\n=== 测试获取所有配置 ===")
    all_configs = manager.get_all_configs()
    print(f"所有配置: {all_configs}")

