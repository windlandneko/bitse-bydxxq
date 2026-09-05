#!/usr/bin/env python3
"""Export a read-only dashboard snapshot from the running C++ service."""

import argparse
import urllib.request
from pathlib import Path


def main():
  parser = argparse.ArgumentParser(description='导出在线大屏数据快照')
  parser.add_argument('--url', default='http://127.0.0.1:8080')
  parser.add_argument('--out', type=Path, default=Path('data/dashboard-snapshot.json'))
  args = parser.parse_args()
  with urllib.request.urlopen(args.url.rstrip('/') + '/api/dashboard', timeout=15) as response:
    payload = response.read()
  args.out.parent.mkdir(parents=True, exist_ok=True)
  args.out.write_bytes(payload)
  print(f'已导出到 {args.out}')


if __name__ == '__main__':
  main()
