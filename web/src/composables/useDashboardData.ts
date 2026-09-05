import { onBeforeUnmount, onMounted, ref } from 'vue'
import type { DashboardData } from '../types/dashboard'
import { emptyDashboard } from '../types/dashboard'

const REFRESH_INTERVAL_MS = 30_000

function isFiniteNumber(value: unknown): value is number {
  return typeof value === 'number' && Number.isFinite(value)
}

function isDashboardData(value: unknown): value is DashboardData {
  if (!value || typeof value !== 'object') return false
  const data = value as Partial<DashboardData>
  return Boolean(
    typeof data.generatedAt === 'string' &&
      typeof data.dataCutoff === 'string' &&
      data.kpis &&
      isFiniteNumber(data.kpis.totalChargingCount) &&
      isFiniteNumber(data.kpis.totalRevenue) &&
      isFiniteNumber(data.kpis.onlineChargers) &&
      isFiniteNumber(data.kpis.registeredUsers) &&
      Array.isArray(data.chargerStatus) &&
      Array.isArray(data.stationRanking) &&
      Array.isArray(data.revenueTrend) &&
      Array.isArray(data.hourlyHeatmap) &&
      Array.isArray(data.chargerTypeRatio) &&
      Array.isArray(data.forecast24h),
  )
}

export function useDashboardData() {
  const data = ref<DashboardData>({ ...emptyDashboard })
  const loading = ref(true)
  const error = ref('')
  const lastSuccessAt = ref('')
  let timer: number | undefined
  let requestController: AbortController | undefined

  async function refresh() {
    requestController?.abort()
    requestController = new AbortController()
    loading.value = true

    try {
      const response = await fetch(`/data/dashboard.json?t=${Date.now()}`, {
        cache: 'no-store',
        signal: requestController.signal,
      })
      if (!response.ok) {
        throw new Error(`数据文件读取失败（${response.status}）`)
      }
      const payload: unknown = await response.json()
      if (!isDashboardData(payload)) {
        throw new Error('数据文件格式不完整')
      }
      data.value = payload
      lastSuccessAt.value = new Date().toISOString()
      error.value = ''
    } catch (reason) {
      if (reason instanceof DOMException && reason.name === 'AbortError') return
      error.value = reason instanceof Error ? reason.message : '数据读取失败'
    } finally {
      loading.value = false
    }
  }

  onMounted(() => {
    void refresh()
    timer = window.setInterval(() => void refresh(), REFRESH_INTERVAL_MS)
  })

  onBeforeUnmount(() => {
    if (timer) window.clearInterval(timer)
    requestController?.abort()
  })

  return {
    data,
    loading,
    error,
    lastSuccessAt,
    refresh,
    refreshIntervalMs: REFRESH_INTERVAL_MS,
  }
}
