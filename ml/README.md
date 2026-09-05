# 负荷预测与公开数据回测

模块提供业务预测与公开数据回测两个入口，均从仓库根目录运行。
C++服务通过JSON交换预测输入和结果，并负责数据库读写。

```sh
uv sync --project ml --frozen
uv run --project ml python -m ml.service --input /tmp/forecast-input.json --output /tmp/forecast-output.json
uv run --project ml python -m ml.backtest
uv run --project ml pytest ml/tests -q
uv run --project ml ruff check ml
```

管理端“负荷预测”可运行预测任务；后台启动命令和JSON字段见
`docs/接口契约.md`。

业务预测：

- 每桩按完整历史会话重叠秒数分摊到小时，kWh/完整小时等于平均 kW。最末不完整小时
  不作为已知训练标签；只用最近完整小时做显式持续值估计。
- 至少 14 天历史、48 个非零小时才能训练固定种子的随机森林；直接输出未来 24 个
  整点小时，不递归灌入未来真实值。稀疏历史为时段均值基线，无历史为短期状态持续估计。
- 每桩输出经额定功率约束，故障/离线/重启桩预测为零；站级电量严格为桩级之和，
  空闲桩估计保守取整且不超出可用设备数。1/6/24 小时均可直接读取。
- 高峰采用历史完整小时（含零值）的85分位数；无历史采用容量70%。相对需求高峰
  可用于提前值守，不代表过载故障。阈值随输出保存，且不使用未来值调阈值。
- 特征包含历史负荷、充电时段、星期/年周期和中国法定节假日/调休。
  可选输入 `weather:[{stationId,date:'YYYY-MM-DD',temperatureC,precipitationMm}]` 提供
  历史日天气；仅使用原点前一个已经结束的自然日，不使用未来实测天气。当前业务服务
  未配置天气源时按缺失值处理。
- 来源标记为“模拟业务数据”，附模型/基线、历史截止和天气来源。
  预测是小时平均负荷估计，不能等同于桩的瞬时遥测。模拟充电的时间倍率也会随结果记录。

公开数据回测：

```sh
uv run --project ml python -m ml.backtest \
  --data reference/datasets/jiaxing_2025/Dataset/Charging_Data.csv \
  --output ml/artifacts/jiaxing
```

数据由 [Figshare 嘉兴充电数据集](https://doi.org/10.6084/m9.figshare.28182251) 下载，
`Weather_Data.csv` 与交易 CSV 放在同一目录。原始数据不提交 Git。
脚本固定按时间前 80% 训练、后 20% 每 6 小时滚动原点测试，所有 24 步标签必须位于
训练截止时间前；标准化仅使用训练数据。特征使用原点已知的历史负荷、前一日真实天气
和法定日历。保留均匀分摊前后的总电量审计，使用 MAE/RMSE/WAPE 与上周同期基线比较。

生成 `report.json`、`report.md`、`models.pkl` 到已忽略的 artifacts 目录。
回测报告位于 `reports/jiaxing/`，当前随机森林误差高于周周期基线。
模型超参数固定，评估范围为嘉兴公开数据；业务站点使用自身历史单独训练。

中国日历来自 [chinese-calendar](https://github.com/liriansu-opus/chinese-calendar)，
当前锁定版本覆盖 2004–2026；超出年份会回退周末特征并标记日历未知，更新数据依赖后
再扩展年份。无需网络调用即可预测。
