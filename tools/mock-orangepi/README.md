# mock-orangepi —— 香橙派视觉服务本地假实现

供 **App 端离线对接联调**（无需真香橙派/真机）。镜像成员B的真实栈（FastAPI + Uvicorn）与
`contracts/vision-stream-api.md` v1.0：推送样例 JPEG 帧 + `frame_meta` JSON，并实现全部 REST 端点。

## 跑起来

```bash
cd tools/mock-orangepi
pip install -r requirements.txt
python server.py                  # 监听 0.0.0.0:8000
```

App 端把香橙派地址改成**跑本脚本的机器 IP**（首页/视觉页可配置，或 storage 默认 192.168.1.5）：
- 视频流：`ws://<本机IP>:8000/ws/video`
- REST：`http://<本机IP>:8000/api/...`

> 平板与 PC 需同一局域网；Windows 防火墙首次放行 8000 端口。

## 验证点（对应 docs/vision-integration-plan.md V1）

1. **WS 视频流**：连上后每帧两条交替消息——二进制 JPEG（画面含表盘 + 红指针 + 4 关键点 + 读数/FPS 叠加）
   与文本 `frame_meta`。指针随正弦摆动，`gauge_angles` 在 0~100 间变化。
2. 🔴 **关键点 name 映射**：mock 故意按**模型真实顺序** `[pointer_tip, center, zero_mark, full_mark]`
   推送（≠契约表 index `[center, pointer_tip, ...]`）。App 若**按 index** 取点，指针/读数会错乱；
   **按 name** 取（`model/vision.ets` 的 `keypointByName`）才正确——这正是用来抓这个坑的。
3. **REST**：`curl http://localhost:8000/api/summary`、`/api/video/status`、`/api/history`、
   `/api/gauge/configs` 等返回契约结构。
4. **错误/重连**：`MOCK_ERROR=1 python server.py` → WS 连上即推 `{"type":"error"}` 后断开，
   验证 App 的 `onStreamError` + 退避重连。

## 离线 schema 自检（不起服务）

```bash
python server.py --selftest
```

打印一帧 `frame_meta` 并断言契约必需字段齐全、4 个关键点 name 完整。

## 说明

- 这是 App 侧自建的联调脚手架；成员B 后续若在 `contracts/fixtures/` 放真实样例（几帧 JPEG + JSON），
  可直接据此校准（见 `contracts/integration-qa.md` 待办）。
- 仅用于开发联调，**不代表**香橙派真实推理；读数为合成正弦，非真实仪表。
