<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import ChartCard from './components/ChartCard.vue'
import ChargerTypeDonut from './components/ChargerTypeDonut.vue'
import ForecastLine from './components/ForecastLine.vue'
import HourlyHeatmap from './components/HourlyHeatmap.vue'
import KpiCards from './components/KpiCards.vue'
import RevenueTrend from './components/RevenueTrend.vue'
import StationRanking from './components/StationRanking.vue'
import StatusDonut from './components/StatusDonut.vue'
import { useDashboardData } from './composables/useDashboardData'

const { data, loading, error, lastSuccessAt, refresh, refreshIntervalMs } = useDashboardData()
const clock = ref(new Date())
let clockTimer: number | undefined

onMounted(() => { clockTimer = window.setInterval(() => { clock.value = new Date() }, 1000) })
onBeforeUnmount(() => { if (clockTimer) window.clearInterval(clockTimer) })

const clockText = computed(() => clock.value.toLocaleString('zh-CN', { year: 'numeric', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false }))
const dataCutoffText = computed(() => data.value.dataCutoff ? formatDate(data.value.dataCutoff) : '暂无')
const lastSuccessText = computed(() => lastSuccessAt.value ? formatDate(lastSuccessAt.value) : '尚未成功读取')

function formatDate(value: string) {
  const date = new Date(value)
  return Number.isNaN(date.getTime()) ? value : date.toLocaleString('zh-CN', { month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false })
}
</script>

<template>
  <div class="dashboard-shell">
    <div class="ambient ambient--one" />
    <div class="ambient ambient--two" />
    <div class="dashboard-frame">
      <header class="topbar">
        <div class="brand-lockup">
          <div class="brand-mark"><span>⚡</span></div>
          <div>
            <div class="brand-kicker">NCS · ENERGY NETWORK</div>
            <h1>东软电动汽车充电桩应用管理平台</h1>
          </div>
        </div>
        <div class="topbar-meta">
          <div class="live-status"><span class="live-dot" />系统运行中</div>
          <div class="topbar-clock">{{ clockText }}</div>
        </div>
      </header>

      <div v-if="error" class="alert-bar" role="status">
        <span>数据源暂不可用：{{ error }}</span>
        <button type="button" @click="refresh">重新读取</button>
      </div>

      <main class="dashboard-grid" :class="{ 'dashboard-grid--loading': loading && !data.dataCutoff }">
        <section class="dashboard-column dashboard-column--left">
          <ChartCard title="电桩状态分布" eyebrow="DEVICE STATUS">
            <StatusDonut :data="data.chargerStatus" />
          </ChartCard>
          <ChartCard title="电站充电量排行" eyebrow="STATION ENERGY">
            <StationRanking :data="data.stationRanking" />
          </ChartCard>
        </section>

        <section class="dashboard-column dashboard-column--center">
          <KpiCards :data="data.kpis" />
          <ChartCard title="近 30 日营收趋势" eyebrow="REVENUE TREND" class-name="chart-card--center">
            <template #tools><span class="legend-chip"><i class="legend-chip__dot legend-chip__dot--cyan" />每日营收</span></template>
            <RevenueTrend :data="data.revenueTrend" />
          </ChartCard>
        </section>

        <section class="dashboard-column dashboard-column--right">
          <ChartCard title="充电时段热力分布" eyebrow="HOURLY LOAD">
            <HourlyHeatmap :data="data.hourlyHeatmap" />
          </ChartCard>
          <div class="split-charts">
            <ChartCard title="快慢充占比" eyebrow="CHARGER TYPE"><ChargerTypeDonut :data="data.chargerTypeRatio" /></ChartCard>
            <ChartCard title="未来 24 小时负荷" eyebrow="FORECAST"><ForecastLine :data="data.forecast24h" /></ChartCard>
          </div>
        </section>
      </main>

      <footer class="dashboard-footer">
        <span><b class="footer-pulse" />数据源：SQLite 聚合快照</span>
        <span>数据截止：{{ dataCutoffText }}</span>
        <span>最近读取：{{ lastSuccessText }}</span>
        <span>自动刷新：{{ refreshIntervalMs / 1000 }} 秒</span>
      </footer>
    </div>
  </div>
</template>
