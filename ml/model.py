"""模型定义、评估指标与产物持久化（对应 UC-M-02 / UC-M-04）。

第一版采用 scikit-learn 的 RandomForestRegressor：可解释、训练快、无需 GPU。
评估指标 MAE / RMSE / MAPE，并与「上周同一时刻」的朴素基线对比。
"""
from __future__ import annotations

import pickle
from pathlib import Path

import numpy as np
from sklearn.ensemble import RandomForestRegressor

from . import config


def train_model(
    X: np.ndarray,
    y: np.ndarray,
    random_state: int = config.RANDOM_SEED,
) -> RandomForestRegressor:
    """训练随机森林回归模型（固定随机种子以保证可复现）。"""
    model = RandomForestRegressor(
        n_estimators=200,
        max_depth=None,
        min_samples_leaf=2,
        random_state=random_state,
        n_jobs=-1,
    )
    model.fit(X, y)
    return model


def predict(model: RandomForestRegressor, X: np.ndarray) -> np.ndarray:
    return np.asarray(model.predict(X), dtype=float)


def mean_absolute_error(y_true: np.ndarray, y_pred: np.ndarray) -> float:
    return float(np.mean(np.abs(np.asarray(y_true) - np.asarray(y_pred))))


def root_mean_squared_error(y_true: np.ndarray, y_pred: np.ndarray) -> float:
    return float(np.sqrt(np.mean((np.asarray(y_true) - np.asarray(y_pred)) ** 2)))


def mean_absolute_percentage_error(y_true: np.ndarray, y_pred: np.ndarray) -> float:
    """MAPE（百分比）。分母用真实值并做下界裁剪，避免除零。"""
    y_true = np.asarray(y_true, dtype=float)
    y_pred = np.asarray(y_pred, dtype=float)
    denom = np.maximum(np.abs(y_true), 1e-6)
    return float(np.mean(np.abs((y_true - y_pred) / denom)) * 100.0)


def metrics_dict(y_true: np.ndarray, y_pred: np.ndarray) -> dict[str, float]:
    """汇总三项评估指标。"""
    return {
        "MAE": mean_absolute_error(y_true, y_pred),
        "RMSE": root_mean_squared_error(y_true, y_pred),
        "MAPE(%)": mean_absolute_percentage_error(y_true, y_pred),
    }


def save_artifact(path: Path, artifact: dict) -> None:
    """把模型 + 元数据整体序列化到 load_rf.pkl。"""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "wb") as f:
        pickle.dump(artifact, f)


def load_artifact(path: Path) -> dict:
    with open(path, "rb") as f:
        return pickle.load(f)
