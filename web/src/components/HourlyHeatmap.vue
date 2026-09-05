<script setup lang="ts">
import { useChart } from '../composables/useChart'
import type { HeatmapItem } from '../types/dashboard'
import { firstTooltipItem } from '../lib/echarts'

const props = defineProps<{ data: HeatmapItem[] }>()
const weekdays = ['周一', '周二', '周三', '周四', '周五', '周六', '周日']

const chartElement = useChart(() => {
  const indexed = new Map(
    props.data.map((item) => [`${item.weekday}-${item.hour}`, item.energyKwh]),
  )
  const values = props.data.length
    ? Array.from({ length: 168 }, (_, index) => {
        const day = Math.floor(index / 24)
        const hour = index % 24
        return [hour, day, indexed.get(`${day}-${hour}`) ?? 0]
      })
    : []
  const max = Math.max(...props.data.map((item) => item.energyKwh), 1)
  return {
    grid: { left: 38, right: 12, top: 8, bottom: 42 },
    tooltip: {
      position: 'top',
      formatter: (params) => {
        const value = values[firstTooltipItem(params)?.dataIndex ?? 0]
        return value
          ? `${weekdays[value[1]]} ${value[0]}:00<br/><b>${value[2].toFixed(2)} kWh</b>`
          : ''
      },
    },
    xAxis: {
      type: 'category',
      data: Array.from({ length: 24 }, (_, index) => index),
      axisLabel: { color: '#71839b', fontSize: 9, formatter: (value: string) => `${value}h` },
      axisLine: { show: false },
      axisTick: { show: false },
    },
    yAxis: {
      type: 'category',
      data: weekdays,
      axisLabel: { color: '#a8b7ca', fontSize: 10 },
      axisLine: { show: false },
      axisTick: { show: false },
    },
    visualMap: {
      min: 0,
      max,
      calculable: false,
      orient: 'horizontal',
      left: 'center',
      bottom: 0,
      itemWidth: 6,
      itemHeight: 70,
      text: ['高', '低'],
      textStyle: { color: '#71839b', fontSize: 9 },
      inRange: { color: ['#102c48', '#165b79', '#19b5bd', '#ffd166', '#ff6b6b'] },
    },
    series: [
      {
        type: 'heatmap',
        data: values,
        label: { show: false },
        itemStyle: { borderColor: '#0b1728', borderWidth: 2 },
      },
    ],
  }
})
</script>

<template><div ref="chartElement" class="chart" /></template>
