<script setup lang="ts">
import { nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import type { ChargerStatusItem } from '../types/dashboard'
import { init, type ChartInstance } from '../lib/echarts'

const props = defineProps<{ data: ChargerStatusItem[] }>()
const chartElement = ref<HTMLDivElement | null>(null)
let chart: ChartInstance | undefined
let resizeObserver: ResizeObserver | undefined

const colors: Record<string, string> = {
  idle: '#39e58c',
  reserved: '#43b8ff',
  charging: '#ffb84d',
  fault: '#ff5f72',
  offline: '#69758c',
  maintenance: '#9d7cff',
}

function render() {
  if (!chartElement.value) return
  chart ??= init(chartElement.value)
  chart.setOption({
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
    series: [{
      type: 'pie',
      radius: ['54%', '76%'],
      center: ['50%', '43%'],
      avoidLabelOverlap: true,
      itemStyle: { borderColor: '#0b1728', borderWidth: 3 },
      label: { show: false },
      data: props.data.map((item) => ({
        name: item.label,
        value: item.value,
        itemStyle: { color: colors[item.key] ?? '#69758c' },
      })),
    }],
    graphic: props.data.length
      ? [{
          type: 'text',
          left: 'center',
          top: '36%',
          style: {
            text: String(props.data.reduce((sum, item) => sum + item.value, 0)),
            fill: '#f2f7ff',
            fontSize: 28,
            fontWeight: 700,
          },
        }, {
          type: 'text',
          left: 'center',
          top: '51%',
          style: { text: '总电桩', fill: '#8090a7', fontSize: 11 },
        }]
      : [{ type: 'text', left: 'center', top: '45%', style: { text: '暂无数据', fill: '#8090a7', fontSize: 13 } }],
  }, true)
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
onBeforeUnmount(() => { resizeObserver?.disconnect(); chart?.dispose() })
</script>

<template><div ref="chartElement" class="chart chart--donut" /></template>
