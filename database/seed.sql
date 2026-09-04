-- Minimal, repeatable demo data. Run after schema.sql.
PRAGMA foreign_keys = ON;

BEGIN;

INSERT OR IGNORE INTO admins
    (id, username, password_hash, display_name, status, created_at, updated_at)
VALUES
    (1, 'admin', '$2b$04$TWlXKuvScD9fG0X9RbRfvOk9vJ5nwiKLAfhfxOAtYk8sK8QDlmkFq', '系统管理员', 'active', '2026-09-01T00:00:00Z', '2026-09-01T00:00:00Z');

INSERT OR IGNORE INTO users
    (id, phone, nickname, wallet_balance, status, created_at, updated_at)
VALUES
    (1, '13800000001', '演示用户', 91.50, 'active', '2026-09-01T00:00:00Z', '2026-09-04T01:00:00Z'),
    (2, '13800000002', '冻结测试用户', 0.00, 'frozen', '2026-09-02T00:00:00Z', '2026-09-04T01:00:00Z');

INSERT OR IGNORE INTO stations
    (id, station_code, name, address, region_code, latitude, longitude, status, created_at, updated_at)
VALUES
    (1, 'ST001', '中心广场充电站', '中心区人民路 1 号', '中心区', 31.230416, 121.473701, 'active', '2026-09-01T00:00:00Z', '2026-09-01T00:00:00Z'),
    (2, 'ST002', '东城充电站', '东城区世纪大道 88 号', '东城区', 31.235929, 121.501116, 'active', '2026-09-01T00:00:00Z', '2026-09-01T00:00:00Z'),
    (3, 'ST003', '西城充电站', '西城区长宁路 18 号', '西城区', 31.220000, 121.420000, 'active', '2026-09-01T00:00:00Z', '2026-09-01T00:00:00Z'),
    (4, 'ST004', '南站充电站', '南区枢纽路 6 号', '南城区', 31.190000, 121.450000, 'active', '2026-09-01T00:00:00Z', '2026-09-01T00:00:00Z'),
    (5, 'ST005', '北郊充电站', '北区环路 99 号', '北城区', 31.300000, 121.480000, 'active', '2026-09-01T00:00:00Z', '2026-09-01T00:00:00Z');

INSERT OR IGNORE INTO chargers
    (id, station_id, charger_code, charger_type, rated_power_kw, status, created_at, updated_at)
VALUES
    (1, 1, 'AC001', 'ac', 7.000, 'idle', '2026-09-01T00:00:00Z', '2026-09-04T01:00:00Z'),
    (2, 1, 'DC001', 'dc', 60.000, 'idle', '2026-09-01T00:00:00Z', '2026-09-04T01:00:00Z'),
    (3, 2, 'AC001', 'ac', 7.000, 'idle', '2026-09-01T00:00:00Z', '2026-09-04T01:00:00Z'),
    (4, 2, 'DC001', 'dc', 60.000, 'idle', '2026-09-01T00:00:00Z', '2026-09-04T01:00:00Z'),
    (5, 3, 'AC001', 'ac', 7.000, 'idle', '2026-09-01T00:00:00Z', '2026-09-04T01:00:00Z'),
    (6, 3, 'DC001', 'dc', 60.000, 'idle', '2026-09-01T00:00:00Z', '2026-09-04T01:00:00Z'),
    (7, 4, 'AC001', 'ac', 7.000, 'idle', '2026-09-01T00:00:00Z', '2026-09-04T01:00:00Z'),
    (8, 4, 'DC001', 'dc', 60.000, 'idle', '2026-09-01T00:00:00Z', '2026-09-04T01:00:00Z'),
    (9, 5, 'AC001', 'ac', 7.000, 'idle', '2026-09-01T00:00:00Z', '2026-09-04T01:00:00Z'),
    (10, 5, 'DC001', 'dc', 60.000, 'idle', '2026-09-01T00:00:00Z', '2026-09-04T01:00:00Z');

INSERT OR IGNORE INTO tariffs
    (id, station_id, charger_type, electricity_price, service_price, effective_from, status, created_by, created_at)
