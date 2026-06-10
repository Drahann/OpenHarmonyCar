#!/bin/bash
export PATH="/usr/local/miniconda3/bin:$PATH"
source /usr/local/miniconda3/etc/profile.d/conda.sh
conda activate base 2>/dev/null

cd /home/HwHiAiUser/simple_baselines/deployment_bundle
exec python3 image_server.py --port 8080 --results-dir /home/HwHiAiUser/simple_baselines/deployment_bundle
