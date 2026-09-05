<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import ChartCard from './components/ChartCard.vue'
import ChargerTypeDonut from './components/ChargerTypeDonut.vue'
import ForecastEval from './components/ForecastEval.vue'
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

const clockTime = computed(() => clock.value.toLocaleString('zh-CN', { hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false }))
const clockDate = computed(() => {
  const d = clock.value
  const week = ['日', '一', '二', '三', '四', '五', '六'][d.getDay()]
  return `${d.getFullYear()}年${String(d.getMonth() + 1).padStart(2, '0')}月${String(d.getDate()).padStart(2, '0')}日 星期${week}`
})
const dataCutoffText = computed(() => data.value.dataCutoff ? formatDate(data.value.dataCutoff) : '暂无')
const lastSuccessText = computed(() => lastSuccessAt.value ? formatDate(lastSuccessAt.value) : '尚未成功读取')

function formatDate(value: string) {
  const date = new Date(value)
  return Number.isNaN(date.getTime()) ? value : date.toLocaleString('zh-CN', { month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false })
}
</script>

<template>
  <div class="dashboard-shell">
    <div class="bg-grid" />
    <div class="scanline" />
    <div class="ambient ambient--one" />
    <div class="ambient ambient--two" />
    <div class="ambient ambient--three" />
    <div class="radar radar--left" />
    <div class="radar radar--right" />

    <div class="dashboard-frame">
      <header class="topbar">
        <div class="topbar__side topbar__side--left">
          <div class="dept-logo"><span>⚡</span></div>
          <div>
            <div class="dept-name">东软集团 · 智慧能源事业部</div>
            <div class="dept-sub">NCS SMART ENERGY COMMAND CENTER</div>
          </div>
        </div>

        <div class="topbar__title">
          <div class="title-wing title-wing--left"><i /><i /><i /></div>
          <div class="title-text">
            <h1>电动汽车充电桩运行监测指挥平台</h1>
            <div class="title-sub">EV CHARGING · OPERATION COMMAND &amp; VISUALIZATION</div>
          </div>
          <div class="title-wing title-wing--right"><i /><i /><i /></div>
        </div>

        <div class="topbar__side topbar__side--right">
          <div class="clock-time">{{ clockTime }}</div>
          <div class="clock-date">{{ clockDate }}</div>
          <div class="live-status"><span class="live-dot" />系统运行中</div>
        </div>
      </header>

      <div class="topbar-rule" />

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

      <section v-if="data.forecastEval" class="eval-panel">
        <ChartCard title="真实数据模型评估" eyebrow="ML EVALUATION">
          <template #tools>
            <span class="legend-chip"><i class="legend-chip__dot legend-chip__dot--cyan" />随机森林 vs 上周同时刻基线</span>
          </template>
          <ForecastEval :data="data.forecastEval" />
        </ChartCard>
      </section>

      <footer class="dashboard-footer">
        <span><b class="footer-pulse" />数据源：SQLite 聚合快照</span>
        <span>数据截止：{{ dataCutoffText }}</span>
        <span>最近读取：{{ lastSuccessText }}</span>
        <span>自动刷新：{{ refreshIntervalMs / 1000 }} 秒</span>
      </footer>
    </div>
  </div>
</template>
