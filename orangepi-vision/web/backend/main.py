import os
import json
import time
import math
import base64
import queue
import threading
import asyncio
from typing import Dict, Any, List, Tuple

from fastapi import FastAPI, WebSocket
from fastapi.responses import FileResponse, JSONResponse, StreamingResponse
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles

from deepseek_service import DEEPSEEK
from video_camera import camera
from data_storage import get_storage
from data_analyzer import get_analyzer
from gauge_config import get_gauge_config_manager

import cv2
import numpy as np
import asyncio

APP_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FRONTEND_DIR = os.path.join(APP_ROOT, 'frontend')

# 确保路径正确
if not os.path.exists(FRONTEND_DIR):
    raise RuntimeError(f"Frontend directory not found: {FRONTEND_DIR}\n"
                      f"Please run from the web directory or use the startup script: bash 启动服务.sh")

app = FastAPI(title="SimpleBaselines Web")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.mount("/static", StaticFiles(directory=FRONTEND_DIR, html=False), name="static")

@app.get("/")
def index():
    return FileResponse(os.path.join(FRONTEND_DIR, 'index.html'))

STATE: Dict[str, Any] = {
    "fps": 0.0,
    "latency_ms": 0.0,
    "frame_id": 0,
    "over_limit": False,
    "readings": [],  # 支持多个仪表：列表形式
}

HISTORY: List[Dict[str, Any]] = []
# 阈值：基于百分比 (0-100%)
THRESHOLDS = {"low": 0.0, "high": 100.0}

def check_gauge_alarm(percentage: float) -> Dict[str, Any]:
    """
    根据仪表百分比读数判断报警状态
    
    Args:
        percentage: 仪表读数百分比 (0-100%)
    
    Returns:
        包含读数、状态等信息的字典
    """
    if percentage is None or math.isnan(percentage):
        return {
            "value": float("nan"),
            "unit": "%",
            "label": "仪表读数",
            "status": "UNKNOWN"
        }
    
    status = "NORMAL"
    if percentage < THRESHOLDS["low"] or percentage > THRESHOLDS["high"]:
        status = "ALARM"
    
    return {
        "value": float(percentage),
        "unit": "%",
        "label": "仪表读数",
        "status": status
    }


@app.get("/api/summary")
def api_summary():
    return JSONResponse(STATE)


@app.get("/api/history")
def api_history():
    return JSONResponse({"history": HISTORY[-600:]})


@app.post("/api/thresholds")
async def api_thresholds(payload: Dict[str, Any]):
    THRESHOLDS.update({
        "low": float(payload.get("low", THRESHOLDS["low"])),
        "high": float(payload.get("high", THRESHOLDS["high"]))
    })
    return {"ok": True, "thresholds": THRESHOLDS}


@app.get("/api/llm/state")
def api_llm_state():
    state = DEEPSEEK.state
    return JSONResponse({
        "available": DEEPSEEK.available,
        "status": state.status,
        "detail": state.detail,
        "model": state.model_name,
        # 性能统计
        "total_queries": state.total_queries,
        "last_query_time": round(state.last_query_time, 2),
        "avg_query_time": round(state.avg_query_time, 2),
    })


