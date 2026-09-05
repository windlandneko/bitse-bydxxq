<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import type { ForecastEval } from '../types/dashboard'
import { init, type ChartInstance } from '../lib/echarts'

const props = defineProps<{ data: ForecastEval }>()
const chartElement = ref<HTMLDivElement | null>(null)
let chart: ChartInstance | undefined
let resizeObserver: ResizeObserver | undefined

const improvement = computed(() => {
  const m = props.data?.metrics?.wape
  const b = props.data?.baseline?.wape
  if (!m || !b) return 0
  return ((b - m) / b) * 100
})

function render() {
  if (!chartElement.value) return
  const series = props.data?.series ?? []
  if (!series.length) return
  chart ??= init(chartElement.value)
  const labels = series.map((point) => point.time.slice(5, 10))
  chart.setOption({
    grid: { left: 48, right: 18, top: 30, bottom: 28 },
    legend: { top: 0, textStyle: { color: '#a8b7ca', fontSize: 10 }, itemWidth: 14, itemHeight: 8 },
    tooltip: {
      trigger: 'axis',
      formatter: (items: any[]) => {
        const point = series[items[0]?.dataIndex ?? 0]
        if (!point) return ''
        const rows = items
          .map((item) => `${item.marker}${item.seriesName}：<b>${Number(item.value).toFixed(1)} kW</b>`)
          .join('<br/>')
        return `${point.time}<br/>${rows}`
      },
    },
    xAxis: { type: 'category', boundaryGap: false, data: labels, axisLabel: { color: '#71839b', fontSize: 10, interval: Math.max(0, Math.floor(labels.length / 8) - 1) }, axisLine: { lineStyle: { color: 'rgba(122,153,190,.2)' } }, axisTick: { show: false } },
    yAxis: { type: 'value', axisLabel: { color: '#71839b', fontSize: 10, formatter: (value: number) => `${value}kW` }, splitLine: { lineStyle: { color: 'rgba(122,153,190,.12)', type: 'dashed' } } },
    series: [
      { name: '实际负荷', type: 'line', smooth: true, symbol: 'none', data: series.map((point) => point.actual), lineStyle: { width: 1.6, color: '#41d9ee', shadowBlur: 10, shadowColor: 'rgba(65,217,238,.5)' }, itemStyle: { color: '#41d9ee' } },
      { name: '模型预测', type: 'line', smooth: true, symbol: 'none', data: series.map((point) => point.pred), lineStyle: { width: 1.4, color: '#ff6b6b', shadowBlur: 10, shadowColor: 'rgba(255,107,107,.5)' }, itemStyle: { color: '#ff6b6b' } },
      { name: '基线', type: 'line', smooth: true, symbol: 'none', data: series.map((point) => point.baseline), lineStyle: { width: 1.1, color: '#7f8ba0', type: 'dashed' }, itemStyle: { color: '#7f8ba0' } },
    ],
  }, true)
}

onMounted(async () => { await nextTick(); render(); if (chartElement.value) { resizeObserver = new ResizeObserver(() => chart?.resize()); resizeObserver.observe(chartElement.value) } })
watch(() => props.data, render, { deep: true })
onBeforeUnmount(() => { resizeObserver?.disconnect(); chart?.dispose() })
</script>

<template>
  <div class="eval-wrap">
    <div class="eval-metrics">
      <div class="eval-metric"><span class="eval-metric__label">MAE</span><b>{{ data.metrics.mae.toFixed(1) }}</b><i>vs {{ data.baseline.mae.toFixed(1) }} kW</i></div>
      <div class="eval-metric"><span class="eval-metric__label">RMSE</span><b>{{ data.metrics.rmse.toFixed(1) }}</b><i>vs {{ data.baseline.rmse.toFixed(1) }} kW</i></div>
      <div class="eval-metric"><span class="eval-metric__label">WAPE</span><b>{{ data.metrics.wape.toFixed(1) }}%</b><i>vs {{ data.baseline.wape.toFixed(1) }}%</i></div>
      <div class="eval-metric eval-metric--gain"><span class="eval-metric__label">WAPE 改善</span><b>{{ improvement.toFixed(1) }}%</b></div>
    </div>
    <div class="eval-caption">{{ data.dataset }} · 测试期 {{ data.testRange }} · 平台总负荷</div>
    <div ref="chartElement" class="chart chart--eval" />
  </div>
</template>
