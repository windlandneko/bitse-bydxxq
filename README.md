# 电动汽车充电桩应用管理平台

本仓库是北京理工大学「软件工程综合实践」小学期课程项目。

## 目录结构

```text
charging-platform/
├── .editorconfig          # 编辑器风格配置
├── .clang-format          # C++ 代码格式化配置
├── .vscode/               # clangd 配置
├── CMakeLists.txt         # 顶层构建配置（CMake 子工程）
├── CMakePresets.json      # 构建预设：Debug / Release
├── database/              # SQLite 建库与演示数据脚本
│   ├── schema.sql         # 表、索引和视图
│   └── seed.sql           # 开发/演示种子数据
├── apps/                  # Qt 用户端与管理端
├── ml/                    # 机器学习负荷预测子系统（UC-M）
├── web/                   # Web 数据大屏（UC-W）
├── spec/                  # 需求与验收清单（for AI）
│   └── TODO.md            # 待实现功能和测试用例
├── docs/                  # 关键设计与使用文档（for human）
│   ├── 00_初始化指南.md    # 环境搭建流程
│   ├── 数据库架构设计.md    # 数据分层、约束和事务边界
│   └── 数据库函数调用说明.md # 服务层数据库调用契约
├── reference/             # 课程要求与参考材料
│   ├── 东软电动汽车充电桩应用管理平台项目要求书.md
│   ├── 需求规格说明书.md
│   └── images/
└── scripts/               # 环境与格式化脚本
```

其中 reference 内的文件仅供参考，以 docs 和 spec 为准。

## 工具链

全员在 WSL Ubuntu-22.04 + WSLg 环境下开发。

- Qt/C++：使用 C++17 标准，Qt Creator 6.0.2 + Qt 6。

- Web：使用 Vite + Vue + SQLite 技术栈；包管理工具使用 npm，格式化工具使用 oxfmt + oxlint。

- Python：包管理工具使用 uv。

## 构建与运行 Qt 应用

```bash
cmake --preset debug
cmake --build build/debug -j

./build/debug/apps/user-app/charging-user
./build/debug/apps/admin-app/charging-admin
```

默认同时构建两个应用。也可以在配置时使用 `BUILD_USER_APP` 或 `BUILD_ADMIN_APP` 开关单独构建，例如：

```bash
cmake -S . -B build/user-only -DBUILD_ADMIN_APP=OFF
```

## 构建与运行 Web 数据大屏

Web 子系统对应需求中的大屏（UC-W），由 Vue 3 + ECharts 展示聚合后的 JSON 数据。浏览器不直接连接 SQLite。

```bash
cd web
npm install
npm run dev
```

默认演示数据位于 `web/public/data/dashboard.json`。连接真实数据库时，从仓库根目录执行：

```bash
python scripts/export_dashboard.py \
  --db database/charge_platform.db \
  --out web/public/data/dashboard.json
```

详细说明见 [`web/README.md`](web/README.md)。

## 构建与运行机器学习负荷预测

机器学习子系统对应需求中的智能分析（UC-M），使用 Python + scikit-learn 从历史充电会话训练负荷预测模型，并把未来 1/6/24 小时预测回写 `load_forecasts`。

```bash
# 安装依赖（uv 或 pip 二选一）
cd ml && uv sync                       # 或 python3 -m pip install -r ml/requirements.txt

# 训练 → 评估 → 预测回写（在仓库根目录执行）
python -m ml.train    --db charge_platform.db
python -m ml.evaluate --db charge_platform.db
python -m ml.predict  --db charge_platform.db
```

详细说明见 [`ml/README.md`](ml/README.md)。

## 团队协作与 Git 规范

建议使用 VSCode Git 可视化功能，并且装一个 gh 命令行，有什么不懂的地方让 AI 来做。

提交前先同步，单独开 PR 提交本轮更改，不要直接 commit 到 main 分支。
