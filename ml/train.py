"""训练模式（UC-M-02 / UC-M-04）：训练随机森林模型并保存产物。

用法：
    python -m ml.train --db database/charge_platform.db [--output ml/models/load_rf.pkl] [--seed 42]
"""
from __future__ import annotations

import argparse

import pandas as pd

from . import config, db, dataset, model


def train_artifact(conn, seed: int) -> dict:
    """完整训练流程：建数据 -> 特征 -> 切分 -> 训练 -> 评估 -> 组装产物。"""
    frame, stations = dataset.build_training_frame(conn, seed)
    station_ids = sorted(stations["station_id"].astype(int).unique().tolist())
    feature_names = dataset.build_feature_names(station_ids)

    model_frame = frame.dropna(subset=[f"lag_{lag}" for lag in (1, 24, 168)])
    split_ts = model_frame["ts"].max() - pd.Timedelta(days=config.TEST_DAYS)
    train_f = model_frame[model_frame["ts"] < split_ts]
    test_f = model_frame[model_frame["ts"] >= split_ts]

    X_train, y_train = dataset.encode_frame(train_f, station_ids, feature_names)
    X_test, y_test = dataset.encode_frame(test_f, station_ids, feature_names)

    rf = model.train_model(X_train, y_train, random_state=seed)
    y_pred = model.predict(rf, X_test)
    # 朴素基线：直接用「上周同一时刻」的负荷（lag_168）
    baseline_pred = test_f["lag_168"].to_numpy(dtype=float)

    metrics_model = model.metrics_dict(y_test, y_pred)
    metrics_baseline = model.metrics_dict(y_test, baseline_pred)

    # 高峰阈值：各站历史负荷的 75 分位
    station_meta: dict[int, dict] = {}
    peak_by_station = frame.groupby("station_id")["load_kw"].quantile(
        config.PEAK_PERCENTILE / 100.0
    )
    for _, st in stations.iterrows():
        sid = int(st["station_id"])
        station_meta[sid] = {
            "name": st.get("name", ""),
            "total_chargers": int(st["total_chargers"]),
            "fault_chargers": int(st["fault_chargers"]),
            "unavailable_chargers": int(st["unavailable_chargers"]),
            "avg_charger_power_kw": float(st["avg_charger_power_kw"]),
            "peak_threshold_kw": float(peak_by_station.get(sid, 0.0) or 0.0),
        }

    artifact = {
        "model": rf,
        "feature_names": feature_names,
        "station_ids": station_ids,
        "station_meta": station_meta,
        "model_version": config.MODEL_VERSION,
        "seed": seed,
        "weather_seed": config.WEATHER_SEED,
        "trained_at": db.now_utc_iso(),
        "metrics": {"model": metrics_model, "baseline": metrics_baseline},
        "n_train": int(len(X_train)),
        "n_test": int(len(X_test)),
    }
    return artifact


def _report(artifact: dict) -> None:
    m = artifact["metrics"]["model"]
    b = artifact["metrics"]["baseline"]
    print("=" * 56)
    print("机器学习智能分析子系统 —— 训练结果")
    print("=" * 56)
    print(f"训练样本 / 测试样本 : {artifact['n_train']} / {artifact['n_test']}")
    print(f"随机种子 / 模型版本 : {artifact['seed']} / {artifact['model_version']}")
    print("-" * 56)
    print(f"{'指标':<10}{'模型':>14}{'基线(上周同时刻)':>18}")
    for key in ("MAE", "RMSE", "MAPE(%)"):
        print(f"{key:<10}{m[key]:>14.4f}{b[key]:>18.4f}")
    print("-" * 56)
    better = m["MAPE(%)"] < b["MAPE(%)"]
    verdict = "优于" if better else "未优于"
    print(f"结论：模型 MAPE {verdict}基线（{m['MAPE(%)']:.2f}% vs {b['MAPE(%)']:.2f}%）")
    print("=" * 56)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="训练负荷预测模型")
    parser.add_argument("--db", default=str(config.DEFAULT_DB_PATH), help="SQLite 数据库路径")
    parser.add_argument("--output", default=str(config.MODEL_PATH), help="模型产物输出路径")
    parser.add_argument("--seed", type=int, default=config.RANDOM_SEED, help="随机种子")
    args = parser.parse_args(argv)

    conn = db.connect(args.db)
    try:
        artifact = train_artifact(conn, args.seed)
    finally:
        conn.close()

    model.save_artifact(args.output, artifact)
    _report(artifact)
    print(f"模型已保存至：{args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
