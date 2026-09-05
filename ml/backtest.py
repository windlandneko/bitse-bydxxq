"""Reproducible Jiaxing public-data backtest, isolated from course live stations."""

from __future__ import annotations

import argparse
import json
import pickle
from datetime import timedelta, timezone
from pathlib import Path

import numpy as np
import pandas as pd

from .forecasting import (
  HOUR,
  features,
  floor_hour,
  iso,
  make_model,
  scores,
  seasonal_baseline,
  split_energy,
  training_samples,
)


def load_jiaxing(path: Path) -> tuple[dict, dict]:
  columns = ['Location Information', 'Start Time', 'End Time', 'Transaction power/kwh']
  raw = pd.read_csv(path, encoding='gb18030', usecols=columns)
  starts = pd.to_datetime(raw['Start Time'].str.strip(), errors='coerce')
  ends = pd.to_datetime(raw['End Time'].str.strip(), errors='coerce')
  energies = pd.to_numeric(raw['Transaction power/kwh'], errors='coerce')
  # Explicit range screening protects against broken dates; no quantile cutoffs
  # learned from future data, and no rows removed to improve reported errors.
  valid = (
    starts.notna()
    & ends.notna()
    & energies.notna()
    & np.isfinite(energies)
    & (energies >= 0)
    & (ends > starts)
    & ((ends - starts).dt.total_seconds() <= 7 * 86400)
  )
  values, energy_total = {}, 0.0
  zone = timezone(timedelta(hours=8))
  for station, start, end, energy in zip(
    raw.loc[valid, columns[0]].str.strip(), starts[valid], ends[valid], energies[valid]
  ):
    series = values.setdefault(station, {})
    for stamp, part in split_energy(
      start.to_pydatetime().replace(tzinfo=zone),
      end.to_pydatetime().replace(tzinfo=zone),
      float(energy),
    ):
      series[stamp] = series.get(stamp, 0.0) + part
    energy_total += float(energy)
  return values, {
    'rawSessions': len(raw),
    'acceptedSessions': int(valid.sum()),
    'excludedSessions': int((~valid).sum()),
    'inputEnergyKwh': round(energy_total, 6),
    'allocatedEnergyKwh': round(sum(sum(s.values()) for s in values.values()), 6),
    'screening': '起止时间有效、结束晚于开始、有限非负电量、会话不超过7天；不按未来分位数删除数据。',
  }


def load_weather(path: Path) -> dict:
  raw = pd.read_csv(path, encoding='gb18030')
  temperature = next(c for c in raw.columns if c.startswith('Temperature'))
  result = {}
  for row in raw.to_dict('records'):
    try:
      date = pd.to_datetime(str(row['Date']), format='%Y%m%d').strftime('%Y-%m-%d')
      values = [float(row[temperature]), float(row['Precipitation(mm)'])]
      if all(np.isfinite(values)):
        result.setdefault(row['District Name'], {})[date] = values
    except (ValueError, TypeError):
      continue
  return result


