import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import type { DashboardData } from '../types/dashboard'
import { emptyDashboard } from '../types/dashboard'

const REFRESH_INTERVAL_MS = 5_000
function isDashboardData(value: unknown): value is DashboardData {
  if (!value || typeof value !== 'object') return false
  const data = value as Partial<DashboardData>
  return (
    typeof data.generatedAt === 'string' &&
    typeof data.dataCutoff === 'string' &&
    Number.isFinite(Date.parse(data.generatedAt)) &&
    Number.isFinite(Date.parse(data.dataCutoff)) &&
    typeof data.source === 'string' &&
    !!data.kpis &&
    Object.values(data.kpis).every((v) => typeof v === 'number' && Number.isFinite(v)) &&
    [
      'totalChargingCount',
      'totalRevenue',
      'onlineChargers',
      'registeredUsers',
      'totalEnergyKwh',
    ].every((k) => k in data.kpis!) &&
    [
      'chargerStatus',
      'stationRanking',
      'revenueTrend',
      'hourlyHeatmap',
      'chargerTypeRatio',
      'forecast24h',
      'stations',
      'recentEvents',
    ].every((k) => Array.isArray(data[k as keyof DashboardData])) &&
    !!data.forecastMeta &&
    typeof data.forecastMeta.generatedAt === 'string' &&
    typeof data.forecastMeta.modelVersion === 'string' &&
    typeof data.forecastMeta.source === 'string'
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
