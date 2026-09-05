"""预测模式（UC-M-03）：加载模型，预测未来负荷并回写 load_forecasts。

用法：
    python -m ml.predict --db charge_platform.db [--model ml/models/load_rf.pkl]
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
        frame, _stations = dataset.build_training_frame(conn, artifact["seed"])
        frame_station_ids = set(frame["station_id"].astype(int).unique())
        station_ids = [sid for sid in artifact["station_ids"] if sid in frame_station_ids]

        rows, origin = dataset.forecast_loads(
            artifact, frame, station_ids, horizons
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
