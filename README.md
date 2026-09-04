# BITSE-BYDXXQ

本仓库是北京理工大学「软件工程综合实践」小学期课程项目。

## 目录结构

```text
bydxxq/
├── .editorconfig          # 编辑器风格配置
├── .clang-format          # C++ 代码格式化配置
├── .vscode/               # clangd 配置
├── CMakeLists.txt         # 顶层构建配置（CMake 子工程）
├── CMakePresets.json      # 构建预设：Debug / Release
├── database/              # SQLite 建库与演示数据脚本
│   ├── schema.sql         # 表、索引和视图
│   └── seed.sql           # 开发/演示种子数据
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
    ├── setup_env.sh
    └── format-cpp.sh
```

其中 reference 内的文件仅供参考，以 docs 和 spec 为准。

## 工具链

全员在 WSL Ubuntu-22.04 + WSLg 环境下开发。

- Qt/C++：使用 C++17 标准，Qt Creator 6.0.2 + Qt 6。

- Web：使用 Vite + Vue + SQLite 技术栈；包管理工具使用 npm，格式化工具使用 oxfmt + oxlint。

- Python：包管理工具使用 uv。

## 团队协作与 Git 规范

建议使用 VSCode Git 可视化功能，并且装一个 gh 命令行，有什么不懂的地方让 AI 来做。

提交前先同步，单独开 PR 提交本轮更改，不要直接 commit 到 main 分支。
