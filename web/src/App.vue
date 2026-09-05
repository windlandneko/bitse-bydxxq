<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import ChartCard from './components/ChartCard.vue'
import ChargerTypeDonut from './components/ChargerTypeDonut.vue'
import ForecastLine from './components/ForecastLine.vue'
import HourlyHeatmap from './components/HourlyHeatmap.vue'
import KpiCards from './components/KpiCards.vue'
import RevenueTrend from './components/RevenueTrend.vue'
import StationRanking from './components/StationRanking.vue'
import StationMap from './components/StationMap.vue'
import StationCapacity from './components/StationCapacity.vue'
import StatusDonut from './components/StatusDonut.vue'
import { useDashboardData } from './composables/useDashboardData'
const { data, loading, error, lastSuccessAt, stale, refresh } = useDashboardData()
const clock = ref(new Date())
const requestedStationId = ref<number>()
const selectedStationId = computed({
  get: () =>
    data.value.stations.find((station) => station.id === requestedStationId.value)?.id ??
    data.value.stations[0]?.id,
  set: (id: number | undefined) => {
    requestedStationId.value = id
  },
})
const operationalEvents = computed(() =>
  data.value.recentEvents
    .filter((event) => !['负荷预测更新', '演示环境初始化'].includes(event.action))
    .slice(0, 3),
)
const viewport = ref({ width: window.innerWidth, height: window.innerHeight })
const scale = computed(() => Math.min(viewport.value.width / 1920, viewport.value.height / 1080))
const fit = computed(() => ({ transform: `translate(-50%, -50%) scale(${scale.value})` }))
const connection = computed(() => {
  if (error.value) return lastSuccessAt.value ? '连接中断 · 保留最近快照' : '暂无有效数据'
  if (!lastSuccessAt.value) return '正在连接服务'
  return stale.value ? '数据已过期' : '业务服务已连接'
})
const forecastStale = computed(
  () =>
    !Number.isFinite(Date.parse(data.value.forecastMeta.generatedAt)) ||
    clock.value.getTime() - Date.parse(data.value.forecastMeta.generatedAt) > 3600_000,
)
const peakCount = computed(() => data.value.forecast24h.filter((hour) => hour.isPeak).length)
let timer: number | undefined
const resize = () => {
  viewport.value = { width: window.innerWidth, height: window.innerHeight }
}
onMounted(() => {
  timer = window.setInterval(() => {
    clock.value = new Date()
  }, 1000)
  window.addEventListener('resize', resize)
})
onBeforeUnmount(() => {
  if (timer) window.clearInterval(timer)
  window.removeEventListener('resize', resize)
})
function date(value: string) {
  return value && Number.isFinite(Date.parse(value))
    ? new Date(value).toLocaleString('zh-CN', { hour12: false })
    : '暂无'
}
async function fullscreen() {
  if (document.fullscreenElement) await document.exitFullscreen()
  else await document.documentElement.requestFullscreen().catch(() => {})
}
</script>
<template>
  <div class="viewport">
    <div class="dashboard-frame" :style="fit">
      <header class="topbar">
        <div class="header-side">
          <span class="brand-kicker">NEUSOFT / SMART ENERGY</span
          ><span class="connection" :class="{ warning: error || stale }"
            ><i />{{ connection }}</span
          >
        </div>
        <div class="title-block">
          <div class="title-decoration" />
          <h1>新能源汽车充电运营指挥中心</h1>
          <p>东软电动汽车充电桩应用管理平台</p>
        </div>
        <div class="header-side header-side--right">
          <time>{{ date(clock.toISOString()) }}</time>
          <div>
            <button @click="refresh" :disabled="loading">
              {{ loading ? '读取中' : '刷新数据' }}</button
            ><button @click="fullscreen">全屏展示</button>
          </div>
        </div>
      </header>
      <div v-if="error" class="dashboard-error warning" role="alert">{{ error }}</div>
      <div class="overview-row">
        <KpiCards :data="data.kpis" />
        <StationCapacity v-model="selectedStationId" :stations="data.stations" />
      </div>
      <main class="dashboard-grid">
        <section class="dashboard-column">
          <ChartCard title="电桩运行状态" eyebrow="01 / DEVICE STATUS"
            ><StatusDonut :data="data.chargerStatus"
          /></ChartCard>
          <ChartCard title="电站充电量排行" eyebrow="02 / STATION RANKING"
            ><StationRanking :data="data.stationRanking"
          /></ChartCard>
          <ChartCard title="充电类型结构" eyebrow="03 / CHARGER MIX"
            ><ChargerTypeDonut :data="data.chargerTypeRatio"
          /></ChartCard>
        </section>
        <section class="dashboard-column dashboard-column--center">
          <StationMap v-model="selectedStationId" :stations="data.stations" />
          <ChartCard title="近 30 日营收与订单" eyebrow="04 / REVENUE TREND"
            ><RevenueTrend :data="data.revenueTrend"
          /></ChartCard>
        </section>
        <section class="dashboard-column">
          <ChartCard title="一周充电时段分布" eyebrow="05 / HOURLY LOAD"
            ><HourlyHeatmap :data="data.hourlyHeatmap"
          /></ChartCard>
          <ChartCard title="未来 24 小时负荷" eyebrow="06 / DEMAND FORECAST">
            <template #tools
              ><span :class="{ warning: forecastStale }">{{
                !data.forecast24h.length
                  ? '等待预测任务'
                  : forecastStale
                    ? '预测已过期'
                    : `${peakCount} 个高峰时段`
              }}</span></template
            >
            <ForecastLine :data="data.forecast24h" />
          </ChartCard>
          <ChartCard title="最近业务动态" eyebrow="07 / ACTIVITY FEED">
            <ol class="event-list">
              <li v-for="(event, index) in operationalEvents" :key="`${event.createdAt}-${index}`">
                <i />
                <div>
                  <p :title="event.detail">
                    <b>{{ event.action }}</b> · {{ event.detail }}
                  </p>
                  <small>{{ date(event.createdAt) }}</small>
                </div>
              </li>
              <li v-if="!operationalEvents.length" class="empty-state">暂无业务事件</li>
            </ol>
          </ChartCard>
        </section>
      </main>
      <footer class="dashboard-footer">
        <span
          >{{ data.stations.length }} 座电站 · 累计电量
          <b>{{
            data.kpis.totalEnergyKwh.toLocaleString('zh-CN', { maximumFractionDigits: 1 })
          }}</b>
          kWh</span
        >
        <span>数据更新 {{ date(data.dataCutoff) }}</span>
        <span>预测更新 {{ date(data.forecastMeta.generatedAt) }}</span>
      </footer>
    </div>
  </div>
</template>