@app.post("/api/llm/query")
async def api_llm_query(payload: Dict[str, Any]):
    """DeepSeek对话接口（流式SSE响应）"""
    if not DEEPSEEK.available:
        return JSONResponse({"error": "DeepSeek dependencies missing"}, status_code=503)

    message = str(payload.get("message", ""))
    history = payload.get("history", [])

    if not message.strip():
        return {"reply": ""}

    print(f"\n{'='*60}")
    print(f"[QUERY] [SSE流式] 问题: {message}")
    print(f"[START] 开始生成... (时间: {time.strftime('%H:%M:%S')})")
    
    # 暂停摄像头推理，释放 NPU 内存给 DeepSeek
    camera.pause()
    
    async def event_generator():
        """SSE事件生成器（真正的流式）"""
        # 创建一个队列用于线程间通信
        chunk_queue = queue.Queue()
        exception_holder = {}
        
        def run_generator_thread():
            """在独立线程中运行生成器，实时将chunk放入队列"""
            try:
                for chunk in DEEPSEEK.generate(message, history):
                    chunk_queue.put(chunk)  # 实时放入队列！
                    if chunk.get("done"):
                        break
            except Exception as e:
                exception_holder['error'] = e
            finally:
                chunk_queue.put(None)  # 发送结束信号
        
        # 启动生成线程
        gen_thread = threading.Thread(target=run_generator_thread, daemon=True)
        gen_thread.start()
        
        try:
            # 实时从队列中取出chunk并yield
            while True:
                # 使用非阻塞方式检查队列（避免阻塞事件循环）
                try:
                    chunk = chunk_queue.get(timeout=0.1)
                    
                    if chunk is None:  # 结束信号
                        break
                    
                    # SSE格式：data: {json}\n\n
                    yield f"data: {json.dumps(chunk, ensure_ascii=False)}\n\n"
                    
                    if chunk.get("done"):
                        break
                        
                except queue.Empty:
                    # 队列为空，继续等待（不阻塞其他请求）
                    await asyncio.sleep(0.01)
                    continue
            
            # 检查是否有异常
            if 'error' in exception_holder:
                error_chunk = {
                    "error": str(exception_holder['error']),
                    "reply": f"生成失败: {str(exception_holder['error'])}",
                    "done": True
                }
                yield f"data: {json.dumps(error_chunk, ensure_ascii=False)}\n\n"
                
        except Exception as e:
            error_chunk = {
                "error": str(e),
                "reply": f"SSE流错误: {str(e)}",
                "done": True
            }
            yield f"data: {json.dumps(error_chunk, ensure_ascii=False)}\n\n"
        finally:
            # 恢复摄像头
            camera.resume()
            # 等待线程结束
            gen_thread.join(timeout=1.0)
    
    # 返回StreamingResponse
    return StreamingResponse(
        event_generator(),
        media_type="text/event-stream; charset=utf-8",
        headers={
            "Cache-Control": "no-cache",
            "Connection": "keep-alive",
            "X-Accel-Buffering": "no",  # 禁用nginx缓冲
        }
    )


@app.get("/api/video/status")
def api_video_status():
    """获取摄像头状态"""
    return {
        "running": camera.initialized,
        "fps": float(camera.fps) if camera.initialized else 0,
        "frame_count": int(camera.frame_count) if camera.initialized else 0
    }


