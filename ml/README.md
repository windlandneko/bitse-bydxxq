# 机器学习智能分析子系统（UC-M）

充电负荷智能预测：从历史充电会话中聚合逐小时负荷，训练随机森林回归模型，
预测未来 1 / 6 / 24 小时的各站充电负荷与空闲桩数，标记高峰时段并回写数据库。

## 需求对照

| 需求 | 实现 |
| --- | --- |
| UC-M-01 数据准备 | `dataset.py`：按 `(电站, 日期, 小时)` 聚合，特征含小时/星期/周末/节假日/滞后 1·24·168h/滑动均值/电站编码/模拟天气 |
| UC-M-02 模型 | `model.py`：`RandomForestRegressor`，MAE / RMSE / MAPE，与「上周同一时刻」基线对比 |
| UC-M-03 输出回写 | `predict.py` → `db.write_forecasts`：写入 `load_forecasts` 表 |
| UC-M-04 可复现性 | 固定随机种子；`train` / `predict` / `evaluate` 三种模式；产物 `ml/models/load_rf.pkl` |

## 目录结构

```text
ml/
├── config.py        # 路径与可调参数
├── db.py            # SQLite 访问：读会话 / 站点，回写 load_forecasts
├── dataset.py       # 合成时序、特征工程、递归预测
├── model.py         # 模型定义、指标、产物持久化
├── train.py         # 训练模式
├── predict.py       # 预测回写模式
├── evaluate.py      # 评估模式
├── requirements.txt
├── pyproject.toml
└── README.md
```

## 环境准备

项目约定使用 `uv` 管理 Python 依赖（见仓库 README）。二选一：

```bash
# 方式 A：uv
cd ml && uv sync

# 方式 B：pip
python3 -m pip install -r ml/requirements.txt
```

依赖：`numpy`、`pandas`、`scikit-learn`。

## 运行（三种模式）

脚本以包形式运行，需在**仓库根目录**执行：

```bash
# 1. 训练：生成模型并打印指标
python -m ml.train --db charge_platform.db

# 2. 评估：留出集指标 + 基线对比（需先训练）
python -m ml.evaluate --db charge_platform.db

# 3. 预测：预测未来 1/6/24 小时并回写 load_forecasts
python -m ml.predict --db charge_platform.db
```

`--db` 默认指向仓库根目录的 `charge_platform.db`，应与两个 Qt 客户端共用同一份
数据库文件。首次运行前请先初始化数据库（`database/schema.sql` + `database/seed.sql`，
或启动应用自动建库）。

`--horizons 1,6,24` 可调整预测时距；`--seed` 可覆盖随机种子。

## 数据说明（已知限制）

演示种子数据刻意稀疏（每站每天仅一单 @08:00），不足以让模型学到日内峰谷。
因此 `dataset.py` 会：

1. 生成**确定性**的合成逐小时负荷（日峰谷形状 × 周末系数 × 天气系数 × 站点系数），
   并按各站**桩容量**估算高峰负荷量级（单位 kW）；
2. 把真实充电会话按小时分摊后**叠加**到合成基线上，让真实数据也进入训练集。

这样既保留了真实充电记录，又让模型有足够的日内变化信号。合成与天气均用固定
种子，训练与预测严格可复现（UC-M-04）。

> 如需更真实的预测，可在 `seed.sql` 中增加每站每日多时段的充电会话，合成数据
> 仍会作为兜底参与训练。
