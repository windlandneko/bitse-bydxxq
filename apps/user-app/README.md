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
- `mobile-flow`：自动启动随机端口、临时数据库的服务，通过 QML 按钮验证登录、精确到分的充值、昵称编辑、预约取消、充电、第二客户端订单恢复、结算、小票和退出；完成后销毁临时数据。

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
