<script setup lang="ts">
import { nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import type { ForecastItem } from '../types/dashboard'
import { graphic, init, type ChartInstance } from '../lib/echarts'

const props = defineProps<{ data: ForecastItem[] }>()
const chartElement = ref<HTMLDivElement | null>(null)
let chart: ChartInstance | undefined
let resizeObserver: ResizeObserver | undefined

function render() {
  if (!chartElement.value) return
  chart ??= init(chartElement.value)
  const labels = props.data.map((item) => item.time.slice(11, 16))
  chart.setOption({
    grid: { left: 45, right: 14, top: 16, bottom: 26 },
    tooltip: { trigger: 'axis', formatter: (items: any[]) => { const item = props.data[items[0]?.dataIndex ?? 0]; return `${item?.time ?? ''}<br/><b>${Number(item?.predictedLoadKw ?? 0).toFixed(1)} kW</b>${item?.isPeak ? '<br/><span style="color:#ff6b6b">高峰预警</span>' : ''}` } },
    xAxis: { type: 'category', boundaryGap: false, data: labels, axisLabel: { color: '#71839b', fontSize: 10, interval: Math.max(0, Math.floor(labels.length / 6) - 1) }, axisLine: { lineStyle: { color: 'rgba(122,153,190,.2)' } }, axisTick: { show: false } },
    yAxis: { type: 'value', axisLabel: { color: '#71839b', fontSize: 10, formatter: (value: number) => `${value}kW` }, splitLine: { lineStyle: { color: 'rgba(122,153,190,.12)', type: 'dashed' } } },
    series: [{ type: 'line', smooth: true, data: props.data.map((item) => ({ value: item.predictedLoadKw, itemStyle: { color: item.isPeak ? '#ff6b6b' : '#a985ff' }, symbol: item.isPeak ? 'diamond' : 'circle' })), lineStyle: { width: 2.5, color: '#a985ff', shadowBlur: 12, shadowColor: 'rgba(169,133,255,.6)' }, areaStyle: { color: new graphic.LinearGradient(0, 0, 0, 1, [{ offset: 0, color: 'rgba(169,133,255,.28)' }, { offset: 1, color: 'rgba(169,133,255,0)' }]) } }],
  }, true)
}

onMounted(async () => { await nextTick(); render(); if (chartElement.value) { resizeObserver = new ResizeObserver(() => chart?.resize()); resizeObserver.observe(chartElement.value) } })
watch(() => props.data, render, { deep: true })
onBeforeUnmount(() => { resizeObserver?.disconnect(); chart?.dispose() })
</script>

<template><div ref="chartElement" class="chart chart--forecast" /></template>
