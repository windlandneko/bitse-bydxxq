"""Read-only browser acceptance against the running C++ service.

uv run --with playwright python web/tests/dashboard_smoke.py --url http://127.0.0.1:8080
Install browser once: uv run --with playwright python -m playwright install chromium
Extra data/empty-state scenarios are intercepted in this browser only, never written to the service.
"""

import argparse
import copy
import re
from pathlib import Path

from playwright.sync_api import sync_playwright

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--url', default='http://127.0.0.1:8080')
parser.add_argument('--output', type=Path, default=Path('/tmp/charging-dashboard-check'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)


def check_contained(child, parent):
  inner = child.bounding_box()
  outer = parent.bounding_box()
  for axis, dimension in [('x', 'width'), ('y', 'height')]:
    assert inner[axis] >= outer[axis] - 1
    assert inner[axis] + inner[dimension] <= outer[axis] + outer[dimension] + 1


def check_layout(page, width, height):
  assert page.evaluate(
    'document.documentElement.scrollWidth === innerWidth && document.documentElement.scrollHeight === innerHeight'
  )
  scale = min(width / 1920, height / 1080)
  for card in page.locator('.kpi-card').all():
    bounds = card.bounding_box()
    value = card.locator('.kpi-card__value > span').bounding_box()
    unit = card.locator('.kpi-card__value > small').bounding_box()
    assert abs(bounds['height'] - 88 * scale) < 1
    assert value['x'] + value['width'] <= unit['x'] + 1
    assert unit['x'] + unit['width'] <= bounds['x'] + bounds['width'] - 12 * scale
  assert page.locator('canvas').count() >= 7


def check_tooltips(page, data):
  for title, field, pattern in [
    ('近 30 日营收与订单', 'revenueTrend', r'¥ \d+\.\d{2}'),
    ('一周充电时段分布', 'hourlyHeatmap', r'\d+\.\d{2} kWh'),
    ('未来 24 小时负荷', 'forecast24h', r'\d+\.\d kW'),
  ]:
    if not data[field]:
      continue
    chart = page.locator('.chart-card').filter(has=page.get_by_role('heading', name=title))
    canvas = chart.locator('.chart')
    bounds = canvas.bounding_box()
    canvas.hover(position={'x': bounds['width'] * 0.52, 'y': bounds['height'] * 0.48})
    chart.get_by_text(re.compile(pattern), exact=True).wait_for()
    assert 'undefined' not in chart.inner_text()
    assert 'NaN' not in chart.inner_text()
  page.mouse.move(0, 0)


with sync_playwright() as playwright:
  browser = playwright.chromium.launch()
  page = browser.new_page(viewport={'width': 1920, 'height': 1080})
  errors = []
  page.on('pageerror', lambda error: errors.append(str(error)))
  with page.expect_response('**/api/dashboard') as first_response:
    page.goto(args.url)
  actual = first_response.value.json()
  page.get_by_text('业务服务已连接').wait_for()
  page.wait_for_timeout(1000)
  for width, height in [(1920, 1080), (1366, 768)]:
    page.set_viewport_size({'width': width, 'height': height})
    page.wait_for_timeout(400)
    check_layout(page, width, height)
    body = page.locator('body').inner_text()
    for noise in ['课程演示', '站点交互可点击', '随机森林', '五秒同步', '多端业务联动']:
      assert noise not in body, noise
    assert page.locator('.capacity-item').count() == len(actual['stations'])
    for station in actual['stations']:
      button = page.locator(f'.capacity-item button[data-station-id="{station["id"]}"]')
      description = button.get_attribute('aria-label')
      assert f'空闲 {station["idleChargers"]} 台' in description
      assert f'共 {station["totalChargers"]} 台' in description
    # Screenshots only record real backend data, before synthetic test scenarios.
    page.screenshot(path=str(args.output / f'dashboard-{width}.png'))
  check_tooltips(page, actual)
  if len(actual['stations']) > 1:
    target = actual['stations'][1]
    capacity = page.locator(f'.capacity-item button[data-station-id="{target["id"]}"]')
    capacity.focus()
    page.keyboard.press('Enter')
    assert page.locator('.station-detail b').inner_text() == target['name']
    assert capacity.get_attribute('aria-pressed') == 'true'
    page.get_by_role('button', name=actual['stations'][0]['name'], exact=True).click()
    assert (
      page.locator(
        f'.capacity-item button[data-station-id="{actual["stations"][0]["id"]}"]'
      ).get_attribute('aria-pressed')
      == 'true'
    )

  expanded = copy.deepcopy(actual)
  expanded['kpis']['totalRevenue'] = 12_345_678.90
  expanded['kpis']['totalChargingCount'] = 1_234_567
  prototype = (
    actual['stations'][0]
    if actual['stations']
    else {
      'latitude': 31.2,
      'longitude': 121.4,
      'region': '测试区',
      'priceCents': 120,
    }
  )
  for index in range(max(0, 7 - len(expanded['stations']))):
    expanded['stations'].append(
      {
        **prototype,
        'id': 1000 + index,
        'name': f'扩展测试电站{index}',
        'address': '浏览器测试夹具',
        'totalChargers': 0,
        'idleChargers': 0,
        'onlineRate': 0,
      }
    )
  expanded['stations'][0].update(totalChargers=12, idleChargers=3, onlineRate=75)
  expanded['stations'][-1].update(totalChargers=0, idleChargers=0, onlineRate=0)
  page.route('**/api/dashboard', lambda route: route.fulfill(json=expanded))
  page.get_by_role('button', name='刷新数据').click()
  page.wait_for_timeout(1000)
  assert '万元' in page.locator('.kpi-card').nth(1).inner_text()
  assert '12,345,678.90' in page.locator('.kpi-card').nth(1).get_attribute('title')
  assert page.locator('.capacity-item').count() == len(expanded['stations'])
  mixed = page.locator(f'.capacity-item button[data-station-id="{expanded["stations"][0]["id"]}"]')
  assert '在用或预约 6 台' in mixed.get_attribute('aria-label')
  assert '暂不可用 3 台' in mixed.get_attribute('aria-label')
  page.get_by_role('button', name='下一组电站').click()
  page.wait_for_timeout(500)
  last = expanded['stations'][-1]
  last_button = page.locator(f'.capacity-item button[data-station-id="{last["id"]}"]')
  last_button.focus()
  page.keyboard.press('Enter')
  assert page.locator('.station-detail b').inner_text() == last['name']
  bounds = last_button.bounding_box()
  viewport = page.locator('.capacity-list').bounding_box()
  assert bounds['x'] >= viewport['x'] - 1
  assert bounds['x'] + bounds['width'] <= viewport['x'] + viewport['width'] + 1
  for width, height in [(1920, 1080), (1366, 768)]:
    page.set_viewport_size({'width': width, 'height': height})
    page.wait_for_timeout(300)
    check_layout(page, width, height)
    header = page.locator('.capacity-header')
    previous_right = 0
    for selector in ['h2', '.capacity-legend', '.capacity-pager']:
      item = header.locator(selector)
      check_contained(item, header)
      bounds = item.bounding_box()
      assert bounds['x'] >= previous_right
      previous_right = bounds['x'] + bounds['width']
    # These images deliberately use browser-only fixtures; keep them separate from real data.
    page.screenshot(path=str(args.output / f'fixture-many-stations-{width}.png'))

  expired = copy.deepcopy(actual)
  expired['generatedAt'] = expired['dataCutoff'] = '2000-01-01T00:00:00Z'
  expired['forecastMeta']['generatedAt'] = '2000-01-01T00:00:00Z'
  page.unroute('**/api/dashboard')
  page.route('**/api/dashboard', lambda route: route.fulfill(json=expired))
  page.get_by_role('button', name='刷新数据').click()
  page.get_by_text('数据已过期', exact=True).wait_for()
  if expired['forecast24h']:
    page.get_by_text('预测已过期', exact=True).wait_for()

  empty = copy.deepcopy(actual)
  empty['stations'] = []
  empty['forecast24h'] = []
  page.unroute('**/api/dashboard')
  page.route('**/api/dashboard', lambda route: route.fulfill(json=empty))
  page.get_by_role('button', name='刷新数据').click()
  page.get_by_text('暂无电站', exact=True).wait_for()
  page.get_by_text('等待预测任务').wait_for()
  for width, height in [(1920, 1080), (1366, 768)]:
    page.set_viewport_size({'width': width, 'height': height})
    page.wait_for_timeout(300)
    check_layout(page, width, height)
    check_contained(page.locator('.capacity-empty'), page.locator('.capacity-card'))
  page.screenshot(path=str(args.output / 'fixture-empty-stations-1366.png'))

  restored = copy.deepcopy(actual)
  restored['stations'] = [
    {
      **prototype,
      'id': 2000 + index,
      'name': f'地图交互测试电站{index}',
      'longitude': 121 + index,
      'latitude': 31 + index,
      'totalChargers': 6,
      'idleChargers': 3,
      'onlineRate': 100,
    }
    for index in range(3)
  ]
  page.unroute('**/api/dashboard')
  page.route('**/api/dashboard', lambda route: route.fulfill(json=restored))
  page.get_by_role('button', name='刷新数据').click()
  page.locator('.capacity-item').nth(2).wait_for()
  page.wait_for_timeout(700)
  check_tooltips(page, restored)
  chart = page.locator('.map-chart')
  bounds = chart.bounding_box()
  # The middle fixture station is at the center of the plotted axes, above the bottom labels.
  chart.click(
    position={
      'x': bounds['width'] / 2,
      'y': bounds['height'] / 2 - 10 * min(1366 / 1920, 768 / 1080),
    }
  )
  assert page.locator('.station-detail b').inner_text() == restored['stations'][1]['name']
  assert (
    page.locator('.capacity-item button[data-station-id="2001"]').get_attribute('aria-pressed')
    == 'true'
  )
  check_layout(page, 1366, 768)

  page.unroute('**/api/dashboard')
  page.route('**/api/dashboard', lambda route: route.fulfill(status=503, body='test disconnect'))
  page.wait_for_timeout(1000)
  count = page.locator('.kpi-card').first.inner_text()
  page.get_by_role('button', name='刷新数据').click()
  page.get_by_text('连接中断 · 保留最近快照').wait_for()
  assert page.locator('.kpi-card').first.inner_text() == count
  assert not errors, errors
  browser.close()
print(
  '通过：真实数据、两尺寸布局、图表提示/空态恢复、地图点击/站点联动/键盘、多站点/大数值、过期与断连'
)
