# Web 数据大屏

这是 NCS 充电平台的 Vue 3 + ECharts 可视化大屏。浏览器只读取 `public/data/dashboard.json`，不直接连接 SQLite。

## 本地运行

```bash
cd web
npm install
npm run dev
```

打开终端输出的地址即可查看大屏。不要直接双击 `index.html`，否则浏览器可能阻止读取 JSON。

## 连接真实数据库

当前仓库中的 `database/` 目录包含建表和种子脚本；如果本机还没有 `charge_platform.db`，先通过项目初始化流程生成它，再执行导出。

从仓库根目录运行导出脚本：

```bash
python scripts/export_dashboard.py \
  --db database/charge_platform.db \
  --out web/public/data/dashboard.json
```

需要持续演示自动更新时，可以让脚本每 30 秒导出一次：

```bash
python scripts/export_dashboard.py \
  --db database/charge_platform.db \
  --out web/public/data/dashboard.json \
  --watch --interval 30
```

脚本读取实际数据库表 `users`、`stations`、`chargers`、`orders`、`charging_sessions` 和 `load_forecasts`，并原子写入前端数据文件。Web 页面每 30 秒自动读取一次。

## 构建验收包

```bash
npm run validate:data
npm run build
npm run preview
```

部署构建产物时，需要让导出脚本更新部署目录中的 `dist/data/dashboard.json`，或让静态服务器把 `/data` 指向一个独立的数据目录。
