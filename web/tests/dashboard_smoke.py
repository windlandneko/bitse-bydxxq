"""Read-only browser acceptance against the running C++ service.

uv run --with playwright python web/tests/dashboard_smoke.py --url http://127.0.0.1:8080
Install browser once: uv run --with playwright python -m playwright install chromium
"""

import argparse
from pathlib import Path

from playwright.sync_api import sync_playwright

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--url', default='http://127.0.0.1:8080')
parser.add_argument('--output', type=Path, default=Path('/tmp/charging-dashboard-check'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
with sync_playwright() as playwright:
  browser = playwright.chromium.launch()
  page = browser.new_page(viewport={'width': 1920, 'height': 1080})
  errors = []
  page.on('pageerror', lambda error: errors.append(str(error)))
  page.goto(args.url)
  page.get_by_text('业务服务已连接').wait_for()
  page.wait_for_timeout(1500)
  for width, height in [(1920, 1080), (1366, 768)]:
    page.set_viewport_size({'width': width, 'height': height})
    page.wait_for_timeout(400)
    assert page.evaluate(
      'document.documentElement.scrollWidth === innerWidth && document.documentElement.scrollHeight === innerHeight'
    )
    assert page.locator('canvas').count() >= 7
    page.screenshot(path=str(args.output / f'dashboard-{width}.png'))
  stations = page.locator('.map-stations button')
  if stations.count() > 1:
    target = stations.nth(1).inner_text()
    stations.nth(1).click()
    assert page.locator('.station-detail b').inner_text() == target
  count = page.locator('.kpi-card').first.inner_text()
  page.route(
    '**/api/dashboard',
    lambda route: route.fulfill(status=503, body='test disconnect'),
  )
  page.get_by_role('button', name='刷新数据').click()
  page.get_by_text('连接中断 · 保留最近快照').wait_for()
  assert page.locator('.kpi-card').first.inner_text() == count
  assert not errors, errors
  browser.close()
print('通过：两尺寸无滚动、七图表、站点切换、断连保留快照、无 JS 异常')
