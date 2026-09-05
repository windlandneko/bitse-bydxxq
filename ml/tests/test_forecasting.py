from datetime import datetime, timedelta, timezone

import numpy as np
import pytest
from ml.forecasting import aggregate_sessions, features, split_energy, training_samples
from ml.service import forecast


def test_energy_conservation_partial_hours():
  start = datetime(2026, 1, 1, 8, 30, tzinfo=timezone.utc)
  allocation = list(split_energy(start, start + timedelta(hours=1), 10.0))
  assert [v for _, v in allocation] == [5.0, 5.0]
  assert sum(
    v for _, v in split_energy(start, start + timedelta(hours=27, minutes=13), 123.45)
  ) == pytest.approx(123.45)


def test_invalid_sessions_are_rejected():
  time = datetime(2026, 1, 1, tzinfo=timezone.utc)
  for end, energy in [
    (time, 1.0),
    (time + timedelta(hours=1), float('nan')),
    (time + timedelta(hours=1), -1),
  ]:
    with pytest.raises(ValueError):
      list(split_energy(time, end, energy))


def test_features_cannot_see_future_or_target():
  values = np.arange(500, dtype=float)
  times = [datetime(2026, 1, 1, tzinfo=timezone.utc) + timedelta(hours=i) for i in range(500)]
  before = features(values, 200, times)
  values[201:] = 999999
  assert np.array_equal(before, features(values, 200, times))
  assert before[9] == np.mean(values[177:201])


def test_training_labels_stay_before_split():
  values = np.arange(500, dtype=float)
  times = [datetime(2026, 1, 1, tzinfo=timezone.utc) + timedelta(hours=i) for i in range(500)]
  x, y = training_samples(values, times, 300, stride=1)
  assert y.max() == 299
  assert x[-1, 4] == 275


def test_aggregation_excludes_future_and_incomplete_hour():
  cutoff = datetime(2026, 1, 1, 9, tzinfo=timezone.utc)
  sessions = [
    {
      'chargerId': 1,
      'startedAt': '2026-01-01T08:30:00Z',
      'endedAt': '2026-01-01T09:30:00Z',
      'energyKwh': 10,
    }
  ]
  values, rejected = aggregate_sessions(sessions, cutoff)
  assert values == {}
  assert rejected == 1


def test_json_service_horizons_capacity_and_station_sums():
  payload = {
    'generatedAt': '2026-09-05T12:15:00Z',
    'stations': [
      {
        'id': 1,
        'name': 'Test',
        'chargers': [
          {'id': 1, 'code': 'A', 'powerKw': 60, 'status': 'charging'},
          {'id': 2, 'code': 'B', 'powerKw': 7, 'status': 'fault'},
        ],
      }
    ],
    'sessions': [],
  }
  result = forecast(payload)
  assert '基线' in result['source']
  station = result['stations'][0]
  assert len(station['hours']) == 24
  for index, hour in enumerate(station['hours']):
    assert hour['hour'] == index + 1
    assert hour['loadKw'] == pytest.approx(
      sum(c['hours'][index]['loadKw'] for c in station['chargers'])
    )
    assert 0 <= hour['availableChargers'] <= 1
    assert 0 <= station['chargers'][0]['hours'][index]['loadKw'] <= 60
    assert station['chargers'][1]['hours'][index]['loadKw'] == 0
  assert station['hours'][0]['time'] == '2026-09-05T13:00:00Z'
  assert station['hours'][5]['hour'] == 6
  assert station['hours'][23]['hour'] == 24


def test_previous_day_weather_and_calendar_no_lookahead():
  from ml.forecasting import calendar_features

  origin = datetime(2026, 10, 1, 4, tzinfo=timezone.utc)
  times = [origin - timedelta(hours=200 - i) for i in range(201)]
  values = np.ones(len(times))
  weather = {'2026-09-30': [18.0, 0.5], '2026-10-01': [9999.0, 9999.0]}
  vector = features(values, 200, times, weather)
  assert vector[-3:].tolist() == [18.0, 0.5, 1.0]
  assert calendar_features(origin) == [1.0, 1.0, 1.0]
  assert calendar_features(origin.replace(year=2035))[-1] == 0.0


def test_trained_service_is_current_and_has_consistent_units():
  generated = datetime(2026, 9, 5, 12, tzinfo=timezone.utc)
  sessions = []
  for index in range(24 * 21):
    start = generated - timedelta(hours=24 * 21 - index)
    sessions.append(
      {
        'chargerId': 1,
        'stationId': 1,
        'startedAt': start.isoformat(),
        'endedAt': (start + timedelta(minutes=30)).isoformat(),
        'energyKwh': 12.0 if 7 <= start.hour <= 20 else 1.0,
      }
    )
  result = forecast(
    {
      'generatedAt': generated.isoformat(),
      'stations': [
        {
          'id': 1,
          'name': 'Model test',
          'chargers': [{'id': 1, 'code': 'C1', 'powerKw': 60, 'status': 'idle'}],
        }
      ],
      'sessions': sessions,
    }
  )
  assert result['evaluation']['trainedChargers'] == 1
  assert result['modelVersion'] == 'course-direct-rf-v2'
  assert result['stations'][0]['hours'][0]['time'] == '2026-09-05T13:00:00Z'
  for point in result['stations'][0]['hours']:
    assert 0 <= point['loadKw'] <= 60
