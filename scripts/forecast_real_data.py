#!/usr/bin/env python3
"""真实充电数据集负荷预测（UC-M 演示，自包含脚本）。

从 reference/datasets/ 下的公开数据集聚合逐小时负荷，训练随机森林并与
「上周同一时刻」基线对比，输出指标并生成结果图。

用法：
    python scripts/forecast_real_data.py --dataset jiaxing [--days 14] [--plot out.png]
    python scripts/forecast_real_data.py --dataset beijing

数据集：
    jiaxing  reference/datasets/jiaxing_2025/Dataset/Charging_Data.csv
             连续两年、13 个站点类型、自带温度/降水，按站点逐小时聚合。
    beijing  reference/datasets/beijing_2026/orders_2025-{01,07}_public.parquet
             8553 站、两个不连续月份，无天气字段，聚合为平台总负荷。

依赖：numpy / pandas / scikit-learn / matplotlib（见 ml/requirements.txt 之外
额外需要 matplotlib，用于绘图）。
"""
from __future__ import annotations

import argparse
import json
import time
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

import numpy as np
import pandas as pd
from sklearn.ensemble import RandomForestRegressor

REPO = Path(__file__).resolve().parents[1]
DATASETS = REPO / "reference" / "datasets"

JIAXING_CSV = DATASETS / "jiaxing_2025" / "Dataset" / "Charging_Data.csv"
BEIJING_ORDERS = [
    DATASETS / "beijing_2026" / "orders_2025-01_public.parquet",
    DATASETS / "beijing_2026" / "orders_2025-07_public.parquet",
]


# --------------------------------------------------------------------------- #
# 数据加载：把不同来源统一成 frame（列：station, ts, load_kw, temp?, precip?）
# --------------------------------------------------------------------------- #
def load_jiaxing() -> pd.DataFrame:
    df = pd.read_csv(JIAXING_CSV, encoding="gb18030", low_memory=False)
    for c in ("Start Time", "End Time"):
        df[c] = pd.to_datetime(df[c].astype(str).str.strip(), errors="coerce")
    df["station"] = df["Location Information"].astype(str).str.strip()
    df["energy"] = pd.to_numeric(df["Transaction power/kwh"], errors="coerce")
    df["temp"] = pd.to_numeric(df["Temperature(℃)"], errors="coerce")
    df["precip"] = pd.to_numeric(df["Precipitation(mm)"], errors="coerce")
    df = df.dropna(subset=["Start Time", "End Time", "energy"])
    df = df[(df["End Time"] > df["Start Time"]) & (df["energy"] > 0)]

    load: dict = defaultdict(float)
    wtemp: dict = defaultdict(float)
    wprec: dict = defaultdict(float)
    wcnt: dict = defaultdict(int)
    for start, end, en, st, t, p in zip(
        df["Start Time"], df["End Time"], df["energy"],
        df["station"], df["temp"], df["precip"],
    ):
        total = (end - start).total_seconds()
        if total <= 0:
            continue
        cur = start.floor("h")
        while cur < end:
            seg_end = min(end, cur + pd.Timedelta(hours=1))
            load[(st, cur)] += en * (seg_end - cur).total_seconds() / total
            cur += pd.Timedelta(hours=1)
        mid = (start + (end - start) / 2).floor("h")
        if not pd.isna(t):
            wtemp[(st, mid)] += t
        if not pd.isna(p):
            wprec[(st, mid)] += p
        wcnt[(st, mid)] += 1

    stations = sorted(df["station"].unique())
    hours = pd.date_range(df["Start Time"].min().floor("h"),
                          df["End Time"].max().ceil("h"), freq="h")
    rows = []
    for st in stations:
        for h in hours:
            n = wcnt[(st, h)]
            rows.append({
                "station": st, "ts": h, "load_kw": load.get((st, h), 0.0),
                "temp": (wtemp[(st, h)] / n) if n else np.nan,
                "precip": (wprec[(st, h)] / n) if n else np.nan,
            })
    return pd.DataFrame(rows)


def load_beijing() -> pd.DataFrame:
    frames = []
    for path in BEIJING_ORDERS:
        o = pd.read_parquet(path, columns=["charge_start_time", "total_elec_kwh"])
        o = o.dropna()
        o["ts"] = pd.to_datetime(o["charge_start_time"]).dt.floor("h")
        o["load_kw"] = pd.to_numeric(o["total_elec_kwh"], errors="coerce")
        frames.append(o[["ts", "load_kw"]].dropna())
    total = pd.concat(frames).groupby("ts", as_index=False)["load_kw"].sum()
    total["station"] = "total"
    total["temp"] = np.nan
    total["precip"] = np.nan
    return total[["station", "ts", "load_kw", "temp", "precip"]]


