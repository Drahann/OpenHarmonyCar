# 协作规范 / 新人上手

## 第一天（各角色）

**全员先做**：读根 `README.md` → 读你要对接的 `contracts/` → 按 `docs/network.md` 配网络。

- **App（owner）**：把 `W:\CarApp\CarApp` 迁入 `app-harmony/`（去掉构建产物，见该目录 README）。
  无真车时用 `tools/mock-purplepi/` 起本地假紫派联调。
- **紫派（成员A）**：在 `purplepi-control/` 建 `udp2lcm / wheel / nav / drivers` 目录；
  **先核对 `contracts/udp-protocol.md` 与 `contracts/lcm/`**，把 UDP↔LCM 桥按命令码表实现。
- **香橙派（成员B）**：在 `orangepi-vision/` 放 `detect / pose / reading / server_client`；
  先和 App 一起把 `contracts/server-api.md` 定稿（视频协议 + JSON schema）。

## 分支策略（从简，trunk-based）

- `main`：始终保持可集成、可编译。**不直接往 main 推**。
- 特性分支：`feat/<area>-<desc>`，`area ∈ {app, pi, vision, contracts, docs}`。
  例：`feat/app-map-render`、`feat/pi-udp2lcm`、`feat/vision-pose`。
- 修复：`fix/<area>-<desc>`。
- 通过 **Pull Request** 合回 main。**改 `contracts/` 的 PR 必须 @ 受影响的另一端并经其确认**。

## 提交信息规范

```
<scope>: <简述>

scope ∈ app | pi | vision | contracts | docs | tools
例：contracts: UDP 协议补充命令 'i' 的 IP 字段说明
    app: 地图 Canvas 渲染支持缩放
```

## 任务管理

- 用 GitHub **Issues** 记任务/缺陷，打 label（`app`/`pi`/`vision`/`contracts`/`bug`/`联调`）。
- 用一个 **Project 看板**（Todo / Doing / Done）跟踪进度与联调日清单。

## 建仓与首推（明天，owner 操作）

```powershell
# 仓库已 git init。确认忽略规则生效：
git status            # 不应出现 oh_modules/ build/ 等

# 关联 GitHub 远端并首推（替换为你们的私有库地址）
git add -A
git commit -m "chore: 初始化仓库骨架与接口契约"
git branch -M main
git remote add origin https://github.com/<org-or-user>/OpenHarmonyCar.git
git push -u origin main

# 邀请两名成员为 Collaborator（Settings → Collaborators），各自 clone 后从 main 开分支。
```

> 私有库建议：GitHub 上 New repository → Private → 不勾选任何模板（仓库里已有 README/.gitignore）。
