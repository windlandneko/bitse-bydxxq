<script setup lang="ts">
import { useChart } from '../composables/useChart'
import type { ChargerTypeRatioItem } from '../types/dashboard'

const props = defineProps<{ data: ChargerTypeRatioItem[] }>()

const chartElement = useChart(() => {
  return {
    tooltip: { trigger: 'item', formatter: '{b}<br/><b>{c}</b> 台（{d}%）' },
    legend: {
      bottom: 0,
      left: 'center',
      icon: 'circle',
      itemWidth: 8,
      itemHeight: 8,
      textStyle: { color: '#9aa9bd', fontSize: 11 },
    },
    series: [
      {
        type: 'pie',
        radius: ['50%', '72%'],
        center: ['50%', '43%'],
        label: { show: false },
        itemStyle: { borderColor: '#0b1728', borderWidth: 3 },
        data: props.data.map((item) => ({
          name: item.label,
          value: item.count,
          itemStyle: { color: item.type === 'dc' ? '#8f75ff' : '#1ed4d2' },
        })),
      },
    ],
  }
})
</script>

<template><div ref="chartElement" class="chart" /></template>
