<script setup lang="ts">
import { useChart } from '../composables/useChart'
import type { ChargerStatusItem, ChargerStatusKey } from '../types/dashboard'

const props = defineProps<{ data: ChargerStatusItem[] }>()

const colors: Record<ChargerStatusKey, string> = {
  idle: '#39e58c',
  reserved: '#43b8ff',
  charging: '#ffb84d',
  fault: '#ff5f72',
  offline: '#69758c',
  restarting: '#9d7cff',
}

const chartElement = useChart(() => {
  return {
    tooltip: { trigger: 'item', formatter: '{b}<br/><b>{c}</b> 台（{d}%）' },
    legend: {
      bottom: 0,
      left: 'center',
      icon: 'circle',
      itemWidth: 8,
      itemHeight: 8,
      itemGap: 12,
      textStyle: { color: '#9aa9bd', fontSize: 11 },
    },
    series: [
      {
        type: 'pie',
        radius: ['54%', '76%'],
        center: ['50%', '43%'],
        avoidLabelOverlap: true,
        itemStyle: { borderColor: '#0b1728', borderWidth: 3 },
        label: { show: false },
        data: props.data.map((item) => ({
          name: item.label,
          value: item.value,
          itemStyle: { color: colors[item.key] },
        })),
      },
    ],
    graphic: props.data.length
      ? [
          {
            type: 'text',
            left: 'center',
            top: '36%',
            style: {
              text: String(props.data.reduce((sum, item) => sum + item.value, 0)),
              fill: '#f2f7ff',
              fontSize: 28,
              fontWeight: 700,
            },
          },
          {
            type: 'text',
            left: 'center',
            top: '51%',
            style: { text: '总电桩', fill: '#8090a7', fontSize: 11 },
          },
        ]
      : [
          {
            type: 'text',
            left: 'center',
            top: '45%',
            style: { text: '暂无数据', fill: '#8090a7', fontSize: 13 },
          },
        ],
  }
})
</script>

<template><div ref="chartElement" class="chart" /></template>
