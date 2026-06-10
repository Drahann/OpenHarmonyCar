#!/bin/bash
# 推理结果图片服务器启动脚本
# 用法: bash start_image_server.sh [port]

PORT=${1:-8080}
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 激活环境
export PATH="/usr/local/miniconda3/bin:$PATH"
source /usr/local/miniconda3/etc/profile.d/conda.sh
conda activate base
source /usr/local/Ascend/ascend-toolkit/set_env.sh 2>/dev/null

echo "========================================"
echo "  仪表推理结果图片服务器"
echo "  端口: $PORT"
echo "  访问: http://192.168.1.5:$PORT"
echo "========================================"

cd "$SCRIPT_DIR"
python3 image_server.py --port "$PORT" --results-dir "$SCRIPT_DIR"
