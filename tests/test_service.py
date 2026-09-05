import json
import socket
import sqlite3
import time
import urllib.request
import uuid
from concurrent.futures import ThreadPoolExecutor

from conftest import Server


def key():
  return str(uuid.uuid4())


def reserve(server, token, charger=1):
  return server.rpc('orders.reserve', {'chargerId': charger, 'idempotencyKey': key()}, token)


def test_phone_login_and_role_boundaries(server):
  server.rpc('user.login', {'phone': "1' OR 1=1--"}, ok=False)
  token, user = server.user(0)
  assert user['nickname'] == '用户' + user['phone'][-4:]
  assert server.rpc('user.login', {'phone': user['phone']})['user']['id'] == user['id']
  assert server.rpc('admin.users', token=token, ok=False)['code'] == 'FORBIDDEN'
  assert server.rpc('user.me', ok=False)['code'] == 'UNAUTHENTICATED'
  server.rpc('admin.login', {'username': 'admin', 'password': 'wrong'}, ok=False)
  server.rpc('auth.logout', token=token)
  server.rpc('user.me', token=token, ok=False)


def test_cent_precision_and_idempotency(server):
  token, _ = server.user(0)
  request = {'amountCents': 36109, 'idempotencyKey': key()}
  assert server.rpc('wallet.recharge', request, token)['balanceCents'] == 36109
  assert server.rpc('wallet.recharge', request, token)['balanceCents'] == 36109
  assert (
    server.rpc('wallet.recharge', {**request, 'amountCents': 1}, token, ok=False)['code']
    == 'IDEMPOTENCY_CONFLICT'
  )
  assert (
    server.rpc('wallet.recharge', {'amountCents': 1, 'idempotencyKey': key()}, token)[
      'balanceCents'
    ]
    == 36110
  )
  for amount in [0, -1, 0.01, 1000001, '100']:
    server.rpc(
      'wallet.recharge',
      {'amountCents': amount, 'idempotencyKey': key()},
      token,
      ok=False,
    )
  with sqlite3.connect(server.directory / 'platform.db') as conn:
    assert conn.execute('PRAGMA integrity_check').fetchone()[0] == 'ok'
    assert conn.execute('PRAGMA foreign_key_check').fetchall() == []
    assert (
      conn.execute("SELECT COUNT(*) FROM users WHERE typeof(balance_cents)!='integer'").fetchone()[
        0
      ]
      == 0
    )


def test_full_charge_flow_price_snapshot_revenue_and_repeat_settlement(server):
  token, _profile = server.user()
  admin = server.admin()
  before = server.rpc('admin.overview', {'days': 7}, admin)['todayRevenueCents']
  current = reserve(server, token)
  assert current['status'] == 'reserved'
  oid = {'orderId': current['id']}
  assert server.rpc('orders.active', token=token)['id'] == current['id']
  server.rpc('orders.settle', oid, token, ok=False)
  started = server.rpc('orders.start', oid, token)
  station = server.rpc('stations.detail', {'stationId': 1})['station']
  server.rpc('admin.station.save', {**station, 'priceCents': 900}, admin)
  time.sleep(0.25)
  stopped = server.rpc('orders.stop', oid, token)
  assert stopped['status'] == 'pending_payment'
  assert stopped['priceCents'] == started['priceCents'] != 900
  assert stopped['amountCents'] > 0
  server.rpc('orders.reserve', {'chargerId': 2, 'idempotencyKey': key()}, token, ok=False)
  paid = server.rpc('orders.settle', oid, token)
  assert paid['balanceCents'] == 10000 - paid['amountCents']
  assert server.rpc('orders.settle', oid, token) == paid
  assert server.rpc('orders.stop', oid, token) == paid
  assert server.rpc('orders.active', token=token) is None
  after = server.rpc('admin.overview', {'days': 7}, admin)['todayRevenueCents']
  assert after - before == paid['amountCents']
  with urllib.request.urlopen(server.url + '/api/dashboard') as response:
    dashboard = json.load(response)
  assert sum(p['revenue'] for p in dashboard['revenueTrend']) > 0
  with sqlite3.connect(server.directory / 'platform.db') as conn:
    assert (
      conn.execute(
        'SELECT COUNT(*) FROM wallet_transactions WHERE order_id=?',
        (current['id'],),
      ).fetchone()[0]
      == 1
    )


