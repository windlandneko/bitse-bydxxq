#!/usr/bin/env python3
"""Export the SQLite facts needed by the Web dashboard.

The browser intentionally never opens SQLite. This script is the bridge between
the existing database schema and web/public/data/dashboard.json.
"""

from __future__ import annotations

import argparse
import json
import sqlite3
import sys
import time
from collections import defaultdict
from datetime import date, datetime, timedelta, timezone
from pathlib import Path
from typing import Any
from urllib.parse import quote


UTC = timezone.utc
try:
    from zoneinfo import ZoneInfo

    LOCAL_ZONE = ZoneInfo("Asia/Shanghai")
except Exception:  # pragma: no cover - Python 3.10 normally has zoneinfo data
    LOCAL_ZONE = timezone(timedelta(hours=8))

STATUS_LABELS = {
    "idle": "闲置",
    "reserved": "预约",
    "charging": "充电中",
    "fault": "故障",
    "offline": "离线",
    "maintenance": "维护",
}


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description="Export NCS dashboard JSON from SQLite")
    parser.add_argument(
        "--db",
        type=Path,
        default=root / "database" / "charge_platform.db",
        help="SQLite database path (default: database/charge_platform.db)",
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=root / "web" / "public" / "data" / "dashboard.json",
        help="Output JSON path",
    )
    parser.add_argument(
        "--station-limit", type=int, default=10, help="Number of stations in ranking"
    )
    parser.add_argument(
        "--watch", action="store_true", help="Keep exporting periodically until interrupted"
    )
    parser.add_argument(
        "--interval", type=int, default=30, help="Export interval in seconds when --watch is used"
    )
    return parser.parse_args()


def parse_timestamp(value: str | None) -> datetime | None:
    if not value:
        return None
    try:
        parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        return None
    if parsed.tzinfo is None:
        parsed = parsed.replace(tzinfo=UTC)
    return parsed.astimezone(LOCAL_ZONE)


def timestamp_text(value: datetime | None) -> str:
    if value is None:
        return ""
    return value.astimezone(LOCAL_ZONE).isoformat(timespec="seconds")


def as_number(value: Any, digits: int = 3) -> float:
    if value is None:
        return 0.0
    return round(float(value), digits)


def rows(conn: sqlite3.Connection, sql: str, params: tuple[Any, ...] = ()) -> list[sqlite3.Row]:
    return list(conn.execute(sql, params).fetchall())


