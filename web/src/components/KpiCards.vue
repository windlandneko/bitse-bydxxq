<script setup lang="ts">
import { computed } from 'vue'
import AnimatedNumber from './AnimatedNumber.vue'
import type { KpiData } from '../types/dashboard'

const props = defineProps<{ data: KpiData }>()

const items = [
  { key: 'totalChargingCount', label: '累计充电次数', unit: '次', accent: 'cyan' },
  { key: 'totalRevenue', label: '累计营收', unit: '元', accent: 'amber' },
  { key: 'onlineChargers', label: '在线电桩', unit: '台', accent: 'green' },
  { key: 'registeredUsers', label: '注册用户', unit: '人', accent: 'violet' },
] as const
const summaries = computed(() =>
  items.map((item) => {
    const raw = props.data[item.key]
    const divisor = raw >= 100_000_000 ? 100_000_000 : raw >= 1_000_000 ? 10_000 : 1
    const prefix = divisor === 100_000_000 ? '亿' : divisor === 10_000 ? '万' : ''
    return {
      ...item,
      value: raw / divisor,
      unit: prefix + item.unit,
      decimals: divisor > 1 || item.key === 'totalRevenue' ? 2 : 0,
      exact: `${item.label} ${raw.toLocaleString('zh-CN', { minimumFractionDigits: item.key === 'totalRevenue' ? 2 : 0, maximumFractionDigits: 2 })} ${item.unit}`,
    }
  }),
)
</script>

<template>
  <div class="kpi-grid">
    <div
      v-for="item in summaries"
      :key="item.key"
      class="kpi-card"
      :class="`kpi-card--${item.accent}`"
      :title="item.exact"
    >
      <div class="kpi-card__label">{{ item.label }}</div>
      <div class="kpi-card__value">
        <AnimatedNumber :key="item.unit" :value="item.value" :decimals="item.decimals" />
        <small>{{ item.unit }}</small>
      </div>
    </div>
  </div>
</template>
