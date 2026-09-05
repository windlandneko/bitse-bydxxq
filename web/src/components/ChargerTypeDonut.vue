<script setup lang="ts">
import { nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import type { ChargerTypeRatioItem } from '../types/dashboard'
import { init, type ChartInstance } from '../lib/echarts'

const props = defineProps<{ data: ChargerTypeRatioItem[] }>()
const chartElement = ref<HTMLDivElement | null>(null)
let chart: ChartInstance | undefined
let resizeObserver: ResizeObserver | undefined

function render() {
  if (!chartElement.value) return
  chart ??= init(chartElement.value)
  chart.setOption({
    tooltip: { trigger: 'item', formatter: '{b}<br/><b>{c}</b> 台（{d}%）' },
    legend: { bottom: 0, left: 'center', icon: 'circle', itemWidth: 8, itemHeight: 8, textStyle: { color: '#9aa9bd', fontSize: 11 } },
    series: [{ type: 'pie', radius: ['50%', '72%'], center: ['50%', '43%'], label: { show: false }, itemStyle: { borderColor: '#0b1728', borderWidth: 3 }, data: props.data.map((item) => ({ name: item.label, value: item.count, itemStyle: { color: item.type === 'dc' ? '#8f75ff' : '#1ed4d2' } })) }],
  }, true)
}

onMounted(async () => { await nextTick(); render(); if (chartElement.value) { resizeObserver = new ResizeObserver(() => chart?.resize()); resizeObserver.observe(chartElement.value) } })
watch(() => props.data, render, { deep: true })
onBeforeUnmount(() => { resizeObserver?.disconnect(); chart?.dispose() })
</script>

<template><div ref="chartElement" class="chart chart--type-donut" /></template>