def export_dashboard(db_path: Path, output_path: Path, station_limit: int) -> None:
    if not db_path.exists():
        raise FileNotFoundError(
            f"数据库文件不存在：{db_path}\n请先生成数据库，或通过 --db 指定已有的 charge_platform.db。"
        )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    uri = f"file:{quote(db_path.resolve().as_posix(), safe='/:')}?mode=ro"
    conn = sqlite3.connect(uri, uri=True)
    conn.row_factory = sqlite3.Row
    try:
        conn.execute("PRAGMA busy_timeout = 5000")
        status_rows = rows(
            conn,
            """
            SELECT status, COUNT(*) AS value
            FROM chargers
            GROUP BY status
            ORDER BY status
            """,
        )
        charger_status = [
            {
                "key": item["status"],
                "label": STATUS_LABELS.get(item["status"], item["status"]),
                "value": int(item["value"]),
            }
            for item in status_rows
        ]

        kpi = conn.execute(
            """
            SELECT
              (SELECT COUNT(*) FROM orders WHERE status = 'paid') AS total_charging_count,
              (SELECT COALESCE(ROUND(SUM(paid_amount), 2), 0) FROM orders WHERE status = 'paid') AS total_revenue,
              (SELECT COUNT(*) FROM chargers WHERE status IN ('idle', 'reserved', 'charging')) AS online_chargers,
              (SELECT COUNT(*) FROM users) AS registered_users
            """
        ).fetchone()

        station_rows = rows(
            conn,
            """
            SELECT s.id AS station_id, s.station_code, s.name AS station_name,
                   COALESCE(ROUND(SUM(o.energy_kwh), 3), 0) AS energy_kwh,
                   COALESCE(ROUND(SUM(o.paid_amount), 2), 0) AS revenue
            FROM stations s
            LEFT JOIN orders o ON o.station_id = s.id AND o.status = 'paid'
            WHERE s.status = 'active'
            GROUP BY s.id, s.station_code, s.name
            ORDER BY energy_kwh DESC, s.id
            LIMIT ?
            """,
            (station_limit,),
        )
        station_ranking = [
            {
                "stationId": int(item["station_id"]),
                "stationCode": item["station_code"],
                "stationName": item["station_name"],
                "energyKwh": as_number(item["energy_kwh"]),
                "revenue": as_number(item["revenue"], 2),
            }
            for item in station_rows
        ]

        daily_source_rows = rows(
            conn,
            """
            SELECT settled_at, paid_amount
            FROM orders
            WHERE status = 'paid' AND settled_at IS NOT NULL
            """,
        )
        daily_accumulator: defaultdict[str, dict[str, float | int]] = defaultdict(
            lambda: {"revenue": 0.0, "orderCount": 0}
        )
        for item in daily_source_rows:
            settled_at = parse_timestamp(item["settled_at"])
            stat_date = (
                settled_at.date().isoformat()
                if settled_at is not None
                else str(item["settled_at"])[:10]
            )
            daily_accumulator[stat_date]["revenue"] += float(item["paid_amount"] or 0)
            daily_accumulator[stat_date]["orderCount"] += 1
        daily_by_date = {
            stat_date: {
                "date": stat_date,
                "revenue": as_number(values["revenue"], 2),
                "orderCount": int(values["orderCount"]),
            }
            for stat_date, values in daily_accumulator.items()
        }
        latest_order_day = max(
            (date.fromisoformat(stat_date) for stat_date in daily_by_date),
            default=datetime.now(LOCAL_ZONE).date(),
        )
        revenue_trend = []
        for offset in range(29, -1, -1):
            current = latest_order_day - timedelta(days=offset)
            key = current.isoformat()
            revenue_trend.append(
                daily_by_date.get(key, {"date": key, "revenue": 0.0, "orderCount": 0})
            )

        heatmap_accumulator: defaultdict[tuple[int, int], float] = defaultdict(float)
        heatmap_rows = rows(
            conn,
            """
            SELECT cs.started_at, o.energy_kwh
            FROM orders o
            JOIN charging_sessions cs ON cs.id = o.session_id
            WHERE o.status = 'paid' AND cs.started_at IS NOT NULL
            """,
        )
        for item in heatmap_rows:
            started_at = parse_timestamp(item["started_at"])
            if started_at is not None:
                heatmap_accumulator[(started_at.weekday(), started_at.hour)] += float(
                    item["energy_kwh"] or 0
                )
        hourly_heatmap = [
            {"weekday": weekday, "hour": hour, "energyKwh": as_number(energy)}
            for (weekday, hour), energy in sorted(heatmap_accumulator.items())
        ]

        type_rows = rows(
            conn,
            """
            SELECT c.charger_type, COUNT(DISTINCT c.id) AS charger_count,
                   COALESCE(ROUND(SUM(CASE WHEN o.status = 'paid' THEN o.energy_kwh ELSE 0 END), 3), 0) AS energy_kwh
            FROM chargers c
            LEFT JOIN orders o ON o.charger_id = c.id
            GROUP BY c.charger_type
            ORDER BY c.charger_type
            """,
        )
        charger_type_ratio = [
            {
                "type": item["charger_type"],
                "label": "慢充 AC" if item["charger_type"] == "ac" else "快充 DC",
                "count": int(item["charger_count"]),
                "energyKwh": as_number(item["energy_kwh"]),
            }
            for item in type_rows
        ]

        latest_forecast = conn.execute(
            "SELECT MAX(generated_at) AS generated_at FROM load_forecasts"
        ).fetchone()["generated_at"]
        forecast_rows = rows(
            conn,
            """
            SELECT horizon_start, predicted_load_kw, predicted_available_chargers,
                   is_peak, actual_load_kw
            FROM load_forecasts
            WHERE generated_at = ?
            ORDER BY horizon_start
            """,
            (latest_forecast,),
        ) if latest_forecast else []
        forecast_accumulator: dict[str, dict[str, Any]] = {}
        for item in forecast_rows:
            key = item["horizon_start"]
            point = forecast_accumulator.setdefault(
                key,
                {
                    "time": timestamp_text(parse_timestamp(key)) or key,
                    "predictedLoadKw": 0.0,
                    "actualLoadKw": None,
                    "availableChargers": 0,
                    "isPeak": False,
                },
            )
            point["predictedLoadKw"] += float(item["predicted_load_kw"] or 0)
            point["availableChargers"] += int(item["predicted_available_chargers"] or 0)
            point["isPeak"] = point["isPeak"] or bool(item["is_peak"])
            if item["actual_load_kw"] is not None:
                point["actualLoadKw"] = (point["actualLoadKw"] or 0) + float(
                    item["actual_load_kw"]
                )
        forecast24h = [
            {
                **point,
                "predictedLoadKw": as_number(point["predictedLoadKw"]),
                "actualLoadKw": (
                    as_number(point["actualLoadKw"])
                    if point["actualLoadKw"] is not None
                    else None
                ),
            }
            for point in forecast_accumulator.values()
        ]

        cutoff_candidates: list[datetime] = []
        for table, column in (
            ("orders", "settled_at"),
            ("chargers", "updated_at"),
            ("load_forecasts", "generated_at"),
        ):
            latest = conn.execute(f"SELECT MAX({column}) AS latest FROM {table}").fetchone()[
                "latest"
            ]
            parsed = parse_timestamp(latest)
            if parsed:
                cutoff_candidates.append(parsed)
        generated_at = datetime.now(LOCAL_ZONE)
        data_cutoff = max(cutoff_candidates, default=generated_at)
        payload = {
            "generatedAt": timestamp_text(generated_at),
            "dataCutoff": timestamp_text(data_cutoff),
            "kpis": {
                "totalChargingCount": int(kpi["total_charging_count"] or 0),
                "totalRevenue": as_number(kpi["total_revenue"], 2),
                "onlineChargers": int(kpi["online_chargers"] or 0),
                "registeredUsers": int(kpi["registered_users"] or 0),
            },
            "chargerStatus": charger_status,
            "stationRanking": station_ranking,
            "revenueTrend": revenue_trend,
            "hourlyHeatmap": hourly_heatmap,
            "chargerTypeRatio": charger_type_ratio,
            "forecast24h": forecast24h,
        }
    finally:
        conn.close()

    temporary_path = output_path.with_name(output_path.name + ".tmp")
    temporary_path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    temporary_path.replace(output_path)
    print(f"已导出大屏数据：{output_path}")
    print(f"数据截止：{payload['dataCutoff']}，营收：{payload['kpis']['totalRevenue']:.2f} 元")


def main() -> int:
    args = parse_args()
    if args.watch:
        try:
            while True:
                try:
                    export_dashboard(args.db, args.out, max(1, args.station_limit))
                except (FileNotFoundError, sqlite3.Error, OSError, ValueError) as error:
                    print(f"导出失败：{error}", file=sys.stderr)
                time.sleep(max(1, args.interval))
        except KeyboardInterrupt:
            print("已停止大屏数据导出。")
        return 0
    try:
        export_dashboard(args.db, args.out, max(1, args.station_limit))
    except (FileNotFoundError, sqlite3.Error, OSError, ValueError) as error:
        print(f"导出失败：{error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
