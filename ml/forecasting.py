"""Hourly energy accounting and chronological, direct 24-hour load forecasting.

A series value is energy in one full hour (kWh), numerically equal to that
hour's average power (kW). Features only read values at or before the origin.
"""

from __future__ import annotations

import math
from collections import defaultdict
from datetime import datetime, timedelta, timezone

import numpy as np
from chinese_calendar import get_holiday_detail
from sklearn.ensemble import RandomForestRegressor

HORIZON = 24
HOUR = timedelta(hours=1)


def parse_time(value: str) -> datetime:
  parsed = datetime.fromisoformat(value.strip().replace('Z', '+00:00'))
  return (
    parsed.replace(tzinfo=timezone.utc)
    if parsed.tzinfo is None
    else parsed.astimezone(timezone.utc)
  )


def iso(value: datetime) -> str:
  return value.astimezone(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')


def floor_hour(value: datetime) -> datetime:
  return value.replace(minute=0, second=0, microsecond=0)


def split_energy(start: datetime, end: datetime, energy: float):
  """Uniformly allocate a session by its exact overlap with each clock hour.

  Invalid records are rejected, never silently clamped into valid observations.
  Uniform charging inside a session is an explicit approximation, not metering.
  """
  seconds = (end - start).total_seconds()
  if seconds <= 0 or not math.isfinite(energy) or energy < 0:
    raise ValueError('Session duration must be positive and energy finite/nonnegative')
  cursor = floor_hour(start)
  while cursor < end:
    overlap = (min(end, cursor + HOUR) - max(start, cursor)).total_seconds()
    yield cursor, energy * overlap / seconds
    cursor += HOUR


def aggregate_sessions(sessions: list[dict], cutoff: datetime) -> tuple[dict, int]:
  """Only completed observations known at cutoff may enter training."""
  series = defaultdict(lambda: defaultdict(float))
  rejected = 0
  for item in sessions:
    try:
      start, end = parse_time(item['startedAt']), parse_time(item['endedAt'])
      energy = float(item['energyKwh'])
      if end > cutoff:
        rejected += 1
        continue
      pieces = list(split_energy(start, end, energy))
      for timestamp, value in pieces:
        if timestamp + HOUR <= cutoff:
          series[int(item['chargerId'])][timestamp] += value
    except (KeyError, ValueError, TypeError, OverflowError):
      rejected += 1
  return dict(series), rejected


def calendar_features(time: datetime) -> list[float]:
  local = time.astimezone(timezone(timedelta(hours=8)))
  try:
    nonworkday, holiday = get_holiday_detail(local.date())
    return [float(nonworkday), float(holiday is not None), 1.0]
  except NotImplementedError:
    return [float(local.weekday() >= 5), 0.0, 0.0]


def features(
  values: np.ndarray, origin: int, times: list[datetime], weather: dict | None = None
) -> np.ndarray:
  """The most recent observed hour is origin; no element after origin is read."""
  local = times[origin].astimezone(timezone(timedelta(hours=8)))
  hour, weekday = local.hour, local.weekday()
  # Daily measurements are only known after that local day finishes.
  previous_day = (local - timedelta(days=1)).strftime('%Y-%m-%d')
  observation = (weather or {}).get(previous_day)
  weather_features = [*observation, 1.0] if observation is not None else [0.0, 0.0, 0.0]
  return np.array(
    [
      math.sin(hour * math.tau / 24),
      math.cos(hour * math.tau / 24),
      math.sin(weekday * math.tau / 7),
      math.cos(weekday * math.tau / 7),
      values[origin],
      values[origin - 1],
      values[origin - 23],
      values[origin - 24],
      values[origin - 167],
      np.mean(values[origin - 23 : origin + 1]),
      np.mean(values[origin - 167 : origin + 1]),
      math.sin(local.timetuple().tm_yday * math.tau / 366),
      math.cos(local.timetuple().tm_yday * math.tau / 366),
      *calendar_features(local),
      *calendar_features(local + timedelta(days=1)),
      *weather_features,
    ],
    dtype=float,
  )


def training_samples(
  values: np.ndarray,
  times: list[datetime],
  train_end: int,
  stride: int = 3,
  weather: dict | None = None,
):
  """All 24 labels must precede exclusive train_end; split before extraction."""
  origins = range(167, train_end - HORIZON, stride)
  x, y = [], []
  for origin in origins:
    x.append(features(values, origin, times, weather))
    y.append(values[origin + 1 : origin + HORIZON + 1])
  return np.asarray(x), np.asarray(y)


def make_model() -> RandomForestRegressor:
  return RandomForestRegressor(
    n_estimators=64,
    max_depth=14,
    min_samples_leaf=4,
    random_state=42,
    n_jobs=1,
  )


def seasonal_baseline(values: np.ndarray, origin: int) -> np.ndarray:
  return np.array([values[origin + hour - 168] for hour in range(1, HORIZON + 1)])


def scores(actual: np.ndarray, predicted: np.ndarray) -> dict:
  error = np.asarray(actual) - np.asarray(predicted)
  denominator = float(np.abs(actual).sum())
  return {
    'maeKw': round(float(np.abs(error).mean()), 4),
    'rmseKw': round(float(np.sqrt(np.square(error).mean())), 4),
    'wapePercent': round(float(np.abs(error).sum()) / denominator * 100, 3)
    if denominator
    else None,
  }
