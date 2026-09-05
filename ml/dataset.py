"""数据准备与特征工程（对应 UC-M-01）。

历史充电会话经 db.load_hourly_load 得到逐小时负荷后，这里负责：

1. 生成真实感合成时序：演示种子数据过于稀疏（每站每天仅一单 @08:00），
   无法支撑模型学到日内峰谷，因此用确定性的合成负荷补齐，并按各站真实
   总电量做标定，使合成数据量级与真实一致；
2. 叠加真实观测值（真实数据覆盖合成基线）；
3. 构造时间 / 天气 / 滞后 / 滑动均值特征；
4. 生成未来特征并做递归多步预测。

所有随机来源都用固定种子，保证训练与预测可复现（UC-M-04）。
"""
from __future__ import annotations

import math

import numpy as np
import pandas as pd

from . import config, db

# 数值特征列（与需求 UC-M-01 对齐）
NUMERIC_FEATURES = [
    "hour",
    "dow",
    "is_weekend",
    "is_holiday",
    "temp",
    "precip",
    "lag_1",
    "lag_24",
    "lag_168",
    "rolling_mean_24",
]

# 典型日充电负荷形状（0~23 时，相对值），反映早/晚高峰与夜间低谷
_BASE_PROFILE = np.array(
    [
        0.10, 0.06, 0.04, 0.04, 0.05, 0.10, 0.25, 0.55, 0.70, 0.60,
        0.45, 0.40, 0.42, 0.45, 0.50, 0.55, 0.60, 0.70, 0.85, 0.95,
        0.90, 0.75, 0.55, 0.30,
    ]
)

# 法定节假日（用于 is_holiday 特征；当前演示窗口 2026-08 无节假日，可按需补充）
_HOLIDAYS: set[str] = set()


def build_feature_names(station_ids: list[int]) -> list[str]:
    """返回特征列名：电站 one-hot + 数值特征。"""
    return [f"station_id_{sid}" for sid in sorted(station_ids)] + NUMERIC_FEATURES


