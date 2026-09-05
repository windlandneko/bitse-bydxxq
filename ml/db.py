"""SQLite 访问层：读取站点 / 历史充电会话，回写负荷预测结果。

只依赖标准库 sqlite3，不引入额外依赖；时间统一按 UTC ISO-8601 文本处理。
"""
from __future__ import annotations

import sqlite3
from datetime import datetime, timedelta, timezone

import pandas as pd


def connect(db_path) -> sqlite3.Connection:
    """打开数据库连接，复用 schema.sql 中的 PRAGMA 约定。"""
    conn = sqlite3.connect(str(db_path))
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA foreign_keys = ON")
    conn.execute("PRAGMA journal_mode = WAL")
    conn.execute("PRAGMA busy_timeout = 5000")
    return conn


def load_stations(conn: sqlite3.Connection) -> pd.DataFrame:
    """返回站点及其桩况：总桩数 / 故障桩数 / 平均额定功率(kW)。"""
    sql = """
        SELECT
            s.id        AS station_id,
            s.name      AS name,
            s.station_code AS station_code,
            COUNT(c.id) AS total_chargers,
            SUM(CASE WHEN c.status = 'fault' THEN 1 ELSE 0 END) AS fault_chargers,
            AVG(c.rated_power_kw) AS avg_charger_power_kw
        FROM stations s
        LEFT JOIN chargers c ON c.station_id = s.id
        WHERE s.status = 'active'
        GROUP BY s.id
        ORDER BY s.id
    """
    df = pd.read_sql_query(sql, conn)
    # LEFT JOIN 未匹配时可能出现 NULL，归一化避免后续计算报错
    for col in ("total_chargers", "fault_chargers", "avg_charger_power_kw"):
        df[col] = df[col].fillna(0).astype(float)
    return df


def load_hourly_load(conn: sqlite3.Connection) -> pd.DataFrame:
    """把已结算会话的充电量按小时分摊，聚合为各站逐小时的负荷(kW)。

    负荷(kW) 与「该小时充电量(度)」在 1 小时窗口内数值相等。
    返回列：station_id、ts（整点）、load_kw。
    """
    sql = """
        SELECT station_id, started_at, ended_at, energy_kwh
        FROM charging_sessions
        WHERE status = 'settled' AND energy_kwh IS NOT NULL
    """
    rows = pd.read_sql_query(sql, conn)

    buckets: dict[tuple[int, datetime], float] = {}
    for _, r in rows.iterrows():
        start = pd.to_datetime(r["started_at"], utc=True).to_pydatetime()
        end = pd.to_datetime(r["ended_at"], utc=True).to_pydatetime()
        energy = float(r["energy_kwh"])
        station_id = int(r["station_id"])
        if end <= start or energy <= 0:
            continue
        # 按时间占比把电量分摊到所覆盖的每个整点小时
        cur = start
        total_seconds = (end - start).total_seconds()
        while cur < end:
            hour = cur.replace(minute=0, second=0, microsecond=0)
            seg_end = min(end, hour + timedelta(hours=1))
            frac = (seg_end - cur).total_seconds() / total_seconds
            key = (station_id, hour)
            buckets[key] = buckets.get(key, 0.0) + energy * frac
            cur = seg_end

    if not buckets:
        return pd.DataFrame(columns=["station_id", "ts", "load_kw"])

    records = [
        {"station_id": sid, "ts": pd.Timestamp(ts), "load_kw": kw}
        for (sid, ts), kw in buckets.items()
    ]
    return pd.DataFrame(records)


def write_forecasts(conn: sqlite3.Connection, rows: list[dict]) -> int:
    """按 (station_id, horizon_start, horizon_end, model_version) 幂等回写预测。

    使用 INSERT ... ON CONFLICT 更新，同一模型重复运行不产生重复行（对应
    文档 writeLoadForecast 约定）。
    """
    if not rows:
        return 0
    sql = """
        INSERT INTO load_forecasts
            (station_id, forecast_time, horizon_start, horizon_end,
             predicted_load_kw, predicted_available_chargers, is_peak,
             model_version, generated_at)
        VALUES
            (:station_id, :forecast_time, :horizon_start, :horizon_end,
             :predicted_load_kw, :predicted_available_chargers, :is_peak,
             :model_version, :generated_at)
        ON CONFLICT (station_id, horizon_start, horizon_end, model_version)
        DO UPDATE SET
            forecast_time = excluded.forecast_time,
            predicted_load_kw = excluded.predicted_load_kw,
            predicted_available_chargers = excluded.predicted_available_chargers,
            is_peak = excluded.is_peak,
            generated_at = excluded.generated_at
    """
    conn.executemany(sql, rows)
    conn.commit()
    return len(rows)


def now_utc_iso() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
