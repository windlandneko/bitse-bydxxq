# 东软电动汽车充电桩应用管理平台

北京理工大学「软件工程综合实践」小学期课程项目。以老师的
[项目要求书](reference/东软电动汽车充电桩应用管理平台项目要求书.md)
为唯一验收依据；`reference/需求规格说明书.md` 为辅助参考，不能覆盖老师原始要求。

已实现 Qt 手机模拟用户端、Qt 桌面管理端、统一 C++ 业务服务、SQLite、Web 运营大屏和
Python 负荷预测。两个客户端与大屏共享实时业务数据，可完整演示预约、充电、计费、结算
以及管理运维。手机号免密登录、充值、充电和设备重启均按课程要求模拟。

![运营大屏](docs/screenshots/dashboard.png)

## 快速运行

环境：Ubuntu 22.04 或更新版本（虚拟机 / WSLg 均可），C++17、Qt 6.2+、CMake、Node.js 22、
pnpm、Python 3.10+、uv。Qt Creator 6.2+ 可直接打开顶层 `CMakeLists.txt`。

```bash
# 首次配置依赖，详细选项见初始化指南
bash scripts/setup_env.sh
bash scripts/build.sh

# 终端一：统一业务服务，同时托管大屏，默认 127.0.0.1:8080
bash scripts/run.sh server

# 终端二、三：打开两个 Qt 客户端
bash scripts/run.sh user
bash scripts/run.sh admin
```

浏览器打开 **http://127.0.0.1:8080**。管理端默认账号 **admin / 123456**；用户端输入以1开头的
11 位数字手机号自动注册，可使用演示账号 **13800000001** 或 **13800000002**。
新用户余额为零，先模拟充值。服务首次启动自动创建五个上海模拟站点、三十个桩及近35天
历史订单，首次预测稍后自动生成；也可在管理端点击“运行预测”。

服务默认以 **60 倍速**模拟充电，便于课堂演示。实际时间、充电时长、电量和金额由服务端
统一计算。关闭客户端后订单继续；服务重启后重新登录即可恢复未完成订单。

```bash
# 完整静态检查、构建产物测试与业务验收
bash scripts/test.sh

# 新建一个不带演示数据的独立环境，原数据保留
bash scripts/run.sh server --data-dir /tmp/charging-empty --no-seed

# 调整模拟倍率或端口
bash scripts/run.sh server --time-scale 600 --port 8081
CHARGING_SERVER_URL=http://127.0.0.1:8081 bash scripts/run.sh user
```

手动地址解析需要腾讯地图 WebService Key，放到被 Git 忽略的本机 `data/map-settings.json`；
没有配置时仍可选择预设区域。格式、签名与 Qt WebEngine 运行说明见
[初始化指南](docs/00_初始化指南.md)。

## 实现与文档

| 模块 | 实现 | 内容 |
| --- | --- | --- |
| 用户端 | Qt Quick/QML + C++ + QWebEngineView | 手机式页面与底部导航、定位、找桩、驾车/步行地图、头像昵称、钱包、完整充电订单 |
| 管理端 | Qt Widgets + QChart | 登录、7/30日营收、站点和电桩管理、远程重启、用户冻结、订单、站桩预测与日志 |
| 业务服务 | C++ / Qt Network / Qt SQL | QTcpServer Socket 通信、QThread 数据库工作线程、事务、定时模拟、地图签名、静态文件托管 |
| 数据库 | SQLite 3 | 整数分金额、订单状态约束、唯一占用、钱包流水、请求幂等、审计与预测结果 |
| 大屏 | Vue 3 / Vite / ECharts | 五秒刷新、站点交互、七类图表、动态装饰边框、断线保留快照并提示 |
| 预测 | Python / scikit-learn | 每站每桩未来24小时、1/6/24时距、空闲桩估计、相对高峰预警、公开数据回测 |

- [需求对照与验收清单](spec/TODO.md)：逐条对应老师要求及代码/验证证据。
- [初始化指南](docs/00_初始化指南.md)：依赖、构建、启动、配置与常见问题。
- [系统设计](docs/系统设计.md)：线程、Socket、订单状态机、数据口径与错误处理。
- [数据库架构](docs/数据库架构设计.md) / [服务端数据库调用](docs/数据库函数调用说明.md)。
- [接口契约](docs/接口契约.md)：前后端 JSON、权限、金额、订单及预测协议。
- [验收报告与演示步骤](docs/验收报告.md)：可复跑检查、课堂演示和边界说明。
- [大屏与预测说明](docs/analytics.md) / [公开回测报告](ml/reports/jiaxing/report.md)。

## 目录

```text
apps/user-app/     Qt Quick 手机模拟应用
apps/admin-app/    Qt Widgets 管理应用
apps/server/       C++ 业务服务，唯一业务数据库访问入口
shared/            两客户端复用的异步 API 客户端
web/               大屏源码、锁文件和第三方许可证
ml/                JSON 预测服务、公开数据回测、单元测试与报告
database/         当前 schema 和旧版结构归档
scripts/           依赖配置、构建、启动、测试与统计快照工具
tests/             使用临时数据库和真实 Socket 的集成测试
docs/、spec/       设计、运行、验收材料
reference/         老师原始材料、公开数据来源说明
data/              本机自动生成数据库、配置、头像、日志；不提交
```

旧版 `database/charging_platform.db` 不被新服务读写，旧结构保留于 `database/legacy/`。
当前数据库为 `data/platform.db`，不自动迁移旧演示数据。原始公开数据、模型缓存、密码配置
和编译产物均不进入 Git。

预测结果会标注模型或基线、生成时间、数据来源；当前业务没有实时天气源，缺失情况明确
呈现。嘉兴公开数据回测中随机森林未超过周周期基线，报告如实保留结果；课程演示预测
不能作为真实电网调度精度承诺。大屏复用的 MIT SVG 边框及图标许可随源码保留。