VALUES
    (1, 1, 'all', 1.2000, 0.5000, '2026-09-01T00:00:00Z', 'active', 1, '2026-09-01T00:00:00Z'),
    (2, 2, 'all', 1.1800, 0.5000, '2026-09-01T00:00:00Z', 'active', 1, '2026-09-01T00:00:00Z'),
    (3, 3, 'all', 1.1500, 0.4500, '2026-09-01T00:00:00Z', 'active', 1, '2026-09-01T00:00:00Z'),
    (4, 4, 'all', 1.2200, 0.5000, '2026-09-01T00:00:00Z', 'active', 1, '2026-09-01T00:00:00Z'),
    (5, 5, 'all', 1.1000, 0.4500, '2026-09-01T00:00:00Z', 'active', 1, '2026-09-01T00:00:00Z');

INSERT OR IGNORE INTO reservations
    (id, reservation_no, user_id, charger_id, status, reserved_at, expires_at, started_at, ended_at, idempotency_key, created_at, updated_at)
VALUES
    (1, 'RSV202609030001', 1, 2, 'completed', '2026-09-03T08:00:00Z', '2026-09-03T08:30:00Z', '2026-09-03T08:05:00Z', '2026-09-03T09:05:00Z', 'seed-rsv-1', '2026-09-03T08:00:00Z', '2026-09-03T09:05:00Z');

INSERT OR IGNORE INTO charging_sessions
    (id, session_no, reservation_id, user_id, charger_id, station_id, status, started_at, ended_at, meter_start_kwh, meter_end_kwh, energy_kwh, duration_seconds, soc_start, soc_end, idempotency_key, created_at, updated_at)
VALUES
    (1, 'SES202609030001', 1, 1, 2, 1, 'settled', '2026-09-03T08:05:00Z', '2026-09-03T09:05:00Z', 120.000, 125.000, 5.000, 3600, 30.00, 42.00, 'seed-session-1', '2026-09-03T08:05:00Z', '2026-09-03T09:05:00Z');

INSERT OR IGNORE INTO orders
    (id, order_no, session_id, user_id, station_id, charger_id, status, energy_kwh, duration_seconds, electricity_unit_price, service_unit_price, gross_amount, discount_amount, payable_amount, paid_amount, settled_at, created_at, updated_at)
VALUES
    (1, 'ORD202609030001', 1, 1, 1, 2, 'paid', 5.000, 3600, 1.2000, 0.5000, 8.50, 0.00, 8.50, 8.50, '2026-09-03T09:05:00Z', '2026-09-03T09:05:00Z', '2026-09-03T09:05:00Z');

INSERT OR IGNORE INTO wallet_transactions
    (id, user_id, transaction_no, transaction_type, amount, balance_before, balance_after, order_id, idempotency_key, remark, created_at)
VALUES
    (1, 1, 'WAL202609010001', 'recharge', 100.00, 0.00, 100.00, NULL, 'seed-recharge-1', '演示充值', '2026-09-01T01:00:00Z'),
    (2, 1, 'WAL202609030001', 'charge', -8.50, 100.00, 91.50, 1, 'seed-charge-1', '充电消费', '2026-09-03T09:05:00Z');

INSERT OR IGNORE INTO charger_status_history
    (id, charger_id, from_status, to_status, reason, source, occurred_at)
VALUES
    (1, 2, 'idle', 'reserved', '用户预约', 'user', '2026-09-03T08:00:00Z'),
    (2, 2, 'reserved', 'charging', '开始充电', 'system', '2026-09-03T08:05:00Z'),
    (3, 2, 'charging', 'idle', '充电完成', 'system', '2026-09-03T09:05:00Z');

INSERT OR IGNORE INTO station_daily_stats
    (station_id, stat_date, order_count, settled_order_count, revenue, energy_kwh, charging_seconds, new_user_count, fault_count)
VALUES
    (1, '2026-09-03', 1, 1, 8.50, 5.000, 3600, 0, 0);

INSERT OR IGNORE INTO load_forecasts
    (station_id, forecast_time, horizon_start, horizon_end, predicted_load_kw, predicted_available_chargers, model_version, generated_at)
VALUES
    (1, '2026-09-04T00:00:00Z', '2026-09-04T08:00:00Z', '2026-09-04T09:00:00Z', 42.500, 1, 'demo-v1', '2026-09-04T00:00:00Z');

INSERT OR IGNORE INTO outbox_events
    (event_id, aggregate_type, aggregate_id, event_type, payload_json, status, retry_count, created_at)
VALUES
    ('seed-order-paid-1', 'order', 1, 'order.paid', '{"order_no":"ORD202609030001","amount":8.50}', 'published', 0, '2026-09-03T09:05:00Z');

COMMIT;
