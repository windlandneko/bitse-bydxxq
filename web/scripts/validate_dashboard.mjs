import { readFile } from 'node:fs/promises'

const dataPath = new URL('../public/data/dashboard.json', import.meta.url)
const requiredArrays = [
  'chargerStatus',
  'stationRanking',
  'revenueTrend',
  'hourlyHeatmap',
  'chargerTypeRatio',
  'forecast24h',
]

const data = JSON.parse(await readFile(dataPath, 'utf8'))
if (typeof data.generatedAt !== 'string' || typeof data.dataCutoff !== 'string') {
  throw new Error('generatedAt 和 dataCutoff 必须是字符串')
}

const kpiKeys = ['totalChargingCount', 'totalRevenue', 'onlineChargers', 'registeredUsers']
for (const key of kpiKeys) {
  if (typeof data.kpis?.[key] !== 'number' || !Number.isFinite(data.kpis[key])) {
    throw new Error(`kpis.${key} 必须是有限数字`)
  }
}

for (const key of requiredArrays) {
  if (!Array.isArray(data[key])) throw new Error(`${key} 必须是数组`)
}

if (data.revenueTrend.length > 30) throw new Error('revenueTrend 不应超过 30 个点')
if (data.forecast24h.length > 24) throw new Error('forecast24h 不应超过 24 个点')
console.log(`dashboard.json 校验通过：${data.revenueTrend.length} 个营收点，${data.forecast24h.length} 个预测点`)
