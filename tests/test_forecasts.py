"""A successful child exit must never publish an old or incomplete prediction."""

import sys
import time

from conftest import Server


def test_prediction_publication_requires_current_complete_output(tmp_path, monkeypatch):
  launcher = tmp_path / 'model-launcher'
  mode = tmp_path / 'mode'
  launcher.write_text(
    f'#!{sys.executable}\n'
    'import json, sys\nfrom pathlib import Path\n'
    f'mode = Path({str(mode)!r}).read_text()\n'
    'if mode == "missing": sys.exit(0)\n'
    'source = json.loads(Path(sys.argv[sys.argv.index("--input") + 1]).read_text())\n'
    'result = {"generatedAt": source["generatedAt"], "stations": [], "source": "test"}\n'
    'if mode == "old": result["generatedAt"] = "2000-01-01T00:00:00Z"\n'
    'if mode == "partial": result["stations"] = [{"stationId": 1, "hours": []}]\n'
    'Path(sys.argv[sys.argv.index("--output") + 1]).write_text(json.dumps(result))\n'
  )
  launcher.chmod(0o700)
  monkeypatch.setenv('CHARGING_UV', str(launcher))
  server = Server(tmp_path / 'state', seed=False)
  try:
    token = server.admin()

    def run(selected):
      mode.write_text(selected)
      server.rpc('forecasts.run', token=token)
      deadline = time.monotonic() + 5
      while time.monotonic() < deadline:
        state = server.rpc('forecasts.status', token=token)
        if not state['running']:
          return state
        time.sleep(0.025)
      raise AssertionError('Prediction did not complete')

    assert run('valid')['lastError'] == ''
    saved = server.rpc('forecasts.list', token=token)
    assert saved['generatedAt']
    for invalid in ['missing', 'old', 'partial']:
      assert run(invalid)['lastError']
      assert server.rpc('forecasts.list', token=token) == saved
    # Persisted last-good result survives service restart after failed publication.
    server.close()
    server.start()
    token = server.admin()
    assert server.rpc('forecasts.list', token=token) == saved
  finally:
    server.close()
