from __future__ import annotations

import json
import os
import signal
import sqlite3
import sys
import textwrap
import time
import uuid
from pathlib import Path

import pytest
from conftest import Server


def wait_until(predicate, timeout=5):
  deadline = time.monotonic() + timeout
  while time.monotonic() < deadline:
    if predicate():
      return True
    time.sleep(0.025)
  return predicate()


def process_is_running(pid):
  """An orphaned zombie cannot write files or consume CPU; an alive child can."""
  try:
    state = (Path('/proc') / str(pid) / 'stat').read_text().rsplit(')', 1)[1].split()[0]
  except FileNotFoundError:
    return False
  return state not in {'Z', 'X'}


@pytest.mark.skipif(
  not Path('/proc/self/stat').exists(), reason='Linux process lifecycle regression'
)
@pytest.mark.parametrize('ignore_sigterm', [False, True], ids=['graceful-child', 'stubborn-child'])
def test_server_shutdown_stops_prediction_process_tree(tmp_path, monkeypatch, ignore_sigterm):
  """The launcher behaves like uv; no actual ML environment or slow model is used."""
  ready = tmp_path / 'prediction-processes.json'
  launcher = tmp_path / 'prediction-launcher'
  child_source = textwrap.dedent("""\
    import json, os, signal, sys, time
    if sys.argv[2] == '1':
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
    with open(sys.argv[1], 'w') as file:
        json.dump({'launcher': os.getppid(), 'child': os.getpid()}, file)
    time.sleep(60)
  """)
  launcher.write_text(
    f'#!{sys.executable}\n'
    'import os, signal, subprocess, sys\n'
    "ignore = os.environ['CHARGING_TEST_IGNORE_TERM']\n"
    "if ignore == '1':\n"
    '    signal.signal(signal.SIGTERM, signal.SIG_IGN)\n'
    f'child = subprocess.Popen([sys.executable, "-c", {child_source!r}, '
    'os.environ["CHARGING_TEST_READY_FILE"], ignore])\n'
    'child.wait()\n'
  )
  launcher.chmod(0o700)
  monkeypatch.setenv('CHARGING_UV', str(launcher))
  monkeypatch.setenv('CHARGING_TEST_READY_FILE', str(ready))
  monkeypatch.setenv('CHARGING_TEST_IGNORE_TERM', '1' if ignore_sigterm else '0')
  server = Server(tmp_path / 'state', seed=False)
  processes = {}
  try:
    admin = server.admin()
    assert server.rpc('forecasts.run', token=admin)['running']
    assert wait_until(lambda: ready.exists() and ready.stat().st_size > 0), 'Launcher did not start'
    processes = json.loads(ready.read_text())
    assert all(process_is_running(pid) for pid in processes.values())
    server.process.terminate()
    server.process.wait(timeout=7)
    assert server.process.returncode == 0
    assert wait_until(lambda: all(not process_is_running(pid) for pid in processes.values())), (
      'Prediction launcher or Python child survived normal server shutdown: '
      f'{processes}; stderr: {server.process.stderr.read()}'
    )
  finally:
    server.close()
    # Also clean up on the original regression, so a failed test never leaves a sleeper behind.
    if not processes and ready.exists():
      processes = json.loads(ready.read_text())
    for pid in processes.values():
      try:
        os.kill(pid, signal.SIGKILL)
      except ProcessLookupError:
        pass


def test_sigkill_recovers_order_and_settlement_once(server):
  token, profile = server.user(10000)
  order = server.rpc('orders.reserve', {'chargerId': 1, 'idempotencyKey': str(uuid.uuid4())}, token)
  params = {'orderId': order['id']}
  server.rpc('orders.start', params, token)
  server.process.kill()
  server.process.wait(timeout=5)
  time.sleep(0.15)
  server.start()
  token = server.rpc('user.login', {'phone': profile['phone']})['token']
  current = server.rpc('orders.active', token=token)
  assert current['id'] == order['id']
  assert current['status'] in {'charging', 'pending_payment'}
  assert current['durationSeconds'] > 0
  server.rpc('orders.stop', params, token)
  paid = server.rpc('orders.settle', params, token)
  assert paid['amountCents'] > 0
  assert paid['balanceCents'] == 10000 - paid['amountCents']
  assert server.rpc('orders.settle', params, token) == paid
  with sqlite3.connect(server.directory / 'platform.db') as connection:
    assert connection.execute('PRAGMA integrity_check').fetchone()[0] == 'ok'
    assert connection.execute('PRAGMA foreign_key_check').fetchall() == []
    assert (
      connection.execute(
        'SELECT COUNT(*) FROM wallet_transactions WHERE order_id=?',
        (order['id'],),
      ).fetchone()[0]
      == 1
    )
