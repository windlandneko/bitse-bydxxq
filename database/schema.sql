PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;
PRAGMA busy_timeout = 5000;
CREATE TABLE IF NOT EXISTS users (
  id INTEGER PRIMARY KEY,
  phone TEXT NOT NULL UNIQUE,
  nickname TEXT NOT NULL,
  avatar_url TEXT NOT NULL DEFAULT '',
  balance_cents INTEGER NOT NULL DEFAULT 0 CHECK (balance_cents >= 0 AND typeof(balance_cents) = 'integer'),
  status TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active','frozen')),
  created_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS admins (
  id INTEGER PRIMARY KEY,
  username TEXT NOT NULL UNIQUE,
  salt TEXT NOT NULL,
  password_hash TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS stations (
  id INTEGER PRIMARY KEY,
  code TEXT NOT NULL UNIQUE,
  name TEXT NOT NULL,
  address TEXT NOT NULL,
  region TEXT NOT NULL,
  latitude REAL NOT NULL CHECK (latitude BETWEEN -90 AND 90),
  longitude REAL NOT NULL CHECK (longitude BETWEEN -180 AND 180),
  price_cents INTEGER NOT NULL CHECK (price_cents > 0 AND price_cents <= 10000),
  created_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS chargers (
  id INTEGER PRIMARY KEY,
  station_id INTEGER NOT NULL REFERENCES stations(id),
  code TEXT NOT NULL UNIQUE,
  type TEXT NOT NULL CHECK (type IN ('ac','dc')),
  power_kw REAL NOT NULL CHECK (power_kw > 0 AND power_kw <= 1000),
  status TEXT NOT NULL DEFAULT 'idle' CHECK (status IN ('idle','reserved','charging','fault','offline','restarting')),
  charging_count INTEGER NOT NULL DEFAULT 0 CHECK (charging_count >= 0),
  charging_seconds INTEGER NOT NULL DEFAULT 0 CHECK (charging_seconds >= 0),
  energy_wh INTEGER NOT NULL DEFAULT 0 CHECK (energy_wh >= 0),
  restart_at TEXT,
  created_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS orders (
  id INTEGER PRIMARY KEY,
  order_no TEXT NOT NULL UNIQUE,
  user_id INTEGER NOT NULL REFERENCES users(id),
  station_id INTEGER NOT NULL REFERENCES stations(id),
  charger_id INTEGER NOT NULL REFERENCES chargers(id),
  station_name TEXT NOT NULL,
  charger_code TEXT NOT NULL,
  charger_type TEXT NOT NULL,
  power_kw REAL NOT NULL,
  price_cents INTEGER NOT NULL,
  status TEXT NOT NULL CHECK (status IN ('reserved','charging','pending_payment','paid','cancelled')),
  created_at TEXT NOT NULL,
  expires_at TEXT NOT NULL,
  started_at TEXT NOT NULL DEFAULT '',
  ended_at TEXT NOT NULL DEFAULT '',
  energy_wh INTEGER NOT NULL DEFAULT 0 CHECK (energy_wh >= 0),
  duration_seconds INTEGER NOT NULL DEFAULT 0 CHECK (duration_seconds >= 0),
  amount_cents INTEGER NOT NULL DEFAULT 0 CHECK (amount_cents >= 0),
  budget_cents INTEGER NOT NULL DEFAULT 0,
  time_scale INTEGER NOT NULL DEFAULT 60 CHECK (time_scale BETWEEN 1 AND 3600),
  stop_reason TEXT NOT NULL DEFAULT '',
  settled_at TEXT NOT NULL DEFAULT ''
);
CREATE UNIQUE INDEX IF NOT EXISTS one_active_order_user ON orders(user_id) WHERE status IN ('reserved','charging','pending_payment');
CREATE UNIQUE INDEX IF NOT EXISTS one_occupant_charger ON orders(charger_id) WHERE status IN ('reserved','charging');
CREATE INDEX IF NOT EXISTS orders_user_time ON orders(user_id,created_at DESC);
CREATE INDEX IF NOT EXISTS orders_stats ON orders(status,settled_at);
CREATE INDEX IF NOT EXISTS chargers_station ON chargers(station_id,status);
CREATE TABLE IF NOT EXISTS wallet_transactions (
  id INTEGER PRIMARY KEY,
  user_id INTEGER NOT NULL REFERENCES users(id),
  order_id INTEGER UNIQUE REFERENCES orders(id),
  kind TEXT NOT NULL CHECK (kind IN ('recharge','charge')),
  amount_cents INTEGER NOT NULL,
  balance_before INTEGER NOT NULL CHECK (balance_before >= 0),
  balance_after INTEGER NOT NULL CHECK (balance_after >= 0),
  created_at TEXT NOT NULL,
  CHECK (balance_after = balance_before + amount_cents),
  CHECK ((kind = 'recharge' AND amount_cents > 0) OR (kind = 'charge' AND amount_cents <= 0))
);
CREATE TABLE IF NOT EXISTS requests (
  principal TEXT NOT NULL,
  action TEXT NOT NULL,
  key TEXT NOT NULL,
  payload TEXT NOT NULL,
  response TEXT NOT NULL,
  created_at TEXT NOT NULL,
  PRIMARY KEY (principal, action, key)
);
CREATE TABLE IF NOT EXISTS audit_logs (
  id INTEGER PRIMARY KEY,
  actor TEXT NOT NULL,
  action TEXT NOT NULL,
  target TEXT NOT NULL,
  detail TEXT NOT NULL,
  created_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS forecasts (
  id INTEGER PRIMARY KEY CHECK (id=1),
  payload TEXT NOT NULL
);
PRAGMA user_version = 2;
