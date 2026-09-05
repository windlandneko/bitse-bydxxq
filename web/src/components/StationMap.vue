<script setup lang="ts">
import { computed } from 'vue'
import { useChart } from '../composables/useChart'
import type { Station } from '../types/dashboard'
import { firstTooltipItem, type ChartEvent } from '../lib/echarts'
const props = defineProps<{ stations: Station[]; modelValue?: number }>()
const emit = defineEmits<{ 'update:modelValue': [id: number] }>()
const selected = computed(
  () => props.stations.find((station) => station.id === props.modelValue) ?? props.stations[0],
)
const plottedStations = computed(() =>
  props.stations.filter(
    (station) => Number.isFinite(station.longitude) && Number.isFinite(station.latitude),
  ),
)
const element = useChart(
  () => ({
    grid: { left: 58, right: 54, top: 35, bottom: 55 },
    tooltip: {
      trigger: 'item',
      renderMode: 'richText',
      formatter: (params) => {
        const station = plottedStations.value[firstTooltipItem(params)?.dataIndex ?? 0]
        return station
          ? `${station.name}\n${station.region} · ${station.idleChargers}/${station.totalChargers} 台空闲\n${station.latitude.toFixed(4)}° N / ${station.longitude.toFixed(4)}° E`
          : ''
      },
    },
    xAxis: {
      type: 'value',
      scale: true,
      min: (range: { min: number; max: number }) =>
        range.min - Math.max(0.004, (range.max - range.min) * 0.08),
      max: (range: { min: number; max: number }) =>
        range.max + Math.max(0.004, (range.max - range.min) * 0.08),
      name: 'E',
      nameTextStyle: { color: '#577a9a' },
      axisLine: { show: true, lineStyle: { color: '#294768' } },
      axisLabel: { color: '#537594', formatter: (value: number) => `${value.toFixed(2)}°` },
      splitLine: { lineStyle: { color: '#132f4d' } },
    },
    yAxis: {
      type: 'value',
      scale: true,
      min: (range: { min: number; max: number }) =>
        range.min - Math.max(0.004, (range.max - range.min) * 0.08),
      max: (range: { min: number; max: number }) =>
        range.max + Math.max(0.004, (range.max - range.min) * 0.08),
      name: 'N',
      nameTextStyle: { color: '#577a9a' },
      axisLabel: { color: '#537594', formatter: (value: number) => `${value.toFixed(2)}°` },
      splitLine: { lineStyle: { color: '#132f4d' } },
    },
    series: [
      {
        type: 'effectScatter',
        coordinateSystem: 'cartesian2d',
        rippleEffect: { brushType: 'stroke', scale: 3, period: 5 },
        animation: !matchMedia('(prefers-reduced-motion: reduce)').matches,
        symbolSize: (_value, params) =>
          12 + Math.min(10, plottedStations.value[params.dataIndex]?.totalChargers ?? 0),
        label: {
          show: true,
          position: 'top',
          distance: 10,
          color: '#bee6ff',
          fontSize: 12,
          formatter: '{b}',
          backgroundColor: 'rgba(4,16,33,.75)',
          padding: [3, 6],
        },
        labelLayout: { hideOverlap: true },
        itemStyle: { shadowBlur: 18, shadowColor: '#17d9f9' },
        data: plottedStations.value.map((station) => ({
          name: station.name,
          value: [station.longitude, station.latitude, station.totalChargers],
          itemStyle: {
            color: station.idleChargers > 0 ? '#3be4d5' : '#ffbd65',
            borderColor: selected.value?.id === station.id ? '#dcfff8' : 'transparent',
            borderWidth: 2,
          },
        })),
      },
    ],
  }),
  (chart) => {
    chart.on('click', 'series.effectScatter', (params: ChartEvent) => {
      const station = plottedStations.value[params.dataIndex]
      if (station) emit('update:modelValue', station.id)
    })
  },
)
</script>
<template>
  <div class="station-map">
    <div class="map-heading"><span>区域充电网络</span><small>站点分布</small></div>
    <div class="map-sweep" aria-hidden="true" />
    <div ref="element" class="map-chart" />
    <div v-if="!stations.length" class="map-empty">等待站点数据</div>
    <div v-if="selected" class="station-detail">
      <div>
        <b>{{ selected.name }}</b
        ><small>{{ selected.address }}</small>
      </div>
      <div>
        <strong
          >{{ selected.idleChargers }}<em>/ {{ selected.totalChargers }}</em></strong
        ><small>空闲电桩</small>
      </div>
      <div>
        <strong>{{ (selected.priceCents / 100).toFixed(2) }}</strong
        ><small>元 / kWh</small>
      </div>
    </div>
    <div class="map-stations" aria-label="站点选择">
      <button
        v-for="station in stations"
        :key="station.id"
        :class="{ selected: selected?.id === station.id }"
        :aria-pressed="selected?.id === station.id"
        @click="emit('update:modelValue', station.id)"
      >
        {{ station.name }}
      </button>
    </div>
  </div>
</template>