def run(path: Path, output: Path):
  sparse, metadata = load_jiaxing(path)
  weather_by_district = load_weather(path.with_name('Weather_Data.csv'))
  mapping = pd.read_csv(
    path, encoding='gb18030', usecols=['Location Information', 'District Name']
  ).drop_duplicates()
  station_district = dict(
    zip(mapping['Location Information'].str.strip(), mapping['District Name'])
  )
  start = floor_hour(min(min(series) for series in sparse.values()))
  end = floor_hour(max(max(series) for series in sparse.values()))
  times = [start + HOUR * i for i in range(int((end - start) / HOUR) + 1)]
  train_end = int(len(times) * 0.8)
  horizons = (1, 6, 24)
  pooled = {h: {'actual': [], 'model': [], 'baseline': []} for h in horizons}
  station_reports, artifacts = [], {}
  for station, hourly in sorted(sparse.items()):
    series = np.array([hourly.get(time, 0.0) for time in times])
    # Station normalization depends strictly on training observations.
    scale = max(1.0, float(np.quantile(series[:train_end], 0.95)))
    normalized = series / scale
    weather = weather_by_district.get(station_district.get(station), {})
    x, y = training_samples(normalized, times, train_end, stride=6, weather=weather)
    model = make_model().fit(x, y)
    origins = list(range(train_end - 1, len(series) - 24, 6))
    predicted = (
      model.predict(np.array([features(normalized, origin, times, weather) for origin in origins]))
      * scale
    )
    actual = np.array([series[o + 1 : o + 25] for o in origins])
    baseline = np.array([seasonal_baseline(series, origin) for origin in origins])
    station_reports.append(
      {
        'station': station,
        'trainingSamples': len(x),
        'testOrigins': len(origins),
        'horizons': {
          str(h): {
            'randomForest': scores(actual[:, h - 1], predicted[:, h - 1]),
            'weeklyBaseline': scores(actual[:, h - 1], baseline[:, h - 1]),
          }
          for h in horizons
        },
      }
    )
    for h in horizons:
      for key, array in [('actual', actual), ('model', predicted), ('baseline', baseline)]:
        pooled[h][key].extend(array[:, h - 1].tolist())
    artifacts[station] = {'model': model, 'scale': scale}
    print(f'{station}: train={len(x)}, test_origins={len(origins)}', flush=True)
  report = {
    'dataset': 'Jiaxing public EV charging transactions 2020–2021',
    'sourceUrl': 'https://doi.org/10.6084/m9.figshare.28182251',
    'scope': '公开历史数据离线回测；与课程业务站点独立，不代表当前实时预测精度。',
    'modelVersion': 'jiaxing-direct-rf-v2',
    'seed': 42,
    'method': '按时间前80%训练、后20%每6小时滚动原点测试；每次预测未来24小时，不向递归步骤提供未来真实值。允许原点前已到达的测试历史作为滞后观测。模型直接输出24步。固定超参数，无测试集调参。',
    'features': '原点小时/星期周期、已知负荷lag0/1/23/24/167、原点及此前24/168小时均值；中国法定节假日及调休、年周期；原点前一自然日已完成的真实温度/降水，缺失显式标记。未使用预测日真实天气。',
    'aggregation': '会话电量依据每个小时实际重叠秒数均匀分摊；无记录的小时视作零交易；数据采集完整性未知，这是评估限制。',
    'metrics': 'MAE/RMSE为kW；WAPE以测试真实总负荷作分母，避免零负荷MAPE爆炸。',
    'trainStart': iso(start),
    'trainEndExclusive': iso(times[train_end]),
    'testEnd': iso(end + HOUR),
    'hours': len(times),
    'stations': len(sparse),
    **metadata,
    'horizons': {
      str(h): {
        'samples': len(pooled[h]['actual']),
        'randomForest': scores(np.array(pooled[h]['actual']), np.array(pooled[h]['model'])),
        'weeklyBaseline': scores(np.array(pooled[h]['actual']), np.array(pooled[h]['baseline'])),
      }
      for h in horizons
    },
    'stationResults': station_reports,
  }
  output.mkdir(parents=True, exist_ok=True)
  (output / 'report.json').write_text(
    json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8'
  )
  with (output / 'models.pkl').open('wb') as stream:
    pickle.dump({'metadata': report, 'models': artifacts}, stream)
  rows = '\n'.join(
    f'| {h} 小时 | {report["horizons"][str(h)]["randomForest"]["maeKw"]} | {report["horizons"][str(h)]["randomForest"]["rmseKw"]} | {report["horizons"][str(h)]["weeklyBaseline"]["maeKw"]} |'
    for h in horizons
  )
  (output / 'report.md').write_text(
    f'# 嘉兴公开数据回测\n\n{report["scope"]}\n\n来源：[{report["dataset"]}]({report["sourceUrl"]})\n\n{report["method"]}\n\n训练：{report["trainStart"]} 至 {report["trainEndExclusive"]}（不含）；测试至 {report["testEnd"]}。\n\n输入 {metadata["rawSessions"]} 条，保留 {metadata["acceptedSessions"]} 条；分摊前后电量分别 {metadata["inputEnergyKwh"]} / {metadata["allocatedEnergyKwh"]} kWh。\n\n| 时距 | RF MAE (kW) | RF RMSE (kW) | 周期基线 MAE (kW) |\n|---|---:|---:|---:|\n{rows}\n\n限制：{report["aggregation"]} 回测历史需求与当前课程演示数据存在明显差异，不能把本报告指标当作业务站点的精度承诺。\n',
    encoding='utf-8',
  )
  print(json.dumps(report['horizons'], ensure_ascii=False, indent=2))


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument(
    '--data', type=Path, default=Path('reference/datasets/jiaxing_2025/Dataset/Charging_Data.csv')
  )
  parser.add_argument('--output', type=Path, default=Path('ml/artifacts/jiaxing'))
  args = parser.parse_args()
  run(args.data, args.output)


if __name__ == '__main__':
  main()
