import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import type { DashboardData } from '../types/dashboard'
import { emptyDashboard } from '../types/dashboard'

const REFRESH_INTERVAL_MS = 5_000
function hasFields(value: unknown, strings: string[], numbers: string[]) {
  if (!value || typeof value !== 'object') return false
  const row = value as Record<string, unknown>
  return (
    strings.every((key) => typeof row[key] === 'string') &&
    numbers.every((key) => typeof row[key] === 'number' && Number.isFinite(row[key]))
  )
}
function isDashboardData(value: unknown): value is DashboardData {
  if (!value || typeof value !== 'object') return false
  const data = value as Partial<DashboardData>
  const collections: [unknown, string[], string[]][] = [
    [data.chargerStatus, ['key', 'label'], ['value']],
    [data.stationRanking, ['stationName'], ['energyKwh']],
    [data.revenueTrend, ['date'], ['revenue', 'orderCount']],
    [data.hourlyHeatmap, [], ['weekday', 'hour', 'energyKwh']],
    [data.chargerTypeRatio, ['type', 'label'], ['count']],
    [data.forecast24h, ['time'], ['predictedLoadKw']],
    [
      data.stations,
      ['name', 'address', 'region'],
      ['id', 'latitude', 'longitude', 'totalChargers', 'idleChargers', 'onlineRate', 'priceCents'],
    ],
    [data.recentEvents, ['action', 'detail', 'createdAt'], []],
  ]
  return (
    hasFields(data, ['generatedAt', 'dataCutoff', 'source'], []) &&
    Number.isFinite(Date.parse(data.generatedAt!)) &&
    Number.isFinite(Date.parse(data.dataCutoff!)) &&
    hasFields(
      data.kpis,
      [],
      ['totalChargingCount', 'totalRevenue', 'onlineChargers', 'registeredUsers', 'totalEnergyKwh'],
    ) &&
    hasFields(data.forecastMeta, ['generatedAt', 'modelVersion', 'source'], []) &&
    collections.every(
      ([items, strings, numbers]) =>
        Array.isArray(items) && items.every((item) => hasFields(item, strings, numbers)),
    ) &&
    data.forecast24h!.every((item) => typeof item.isPeak === 'boolean')
  )
}
export function useDashboardData() {
  const data = ref<DashboardData>(structuredClone(emptyDashboard))
  const loading = ref(true)
  const error = ref('')
  const lastSuccessAt = ref('')
  const now = ref(Date.now())
  const stale = computed(
    () => !lastSuccessAt.value || now.value - Date.parse(data.value.generatedAt) > 30_000,
  )
  let timer: number | undefined
  let controller: AbortController | undefined
  async function refresh() {
    if (controller) return
    controller = new AbortController()
    const timeout = window.setTimeout(() => controller?.abort(), 8_000)
    loading.value = true
    try {
      const response = await fetch('/api/dashboard', {
        cache: 'no-store',
        signal: controller.signal,
      })
      if (!response.ok) throw new Error(`服务返回 ${response.status}`)
      const payload: unknown = await response.json()
      if (!isDashboardData(payload)) throw new Error('统计接口格式不完整')
      data.value = payload
      lastSuccessAt.value = new Date().toISOString()
      error.value = ''
    } catch (reason) {
      error.value =
        reason instanceof Error && reason.name !== 'AbortError' ? reason.message : '连接超时'
    } finally {
      window.clearTimeout(timeout)
      controller = undefined
      loading.value = false
      now.value = Date.now()
    }
  }
  onMounted(() => {
    void refresh()
    timer = window.setInterval(() => {
      now.value = Date.now()
      void refresh()
    }, REFRESH_INTERVAL_MS)
  })
  onBeforeUnmount(() => {
    if (timer) window.clearInterval(timer)
    controller?.abort()
  })
  return {
    data,
    loading,
    error,
    lastSuccessAt,
    stale,
    refresh,
  }
}
