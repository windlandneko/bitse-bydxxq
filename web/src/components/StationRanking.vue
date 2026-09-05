<script setup lang="ts">
import { nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import type { StationRankingItem } from '../types/dashboard'
import { graphic, init, type ChartInstance } from '../lib/echarts'

const props = defineProps<{ data: StationRankingItem[] }>()
const chartElement = ref<HTMLDivElement | null>(null)
let chart: ChartInstance | undefined
let resizeObserver: ResizeObserver | undefined

function render() {
  if (!chartElement.value) return
  chart ??= init(chartElement.value)
  const rows = [...props.data].sort((a, b) => b.energyKwh - a.energyKwh).slice(0, 6)
  chart.setOption(
    {
      grid: { left: 82, right: 24, top: 10, bottom: 20 },
      tooltip: {
        trigger: 'axis',
        renderMode: 'richText',
        axisPointer: { type: 'shadow' },
        formatter: (items: any[]) =>
          `${items[0]?.name}\n${Number(items[0]?.value ?? 0).toFixed(1)} kWh`,
      },
      xAxis: {
        type: 'value',
        axisLabel: { color: '#6f819a', fontSize: 10 },
        splitLine: { lineStyle: { color: 'rgba(122,153,190,.12)' } },
      },
      yAxis: {
        type: 'category',
        inverse: true,
        data: rows.map((row) => row.stationName),
        axisLabel: { color: '#b8c5d7', fontSize: 11, width: 72, overflow: 'truncate' },
        axisLine: { show: false },
        axisTick: { show: false },
      },
      series: [
        {
          type: 'bar',
          barWidth: 12,
          data: rows.map((row) => ({
            value: row.energyKwh,
            itemStyle: {
              color: new graphic.LinearGradient(1, 0, 0, 0, [
                { offset: 0, color: '#20d7ed' },
                { offset: 1, color: '#1f6eff' },
              ]),
            },
          })),
          label: {
            show: true,
            position: 'right',
            color: '#c8f6ff',
            fontSize: 10,
            formatter: ({ value }: { value: number }) => `${Number(value).toFixed(1)}`,
          },
          itemStyle: { borderRadius: [0, 6, 6, 0] },
        },
      ],
    },
    true,
  )
}

onMounted(async () => {
  await nextTick()
  render()
  if (chartElement.value) {
    resizeObserver = new ResizeObserver(() => chart?.resize())
    resizeObserver.observe(chartElement.value)
  }
})
watch(() => props.data, render, { deep: true })
onBeforeUnmount(() => {
  resizeObserver?.disconnect()
  chart?.dispose()
})
</script>

<template><div ref="chartElement" class="chart chart--ranking" /></template>
