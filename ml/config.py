"""机器学习智能分析子系统的全局配置。

集中管理路径与可调参数，训练 / 预测 / 评估三个脚本共用，保证
同一份配置在三处行为一致、可复现。
"""
from __future__ import annotations

from pathlib import Path

# 仓库根目录（ml/ 的上一级）
REPO_ROOT = Path(__file__).resolve().parent.parent

# 训练产物目录与模型文件（对应需求 UC-M-04 的 ml/models/load_rf.pkl）
MODEL_DIR = REPO_ROOT / "ml" / "models"
MODEL_PATH = MODEL_DIR / "load_rf.pkl"

# 默认数据库文件：与两个 Qt 客户端和 Web 导出脚本共用同一份 SQLite 文件
DEFAULT_DB_PATH = REPO_ROOT / "database" / "charge_platform.db"

# 可复现性（UC-M-04）：固定随机种子
RANDOM_SEED = 42
# 模拟天气的独立种子，保证温度 / 降水与训练时一致
WEATHER_SEED = 2026_08_01

# 回写 load_forecasts 时使用的模型版本号
MODEL_VERSION = "rf-v1"

# 预测时距（小时）：未来 1 小时 / 6 小时 / 24 小时（UC-A-08 / 项目要求书）
HORIZONS_HOURS = (1, 6, 24)

# 高峰判定：预测负荷 >= 该站历史负荷的该百分位数时标记 is_peak=1
PEAK_PERCENTILE = 75

# 训练 / 测试按时间切分：最后 TEST_DAYS 天作为留出测试集
TEST_DAYS = 7