# --------------------------------------------------------------------------- #
# 特征工程 + 训练评估
# --------------------------------------------------------------------------- #
def build_features(frame: pd.DataFrame) -> tuple[pd.DataFrame, list[str]]:
    frame = frame.sort_values(["station", "ts"]).reset_index(drop=True)
    frame["hour"] = frame["ts"].dt.hour.astype(int)
    frame["dow"] = frame["ts"].dt.dayofweek.astype(int)
    frame["is_weekend"] = (frame["dow"] >= 5).astype(int)

    numeric = ["hour", "dow", "is_weekend"]
    for col in ("temp", "precip"):
        if frame[col].notna().any():
            frame[col] = frame.groupby("station")[col].ffill()
            frame[col] = frame[col].fillna(frame[col].median())
            numeric.append(col)
        else:
            frame = frame.drop(columns=[col])

    for lag in (1, 24, 168):
        frame[f"lag_{lag}"] = frame.groupby("station")["load_kw"].shift(lag)
    frame["rolling_mean_24"] = frame.groupby("station")["load_kw"].transform(
        lambda s: s.rolling(24, min_periods=1).mean()
    )
    numeric += ["lag_1", "lag_24", "lag_168", "rolling_mean_24"]

    stations = sorted(frame["station"].unique())
    if len(stations) > 1:
        for st in stations:
            frame[f"st_{st}"] = 0
        model_frame = frame.dropna(subset=["lag_1", "lag_24", "lag_168"]).copy()
        for st in stations:
            model_frame[f"st_{st}"] = (model_frame["station"] == st).astype(int)
        numeric = numeric + [f"st_{st}" for st in stations]
    else:
        model_frame = frame.dropna(subset=["lag_1", "lag_24", "lag_168"]).copy()

    return model_frame, numeric


def _metrics(y: np.ndarray, p: np.ndarray) -> dict[str, float]:
    mae = float(np.mean(np.abs(y - p)))
    rmse = float(np.sqrt(np.mean((y - p) ** 2)))
    wape = float(np.sum(np.abs(y - p)) / (np.sum(np.abs(y)) + 1e-9) * 100)
    denom = np.where(np.abs(y) < 1e-6, 1e-6, np.abs(y))
    med_ape = float(np.median(np.abs((y - p) / denom)) * 100)
    return {"MAE": mae, "RMSE": rmse, "WAPE": wape, "中位APE": med_ape}


def train_evaluate(
    frame: pd.DataFrame, feature_cols: list[str], test_days: int, estimators: int
) -> dict:
    split_ts = frame["ts"].max() - pd.Timedelta(days=test_days)
    train = frame[frame["ts"] < split_ts]
    test = frame[frame["ts"] >= split_ts].copy()

    model = RandomForestRegressor(
        n_estimators=estimators, random_state=42, n_jobs=-1
    )
    model.fit(train[feature_cols].to_numpy(), train["load_kw"].to_numpy())
    test["pred"] = np.maximum(model.predict(test[feature_cols].to_numpy()), 0.0)
    test["baseline"] = test["lag_168"].to_numpy()

    m = _metrics(test["load_kw"].to_numpy(), test["pred"].to_numpy())
    b = _metrics(test["load_kw"].to_numpy(), test["baseline"].to_numpy())
    return {
        "model": model, "test": test, "metrics": m, "baseline": b,
        "split_ts": split_ts,
    }


# --------------------------------------------------------------------------- #
# 导出：把测试期聚合为平台总负荷，产出大屏可消费的评估对比 JSON
# --------------------------------------------------------------------------- #
def _eval_payload(result: dict, dataset: str) -> dict:
    test = result["test"]
    total = test.groupby("ts").agg(
        actual=("load_kw", "sum"), pred=("pred", "sum"), baseline=("baseline", "sum")
    )
    m, b = result["metrics"], result["baseline"]
    return {
        "dataset": dataset,
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "testRange": f"{total.index.min()} ~ {total.index.max()}",
        "metrics": {
            "mae": round(m["MAE"], 2), "rmse": round(m["RMSE"], 2),
            "wape": round(m["WAPE"], 2), "medianApe": round(m["中位APE"], 2),
        },
        "baseline": {
            "mae": round(b["MAE"], 2), "rmse": round(b["RMSE"], 2),
            "wape": round(b["WAPE"], 2), "medianApe": round(b["中位APE"], 2),
        },
        "series": [
            {
                "time": ts.strftime("%Y-%m-%dT%H:%M:%S"),
                "actual": round(float(row.actual), 2),
                "pred": round(float(row.pred), 2),
                "baseline": round(float(row.baseline), 2),
            }
            for ts, row in total.iterrows()
        ],
    }


def export_eval(result: dict, dataset: str, export_json: Path | None,
                inject_dashboard: Path | None) -> None:
    payload = _eval_payload(result, dataset)
    if export_json is not None:
        export_json.parent.mkdir(parents=True, exist_ok=True)
        export_json.write_text(
            json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
        )
        print(f"评估 JSON 已导出：{export_json}")
    if inject_dashboard is not None:
        if not inject_dashboard.exists():
            print(f"警告：目标 dashboard.json 不存在，跳过注入：{inject_dashboard}")
            return
        dash = json.loads(inject_dashboard.read_text(encoding="utf-8"))
        dash["forecastEval"] = payload
        tmp = inject_dashboard.with_name(inject_dashboard.name + ".tmp")
        tmp.write_text(json.dumps(dash, ensure_ascii=False, indent=2) + "\n",
                       encoding="utf-8")
        tmp.replace(inject_dashboard)
        print(f"已注入 forecastEval 到大屏数据：{inject_dashboard}")