def test_concurrent_reservations_and_order_ownership(server):
  one, _ = server.user()
  two, _ = server.user()

  def attempt(token):
    request = urllib.request.Request(
      server.url + '/api/rpc',
      json.dumps(
        {
          'action': 'orders.reserve',
          'params': {'chargerId': 1, 'idempotencyKey': key()},
        }
      ).encode(),
      {'Content-Type': 'application/json', 'Authorization': 'Bearer ' + token},
    )
    try:
      response = urllib.request.urlopen(request)
    except urllib.error.HTTPError as exc:
      response = exc
    with response:
      return json.load(response)

  with ThreadPoolExecutor(max_workers=2) as executor:
    results = list(executor.map(attempt, [one, two]))
  assert sum(r['ok'] for r in results) == 1
  winner = 0 if results[0]['ok'] else 1
  order = results[winner]['data']
  tokens = [one, two]
  assert (
    server.rpc('orders.get', {'orderId': order['id']}, tokens[1 - winner], ok=False)['code']
    == 'FORBIDDEN'
  )
  server.rpc(
    'orders.reserve',
    {'chargerId': 2, 'idempotencyKey': key()},
    tokens[winner],
    ok=False,
  )
  server.rpc('orders.cancel', {'orderId': order['id']}, tokens[winner])
  assert reserve(server, tokens[1 - winner])['status'] == 'reserved'


def test_freeze_inflight_and_safe_device_restart(server):
  token, profile = server.user()
  admin = server.admin()
  current = reserve(server, token)
  oid = {'orderId': current['id']}
  server.rpc('orders.start', oid, token)
  server.rpc('admin.charger.restart', {'chargerId': 1}, admin, ok=False)
  server.rpc('admin.charger.status', {'chargerId': 1, 'status': 'fault'}, admin, ok=False)
  server.rpc('admin.user.status', {'userId': profile['id'], 'status': 'frozen'}, admin)
  server.rpc('user.login', {'phone': profile['phone']}, ok=False)
  server.rpc('orders.stop', oid, token)
  server.rpc('orders.settle', oid, token)
  server.rpc('orders.reserve', {'chargerId': 2, 'idempotencyKey': key()}, token, ok=False)
  server.rpc('admin.charger.restart', {'chargerId': 1}, admin)
  deadline = time.monotonic() + 4
  while time.monotonic() < deadline:
    items = server.rpc('admin.chargers', {'stationId': 1}, admin)
    if next(c for c in items if c['id'] == 1)['status'] == 'idle':
      break
    time.sleep(0.1)
  assert next(c for c in items if c['id'] == 1)['status'] == 'idle'
  assert any(log['action'] == '设备重启完成' for log in server.rpc('admin.logs', token=admin))


def test_automatic_budget_stop_and_reservation_expiry(server):
  token, _ = server.user(1)
  current = reserve(server, token)
  server.rpc('orders.start', {'orderId': current['id']}, token)
  time.sleep(1.3)
  active = server.rpc('orders.active', token=token)
  assert active['status'] == 'pending_payment'
  assert active['amountCents'] == 1
  assert server.rpc('orders.settle', {'orderId': current['id']}, token)['balanceCents'] == 0
  other, _ = server.user()
  current = reserve(server, other)
  with sqlite3.connect(server.directory / 'platform.db') as conn:
    conn.execute(
      "UPDATE orders SET expires_at='2000-01-01T00:00:00.000Z' WHERE id=?",
      (current['id'],),
    )
  time.sleep(1.2)
  assert server.rpc('orders.get', {'orderId': current['id']}, other)['status'] == 'cancelled'
  assert server.rpc('orders.active', token=other) is None


def test_restart_recovers_charge_and_does_not_reseed(server):
  token, profile = server.user()
  current = reserve(server, token)
  server.rpc('orders.start', {'orderId': current['id']}, token)
  with sqlite3.connect(server.directory / 'platform.db') as conn:
    counts = conn.execute(
      'SELECT (SELECT COUNT(*) FROM users),(SELECT COUNT(*) FROM chargers)'
    ).fetchone()
  server.close()
  time.sleep(0.2)
  server.start()
  token = server.rpc('user.login', {'phone': profile['phone']})['token']
  active = server.rpc('orders.active', token=token)
  assert active['id'] == current['id']
  assert active['durationSeconds'] > 0
  server.rpc('orders.stop', {'orderId': current['id']}, token)
  server.rpc('orders.settle', {'orderId': current['id']}, token)
  with sqlite3.connect(server.directory / 'platform.db') as conn:
    assert (
      conn.execute('SELECT (SELECT COUNT(*) FROM users),(SELECT COUNT(*) FROM chargers)').fetchone()
      == counts
    )


