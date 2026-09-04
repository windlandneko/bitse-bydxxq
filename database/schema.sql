-- SQLite 3 schema for the charging platform.
-- The application must execute this file with foreign_keys enabled.

PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;
PRAGMA busy_timeout = 5000;

CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY,
    phone TEXT NOT NULL UNIQUE,
    nickname TEXT NOT NULL,
    avatar_path TEXT,
    wallet_balance NUMERIC NOT NULL DEFAULT 0,
    status TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'frozen', 'disabled')),
    freeze_reason TEXT,
    last_login_at TEXT,
    version INTEGER NOT NULL DEFAULT 0 CHECK (version >= 0),
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS admins (
    id INTEGER PRIMARY KEY,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    display_name TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'locked', 'disabled')),
    last_login_at TEXT,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS stations (
    id INTEGER PRIMARY KEY,
    station_code TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    address TEXT NOT NULL,
    region_code TEXT,
    latitude NUMERIC CHECK (latitude IS NULL OR latitude BETWEEN -90 AND 90),
    longitude NUMERIC CHECK (longitude IS NULL OR longitude BETWEEN -180 AND 180),
    status TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'closed', 'maintenance')),
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS tariffs (
    id INTEGER PRIMARY KEY,
    station_id INTEGER NOT NULL REFERENCES stations(id) ON DELETE RESTRICT,
    charger_type TEXT NOT NULL CHECK (charger_type IN ('ac', 'dc', 'all')),
    electricity_price NUMERIC NOT NULL CHECK (electricity_price >= 0),
    service_price NUMERIC NOT NULL CHECK (service_price >= 0),
    created_by INTEGER REFERENCES admins(id) ON DELETE RESTRICT,
    created_at TEXT NOT NULL,
    UNIQUE (station_id, charger_type)
);

CREATE TABLE IF NOT EXISTS chargers (
    id INTEGER PRIMARY KEY,
    station_id INTEGER NOT NULL REFERENCES stations(id) ON DELETE RESTRICT,
    charger_code TEXT NOT NULL,
    charger_type TEXT NOT NULL CHECK (charger_type IN ('ac', 'dc')),
    rated_power_kw NUMERIC NOT NULL CHECK (rated_power_kw > 0),
    status TEXT NOT NULL DEFAULT 'idle' CHECK (status IN ('idle', 'reserved', 'charging', 'fault', 'offline', 'maintenance')),
    cumulative_charging_count INTEGER NOT NULL DEFAULT 0 CHECK (cumulative_charging_count >= 0),
    cumulative_charging_seconds INTEGER NOT NULL DEFAULT 0 CHECK (cumulative_charging_seconds >= 0),
    cumulative_energy_kwh NUMERIC NOT NULL DEFAULT 0 CHECK (cumulative_energy_kwh >= 0),
    last_heartbeat_at TEXT,
    version INTEGER NOT NULL DEFAULT 0 CHECK (version >= 0),
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    UNIQUE (station_id, charger_code)
);

CREATE TABLE IF NOT EXISTS reservations (
    id INTEGER PRIMARY KEY,
    reservation_no TEXT NOT NULL UNIQUE,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    charger_id INTEGER NOT NULL REFERENCES chargers(id) ON DELETE RESTRICT,
    status TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'started', 'cancelled', 'expired', 'completed')),
    reserved_at TEXT NOT NULL,
    expires_at TEXT NOT NULL,
    started_at TEXT,
    ended_at TEXT,
    idempotency_key TEXT NOT NULL UNIQUE,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    CHECK (expires_at > reserved_at),
    CHECK (ended_at IS NULL OR started_at IS NULL OR ended_at >= started_at),
    CHECK (status <> 'active' OR (started_at IS NULL AND ended_at IS NULL)),
    CHECK (status <> 'started' OR (started_at IS NOT NULL AND ended_at IS NULL)),
    CHECK (status NOT IN ('cancelled', 'expired') OR ended_at IS NOT NULL),
    CHECK (status <> 'completed' OR (started_at IS NOT NULL AND ended_at IS NOT NULL))
);

