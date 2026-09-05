export type ChargerStatusKey =
  | 'idle'
  | 'reserved'
  | 'charging'
  | 'fault'
  | 'offline'
  | 'maintenance'
  | 'restarting'

export interface KpiData {
  totalChargingCount: number
  totalRevenue: number
  onlineChargers: number
  registeredUsers: number
  totalEnergyKwh: number
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

export interface Station {
  id: number
  code: string
  name: string
  address: string
  region: string
  latitude: number
  longitude: number
  totalChargers: number
  idleChargers: number
  onlineRate: number
  priceCents: number
}

export interface DashboardData {
  source: string
  stations: Station[]
  recentEvents: { action: string; detail: string; createdAt: string }[]
  forecastMeta: { generatedAt: string; modelVersion: string; source: string }
  generatedAt: string
  dataCutoff: string
  kpis: KpiData
  chargerStatus: ChargerStatusItem[]
  stationRanking: StationRankingItem[]
  revenueTrend: RevenueTrendItem[]
  hourlyHeatmap: HeatmapItem[]
  chargerTypeRatio: ChargerTypeRatioItem[]
  forecast24h: ForecastItem[]
}

export const emptyDashboard: DashboardData = {
  source: '',
  stations: [],
  recentEvents: [],
  forecastMeta: { generatedAt: '', modelVersion: '', source: '' },
  generatedAt: '',
  dataCutoff: '',
  kpis: {
    totalChargingCount: 0,
    totalRevenue: 0,
    onlineChargers: 0,
    registeredUsers: 0,
    totalEnergyKwh: 0,
  },
  chargerStatus: [],
  stationRanking: [],
  revenueTrend: [],
  hourlyHeatmap: [],
  chargerTypeRatio: [],
  forecast24h: [],
}
