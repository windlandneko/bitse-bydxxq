"""Generate load forecasts from the C++ service JSON input."""

from __future__ import annotations

import argparse
import json
import math
from datetime import timedelta
from pathlib import Path

import numpy as np

from .forecasting import (
  HOUR,
  aggregate_sessions,
  features,
  floor_hour,
  iso,
  make_model,
  parse_time,
  training_samples,
)


def forecast(payload: dict) -> dict:
  generated = parse_time(payload['generatedAt'])
  cutoff = floor_hour(generated)
  observed, rejected = aggregate_sessions(payload.get('sessions', []), cutoff)
  baseline_count, trained_count = 0, 0
  output_stations = []
  # Forecast hour 1 is the next whole clock hour, strictly after generatedAt.
  future = [cutoff + HOUR * hour for hour in range(1, 25)]
  for station in payload.get('stations', []):
    chargers = station.get('chargers', [])
    charger_results = []
    station_observed = {}
    for charger in chargers:
      for timestamp, energy in observed.get(int(charger['id']), {}).items():
        station_observed[timestamp] = station_observed.get(timestamp, 0.0) + energy
    station_threshold = None
    if station_observed:
      first = max(min(station_observed), cutoff - timedelta(days=180))
      values = [
        station_observed.get(first + HOUR * i, 0.0) for i in range(int((cutoff - first) / HOUR))
      ]
      station_threshold = max(0.001, float(np.quantile(values, 0.85)))
    weather = {
      item['date']: [float(item['temperatureC']), float(item['precipitationMm'])]
      for item in payload.get('weather', [])
      if item.get('stationId') == station['id']
    }
    for charger in chargers:
      cid = int(charger['id'])
      power = max(0.0, float(charger.get('powerKw', 0)))
      history = observed.get(cid, {})
      model_kind = 'cold-start-baseline'
      peak_threshold = power * 0.7
      predicted = np.zeros(24)
      if history and power > 0:
        first = max(min(history), cutoff - timedelta(days=180))
        length = max(0, int((cutoff - first) / HOUR))
        times = [first + HOUR * index for index in range(length)]
        values = np.array([history.get(time, 0.0) / power for time in times])
        peak_threshold = max(0.001, float(np.quantile(values * power, 0.85)))
        if length >= 24 * 14 and np.count_nonzero(values) >= 48:
          x, y = training_samples(values, times, length, weather=weather)
          model = make_model().fit(x, y)
          # Estimate the incomplete hour from the latest complete hour.
          nowcast = values[-1]
          context = np.append(values, nowcast)
          predicted = (
            model.predict(features(context, length, times + [cutoff], weather).reshape(1, -1))[0]
            * power
          )
          model_kind = 'random-forest-direct-24h'
          trained_count += 1
        else:
          for index, time in enumerate(future):
            matches = [v for t, v in history.items() if t.hour == time.hour]
            # Include zero-transaction days in a same-hour denominator.
            days = max(1, sum(t.hour == time.hour for t in times))
            predicted[index] = sum(matches) / days
          model_kind = 'historical-hour-baseline'
          baseline_count += 1
      else:
        # Use current occupancy for a one-hour cold-start estimate.
        if charger.get('status') == 'charging':
          predicted[0] = power * 0.5
        baseline_count += 1
      unavailable = charger.get('status') in ('fault', 'offline', 'restarting', 'maintenance')
      if unavailable:
        predicted[:] = 0
      predicted = np.clip(predicted, 0, power)
      charger_results.append(
        {
          'chargerId': cid,
          'code': charger.get('code', str(cid)),
          'method': model_kind,
          'peakThresholdKw': round(peak_threshold, 3),
          'hours': [
            {
              'hour': i + 1,
              'time': iso(time),
              'loadKw': round(float(predicted[i]), 3),
              'isPeak': bool(power > 0 and predicted[i] >= peak_threshold),
            }
            for i, time in enumerate(future)
          ],
        }
      )
    usable = [
      c
      for c in chargers
      if c.get('status') not in ('fault', 'offline', 'restarting', 'maintenance')
    ]
    capacity = sum(max(0.0, float(c.get('powerKw', 0))) for c in usable)
    hours = []
    for index, time in enumerate(future):
      load = sum(item['hours'][index]['loadKw'] for item in charger_results)
      # Round occupied equivalents up to estimate available chargers conservatively.
      busy = sum(
        item['hours'][index]['loadKw'] / max(0.001, float(c.get('powerKw', 0)))
        for item, c in zip(charger_results, chargers)
      )
      hours.append(
        {
          'hour': index + 1,
          'time': iso(time),
          'loadKw': round(load, 3),
          'availableChargers': max(0, len(usable) - math.ceil(busy - 1e-9)),
          'isPeak': bool(
            capacity > 0
            and load >= (station_threshold if station_threshold is not None else capacity * 0.7)
          ),
        }
      )
    output_stations.append(
      {
        'stationId': station['id'],
        'stationName': station['name'],
        'peakThresholdKw': station_threshold,
        'hours': hours,
        'chargers': charger_results,
      }
    )
  method = (
    '随机森林与基线混合'
    if trained_count and baseline_count
    else ('随机森林' if trained_count else '历史/冷启动基线估计')
  )
  return {
    'generatedAt': iso(generated),
    'modelVersion': 'course-direct-rf-v2' if trained_count else 'course-baseline-v2',
    'source': f'模拟业务数据 · {method}',
    'stations': output_stations,
    'evaluation': {
      'trainedChargers': trained_count,
      'baselineChargers': baseline_count,
      'excludedSessions': rejected,
      'historyCutoff': iso(cutoff),
      'note': '会话电量按时长分摊；当前小时沿用最近完整小时估计；1/6/24h输出对应小时的平均功率。',
      'weatherSource': '外部提供的历史日天气'
      if payload.get('weather')
      else '未配置天气数据，特征按缺失处理',
      'peakDefinition': '历史完整小时负荷85分位数（含零负荷）；无历史按额定容量70%；用于识别需求高峰',
      'calendarSource': 'chinese-calendar，支持2004至2026年；其他年份使用周末特征并标记未知',
      'timeScale': payload.get('timeScale', 1),
    },
  }


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--input', type=Path, required=True)
  parser.add_argument('--output', type=Path, required=True)
  args = parser.parse_args()
  payload = json.loads(args.input.read_text(encoding='utf-8'))
  result = forecast(payload)
  args.output.parent.mkdir(parents=True, exist_ok=True)
  temporary = args.output.with_suffix(args.output.suffix + '.tmp')
  temporary.write_text(json.dumps(result, ensure_ascii=False, allow_nan=False), encoding='utf-8')
  temporary.replace(args.output)


if __name__ == '__main__':
  main()
