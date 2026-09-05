export type ChargerStatusKey =
  | 'idle'
  | 'reserved'
  | 'charging'
  | 'fault'
  | 'offline'
  | 'maintenance'

export interface KpiData {
  totalChargingCount: number
  totalRevenue: number
  onlineChargers: number
  registeredUsers: number
}

export interface ChargerStatusItem {
  key: ChargerStatusKey
  label: string
  value: number
}

export interface StationRankingItem {
  stationId: number
  stationCode: string
  stationName: string
  energyKwh: number
  revenue: number
}

export interface RevenueTrendItem {
  date: string
  revenue: number
  orderCount: number
}

export interface HeatmapItem {
  weekday: number
  hour: number
  energyKwh: number
}

export interface ChargerTypeRatioItem {
  type: 'ac' | 'dc'
  label: string
  count: number
  energyKwh: number
}

export interface ForecastItem {
  time: string
  predictedLoadKw: number
  actualLoadKw: number | null
  availableChargers: number
  isPeak: boolean
}

export interface ForecastEvalPoint {
  time: string
  actual: number
  pred: number
  baseline: number
}

export interface ForecastEvalMetrics {
  mae: number
  rmse: number
  wape: number
  medianApe: number
}

export interface ForecastEval {
  dataset: string
  generatedAt: string
  testRange: string
  metrics: ForecastEvalMetrics
  baseline: ForecastEvalMetrics
  series: ForecastEvalPoint[]
}

export interface DashboardData {
  generatedAt: string
  dataCutoff: string
  kpis: KpiData
  chargerStatus: ChargerStatusItem[]
  stationRanking: StationRankingItem[]
  revenueTrend: RevenueTrendItem[]
  hourlyHeatmap: HeatmapItem[]
  chargerTypeRatio: ChargerTypeRatioItem[]
  forecast24h: ForecastItem[]
  forecastEval?: ForecastEval
}

export const emptyDashboard: DashboardData = {
  generatedAt: '',
  dataCutoff: '',
  kpis: {
    totalChargingCount: 0,
    totalRevenue: 0,
    onlineChargers: 0,
    registeredUsers: 0,
  },
  chargerStatus: [],
  stationRanking: [],
  revenueTrend: [],
  hourlyHeatmap: [],
  chargerTypeRatio: [],
  forecast24h: [],
}
