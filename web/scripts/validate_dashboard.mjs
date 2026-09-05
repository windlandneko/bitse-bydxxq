const endpoint = process.argv[2] ?? 'http://127.0.0.1:8080/api/dashboard'
const response = await fetch(endpoint)
if (!response.ok) throw new Error(`统计接口返回 ${response.status}`)
const data = await response.json()
for (const key of ['generatedAt', 'dataCutoff', 'source']) {
  if (typeof data[key] !== 'string') throw new Error(`${key} 必须是字符串`)
}
for (const key of [
  'totalChargingCount',
  'totalRevenue',
  'onlineChargers',
  'registeredUsers',
  'totalEnergyKwh',
]) {
  if (typeof data.kpis?.[key] !== 'number' || !Number.isFinite(data.kpis[key]))
    throw new Error(`kpis.${key} 必须是有限数字`)
}
for (const key of [
  'chargerStatus',
  'stationRanking',
  'revenueTrend',
  'hourlyHeatmap',
  'chargerTypeRatio',
  'forecast24h',
  'stations',
  'recentEvents',
]) {
  if (!Array.isArray(data[key])) throw new Error(`${key} 必须是数组`)
}
if (data.revenueTrend.length > 30) throw new Error('revenueTrend 不应超过 30 个点')
if (![0, 24].includes(data.forecast24h.length)) throw new Error('forecast24h 必须为空或完整 24 点')
if (!data.forecastMeta || typeof data.forecastMeta.source !== 'string')
  throw new Error('缺少预测来源')
for (const station of data.stations) {
  if (station.idleChargers > station.totalChargers || station.idleChargers < 0)
    throw new Error('站点空闲数量越界')
}
console.log(`统计接口校验通过：${data.stations.length} 个电站，${data.forecast24h.length} 个预测点`)