CREATE TABLE IF NOT EXISTS charging_sessions (
    id INTEGER PRIMARY KEY,
    session_no TEXT NOT NULL UNIQUE,
    reservation_id INTEGER NOT NULL UNIQUE REFERENCES reservations(id) ON DELETE RESTRICT,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    charger_id INTEGER NOT NULL REFERENCES chargers(id) ON DELETE RESTRICT,
    station_id INTEGER NOT NULL REFERENCES stations(id) ON DELETE RESTRICT,
    status TEXT NOT NULL DEFAULT 'preparing' CHECK (status IN ('preparing', 'charging', 'interrupted', 'completed', 'settling', 'settled', 'failed')),
    started_at TEXT,
    ended_at TEXT,
    meter_start_kwh NUMERIC CHECK (meter_start_kwh IS NULL OR meter_start_kwh >= 0),
    meter_end_kwh NUMERIC CHECK (meter_end_kwh IS NULL OR meter_end_kwh >= 0),
    energy_kwh NUMERIC NOT NULL DEFAULT 0 CHECK (energy_kwh >= 0),
    duration_seconds INTEGER NOT NULL DEFAULT 0 CHECK (duration_seconds >= 0),
    soc_start NUMERIC CHECK (soc_start IS NULL OR soc_start BETWEEN 0 AND 100),
    soc_end NUMERIC CHECK (soc_end IS NULL OR soc_end BETWEEN 0 AND 100),
    interrupt_reason TEXT,
    idempotency_key TEXT NOT NULL UNIQUE,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    CHECK (ended_at IS NULL OR started_at IS NULL OR ended_at >= started_at),
    CHECK (meter_end_kwh IS NULL OR meter_start_kwh IS NULL OR meter_end_kwh >= meter_start_kwh)
);

CREATE TABLE IF NOT EXISTS orders (
    id INTEGER PRIMARY KEY,
    order_no TEXT NOT NULL UNIQUE,
    session_id INTEGER NOT NULL UNIQUE REFERENCES charging_sessions(id) ON DELETE RESTRICT,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    station_id INTEGER NOT NULL REFERENCES stations(id) ON DELETE RESTRICT,
    charger_id INTEGER NOT NULL REFERENCES chargers(id) ON DELETE RESTRICT,
    status TEXT NOT NULL DEFAULT 'pending_payment' CHECK (status IN ('pending_payment', 'paid', 'payment_failed', 'cancelled', 'refunded')),
    energy_kwh NUMERIC NOT NULL CHECK (energy_kwh >= 0),
    duration_seconds INTEGER NOT NULL CHECK (duration_seconds >= 0),
    electricity_unit_price NUMERIC NOT NULL CHECK (electricity_unit_price >= 0),
    service_unit_price NUMERIC NOT NULL CHECK (service_unit_price >= 0),
    gross_amount NUMERIC NOT NULL CHECK (gross_amount >= 0),
    discount_amount NUMERIC NOT NULL DEFAULT 0 CHECK (discount_amount >= 0),
    payable_amount NUMERIC NOT NULL CHECK (payable_amount >= 0),
    paid_amount NUMERIC NOT NULL DEFAULT 0 CHECK (paid_amount >= 0),
    failure_reason TEXT,
    settled_at TEXT,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    CHECK (payable_amount = round(gross_amount - discount_amount, 2)),
    CHECK (paid_amount <= payable_amount OR status = 'refunded'),
    CHECK (
        (status IN ('paid', 'refunded') AND paid_amount = payable_amount AND settled_at IS NOT NULL)
        OR
        (status IN ('pending_payment', 'payment_failed', 'cancelled') AND paid_amount = 0 AND settled_at IS NULL)
    )
);

CREATE TABLE IF NOT EXISTS wallet_transactions (
    id INTEGER PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    transaction_no TEXT NOT NULL UNIQUE,
    transaction_type TEXT NOT NULL CHECK (transaction_type IN ('recharge', 'charge', 'refund', 'adjustment')),
    amount NUMERIC NOT NULL CHECK (amount <> 0),
    balance_before NUMERIC NOT NULL,
    balance_after NUMERIC NOT NULL,
    order_id INTEGER REFERENCES orders(id) ON DELETE RESTRICT,
    idempotency_key TEXT NOT NULL UNIQUE,
    remark TEXT,
    created_at TEXT NOT NULL,
    CHECK (
        (transaction_type IN ('recharge', 'refund') AND amount > 0)
        OR (transaction_type = 'charge' AND amount < 0)
        OR transaction_type = 'adjustment'
    ),
    CHECK (transaction_type <> 'charge' OR balance_before > 0),
    CHECK (balance_after = round(balance_before + amount, 2)),
    CHECK (transaction_type NOT IN ('charge', 'refund') OR order_id IS NOT NULL)
);