def _weather(hours: pd.DatetimeIndex, seed: int) -> tuple[np.ndarray, np.ndarray]:
    """确定性模拟天气（温度 ℃ 与是否降水），每个整点只由时间戳与种子决定。"""
    temp = np.empty(len(hours), dtype=float)
    precip = np.zeros(len(hours), dtype=float)
    for i, t in enumerate(hours):
        hkey = int(t.timestamp() // 3600)
        rng = np.random.default_rng(seed ^ hkey)
        doy = t.dayofyear
        seasonal = 22.0 + 8.0 * math.sin(2.0 * math.pi * (doy - 105) / 365.0)
        diurnal = 5.0 * math.sin(2.0 * math.pi * (t.hour - 14) / 24.0)
        temp[i] = seasonal + diurnal + rng.normal(0.0, 1.5)
        precip[i] = 1.0 if rng.random() < 0.15 else 0.0
    return temp, precip


def _weather_factor(temp: np.ndarray, precip: np.ndarray) -> np.ndarray:
    """天气对负荷的影响：降水显著降低，温度偏离舒适区轻微降低。"""
    f = np.ones_like(temp)
    f = np.where(precip > 0.0, f * 0.65, f)
    f = f * (1.0 - 0.12 * np.clip(np.abs(temp - 20.0) / 15.0, 0.0, 1.0))
    return f


def _peak_target_kw(st: pd.Series) -> float:
    """由桩容量估算该站典型高峰负荷(kW)：总容量 × 利用率。"""
    total_capacity = float(st["total_chargers"]) * float(st["avg_charger_power_kw"])
    return max(8.0, total_capacity * 0.18)


def generate_synthetic_history(
    stations: pd.DataFrame,
    hours: pd.DatetimeIndex,
    seed: int,
    weather_seed: int = config.WEATHER_SEED,
) -> pd.DataFrame:
    """生成各站在 hours 上的逐小时合成负荷（含天气列）。

    演示种子数据过于稀疏（每站每天仅一单），不足以反映真实站点负荷量级，
    因此按桩容量估算各站高峰负荷，生成确定性的日内峰谷曲线（单位 kW）。
    """
    # Keep weather independent from the synthetic-load noise seed.  The same
    # fixed weather series must be used while training and forecasting.
    temp, precip = _weather(hours, weather_seed)
    wf = _weather_factor(temp, precip)
    hour_arr = hours.hour.to_numpy()
    dow_arr = hours.dayofweek.to_numpy()
    weekend_mask = (dow_arr >= 5).astype(float)

    rng = np.random.default_rng(seed)
    frames: list[pd.DataFrame] = []
    for _, st in stations.iterrows():
        sid = int(st["station_id"])
        load = _BASE_PROFILE[hour_arr] * (0.82 * weekend_mask + 1.0 * (1.0 - weekend_mask))
        load = load * wf
        load = load * rng.normal(1.0, 0.08, size=len(hours))
        load = np.maximum(load, 0.0)
        # 站点差异系数（确定性）
        load = load * (0.85 + 0.10 * (sid % 5))
        # 缩放到该站高峰目标量级（kW）
        load = load * (_peak_target_kw(st) / float(_BASE_PROFILE.max()))

        frames.append(
            pd.DataFrame(
                {
                    "station_id": sid,
                    "ts": hours,
                    "load_kw": load,
                    "temp": temp,
                    "precip": precip,
                }
            )
        )
    return pd.concat(frames, ignore_index=True)


def _time_window(real: pd.DataFrame) -> tuple[pd.Timestamp, pd.Timestamp]:
    """由真实数据推导历史窗口；无数据时回退到最近 31 天。"""
    if real is not None and not real.empty:
        start = real["ts"].min().floor("h")
        end = real["ts"].max().ceil("h")
    else:
        end = pd.Timestamp.now(tz="UTC").floor("h")
        start = end - pd.Timedelta(days=31)
    return start, end


def _add_temporal_features(frame: pd.DataFrame) -> pd.DataFrame:
    frame = frame.copy()
    frame["hour"] = frame["ts"].dt.hour.astype(int)
    frame["dow"] = frame["ts"].dt.dayofweek.astype(int)
    frame["is_weekend"] = (frame["dow"] >= 5).astype(int)
    frame["is_holiday"] = (
        frame["ts"].dt.strftime("%Y-%m-%d").isin(_HOLIDAYS).astype(int)
    )
    return frame


def _add_lag_features(frame: pd.DataFrame) -> pd.DataFrame:
    frame = frame.sort_values(["station_id", "ts"]).copy()
    for lag in (1, 24, 168):
        frame[f"lag_{lag}"] = frame.groupby("station_id")["load_kw"].shift(lag)
    # The current load is the label, so it must not be part of a feature.
    # Forecasting uses the previous 24 hours as well.
    frame["rolling_mean_24"] = frame.groupby("station_id")["load_kw"].transform(
        lambda s: s.shift(1).rolling(24, min_periods=1).mean()
    )
    return frame


def build_training_frame(
    conn,
    seed: int,
    weather_seed: int = config.WEATHER_SEED,
) -> tuple[pd.DataFrame, pd.DataFrame]:
    """构造完整训练帧：真实负荷 + 合成负荷 + 全部特征。

    返回 (frame, stations)，frame 含数值特征与 load_kw，已按 (station_id, ts) 排序。
    """
    stations = db.load_stations(conn)
    real = db.load_hourly_load(conn)
    start, end = _time_window(real)
    # A static demo database may be older than the day on which the command is
    # run. Extend the deterministic synthetic history to the current UTC hour
    # so the generated horizons are genuinely in the future.
    end = max(end, pd.Timestamp.now(tz="UTC").floor("h"))
    hours = pd.date_range(start, end, freq="h", tz="UTC")

    synthetic = generate_synthetic_history(stations, hours, seed, weather_seed)

    # 真实观测叠加到合成基线之上（真实数据仍进入训练集）
    if real is not None and not real.empty:
        real_sum = (
            real.groupby(["station_id", "ts"], as_index=False)["load_kw"].sum()
        )
        synthetic = synthetic.merge(
            real_sum, on=["station_id", "ts"], how="left", suffixes=("", "_real")
        )
        synthetic["load_kw"] = synthetic["load_kw"] + synthetic["load_kw_real"].fillna(0.0)
        synthetic = synthetic.drop(columns=["load_kw_real"])

    frame = _add_temporal_features(synthetic)
    frame = _add_lag_features(frame)
    return frame, stations


def encode_frame(
    frame: pd.DataFrame,
    station_ids: list[int],
    feature_names: list[str],
) -> tuple[np.ndarray, np.ndarray]:
    """把帧编码为 (X, y)，并去掉含 NaN（早期滞后缺失）的行。"""
    df = frame.dropna(subset=[f"lag_{lag}" for lag in (1, 24, 168)]).copy()
    for sid in station_ids:
        df[f"station_id_{sid}"] = (df["station_id"].astype(int) == sid).astype(float)
    X = df[feature_names].to_numpy(dtype=float)
    y = df["load_kw"].to_numpy(dtype=float)
    return X, y


def encode_one(
    sid: int,
    numeric: dict[str, float],
    station_ids: list[int],
    feature_names: list[str],
) -> np.ndarray:
    """构造单条特征向量，与训练时列顺序严格一致。"""
    vec = np.zeros(len(feature_names), dtype=float)
    for j, name in enumerate(feature_names):
        if name == f"station_id_{sid}":
            vec[j] = 1.0
        elif name in numeric:
            vec[j] = numeric[name]
    return vec


def _feature_vector(
    sid: int,
    t: pd.Timestamp,
    load_map: dict[pd.Timestamp, float],
    weather_seed: int,
) -> dict[str, float]:
    """为 (sid, t) 构造数值特征字典（不含电站 one-hot）。"""
    temp, precip = _weather(pd.DatetimeIndex([t]), weather_seed)
    lag = lambda h: load_map.get(t - pd.Timedelta(hours=h), 0.0)
    window = [load_map.get(t - pd.Timedelta(hours=h), 0.0) for h in range(24, 0, -1)]
    return {
        "hour": float(t.hour),
        "dow": float(t.dayofweek),
        "is_weekend": 1.0 if t.dayofweek >= 5 else 0.0,
        "is_holiday": 1.0 if t.strftime("%Y-%m-%d") in _HOLIDAYS else 0.0,
        "temp": float(temp[0]),
        "precip": float(precip[0]),
        "lag_1": lag(1),
        "lag_24": lag(24),
        "lag_168": lag(168),
        "rolling_mean_24": float(np.mean(window)),
    }


def forecast_loads(
    artifact: dict,
    frame: pd.DataFrame,
    station_ids: list[int],
    horizons: tuple[int, ...],
) -> tuple[list[dict], pd.Timestamp]:
    """递归多步预测，返回 (预测行列表, 预测起点)。

    预测行含 station_id、horizon_start/end、predicted_load_kw 等，供回写。
    """
    model = artifact["model"]
    feature_names = artifact["feature_names"]
    weather_seed = artifact.get("weather_seed", config.WEATHER_SEED)

    # 历史负荷序列（含合成基线），供滞后特征查询
    load_map: dict[int, dict[pd.Timestamp, float]] = {}
    origin: pd.Timestamp | None = None
    for sid in station_ids:
        s = frame[frame["station_id"] == sid].set_index("ts")["load_kw"].sort_index()
        load_map[sid] = dict(s.items())
        if origin is None or s.index.max() > origin:
            origin = s.index.max()

    max_h = max(horizons)
    # 一次预测运行共用同一个 generated_at：大屏脚本按 MAX(generated_at) 取
    # 最新一批预测，若每行各自打时间戳会导致只取到最后一个时距的部分行。
    generated_at = db.now_utc_iso()
    rows: list[dict] = []
    for i in range(1, max_h + 1):
        t = origin + pd.Timedelta(hours=i)
        for sid in station_ids:
            numeric = _feature_vector(sid, t, load_map[sid], weather_seed)
            vec = encode_one(sid, numeric, station_ids, feature_names).reshape(1, -1)
            load = max(0.0, float(model.predict(vec)[0]))
            load_map[sid][t] = load
            if i in horizons:
                rows.append(
                    _forecast_row(artifact, sid, origin, t, load, generated_at)
                )
    return rows, origin


def _forecast_row(
    artifact: dict,
    sid: int,
    origin: pd.Timestamp,
    t: pd.Timestamp,
    load: float,
    generated_at: str,
) -> dict:
    """由预测负荷推导空闲桩数与高峰标记，组装一条回写记录。"""
    meta = artifact["station_meta"].get(sid, {})
    total = int(meta.get("total_chargers", 0))
    fault = int(meta.get("fault_chargers", 0))
    unavailable = int(meta.get("unavailable_chargers", fault) or 0)
    avg_power = float(meta.get("avg_charger_power_kw", 0.0) or 0.0)
    peak_threshold = float(meta.get("peak_threshold_kw", 0.0) or 0.0)

    if avg_power > 0:
        busy = math.ceil(load / avg_power)
    else:
        busy = 0
    available = max(0, min(total - unavailable, total - unavailable - busy))

    return {
        "station_id": sid,
        "forecast_time": origin.strftime("%Y-%m-%dT%H:%M:%SZ"),
        "horizon_start": t.strftime("%Y-%m-%dT%H:%M:%SZ"),
        "horizon_end": (t + pd.Timedelta(hours=1)).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "predicted_load_kw": round(load, 3),
        "predicted_available_chargers": available,
        "is_peak": 1 if load >= peak_threshold else 0,
        "model_version": artifact.get("model_version", config.MODEL_VERSION),
        "generated_at": generated_at,
    }
