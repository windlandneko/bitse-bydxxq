-- Repeatable demo data for the charging platform. Run after schema.sql.
-- The data is intentionally deterministic so dashboards and local demos are stable.
PRAGMA foreign_keys = ON;

BEGIN;

INSERT OR IGNORE INTO admins
    (id, username, password_hash, display_name, status, created_at, updated_at)
VALUES
    (1, 'admin', '$2b$04$TWlXKuvScD9fG0X9RbRfvOk9vJ5nwiKLAfhfxOAtYk8sK8QDlmkFq', '系统管理员', 'active', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z');

INSERT OR IGNORE INTO users
    (id, phone, nickname, wallet_balance, status, created_at, updated_at)
VALUES
    (1, '13800000001', '用户0001', 0, 'active', '2026-08-01T01:00:00Z', '2026-08-31T23:59:00Z'),
    (2, '13800000002', '用户0002', 0, 'active', '2026-08-02T01:00:00Z', '2026-08-31T23:59:00Z'),
    (3, '13800000003', '用户0003', 0, 'active', '2026-08-03T01:00:00Z', '2026-08-31T23:59:00Z'),
    (4, '13800000004', '用户0004', 0, 'active', '2026-08-04T01:00:00Z', '2026-08-31T23:59:00Z'),
    (5, '13800000005', '用户0005', 0, 'active', '2026-08-05T01:00:00Z', '2026-08-15T10:00:00Z');

INSERT OR IGNORE INTO stations
    (id, station_code, name, address, region_code, latitude, longitude, status, created_at, updated_at)
VALUES
    (1, 'ST001', '中心广场充电站', '中心区人民路 1 号', '中心区', 31.230416, 121.473701, 'active', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (2, 'ST002', '东城充电站', '东城区世纪大道 88 号', '东城区', 31.235929, 121.501116, 'active', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (3, 'ST003', '西城充电站', '西城区长宁路 18 号', '西城区', 31.220000, 121.420000, 'active', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (4, 'ST004', '南站充电站', '南区枢纽路 6 号', '南城区', 31.190000, 121.450000, 'active', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (5, 'ST005', '北郊充电站', '北区环路 99 号', '北城区', 31.300000, 121.480000, 'active', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z');

-- Six chargers per station: three AC and three DC. Each station has at least
-- one fault charger; ST002 and ST004 have two for fault-state dashboards.
INSERT OR IGNORE INTO chargers
    (station_id, charger_code, charger_type, rated_power_kw, status, cumulative_charging_count,
     cumulative_charging_seconds, cumulative_energy_kwh, last_heartbeat_at, created_at, updated_at)
VALUES
    (1, 'AC001', 'ac', 7.000, 'idle', 18, 64800, 112.500, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (1, 'AC002', 'ac', 7.000, 'idle', 17, 61200, 105.750, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (1, 'AC003', 'ac', 7.000, 'fault', 6, 21600, 34.200, '2026-08-30T11:00:00Z', '2026-07-31T23:00:00Z', '2026-08-30T11:00:00Z'),
    (1, 'DC001', 'dc', 60.000, 'idle', 24, 86400, 286.800, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (1, 'DC002', 'dc', 60.000, 'idle', 21, 75600, 252.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (1, 'DC003', 'dc', 60.000, 'idle', 20, 72000, 240.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (2, 'AC001', 'ac', 7.000, 'idle', 19, 68400, 118.750, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (2, 'AC002', 'ac', 7.000, 'idle', 16, 57600, 98.500, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (2, 'AC003', 'ac', 7.000, 'fault', 7, 25200, 40.250, '2026-08-29T14:00:00Z', '2026-07-31T23:00:00Z', '2026-08-29T14:00:00Z'),
    (2, 'DC001', 'dc', 60.000, 'idle', 22, 79200, 264.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (2, 'DC002', 'dc', 60.000, 'idle', 23, 82800, 276.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (2, 'DC003', 'dc', 60.000, 'fault', 5, 18000, 67.500, '2026-08-28T09:00:00Z', '2026-07-31T23:00:00Z', '2026-08-28T09:00:00Z'),
    (3, 'AC001', 'ac', 7.000, 'idle', 20, 72000, 125.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (3, 'AC002', 'ac', 7.000, 'idle', 18, 64800, 111.750, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (3, 'AC003', 'ac', 7.000, 'fault', 8, 28800, 49.600, '2026-08-27T12:00:00Z', '2026-07-31T23:00:00Z', '2026-08-27T12:00:00Z'),
    (3, 'DC001', 'dc', 60.000, 'idle', 25, 90000, 300.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (3, 'DC002', 'dc', 60.000, 'idle', 21, 75600, 252.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (3, 'DC003', 'dc', 60.000, 'idle', 19, 68400, 228.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (4, 'AC001', 'ac', 7.000, 'idle', 17, 61200, 106.250, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (4, 'AC002', 'ac', 7.000, 'idle', 18, 64800, 112.500, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (4, 'AC003', 'ac', 7.000, 'fault', 6, 21600, 36.000, '2026-08-26T16:00:00Z', '2026-07-31T23:00:00Z', '2026-08-26T16:00:00Z'),
    (4, 'DC001', 'dc', 60.000, 'idle', 24, 86400, 288.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (4, 'DC002', 'dc', 60.000, 'idle', 22, 79200, 264.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (4, 'DC003', 'dc', 60.000, 'fault', 5, 18000, 65.000, '2026-08-25T08:00:00Z', '2026-07-31T23:00:00Z', '2026-08-25T08:00:00Z'),
    (5, 'AC001', 'ac', 7.000, 'idle', 19, 68400, 120.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (5, 'AC002', 'ac', 7.000, 'idle', 20, 72000, 126.250, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (5, 'AC003', 'ac', 7.000, 'fault', 7, 25200, 42.000, '2026-08-24T13:00:00Z', '2026-07-31T23:00:00Z', '2026-08-24T13:00:00Z'),
    (5, 'DC001', 'dc', 60.000, 'idle', 23, 82800, 276.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (5, 'DC002', 'dc', 60.000, 'idle', 22, 79200, 264.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z'),
    (5, 'DC003', 'dc', 60.000, 'idle', 21, 75600, 252.000, '2026-09-01T00:00:00Z', '2026-07-31T23:00:00Z', '2026-09-01T00:00:00Z');

WITH tariff_seed(id, station_id, charger_type, electricity_price, service_price, effective_from,
                 status, created_by, created_at) AS (
    VALUES
        (1, 1, 'all', 1.2000, 0.5000, '2026-08-01T00:00:00Z', 'active', 1, '2026-07-31T23:00:00Z'),
        (2, 2, 'all', 1.1800, 0.5000, '2026-08-01T00:00:00Z', 'active', 1, '2026-07-31T23:00:00Z'),
        (3, 3, 'all', 1.1500, 0.4500, '2026-08-01T00:00:00Z', 'active', 1, '2026-07-31T23:00:00Z'),
        (4, 4, 'all', 1.2200, 0.5000, '2026-08-01T00:00:00Z', 'active', 1, '2026-07-31T23:00:00Z'),
        (5, 5, 'all', 1.1000, 0.4500, '2026-08-01T00:00:00Z', 'active', 1, '2026-07-31T23:00:00Z')
)
INSERT INTO tariffs
    (id, station_id, charger_type, electricity_price, service_price, effective_from,
     status, created_by, created_at)
SELECT
    ts.id,
    ts.station_id,
    ts.charger_type,
    ts.electricity_price,
    ts.service_price,
    ts.effective_from,
    ts.status,
    ts.created_by,
    ts.created_at
FROM tariff_seed ts
WHERE NOT EXISTS (
    SELECT 1
    FROM tariffs t
    WHERE t.station_id = ts.station_id
      AND t.charger_type = ts.charger_type
      AND t.effective_from = ts.effective_from
);

-- One completed reservation and session per station per day, covering 31 days.
-- Users 1-4 rotate through normal demand. User 5 is reserved for the debt case below.
WITH RECURSIVE dates(day_index, stat_date) AS (
    SELECT 0, '2026-08-01'
    UNION ALL
    SELECT day_index + 1, date(stat_date, '+1 day')
    FROM dates
    WHERE day_index < 30
), scheduled AS (
    SELECT
        d.day_index,
        d.stat_date,
        s.id AS station_id,
        ((d.day_index + s.id - 1) % 4) + 1 AS user_id,
        c.id AS charger_id,
        strftime('%Y-%m-%dT%H:%M:%SZ', d.stat_date || ' 08:00:00', printf('+%d minutes', s.id * 7)) AS reserved_at,
        strftime('%Y-%m-%dT%H:%M:%SZ', d.stat_date || ' 08:00:00', printf('+%d minutes', s.id * 7 + 30)) AS expires_at,
        strftime('%Y-%m-%dT%H:%M:%SZ', d.stat_date || ' 08:00:00', printf('+%d minutes', s.id * 7 + 5)) AS started_at,
        strftime('%Y-%m-%dT%H:%M:%SZ', d.stat_date || ' 08:00:00', printf('+%d minutes', s.id * 7 + 75)) AS ended_at
    FROM dates d
    CROSS JOIN stations s
    JOIN chargers c ON c.station_id = s.id
        AND c.charger_code = CASE d.day_index % 4
            WHEN 0 THEN 'AC001'
            WHEN 1 THEN 'AC002'
            WHEN 2 THEN 'DC001'
            ELSE 'DC002'
        END
)
INSERT OR IGNORE INTO reservations
    (reservation_no, user_id, charger_id, status, reserved_at, expires_at, started_at, ended_at,
     idempotency_key, created_at, updated_at)
SELECT
    'RSV' || replace(stat_date, '-', '') || printf('%02d', station_id),
    user_id,
    charger_id,
    'completed',
    reserved_at,
    expires_at,
    started_at,
    ended_at,
    'seed-rsv-' || replace(stat_date, '-', '') || printf('%02d', station_id),
    reserved_at,
    ended_at
FROM scheduled;

WITH RECURSIVE dates(day_index, stat_date) AS (
    SELECT 0, '2026-08-01'
    UNION ALL
    SELECT day_index + 1, date(stat_date, '+1 day')
    FROM dates
    WHERE day_index < 30
), session_rows AS (
    SELECT
        r.id AS reservation_id,
        r.reservation_no,
        r.user_id,
        r.charger_id,
        c.station_id,
        d.day_index,
        r.started_at,
        round(5 + c.station_id * 0.5 + (d.day_index % 5) * 0.25, 3) AS energy_kwh,
        2700 + ((c.station_id + d.day_index) % 4) * 600 AS duration_seconds,
        100 + ((d.day_index * 17 + c.station_id * 11) % 90) AS meter_start_kwh,
        25 + ((d.day_index + c.station_id) % 35) AS soc_start
    FROM reservations r
    JOIN chargers c ON c.id = r.charger_id
    JOIN dates d ON d.stat_date = substr(r.reserved_at, 1, 10)
    WHERE r.reservation_no LIKE 'RSV2026%'
)
INSERT OR IGNORE INTO charging_sessions
    (session_no, reservation_id, user_id, charger_id, station_id, status, started_at, ended_at,
     meter_start_kwh, meter_end_kwh, energy_kwh, duration_seconds, soc_start, soc_end,
     idempotency_key, created_at, updated_at)
SELECT
    replace(reservation_no, 'RSV', 'SES'),
    reservation_id,
    user_id,
    charger_id,
    station_id,
    'settled',
    started_at,
    strftime('%Y-%m-%dT%H:%M:%SZ', started_at, printf('+%d seconds', duration_seconds)),
    meter_start_kwh,
    round(meter_start_kwh + energy_kwh, 3),
    energy_kwh,
    duration_seconds,
    soc_start,
    min(100, soc_start + round(energy_kwh * 2.5, 0)),
    'seed-session-' || replace(reservation_no, 'RSV', ''),
    started_at,
    strftime('%Y-%m-%dT%H:%M:%SZ', started_at, printf('+%d seconds', duration_seconds))
FROM session_rows;

INSERT OR IGNORE INTO orders
    (order_no, session_id, user_id, station_id, charger_id, status, energy_kwh, duration_seconds,
     electricity_unit_price, service_unit_price, gross_amount, discount_amount, payable_amount,
     paid_amount, settled_at, created_at, updated_at)
SELECT
    replace(cs.session_no, 'SES', 'ORD'),
    cs.id,
    cs.user_id,
    cs.station_id,
    cs.charger_id,
    'paid',
    cs.energy_kwh,
    cs.duration_seconds,
    t.electricity_price,
    t.service_price,
    round(cs.energy_kwh * (t.electricity_price + t.service_price), 2),
    0,
    round(cs.energy_kwh * (t.electricity_price + t.service_price), 2),
    round(cs.energy_kwh * (t.electricity_price + t.service_price), 2),
    cs.ended_at,
    cs.ended_at,
    cs.ended_at
FROM charging_sessions cs
JOIN tariffs t ON t.station_id = cs.station_id
    AND t.charger_type = 'all'
    AND t.status = 'active'
WHERE cs.session_no LIKE 'SES2026%';

-- A completed high-energy session for user 5 intentionally exceeds the wallet's
-- 30.00 balance. Its charge starts with a positive balance and ends at -7.20;
-- no later reservation or charge is created for this user, modelling the <= 0 stop rule.
INSERT OR IGNORE INTO reservations
    (reservation_no, user_id, charger_id, status, reserved_at, expires_at, started_at, ended_at,
     idempotency_key, created_at, updated_at)
SELECT
    'RSV2026081550',
    5,
    c.id,
    'completed',
    '2026-08-15T09:00:00Z',
    '2026-08-15T09:30:00Z',
    '2026-08-15T09:05:00Z',
    '2026-08-15T10:05:00Z',
    'seed-rsv-debt-1',
    '2026-08-15T09:00:00Z',
    '2026-08-15T10:05:00Z'
FROM chargers c
WHERE c.station_id = 5 AND c.charger_code = 'DC002';

INSERT OR IGNORE INTO charging_sessions
    (session_no, reservation_id, user_id, charger_id, station_id, status, started_at, ended_at,
     meter_start_kwh, meter_end_kwh, energy_kwh, duration_seconds, soc_start, soc_end,
     idempotency_key, created_at, updated_at)
SELECT
    'SES2026081550',
    r.id,
    r.user_id,
    r.charger_id,
    c.station_id,
    'settled',
    '2026-08-15T09:05:00Z',
    '2026-08-15T10:05:00Z',
    450.000,
    474.000,
    24.000,
    3600,
    15.00,
    65.00,
    'seed-session-debt-1',
    '2026-08-15T09:05:00Z',
    '2026-08-15T10:05:00Z'
FROM reservations r
JOIN chargers c ON c.id = r.charger_id
WHERE r.reservation_no = 'RSV2026081550';

INSERT OR IGNORE INTO orders
    (order_no, session_id, user_id, station_id, charger_id, status, energy_kwh, duration_seconds,
     electricity_unit_price, service_unit_price, gross_amount, discount_amount, payable_amount,
     paid_amount, settled_at, created_at, updated_at)
SELECT
    'ORD2026081550',
    cs.id,
    cs.user_id,
    cs.station_id,
    cs.charger_id,
    'paid',
    cs.energy_kwh,
    cs.duration_seconds,
    t.electricity_price,
    t.service_price,
    round(cs.energy_kwh * (t.electricity_price + t.service_price), 2),
    0,
    round(cs.energy_kwh * (t.electricity_price + t.service_price), 2),
    round(cs.energy_kwh * (t.electricity_price + t.service_price), 2),
    cs.ended_at,
    cs.ended_at,
    cs.ended_at
FROM charging_sessions cs
JOIN tariffs t ON t.station_id = cs.station_id AND t.charger_type = 'all' AND t.status = 'active'
WHERE cs.session_no = 'SES2026081550';

-- Initial balances are deliberately different for the normal and debt users.
INSERT OR IGNORE INTO wallet_transactions
    (user_id, transaction_no, transaction_type, amount, balance_before, balance_after,
     order_id, idempotency_key, remark, created_at)
VALUES
    (1, 'WAL202607310001', 'recharge', 1000.00, 0.00, 1000.00, NULL, 'seed-recharge-1', '演示充值', '2026-07-31T23:10:00Z'),
    (2, 'WAL202607310002', 'recharge', 1000.00, 0.00, 1000.00, NULL, 'seed-recharge-2', '演示充值', '2026-07-31T23:11:00Z'),
    (3, 'WAL202607310003', 'recharge', 1000.00, 0.00, 1000.00, NULL, 'seed-recharge-3', '演示充值', '2026-07-31T23:12:00Z'),
    (4, 'WAL202607310004', 'recharge', 1000.00, 0.00, 1000.00, NULL, 'seed-recharge-4', '演示充值', '2026-07-31T23:13:00Z'),
    (5, 'WAL202607310005', 'recharge', 30.00, 0.00, 30.00, NULL, 'seed-recharge-5', '欠费案例初始余额', '2026-07-31T23:14:00Z');

-- Charge transactions are derived from paid orders so every row reconciles:
-- balance_after = balance_before + amount. The debt row is the only negative end.
WITH ordered AS (
    SELECT
        o.id AS order_id,
        o.user_id,
        o.order_no,
        o.paid_amount,
        o.settled_at,
        CASE WHEN o.user_id = 5 THEN 30.00 ELSE 1000.00 END AS initial_balance,
        COALESCE(SUM(o.paid_amount) OVER (
            PARTITION BY o.user_id
            ORDER BY o.settled_at, o.id
            ROWS BETWEEN UNBOUNDED PRECEDING AND 1 PRECEDING
        ), 0) AS prior_spend,
        SUM(o.paid_amount) OVER (
            PARTITION BY o.user_id
            ORDER BY o.settled_at, o.id
            ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
        ) AS cumulative_spend
    FROM orders o
    WHERE o.status = 'paid'
      AND o.order_no LIKE 'ORD2026%'
)
INSERT OR IGNORE INTO wallet_transactions
    (user_id, transaction_no, transaction_type, amount, balance_before, balance_after,
     order_id, idempotency_key, remark, created_at)
SELECT
    user_id,
    'WAL' || substr(order_no, 4),
    'charge',
    round(-paid_amount, 2),
    round(initial_balance - prior_spend, 2),
    round(initial_balance - cumulative_spend, 2),
    order_id,
    'seed-charge-' || order_no,
    CASE WHEN user_id = 5 THEN '欠费扣款，余额不足后停止后续扣款' ELSE '充电消费' END,
    settled_at
FROM ordered;

-- Keep the user snapshot equal to the final wallet ledger balance.
UPDATE users
SET wallet_balance = (
    SELECT wt.balance_after
    FROM wallet_transactions wt
    WHERE wt.user_id = users.id
    ORDER BY wt.created_at DESC, wt.id DESC
    LIMIT 1
),
    updated_at = (
        SELECT wt.created_at
        FROM wallet_transactions wt
        WHERE wt.user_id = users.id
        ORDER BY wt.created_at DESC, wt.id DESC
        LIMIT 1
    )
WHERE id BETWEEN 1 AND 5;

-- Daily statistics for every station and every historical date. Revenue and
-- settled metrics are sourced from orders, so the dashboard can cross-check v_daily_revenue.
WITH RECURSIVE dates(day_index, stat_date) AS (
    SELECT 0, '2026-08-01'
    UNION ALL
    SELECT day_index + 1, date(stat_date, '+1 day')
    FROM dates
    WHERE day_index < 30
), daily AS (
    SELECT
        o.station_id,
        substr(o.settled_at, 1, 10) AS stat_date,
        COUNT(*) AS order_count,
        COUNT(CASE WHEN o.status IN ('paid', 'refunded') THEN 1 END) AS settled_order_count,
        round(SUM(CASE WHEN o.status IN ('paid', 'refunded') THEN o.paid_amount ELSE 0 END), 2) AS revenue,
        round(SUM(CASE WHEN o.status IN ('paid', 'refunded') THEN o.energy_kwh ELSE 0 END), 3) AS energy_kwh,
        SUM(CASE WHEN o.status IN ('paid', 'refunded') THEN o.duration_seconds ELSE 0 END) AS charging_seconds
    FROM orders o
    GROUP BY o.station_id, substr(o.settled_at, 1, 10)
)
INSERT OR IGNORE INTO station_daily_stats
    (station_id, stat_date, order_count, settled_order_count, revenue, energy_kwh, charging_seconds,
     new_user_count, fault_count)
SELECT
    s.id,
    d.stat_date,
    COALESCE(x.order_count, 0),
    COALESCE(x.settled_order_count, 0),
    COALESCE(x.revenue, 0),
    COALESCE(x.energy_kwh, 0),
    COALESCE(x.charging_seconds, 0),
    COUNT(DISTINCT CASE WHEN substr(u.created_at, 1, 10) = d.stat_date THEN u.id END),
    (SELECT COUNT(*) FROM chargers fc WHERE fc.station_id = s.id AND fc.status = 'fault')
FROM stations s
CROSS JOIN dates d
LEFT JOIN daily x ON x.station_id = s.id AND x.stat_date = d.stat_date
LEFT JOIN orders o ON o.station_id = s.id AND substr(o.settled_at, 1, 10) = d.stat_date
LEFT JOIN users u ON u.id = o.user_id
GROUP BY s.id, d.stat_date, x.order_count, x.settled_order_count, x.revenue, x.energy_kwh, x.charging_seconds;

INSERT OR IGNORE INTO charger_status_history
    (id, charger_id, from_status, to_status, reason, source, operator_admin_id, occurred_at)
VALUES
    (1, (SELECT id FROM chargers WHERE station_id = 1 AND charger_code = 'AC003'), 'idle', 'fault', '绝缘检测失败', 'system', NULL, '2026-08-30T11:00:00Z'),
    (2, (SELECT id FROM chargers WHERE station_id = 2 AND charger_code = 'AC003'), 'idle', 'fault', '通信异常', 'system', NULL, '2026-08-29T14:00:00Z'),
    (3, (SELECT id FROM chargers WHERE station_id = 2 AND charger_code = 'DC003'), 'idle', 'fault', '枪口温度过高', 'system', NULL, '2026-08-28T09:00:00Z'),
    (4, (SELECT id FROM chargers WHERE station_id = 3 AND charger_code = 'AC003'), 'idle', 'fault', '漏电保护触发', 'system', NULL, '2026-08-27T12:00:00Z'),
    (5, (SELECT id FROM chargers WHERE station_id = 4 AND charger_code = 'AC003'), 'idle', 'fault', '急停按钮触发', 'admin', 1, '2026-08-26T16:00:00Z'),
    (6, (SELECT id FROM chargers WHERE station_id = 4 AND charger_code = 'DC003'), 'idle', 'fault', '模块过热', 'system', NULL, '2026-08-25T08:00:00Z'),
    (7, (SELECT id FROM chargers WHERE station_id = 5 AND charger_code = 'AC003'), 'idle', 'fault', '输出异常', 'system', NULL, '2026-08-24T13:00:00Z');

WITH horizons(slot, horizon_start, horizon_end) AS (
    VALUES
        (1, '2026-09-01T08:00:00Z', '2026-09-01T09:00:00Z'),
        (2, '2026-09-01T18:00:00Z', '2026-09-01T19:00:00Z'),
        (3, '2026-09-02T08:00:00Z', '2026-09-02T09:00:00Z')
)
INSERT OR IGNORE INTO load_forecasts
    (station_id, forecast_time, horizon_start, horizon_end, predicted_load_kw,
     predicted_available_chargers, model_version, actual_load_kw, generated_at)
SELECT
    s.id,
    '2026-09-01T00:00:00Z',
    h.horizon_start,
    h.horizon_end,
    round(18 + s.id * 4.5 + h.slot * 3.25, 3),
    6 - (SELECT COUNT(*) FROM chargers fc WHERE fc.station_id = s.id AND fc.status = 'fault'),
    'demo-v2',
    CASE WHEN h.slot = 1 THEN round(16 + s.id * 3.8, 3) ELSE NULL END,
    '2026-08-31T23:00:00Z'
FROM stations s
CROSS JOIN horizons h;

INSERT OR IGNORE INTO outbox_events
    (event_id, aggregate_type, aggregate_id, event_type, payload_json, status, retry_count, created_at)
VALUES
    ('seed-station-1', 'station', 1, 'station.seeded', '{"station_code":"ST001","charger_count":6}', 'published', 0, '2026-08-01T00:00:00Z'),
    ('seed-station-2', 'station', 2, 'station.seeded', '{"station_code":"ST002","charger_count":6}', 'published', 0, '2026-08-01T00:00:00Z'),
    ('seed-station-3', 'station', 3, 'station.seeded', '{"station_code":"ST003","charger_count":6}', 'published', 0, '2026-08-01T00:00:00Z'),
    ('seed-station-4', 'station', 4, 'station.seeded', '{"station_code":"ST004","charger_count":6}', 'published', 0, '2026-08-01T00:00:00Z'),
    ('seed-station-5', 'station', 5, 'station.seeded', '{"station_code":"ST005","charger_count":6}', 'published', 0, '2026-08-01T00:00:00Z');

COMMIT;