# --------------------------------------------------------------------------- #
# 绘图：把测试期聚合为平台总负荷，画 实际 / 模型 / 基线
# --------------------------------------------------------------------------- #
def plot_results(result: dict, dataset: str, out_path: Path | None, show: bool) -> None:
    import matplotlib
    matplotlib.use("TkAgg" if show else "Agg")
    import matplotlib.pyplot as plt

    plt.rcParams["font.sans-serif"] = [
        "Noto Sans CJK SC", "Noto Sans CJK JP", "WenQuanYi Micro Hei",
        "Microsoft YaHei", "PingFang SC", "DejaVu Sans",
    ]
    plt.rcParams["axes.unicode_minus"] = False

    test = result["test"]
    total = test.groupby("ts").agg(
        actual=("load_kw", "sum"), pred=("pred", "sum"), base=("baseline", "sum")
    )

    fig, ax = plt.subplots(figsize=(13, 5.5))
    ax.plot(total.index, total["actual"], color="#1f77b4", lw=1.4, label="实际负荷")
    ax.plot(total.index, total["pred"], color="#d62728", lw=1.2, label="模型预测")
    ax.plot(total.index, total["base"], color="#7f7f7f", lw=1.0,
            ls="--", alpha=0.7, label="基线（上周同时刻）")

    m, b = result["metrics"], result["baseline"]
    title = (
        f"充电负荷预测（{dataset}）\n"
        f"MAE {m['MAE']:.1f} vs 基线 {b['MAE']:.1f} kW   |   "
        f"RMSE {m['RMSE']:.1f} vs {b['RMSE']:.1f}   |   "
        f"WAPE {m['WAPE']:.1f}% vs {b['WAPE']:.1f}%"
    )
    ax.set_title(title, fontsize=11)
    ax.set_xlabel("时间")
    ax.set_ylabel("平台总负荷 (kW)")
    ax.legend(loc="upper right")
    ax.grid(alpha=0.3)
    fig.tight_layout()
    if out_path is not None:
        fig.savefig(out_path, dpi=130)
        print(f"结果图已保存：{out_path}")
    if show:
        print("已打开结果窗口（关闭窗口即退出）...")
        plt.show()


# --------------------------------------------------------------------------- #
def main(argv=None) -> int:
    p = argparse.ArgumentParser(description="真实数据负荷预测演示")
    p.add_argument("--dataset", choices=["jiaxing", "beijing"], default="jiaxing")
    p.add_argument("--days", type=int, default=14, help="留出测试天数")
    p.add_argument("--estimators", type=int, default=200)
    p.add_argument("--plot", type=Path, default=None, help="结果图 PNG 输出路径")
    p.add_argument("--show", action="store_true", help="弹出交互式结果窗口")
    p.add_argument("--export-json", type=Path, default=None,
                   help="导出评估对比 JSON 到指定路径")
    p.add_argument("--inject-dashboard", type=Path, default=None,
                   help="把 forecastEval 注入指定 dashboard.json（大屏显示用）")
    args = p.parse_args(argv)

    t0 = time.time()
    loader = {"jiaxing": load_jiaxing, "beijing": load_beijing}[args.dataset]
    print(f"加载 {args.dataset} 数据 ...")
    frame = loader()
    frame, feature_cols = build_features(frame)
    print(f"特征帧：{len(frame)} 样本 × {len(feature_cols)} 特征")

    result = train_evaluate(frame, feature_cols, args.days, args.estimators)
    m, b = result["metrics"], result["baseline"]
    test = result["test"]

    print("=" * 64)
    print(f"真实充电数据负荷预测（{args.dataset}）")
    print("=" * 64)
    print(f"站点数 / 逐小时样本 : {frame['station'].nunique()} / {len(frame)}")
    print(f"训练样本 / 测试样本 : "
          f"{int((frame['ts'] < result['split_ts']).sum())} / {len(test)}  "
          f"（测试 {test['ts'].min()} ~ {test['ts'].max()}）")
    print(f"零负荷小时占比      : {(test['load_kw'] < 1e-6).mean()*100:.1f}%")
    print("-" * 64)
    print(f"{'指标':<10}{'模型':>14}{'基线':>14}{'改善':>12}")
    for key in ("MAE", "RMSE", "WAPE", "中位APE"):
        imp = (b[key] - m[key]) / (b[key] + 1e-9) * 100
        unit = " kW" if key in ("MAE", "RMSE") else " %"
        print(f"{key:<10}{m[key]:>13.2f}{unit}{b[key]:>14.2f}{unit}{imp:>11.1f}%")
    print("-" * 64)
    print(f"结论：模型 {'优于' if m['WAPE'] < b['WAPE'] else '未优于'}基线（按 WAPE）")
    print(f"耗时 {time.time() - t0:.1f}s")

    if args.export_json is not None or args.inject_dashboard is not None:
        export_eval(result, args.dataset, args.export_json, args.inject_dashboard)

    if args.plot is not None or args.show:
        plot_results(result, args.dataset, args.plot, args.show)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
