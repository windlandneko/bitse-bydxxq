<script setup lang="ts">
import AnimatedNumber from './AnimatedNumber.vue'
import type { KpiData } from '../types/dashboard'

defineProps<{ data: KpiData }>()

const items = [
  { key: 'totalChargingCount', label: '累计充电次数', unit: '次', icon: '↗', accent: 'cyan' },
  { key: 'totalRevenue', label: '累计营收', unit: '元', icon: '¥', accent: 'amber' },
  { key: 'onlineChargers', label: '在线电桩', unit: '台', icon: '◉', accent: 'green' },
  { key: 'registeredUsers', label: '注册用户', unit: '人', icon: '♙', accent: 'violet' },
] as const
</script>

<template>
  <div class="kpi-grid">
    <div v-for="item in items" :key="item.key" class="kpi-card" :class="`kpi-card--${item.accent}`">
      <div class="kpi-card__icon">{{ item.icon }}</div>
      <div class="kpi-card__label">{{ item.label }}</div>
      <div class="kpi-card__value">
        <AnimatedNumber :value="data[item.key]" :decimals="item.key === 'totalRevenue' ? 2 : 0" />
        <small>{{ item.unit }}</small>
      </div>
    </div>
  </div>
</template>
