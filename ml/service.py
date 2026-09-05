"""JSON-only forecast worker launched by the C++ service. No database access."""

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
    charger_results = []
    station_observed = {}
    for charger in station.get('chargers', []):
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
    for charger in station.get('chargers', []):
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
        # Short histories remain historical-hour estimates, explicitly labelled.
        if length >= 24 * 14 and np.count_nonzero(values) >= 48:
          x, y = training_samples(values, times, length, weather=weather)
          model = make_model().fit(x, y)
          # The current incomplete hour is a persistence nowcast. Mark it
          # explicitly; never train on unknown current/future measurements.
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
        # With no history, current occupancy only supports a short persistence
        # estimate. It is not a learned demand profile.
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
    hours = []
    for index, time in enumerate(future):
      load = sum(item['hours'][index]['loadKw'] for item in charger_results)
      usable = [
        c
        for c in station.get('chargers', [])
        if c.get('status') not in ('fault', 'offline', 'restarting', 'maintenance')
      ]
      capacity = sum(max(0.0, float(c.get('powerKw', 0))) for c in usable)
      # Availability is an estimate of equivalent occupied chargers, rounded
      # conservatively; it never exceeds the count of operational chargers.
      busy = sum(
        item['hours'][index]['loadKw'] / max(0.001, float(c.get('powerKw', 0)))
        for item, c in zip(charger_results, station.get('chargers', []))
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
    'source': f'课程演示业务数据 · {method}（非公开数据实时预测）',
    'stations': output_stations,
    'evaluation': {
      'trainedChargers': trained_count,
      'baselineChargers': baseline_count,
      'excludedSessions': rejected,
      'historyCutoff': iso(cutoff),
      'note': '会话电量按实际时间均匀分摊；当前未完成小时以最近完整小时持续值估计；1/6/24h为对应小时平均功率；非实测桩级瞬时功率。',
      'weatherSource': '外部提供的历史日天气'
      if payload.get('weather')
      else '未接天气源，天气特征带缺失标记；未伪造天气',
      'peakDefinition': '历史完整小时负荷85分位数（包含零负荷小时）；无历史按额定容量70%；表示相对高峰，不等同过载故障',
      'calendarSource': 'chinese-calendar 2004–2026；超出支持年份回退周末并标记未知',
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
