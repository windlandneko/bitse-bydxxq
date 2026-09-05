<script setup lang="ts">
import { nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import type { RevenueTrendItem } from '../types/dashboard'
import { graphic, init, type ChartInstance } from '../lib/echarts'

const props = defineProps<{ data: RevenueTrendItem[] }>()
const chartElement = ref<HTMLDivElement | null>(null)
let chart: ChartInstance | undefined
let resizeObserver: ResizeObserver | undefined

function render() {
  if (!chartElement.value) return
  chart ??= init(chartElement.value)
  const dates = props.data.map((item) => item.date.slice(5))
  const values = props.data.map((item) => item.revenue)
  chart.setOption(
    {
      grid: { left: 48, right: 44, top: 22, bottom: 26 },
      legend: {
        top: 0,
        right: 0,
        textStyle: { color: '#7196b1', fontSize: 10 },
        data: ['每日营收', '订单数量'],
      },
      tooltip: {
        trigger: 'axis',
        formatter: (items: any[]) =>
          `${items[0]?.axisValue}<br/><b>¥ ${Number(props.data[items[0]?.dataIndex ?? 0]?.revenue ?? 0).toFixed(2)}</b><br/>${props.data[items[0]?.dataIndex ?? 0]?.orderCount ?? 0} 单`,
      },
      xAxis: {
        type: 'category',
        boundaryGap: false,
        data: dates,
        axisLabel: {
          color: '#71839b',
          fontSize: 10,
          interval: Math.max(0, Math.floor(dates.length / 6) - 1),
        },
        axisLine: { lineStyle: { color: 'rgba(122,153,190,.2)' } },
        axisTick: { show: false },
      },
      yAxis: [
        {
          type: 'value',
          axisLabel: { color: '#71839b', fontSize: 10, formatter: (value: number) => `¥${value}` },
          splitLine: { lineStyle: { color: 'rgba(122,153,190,.12)', type: 'dashed' } },
        },
        {
          type: 'value',
          minInterval: 1,
          axisLabel: { color: '#52718d', fontSize: 10, formatter: '{value} 单' },
          splitLine: { show: false },
        },
      ],
      series: [
        {
          name: '订单数量',
          type: 'bar',
          yAxisIndex: 1,
          data: props.data.map((item) => item.orderCount),
          itemStyle: { color: '#35729855' },
          barMaxWidth: 14,
        },
        {
          name: '每日营收',
          type: 'line',
          smooth: true,
          symbol: 'circle',
          symbolSize: 5,
          data: values,
          lineStyle: { width: 3, color: '#41d9ee' },
          itemStyle: { color: '#b5f8ff', borderColor: '#41d9ee', borderWidth: 2 },
          areaStyle: {
            color: new graphic.LinearGradient(0, 0, 0, 1, [
              { offset: 0, color: 'rgba(45,220,238,.34)' },
              { offset: 1, color: 'rgba(45,220,238,0)' },
            ]),
          },
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

<template><div ref="chartElement" class="chart chart--revenue" /></template>