@app.websocket("/ws/video")
async def ws_video_stream(ws: WebSocket):
    """摄像头实时流 WebSocket"""
    await ws.accept()
    print("📺 WebSocket 连接已建立")
    
    try:
        # 确保摄像头已初始化
        if not camera.initialized:
            print("[WARNING] 摄像头未初始化，尝试初始化...")
            # 使用 video1 而非 video0（系统中只有 video1 和 video2）
            if not camera.init(video_source="0"):
                await ws.send_json({"type": "error", "message": "摄像头初始化失败"})
                await ws.close()
                return
        
        # 等待后台线程生成第一帧（最多等待10秒）
        wait_count = 0
        while camera.process_frame() is None and wait_count < 100:
            await asyncio.sleep(0.1)
            wait_count += 1
        
        if wait_count >= 100:
            await ws.send_json({"type": "error", "message": "等待视频帧超时"})
            return
        
        print("[SUCCESS] 开始发送视频流...")
        
        last_sent_frame_id = -1  # 记录上次发送的帧号，避免重复发送
        
        while True:
            # 获取最新帧（非阻塞）
            result = camera.process_frame()
            
            if result is None:
                # 队列暂时为空，等待一下
                await asyncio.sleep(0.05)
                continue
            
            # 跳过已发送的帧（避免重复发送同一帧）
            if result['frame_id'] == last_sent_frame_id:
                await asyncio.sleep(0.01)
                continue
            
            last_sent_frame_id = result['frame_id']
            
            # 发送图像（激进优化：大幅降低JPEG质量）
            _, buffer = cv2.imencode('.jpg', result['vis_frame'], [cv2.IMWRITE_JPEG_QUALITY, 50])  # 85→50
            await ws.send_bytes(buffer.tobytes())
            
            # 更新全局 STATE 用于 /api/summary
            gauge_angles = result.get('gauge_angles', [])
            readings_list = []
            
            if gauge_angles and len(gauge_angles) > 0:
                # 处理所有仪表的读数
                for idx, angle in enumerate(gauge_angles):
                    if angle is not None and not math.isnan(angle):
                        reading_info = check_gauge_alarm(angle)
                        reading_info['gauge_id'] = idx + 1  # 仪表编号从1开始
                        readings_list.append(reading_info)
            
            STATE["readings"] = readings_list
            STATE["fps"] = result['fps']
            STATE["latency_ms"] = result['inference_time_ms']
            STATE["frame_id"] = result['frame_id']
            
            # 组装契约格式 detections（contracts/vision-stream-api.md §2）：
            #   裸数组 [x,y,w,h,score] + keypoints_list → [{bbox:[x,y,w,h], score, keypoints:[{name,x,y,conf}]}]
            # 关键点 name 按模型实际输出顺序（0=指针 1=圆心 2=零刻度 3=满刻度）；平板一律按 name 取点
            KP_NAMES = ["pointer_tip", "center", "zero_mark", "full_mark"]
            raw_dets = result.get('detections', [])
            kp_lists = result.get('keypoints_list', [])
            det_objs = []
            for i, det in enumerate(raw_dets[:3]):
                if len(det) < 5:
                    continue
                kps = []
                if i < len(kp_lists) and kp_lists[i]:
                    for j, kp in enumerate(kp_lists[i][:4]):
                        kps.append({
                            "name": KP_NAMES[j] if j < len(KP_NAMES) else f"kp{j}",
                            "x": float(kp[0]), "y": float(kp[1]), "conf": float(kp[2])
                        })
                det_objs.append({
                    "bbox": [float(det[0]), float(det[1]), float(det[2]), float(det[3])],
                    "score": float(det[4]),
                    "keypoints": kps
                })

            # 发送元数据（契约 frame_meta）
            meta_data = {
                "type": "frame_meta",
                "frame_id": result['frame_id'],
                "timestamp": result['timestamp'],
                "fps": result['fps'],
                "inference_time_ms": result['inference_time_ms'],
                "yolo_time_ms": result['yolo_time_ms'],
                "pose_time_ms": result['pose_time_ms'],
                "num_detections": len(det_objs),
                "detections": det_objs,
                "gauge_angles": result.get('gauge_angles', [])
            }
            
            await ws.send_json(meta_data)
            
            # 让出控制权给事件循环（允许处理其他请求）
            await asyncio.sleep(0)
            
    except Exception as e:
        print(f"[ERROR] WebSocket 错误: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("📺 WebSocket 连接已关闭")


# 移除模拟视频流，只保留真实摄像头


# ========== 数据分析相关API ==========

@app.get("/api/data/stats")
async def get_data_statistics(
    start_time: str = None,
    end_time: str = None,
    gauge_type: str = None
):
    """
    获取数据统计（不生成报告）
    
    Query参数:
        start_time: 开始时间（ISO格式，如：2025-10-10T00:00:00）
        end_time: 结束时间（ISO格式）
        gauge_type: 仪表类型筛选（可选）
    """
    try:
        from datetime import datetime
        
        analyzer = get_analyzer()
        
        # 解析时间参数
        if start_time and end_time:
            start_dt = datetime.fromisoformat(start_time)
            end_dt = datetime.fromisoformat(end_time)
        else:
            # 默认最近24小时
            start_dt, end_dt = analyzer.get_default_time_range(hours=24)
        
        # 生成统计
        stats = analyzer.generate_statistics(start_dt, end_dt)
        
        # 应用仪表配置转换（百分比 → 实际值）
        manager = get_gauge_config_manager()
        
        for gauge_type, type_stats in stats.get('by_type', {}).items():
            config = manager.get_config(gauge_type)
            
            # 转换平均值、最小值、最大值
            min_range = config['min_range']
            max_range = config['max_range']
            
            type_stats['avg_actual'] = min_range + (type_stats['avg'] / 100.0) * (max_range - min_range)
            type_stats['min_actual'] = min_range + (type_stats['min'] / 100.0) * (max_range - min_range)
            type_stats['max_actual'] = min_range + (type_stats['max'] / 100.0) * (max_range - min_range)
            type_stats['unit'] = config['unit']
            type_stats['display_name'] = config['display_name']
            type_stats['low_threshold'] = config['low_threshold']
            type_stats['high_threshold'] = config['high_threshold']
            
            # 保留原始百分比值
            type_stats['avg_percent'] = type_stats['avg']
            type_stats['min_percent'] = type_stats['min']
            type_stats['max_percent'] = type_stats['max']
        
        return JSONResponse(content={
            "success": True,
            "statistics": stats
        })
        
    except Exception as e:
        import traceback
        traceback.print_exc()
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


@app.post("/api/data/report")
async def generate_analysis_report(
    start_time: str = None,
    end_time: str = None,
    hours: int = 24
):
    """
    生成分析报告（调用DeepSeek）
    
    Body参数（可选）:
        start_time: 开始时间（ISO格式）
        end_time: 结束时间（ISO格式）
        hours: 如果未指定时间，则使用最近N小时（默认24）
    """
    try:
        from datetime import datetime
        
        analyzer = get_analyzer()
        
        # 解析时间参数
        if start_time and end_time:
            start_dt = datetime.fromisoformat(start_time)
            end_dt = datetime.fromisoformat(end_time)
        else:
            start_dt, end_dt = analyzer.get_default_time_range(hours=hours)
        
        # 检查是否有数据
        storage = get_storage()
        stats = storage.get_statistics(start_dt, end_dt)
        
        if stats["total_readings"] == 0:
            return JSONResponse(content={
                "success": False,
                "error": "该时间段内没有数据"
            }, status_code=400)
        
        # 暂停摄像头推理，释放 NPU 内存给 DeepSeek
        print("[REPORT] 暂停摄像头以生成报告...")
        camera.pause()
        
        try:
            # 生成报告（异步）
            result = await analyzer.generate_report_with_deepseek(
                start_dt, end_dt, DEEPSEEK
            )
            
            return JSONResponse(content={
                "success": True,
                "report": result
            })
        finally:
            # 恢复摄像头
            print("[REPORT] 恢复摄像头运行")
            camera.resume()
        
    except Exception as e:
        import traceback
        traceback.print_exc()
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


@app.get("/api/data/reports")
async def list_reports(limit: int = 10, offset: int = 0):
    """
    获取历史报告列表
    
    Query参数:
        limit: 返回数量（默认10）
        offset: 偏移量（默认0）
    """
    try:
        storage = get_storage()
        reports = storage.get_reports(limit=limit, offset=offset)
        
        return JSONResponse(content={
            "success": True,
            "reports": reports,
            "count": len(reports)
        })
        
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


@app.get("/api/data/reports/{report_id}")
async def get_report_detail(report_id: int):
    """获取单个报告详情"""
    try:
        storage = get_storage()
        report = storage.get_report(report_id)
        
        if not report:
            return JSONResponse(content={
                "success": False,
                "error": "报告不存在"
            }, status_code=404)
        
        return JSONResponse(content={
            "success": True,
            "report": report
        })
        
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


@app.delete("/api/data/reports/{report_id}")
async def delete_report(report_id: int):
    """删除报告"""
    try:
        storage = get_storage()
        success = storage.delete_report(report_id)
        
        if not success:
            return JSONResponse(content={
                "success": False,
                "error": "报告不存在或删除失败"
            }, status_code=404)
        
        return JSONResponse(content={
            "success": True,
            "message": "报告已删除"
        })
        
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


@app.get("/api/data/reports/{report_id}/export")
async def export_report(report_id: int, format: str = "md"):
    """
    导出报告为文件
    
    Query参数:
        format: 导出格式（默认 md，支持 md/txt）
    """
    try:
        storage = get_storage()
        report = storage.get_report(report_id)
        
        if not report:
            return JSONResponse(content={
                "success": False,
                "error": "报告不存在"
            }, status_code=404)
        
        # 生成文件名
        from datetime import datetime
        created_at = datetime.fromisoformat(report['created_at'])
        filename = f"report_{report_id}_{created_at.strftime('%Y%m%d_%H%M%S')}.{format}"
        
        # 准备内容
        content = report['report_text'] or "暂无报告内容"
        
        # 添加元数据头部
        header = f"""# 仪表读数分析报告 #{report_id}

**生成时间**: {report['created_at']}  
**数据周期**: {report['period_start']} 至 {report['period_end']}  
**数据点数**: {report['data_points']}

---

"""
        full_content = header + content
        
        # 根据格式设置 MIME 类型
        if format == "txt":
            media_type = "text/plain; charset=utf-8"
        else:  # md
            media_type = "text/markdown; charset=utf-8"
        
        # 返回文件
        from fastapi.responses import Response
        return Response(
            content=full_content.encode('utf-8'),
            media_type=media_type,
            headers={
                "Content-Disposition": f"attachment; filename*=UTF-8''{filename}"
            }
        )
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


@app.get("/api/data/database-info")
async def get_database_info():
    """获取数据库信息（用于调试）"""
    try:
        storage = get_storage()
        info = storage.get_database_info()
        
        return JSONResponse(content={
            "success": True,
            "info": info
        })
        
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


# ==================== 仪表配置 API ====================

@app.get("/api/gauge/configs")
async def get_gauge_configs():
    """获取所有仪表配置"""
    try:
        manager = get_gauge_config_manager()
        configs = manager.get_all_configs()
        
        return JSONResponse(content={
            "success": True,
            "configs": configs
        })
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


@app.get("/api/gauge/configs/{gauge_type}")
async def get_gauge_config(gauge_type: str):
    """获取指定仪表的配置"""
    try:
        manager = get_gauge_config_manager()
        config = manager.get_config(gauge_type)
        
        return JSONResponse(content={
            "success": True,
            "config": config
        })
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


@app.post("/api/gauge/configs/{gauge_type}")
async def save_gauge_config(gauge_type: str, config: Dict[str, Any]):
    """
    保存仪表配置
    
    Body 参数:
        unit: 单位（如 MPa, °C）
        min_range: 量程最小值
        max_range: 量程最大值
        low_threshold: 低阈值（报警）
        high_threshold: 高阈值（报警）
        display_name: 显示名称
    """
    try:
        manager = get_gauge_config_manager()
        success = manager.save_config(gauge_type, config)
        
        if success:
            return JSONResponse(content={
                "success": True,
                "message": "配置已保存"
            })
        else:
            return JSONResponse(content={
                "success": False,
                "error": "保存失败"
            }, status_code=500)
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


@app.delete("/api/gauge/configs/{gauge_type}")
async def delete_gauge_config(gauge_type: str):
    """删除仪表配置（恢复为默认）"""
    try:
        manager = get_gauge_config_manager()
        success = manager.delete_config(gauge_type)
        
        return JSONResponse(content={
            "success": success,
            "message": "配置已删除" if success else "配置不存在"
        })
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


# ==================== 数据删除 API ====================

@app.delete("/api/data/readings/{gauge_type}")
async def delete_gauge_data(gauge_type: str):
    """
    删除指定仪表类型的所有读数
    
    Path参数:
        gauge_type: 仪表类型（如 "压力表"）
    """
    try:
        storage = get_storage()
        deleted_count = storage.delete_readings_by_type(gauge_type)
        
        return JSONResponse(content={
            "success": True,
            "deleted_count": deleted_count,
            "message": f"已删除 {gauge_type} 的 {deleted_count} 条读数"
        })
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


@app.delete("/api/data/readings")
async def delete_readings_by_time(start_time: str = None, end_time: str = None):
    """
    删除指定时间范围的读数
    
    Query参数:
        start_time: 开始时间（ISO格式，可选）
        end_time: 结束时间（ISO格式，可选）
        都不提供则删除所有读数
    """
    try:
        from datetime import datetime
        
        start_dt = datetime.fromisoformat(start_time) if start_time else None
        end_dt = datetime.fromisoformat(end_time) if end_time else None
        
        storage = get_storage()
        deleted_count = storage.delete_readings_by_time_range(start_dt, end_dt)
        
        return JSONResponse(content={
            "success": True,
            "deleted_count": deleted_count,
            "message": f"已删除 {deleted_count} 条读数"
        })
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


@app.post("/api/data/clear-all")
async def clear_all_data():
    """清空所有数据（读数和报告）- 危险操作！"""
    try:
        storage = get_storage()
        result = storage.clear_all_data()
        
        return JSONResponse(content={
            "success": True,
            "deleted": result,
            "message": f"已清空 {result['readings']} 条读数和 {result['reports']} 份报告"
        })
    except Exception as e:
        return JSONResponse(content={
            "success": False,
            "error": str(e)
        }, status_code=500)


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)


