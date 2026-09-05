"""预测模式（UC-M-03）：加载模型，预测未来负荷并回写 load_forecasts。

用法：
    python -m ml.predict --db database/charge_platform.db [--model ml/models/load_rf.pkl]
                         [--horizons 1,6,24]
"""
from __future__ import annotations

import argparse

from . import config, db, dataset, model


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="预测负荷并回写数据库")
    parser.add_argument("--db", default=str(config.DEFAULT_DB_PATH), help="SQLite 数据库路径")
    parser.add_argument("--model", default=str(config.MODEL_PATH), help="训练产物路径")
    parser.add_argument(
        "--horizons",
        default=",".join(map(str, config.HORIZONS_HOURS)),
        help="预测时距（小时），逗号分隔，如 1,6,24",
    )
    args = parser.parse_args(argv)

    horizons = tuple(int(h) for h in args.horizons.split(",") if h.strip())

    artifact = model.load_artifact(args.model)
    conn = db.connect(args.db)
    try:
        frame, stations = dataset.build_training_frame(
            conn,
            artifact.get("seed", config.RANDOM_SEED),
            artifact.get("weather_seed", config.WEATHER_SEED),
        )
        frame_station_ids = set(frame["station_id"].astype(int).unique())
        station_ids = [sid for sid in artifact["station_ids"] if sid in frame_station_ids]

        # Station capacity/status can change after training; use current DB
        # metadata for charger availability while retaining the model artifact
        # for feature columns and thresholds.
        station_meta = {
            int(sid): dict(meta)
            for sid, meta in artifact.get("station_meta", {}).items()
        }
        for _, station in stations.iterrows():
            sid = int(station["station_id"])
            station_meta.setdefault(sid, {}).update(
                {
                    "total_chargers": int(station["total_chargers"]),
                    "fault_chargers": int(station["fault_chargers"]),
                    "unavailable_chargers": int(station["unavailable_chargers"]),
                    "avg_charger_power_kw": float(station["avg_charger_power_kw"]),
                }
            )
        runtime_artifact = {**artifact, "station_meta": station_meta}

        rows, origin = dataset.forecast_loads(
            runtime_artifact, frame, station_ids, horizons
        )
        written = db.write_forecasts(conn, rows)
    finally:
        conn.close()

    print("=" * 56)
    print("机器学习智能分析子系统 —— 预测回写")
    print("=" * 56)
    print(f"预测起点(forecast_time): {origin.strftime('%Y-%m-%dT%H:%M:%SZ')}")
    print(f"预测时距              : {list(horizons)} 小时")
    print(f"写入 load_forecasts   : {written} 行（模型版本 {artifact['model_version']}）")
    print("-" * 56)
    for r in rows:
        print(
            f"  站 {r['station_id']:>2}  {r['horizon_start']} -> {r['horizon_end']}  "
            f"负荷 {r['predicted_load_kw']:>8.3f} kW  空闲桩 {r['predicted_available_chargers']:>2}  "
            f"{'★高峰' if r['is_peak'] else '  '}"
        )
    print("=" * 56)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
