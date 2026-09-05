from __future__ import annotations

import json
import os
import select
import subprocess
import time
import urllib.error
import urllib.request
import uuid
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]


class Server:
  def __init__(self, directory, seed=True):
    self.directory = directory
    self.seed = seed
    self.process = None
    self.url = ''
    self.start()

  def start(self):
    executable = Path(
      os.environ.get(
        'CHARGING_SERVER_BIN',
        ROOT / 'build/full/apps/server/charging-server',
      )
    )
    if not executable.exists():
      pytest.fail(f'Build charging-server first: {executable}')
    args = [
      str(executable),
      '--data-dir',
      str(self.directory),
      '--port',
      '0',
      '--time-scale',
      '600',
    ]
    if not self.seed:
      args.append('--no-seed')
    self.process = subprocess.Popen(
      args,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True,
      env={**os.environ, 'CHARGING_DISABLE_AUTO_FORECAST': '1'},
    )
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
      if self.process.poll() is not None:
        pytest.fail(self.process.stderr.read())
      if select.select([self.process.stdout], [], [], 0.2)[0]:
        line = self.process.stdout.readline()
        if line.startswith('LISTENING '):
          self.url = line.strip().split(' ', 1)[1]
          return
    self.close()
    pytest.fail('Service did not become ready')

  def close(self):
    if self.process and self.process.poll() is None:
      self.process.terminate()
      try:
        self.process.wait(timeout=10)
      except subprocess.TimeoutExpired:
        self.process.kill()
        self.process.wait()

  def rpc(self, action, params=None, token='', ok=True):
    request = urllib.request.Request(
      self.url + '/api/rpc',
      json.dumps({'action': action, 'params': params or {}}).encode(),
      {'Content-Type': 'application/json', 'Authorization': 'Bearer ' + token},
    )
    try:
      response = urllib.request.urlopen(request, timeout=15)
    except urllib.error.HTTPError as exc:
      response = exc
    with response:
      result = json.load(response)
    assert result['ok'] is ok, result
    return result.get('data') if ok else result['error']

  def user(self, cents=10000):
    phone = '139' + str(uuid.uuid4().int % 100000000).zfill(8)
    result = self.rpc('user.login', {'phone': phone})
    token = result['token']
    if cents:
      self.rpc(
        'wallet.recharge',
        {'amountCents': cents, 'idempotencyKey': str(uuid.uuid4())},
        token,
      )
    return token, result['user']

  def admin(self):
    return self.rpc('admin.login', {'username': 'admin', 'password': '123456'})['token']


@pytest.fixture
def server(tmp_path):
  instance = Server(tmp_path / 'state')
  yield instance
  instance.close()
