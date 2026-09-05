# 智充出行用户端

Qt 6.2+ 的手机交互模拟客户端，运行在 Linux 桌面。`QQuickWidget` 承载 QML 页面，`QStackedWidget` 在主页面与老师要求的 `QWebEngineView` 内嵌腾讯导航之间切换。客户端通过共享 `ApiClient` 访问独立 C++ 服务，不打开 SQLite。

- 手机号免密登录、自动注册、修改昵称/头像和模拟充值。
- 预设区域、腾讯地址解析、距离排序、名称/地址搜索、价格/空闲排序和仅快充筛选。
- 预测空闲推荐、站点及电桩详情、预约/取消、充电进度、停止和钱包结算。
- 服务端保持充电状态；重新登录恢复未完成订单并提示前往结算。
- 历史订单与充电小票、个人信息、钱包和退出登录。
- 腾讯内嵌导航传递选中的起终点；支持驾车、步行，后者使用移动浏览器 UA。

`MobileController` 负责异步请求、页面状态、活动订单刷新、幂等键和错误提示。金额按整数分收发。写入操作期间禁用重复点击，过时会话及订单响应不覆盖新状态。昵称编辑使用本地草稿，后台余额刷新不会覆盖正在输入的内容。

## 构建与测试

按仓库根目录说明安装依赖、启动 `charging-server` 后运行 `charging-user`。可使用 `CHARGING_SERVER_URL` 指定服务地址，默认 `http://127.0.0.1:8080`。

构建启用 `BUILD_TESTING=ON` 时默认建立 `charging-user-flow-test`，可以用 `BUILD_MOBILE_FLOW_TESTS=OFF` 单独关闭。CTest 包含：

- `mobile-qml-smoke`：在 offscreen 模式加载全部 11 个页面。
- `mobile-flow`：自动启动随机端口、临时数据库的服务，通过 QML 按钮验证登录、精确到分的充值、昵称编辑、预约取消、充电、第二客户端订单恢复、结算、小票和退出；分别在 430×860 与 360×800 窗口运行，检查页面加载、最小点击区域、字号和金额基线，完成后销毁临时数据。

```bash
ctest --test-dir build/full -R 'mobile-' --output-on-failure
```

真实在线地图检查需要可用的图形会话与网络，单独运行，不作为离线 CI 的前置条件：

```bash
CHARGING_SERVER_URL=http://127.0.0.1:8080 CHARGING_UI_TEST_MAP=1 \
  build/full/apps/user-app/charging-user-flow-test embeddedNavigation -v1
```

`charging-user --smoke-test` 可直接检查页面加载；`--screenshot /tmp/mobile.png` 保存窗口截图；`--phone 13900000000 --page home` 用于指定演示账号与页面。截图时可以设置 `QT_SCALE_FACTOR=1` 保持一致分辨率。请对临时演示账号使用上述选项。

腾讯 URI 的 `referer` 参数使用应用名，符合[官方路线规划文档](https://lbs.qq.com/webApi/uriV1/uriGuide/uriWebRoute)。地址解析密钥由后端持有，不进入用户端。`assets/brand.svg` 复用仓库中的 `design/logo-final/charging-platform.svg`；图标沿用项目已有 Lucide 图标及许可。


## 界面约定与参考

保留绿色与奶白配色，以 `Theme.qml` 统一页面/卡片内边距 16、常规间距 8/16/24、图标 24、点击区域至少 48、顶部栏 64、底部导航 80。12 用于图标与文字的组合间距，4 用于标签等细节；正文 14/16、辅助信息至少 12、页面标题 22/28。普通卡片圆角 16、主视觉卡片 24。金额组件使用字体基线对齐，窄屏将空闲数量放在价格下方，保持原字号。

参考以下开源 Android 实现的组件关系与交互，使用 Qt Quick 重新实现，并未引入 Android 依赖或复制 Compose 代码：

- [Google Now in Android 的 TopAppBar](https://github.com/android/nowinandroid/blob/main/core/designsystem/src/main/kotlin/com/google/samples/apps/nowinandroid/core/designsystem/component/TopAppBar.kt)：独立导航/操作图标按钮、语义名称和可测试入口。
- [Google Jetsnack 的 Feed](https://github.com/android/compose-samples/blob/main/Jetsnack/app/src/main/java/com/example/jetsnack/ui/home/Feed.kt)：滚动内容与顶部目的地选择、筛选控件分层布局，并针对紧凑窗口检查。
- [Lucide 官方 SVG](https://github.com/lucide-icons/lucide/tree/main/icons)：返回、展开、导航、关闭与成功图标；许可保存在 `icons/LICENSE.lucide`。

车辆与充电桩主视觉使用透明插图 SVG，并在解析失败时回退 PNG；素材说明见 [插图 README](assets/illustrations/README.md)。在线地图额外检查真实鼠标点击站点卡片内的导航按钮，确保不误触整张卡片，并保存两个尺寸的导航页面截图。运行上述测试时可设置 `CHARGING_UI_ARTIFACT_DIR=/tmp/mobile-review` 生成按尺寸分组的截图。
