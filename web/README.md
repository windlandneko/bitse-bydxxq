# 充电运营指挥中心

Vue 3 + TypeScript + ECharts，1920×1080 设计尺寸等比适配，1366×768 无页面滚动。
大屏读取统一 C++ 服务 `/api/dashboard`，每 5 秒刷新。连接失败保留最近快照并显示
中断提示，统计快照超过 30 秒标记过期，预测超过 1 小时标记过期。

从仓库根目录执行：

```sh
pnpm --dir web install --frozen-lockfile
pnpm --dir web build
```

统一后端直接提供 `web/dist`。开发时先启动默认 8080 后端，再运行
`pnpm --dir web dev`，Vite 会代理 `/api` 到后端，无需另外配置跨域。
生产环境与开发环境均没有静态演示数据回退、CDN 或外网字体。

```sh
pnpm --dir web lint
pnpm --dir web format:check
pnpm --dir web validate:data
# 其他后端端口：
pnpm --dir web validate:data http://127.0.0.1:18080/api/dashboard
```

中心空间图以实际站点经纬度定位，点击光点或站名切换详情；明确标注“经纬度示意 ·
无道路底图”。四个 KPI、状态分布、排名、快慢充、营收趋势、时段热力图、预测和动态
均来自当前业务数据库聚合，不制造增长百分比。预测来源区分模型/基线和课程数据。
公开历史数据回测结果在 `ml/reports/jiaxing/` 独立记录，不冒充当前业务预测精度。

第三方实际复用与完整许可证见 `third-party/README.md`。
