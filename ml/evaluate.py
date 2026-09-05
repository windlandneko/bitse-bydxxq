"""评估模式（UC-M-02）：对已训练模型做留出集评估，并与朴素基线对比。

用法：
    python -m ml.evaluate --db charge_platform.db [--model ml/models/load_rf.pkl] [--seed 42]
"""
from __future__ import annotations

import argparse

import pandas as pd

from . import config, db, dataset, model


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="评估负荷预测模型")
    parser.add_argument("--db", default=str(config.DEFAULT_DB_PATH), help="SQLite 数据库路径")
    parser.add_argument("--model", default=str(config.MODEL_PATH), help="训练产物路径")
    parser.add_argument("--seed", type=int, default=config.RANDOM_SEED, help="随机种子")
    args = parser.parse_args(argv)

    try:
        artifact = model.load_artifact(args.model)
    except FileNotFoundError:
        print(f"未找到模型产物 {args.model}，请先运行 `python -m ml.train`。")
        return 1

    conn = db.connect(args.db)
    try:
        frame, stations = dataset.build_training_frame(conn, args.seed)
    finally:
        conn.close()

    station_ids = sorted(stations["station_id"].astype(int).unique().tolist())
    feature_names = dataset.build_feature_names(station_ids)

    model_frame = frame.dropna(subset=[f"lag_{lag}" for lag in (1, 24, 168)])
    split_ts = model_frame["ts"].max() - pd.Timedelta(days=config.TEST_DAYS)
    test_f = model_frame[model_frame["ts"] >= split_ts].copy()

    X_test, y_test = dataset.encode_frame(test_f, station_ids, feature_names)
    y_pred = model.predict(artifact["model"], X_test)
    baseline_pred = test_f["lag_168"].to_numpy(dtype=float)

    m = model.metrics_dict(y_test, y_pred)
    b = model.metrics_dict(y_test, baseline_pred)

    print("=" * 60)
    print("机器学习智能分析子系统 —— 留出集评估")
    print("=" * 60)
    print(f"测试样本数          : {len(y_test)}")
    print(f"模型版本 / 训练时间 : {artifact['model_version']} / {artifact['trained_at']}")
    print("-" * 60)
    print(f"{'指标':<10}{'模型':>14}{'基线(上周同时刻)':>18}{'改善':>12}")
    for key in ("MAE", "RMSE", "MAPE(%)"):
        improve = (b[key] - m[key]) / (b[key] + 1e-9) * 100
        print(f"{key:<10}{m[key]:>14.4f}{b[key]:>18.4f}{improve:>11.1f}%")
    print("-" * 60)
    better = m["MAPE(%)"] < b["MAPE(%)"]
    print(f"结论：模型 {'优于' if better else '未优于'}基线（按 MAPE）")
    print("=" * 60)

    # 分站 MAE 概览
    test_f["pred"] = y_pred
    per_station = (
        test_f.groupby("station_id")
        .apply(
            lambda g: model.mean_absolute_error(g["load_kw"], g["pred"]),
            include_groups=False,
        )
    )
    print("\n分站 MAE (kW)：")
    for sid, mae in per_station.items():
        print(f"  站 {int(sid):>2} : {mae:.4f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
