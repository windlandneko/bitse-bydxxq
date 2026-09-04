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
├── spec/                  # 需求/实现文档（for AI）
├── docs/                  # 普通文档（for human）
│   └── 00_初始化指南.md    # 环境搭建流程
├── reference/             # 课程要求与参考材料
│   ├── 东软电动汽车充电桩应用管理平台项目要求书.md
│   ├── 需求规格说明书.md
│   └── images/
├── scripts/               # 脚本目录
│   └── setup_env.sh
├── test-qt/               # 测试工程
└── test-qt-another/       # 测试工程
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

BYDXXQ