def test_station_creation_validation_and_stats(server):
  admin = server.admin()
  params = {
    'name': "测试'电站",
    'address': '测试地址',
    'region': '测试区',
    'latitude': 31.1,
    'longitude': 121.1,
    'priceCents': 100,
    'chargerCount': 4,
    'powerKw': 60,
  }
  for invalid in [{'latitude': 91}, {'priceCents': -1}, {'chargerCount': 0}]:
    server.rpc('admin.station.save', {**params, **invalid}, admin, ok=False)
  station = server.rpc('admin.station.save', params, admin)
  assert station['totalChargers'] == 4
  piles = server.rpc('admin.chargers', {'stationId': station['id']}, admin)
  assert {c['type'] for c in piles} == {'ac', 'dc'}
  data = server.rpc('stations.list', {'latitude': 31.1, 'longitude': 121.1})
  assert data[0]['id'] == station['id']
  assert data[0]['distanceKm'] < 0.001
  assert len(server.rpc('admin.overview', {'days': 30}, admin)['revenueTrend']) == 30
  assert (
    abs(sum(v['percent'] for v in server.rpc('admin.overview', token=admin)['statusCounts']) - 100)
    < 0.001
  )


def test_transaction_rollback_on_ledger_failure(server):
  token, profile = server.user(0)
  with sqlite3.connect(server.directory / 'platform.db') as conn:
    conn.execute(
      "CREATE TRIGGER simulate_failure BEFORE INSERT ON wallet_transactions BEGIN SELECT RAISE(ABORT,'injected test failure'); END"
    )
  server.rpc(
    'wallet.recharge',
    {'amountCents': 100, 'idempotencyKey': key()},
    token,
    ok=False,
  )
  assert server.rpc('user.me', token=token)['balanceCents'] == 0
  with sqlite3.connect(server.directory / 'platform.db') as conn:
    assert (
      conn.execute(
        'SELECT COUNT(*) FROM wallet_transactions WHERE user_id=?',
        (profile['id'],),
      ).fetchone()[0]
      == 0
    )


def test_http_fragmentation_validation_and_no_path_traversal(server):
  port = int(server.url.rsplit(':', 1)[1])

  def send(chunks):
    with socket.create_connection(('127.0.0.1', port), timeout=3) as sock:
      for chunk in chunks:
        sock.sendall(chunk)
        time.sleep(0.015)
      result = b''
      while part := sock.recv(65536):
        result += part
      return result

  body = json.dumps({'action': 'health'}).encode()
  request = (
    b'POST /api/rpc HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nContent-Length: '
    + str(len(body)).encode()
    + b'\r\n\r\n'
    + body
  )
  assert b'200 OK' in send([request[:20], request[20:77], request[77:]])
  assert b'400 Error' in send(
    [b'POST /api/rpc HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\nContent-Length: 5\r\n\r\n']
  )
  assert b'400 Error' in send(
    [b'POST /api/rpc HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n']
  )
  assert b'404 Error' in send(
    [b'GET /%2e%2e/data/map-settings.json HTTP/1.1\r\nHost: localhost\r\n\r\n']
  )
  assert b'403 Error' in send(
    [b'GET /api/dashboard HTTP/1.1\r\nHost: localhost\r\nOrigin: https://untrusted.example\r\n\r\n']
  )


def test_empty_database_and_missing_map_configuration(tmp_path):
  server = Server(tmp_path / 'empty', seed=False)
  try:
    assert server.rpc('stations.list') == []
    assert (
      server.rpc('location.geocode', {'address': '上海人民广场'}, ok=False)['code']
      == 'MAP_NOT_CONFIGURED'
    )
    admin = server.admin()
    assert server.rpc('admin.overview', token=admin)['todayRevenueCents'] == 0
  finally:
    server.close()


def test_avatar_validation_profile_and_public_image(server):
  import base64
  import struct
  import zlib

  token, profile = server.user(0)

  def chunk(kind, data):
    return struct.pack('!I', len(data)) + kind + data + struct.pack('!I', zlib.crc32(kind + data))

  png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('!IIBBBBB', 1, 1, 8, 2, 0, 0, 0))
  png += chunk(b'IDAT', zlib.compress(b'\x00\x80\x80\x80')) + chunk(b'IEND', b'')
  result = server.rpc(
    'user.update',
    {'nickname': "小林'同学", 'avatarBase64': base64.b64encode(png).decode()},
    token,
  )
  assert result['nickname'] == "小林'同学"
  assert result['avatarUrl'].startswith('/uploads/')
  with urllib.request.urlopen(server.url + result['avatarUrl']) as response:
    assert response.headers['Content-Type'] == 'image/png'
    assert response.read().startswith(b'\x89PNG\r\n\x1a\n')
  for params in [
    {'nickname': ' '},
    {'nickname': 'a' * 21},
    {'avatarBase64': base64.b64encode(b'<svg/>').decode()},
  ]:
    server.rpc('user.update', params, token, ok=False)
  assert server.rpc('user.me', token=token)['avatarUrl'] == result['avatarUrl']
  admin = server.admin()
  assert any(u['id'] == profile['id'] for u in server.rpc('admin.users', token=admin))
  assert server.rpc('admin.users', {'query': "' OR 1=1 --"}, admin) == []
