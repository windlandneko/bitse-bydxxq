<script setup lang="ts">
import { useChart } from '../composables/useChart'
import type { ForecastItem } from '../types/dashboard'
import { graphic, firstTooltipItem } from '../lib/echarts'

const props = defineProps<{ data: ForecastItem[] }>()

const chartElement = useChart(() => {
  const labels = props.data.map((item) =>
    new Date(item.time).toLocaleTimeString('zh-CN', {
      hour: '2-digit',
      minute: '2-digit',
      hour12: false,
    }),
  )
  return {
    grid: { left: 45, right: 14, top: 16, bottom: 26 },
    tooltip: {
      trigger: 'axis',
      formatter: (params) => {
        const item = props.data[firstTooltipItem(params)?.dataIndex ?? 0]
        return `${item?.time ?? ''}<br/><b>${Number(item?.predictedLoadKw ?? 0).toFixed(1)} kW</b>${item?.isPeak ? '<br/><span style="color:#ff6b6b">高峰预警</span>' : ''}`
      },
    },
    xAxis: {
      type: 'category',
      boundaryGap: false,
      data: labels,
      axisLabel: {
        color: '#71839b',
        fontSize: 10,
        interval: Math.max(0, Math.floor(labels.length / 6) - 1),
      },
      axisLine: { lineStyle: { color: 'rgba(122,153,190,.2)' } },
      axisTick: { show: false },
    },
    yAxis: {
      type: 'value',
      axisLabel: { color: '#71839b', fontSize: 10, formatter: (value: number) => `${value}kW` },
      splitLine: { lineStyle: { color: 'rgba(122,153,190,.12)', type: 'dashed' } },
    },
    series: [
      {
        type: 'line',
        smooth: true,
        data: props.data.map((item) => ({
          value: item.predictedLoadKw,
          itemStyle: { color: item.isPeak ? '#ff6b6b' : '#a985ff' },
          symbol: item.isPeak ? 'diamond' : 'circle',
        })),
        lineStyle: { width: 2.5, color: '#a985ff' },
        areaStyle: {
          color: new graphic.LinearGradient(0, 0, 0, 1, [
            { offset: 0, color: 'rgba(169,133,255,.28)' },
            { offset: 1, color: 'rgba(169,133,255,0)' },
          ]),
        },
      },
    ],
  }
})
</script>

<template><div ref="chartElement" class="chart" /></template>
