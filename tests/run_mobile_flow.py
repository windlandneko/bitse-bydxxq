"""Run the QML user journey against an isolated, disposable Qt service."""

import argparse
import os
import selectors
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--server', required=True, type=Path)
  parser.add_argument('--client', required=True, type=Path)
  parser.add_argument('--artifacts', type=Path)
  args = parser.parse_args()
  environment = os.environ.copy()
  environment.update(
    {
      'QT_QPA_PLATFORM': 'offscreen',
      'QT_SCALE_FACTOR': '1',
      'CHARGING_DISABLE_AUTO_FORECAST': '1',
    }
  )
  if args.artifacts:
    environment['CHARGING_UI_ARTIFACT_DIR'] = str(args.artifacts.resolve())
  with tempfile.TemporaryDirectory(prefix='charging-mobile-test-') as temporary:
    directory = Path(temporary)
    with (directory / 'server.log').open('w+', encoding='utf-8') as log:
      server = subprocess.Popen(
        [
          str(args.server.resolve()),
          '--data-dir',
          str(directory / 'data'),
          '--port',
          '0',
          '--time-scale',
          '600',
        ],
        stdout=subprocess.PIPE,
        stderr=log,
        text=True,
        env=environment,
      )
      try:
        with selectors.DefaultSelector() as selector:
          selector.register(server.stdout, selectors.EVENT_READ)
          deadline = time.monotonic() + 20
          url = None
          while time.monotonic() < deadline and server.poll() is None:
            if selector.select(timeout=0.2):
              line = server.stdout.readline().strip()
              if line.startswith('LISTENING '):
                url = line.removeprefix('LISTENING ')
                break
          if url is None:
            log.seek(0)
            print(log.read(), file=sys.stderr)
            raise RuntimeError('The disposable charging service did not start')
        environment['CHARGING_SERVER_URL'] = url
        result = subprocess.run(
          [str(args.client.resolve()), 'completeJourney', '-v1'],
          env=environment,
          timeout=60,
          check=False,
        )
        if result.returncode:
          log.seek(0)
          print(log.read(), file=sys.stderr)
        return result.returncode
      finally:
        server.terminate()
        try:
          server.wait(timeout=10)
        except subprocess.TimeoutExpired:
          server.kill()
          server.wait(timeout=5)
        if server.stdout:
          server.stdout.close()


if __name__ == '__main__':
  sys.exit(main())