CREATE TABLE IF NOT EXISTS charger_status_history (
    id INTEGER PRIMARY KEY,
    charger_id INTEGER NOT NULL REFERENCES chargers(id) ON DELETE RESTRICT,
    from_status TEXT CHECK (
        from_status IS NULL OR from_status IN
        ('idle', 'reserved', 'charging', 'fault', 'offline', 'maintenance')
    ),
    to_status TEXT NOT NULL CHECK (
        to_status IN
        ('idle', 'reserved', 'charging', 'fault', 'offline', 'maintenance')
    ),
    reason TEXT,
    source TEXT NOT NULL CHECK (source IN ('user', 'admin', 'system')),
    operator_admin_id INTEGER REFERENCES admins(id) ON DELETE RESTRICT,
    occurred_at TEXT NOT NULL,
    CHECK (from_status IS NULL OR from_status <> to_status),
    CHECK (source <> 'admin' OR operator_admin_id IS NOT NULL)
);

CREATE TABLE IF NOT EXISTS admin_audit_logs (
    id INTEGER PRIMARY KEY,
    admin_id INTEGER REFERENCES admins(id) ON DELETE RESTRICT,
    action TEXT NOT NULL,
    target_type TEXT NOT NULL,
    target_id INTEGER,
    request_id TEXT,
    before_json TEXT,
    after_json TEXT,
    ip_address TEXT,
    created_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS load_forecasts (
    id INTEGER PRIMARY KEY,
    station_id INTEGER NOT NULL REFERENCES stations(id) ON DELETE RESTRICT,
    forecast_time TEXT NOT NULL,
    horizon_start TEXT NOT NULL,
    horizon_end TEXT NOT NULL,
    predicted_load_kw NUMERIC NOT NULL CHECK (predicted_load_kw >= 0),
    predicted_available_chargers INTEGER NOT NULL CHECK (predicted_available_chargers >= 0),
    is_peak INTEGER NOT NULL DEFAULT 0 CHECK (is_peak IN (0, 1)),
    model_version TEXT NOT NULL,
    actual_load_kw NUMERIC CHECK (actual_load_kw IS NULL OR actual_load_kw >= 0),
    generated_at TEXT NOT NULL,
    UNIQUE (station_id, horizon_start, horizon_end, model_version),
    CHECK (horizon_end > horizon_start)
);

CREATE INDEX IF NOT EXISTS idx_chargers_station_status ON chargers(station_id, status);
CREATE INDEX IF NOT EXISTS idx_stations_coordinates ON stations(latitude, longitude);
CREATE INDEX IF NOT EXISTS idx_reservations_user_created ON reservations(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_reservations_charger_created ON reservations(charger_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_sessions_user_created ON charging_sessions(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_orders_user_created ON orders(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_orders_status_created ON orders(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_wallet_user_created ON wallet_transactions(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_history_charger_time ON charger_status_history(charger_id, occurred_at DESC);
CREATE INDEX IF NOT EXISTS idx_audit_target_time ON admin_audit_logs(target_type, target_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_forecasts_station_horizon ON load_forecasts(station_id, horizon_start);

CREATE UNIQUE INDEX IF NOT EXISTS uq_active_reservation_user
    ON reservations(user_id) WHERE status IN ('active', 'started');
CREATE UNIQUE INDEX IF NOT EXISTS uq_active_reservation_charger
    ON reservations(charger_id) WHERE status IN ('active', 'started');
CREATE UNIQUE INDEX IF NOT EXISTS uq_active_session_user
    ON charging_sessions(user_id) WHERE status IN ('preparing', 'charging', 'interrupted', 'settling');
CREATE UNIQUE INDEX IF NOT EXISTS uq_active_session_charger
    ON charging_sessions(charger_id) WHERE status IN ('preparing', 'charging', 'interrupted', 'settling');

CREATE VIEW IF NOT EXISTS v_station_availability AS
SELECT
    s.id AS station_id,
    s.station_code,
    s.name,
    s.address,
    s.region_code,
    s.latitude,
    s.longitude,
    COUNT(c.id) AS charger_count,
    SUM(CASE WHEN c.status = 'idle' THEN 1 ELSE 0 END) AS idle_charger_count,
    SUM(CASE WHEN c.status = 'charging' THEN 1 ELSE 0 END) AS charging_charger_count,
    SUM(CASE WHEN c.status = 'fault' THEN 1 ELSE 0 END) AS fault_charger_count
FROM stations s
LEFT JOIN chargers c ON c.station_id = s.id
WHERE s.status = 'active'
GROUP BY s.id;

CREATE VIEW IF NOT EXISTS v_daily_revenue AS
SELECT
    station_id,
    substr(settled_at, 1, 10) AS stat_date,
    COUNT(*) AS settled_order_count,
    round(SUM(paid_amount), 2) AS revenue,
    round(SUM(energy_kwh), 3) AS energy_kwh
FROM orders
WHERE status = 'paid' AND settled_at IS NOT NULL
GROUP BY station_id, substr(settled_at, 1, 10);
