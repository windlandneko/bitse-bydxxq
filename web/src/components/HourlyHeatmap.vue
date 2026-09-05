<script setup lang="ts">
import { nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import type { HeatmapItem } from '../types/dashboard'
import { init, type ChartInstance } from '../lib/echarts'

const props = defineProps<{ data: HeatmapItem[] }>()
const chartElement = ref<HTMLDivElement | null>(null)
let chart: ChartInstance | undefined
let resizeObserver: ResizeObserver | undefined
const weekdays = ['周一', '周二', '周三', '周四', '周五', '周六', '周日']

function render() {
  if (!chartElement.value) return
  chart ??= init(chartElement.value)
  const values = props.data.map((item) => [item.hour, item.weekday, item.energyKwh])
  const max = Math.max(...props.data.map((item) => item.energyKwh), 1)
  chart.setOption({
    grid: { left: 38, right: 12, top: 8, bottom: 28 },
    tooltip: { position: 'top', formatter: (params: any) => `${weekdays[params.value[1]]} ${params.value[0]}:00<br/><b>${Number(params.value[2]).toFixed(2)} kWh</b>` },
    xAxis: { type: 'category', data: Array.from({ length: 24 }, (_, index) => index), axisLabel: { color: '#71839b', fontSize: 9, formatter: (value: string) => `${value}h` }, axisLine: { show: false }, axisTick: { show: false } },
    yAxis: { type: 'category', data: weekdays, axisLabel: { color: '#a8b7ca', fontSize: 10 }, axisLine: { show: false }, axisTick: { show: false } },
    visualMap: { min: 0, max, calculable: false, orient: 'horizontal', left: 'center', bottom: 0, itemWidth: 70, itemHeight: 6, text: ['高', '低'], textStyle: { color: '#71839b', fontSize: 9 }, inRange: { color: ['#102c48', '#165b79', '#19b5bd', '#ffd166', '#ff6b6b'] } },
    series: [{ type: 'heatmap', data: values, label: { show: false }, itemStyle: { borderColor: '#0b1728', borderWidth: 2 } }],
  }, true)
}

onMounted(async () => { await nextTick(); render(); if (chartElement.value) { resizeObserver = new ResizeObserver(() => chart?.resize()); resizeObserver.observe(chartElement.value) } })
watch(() => props.data, render, { deep: true })
onBeforeUnmount(() => { resizeObserver?.disconnect(); chart?.dispose() })
</script>

<template><div ref="chartElement" class="chart chart--heatmap" /></template>
