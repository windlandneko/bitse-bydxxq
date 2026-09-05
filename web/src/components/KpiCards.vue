<script setup lang="ts">
import type { KpiData } from '../types/dashboard'
import AnimatedNumber from './AnimatedNumber.vue'

defineProps<{ data: KpiData }>()

const items = [
  { key: 'totalChargingCount', label: '累计充电次数', unit: '次', icon: '↗', accent: 'cyan', decimals: 0 },
  { key: 'totalRevenue', label: '累计营收', unit: '元', icon: '¥', accent: 'amber', decimals: 2 },
  { key: 'onlineChargers', label: '在线电桩', unit: '台', icon: '◉', accent: 'green', decimals: 0 },
  { key: 'registeredUsers', label: '注册用户', unit: '人', icon: '♙', accent: 'violet', decimals: 0 },
] as const
</script>

<template>
  <div class="kpi-grid">
    <div v-for="item in items" :key="item.key" class="kpi-card" :class="`kpi-card--${item.accent}`">
      <div class="kpi-card__icon"><span>{{ item.icon }}</span></div>
      <div class="kpi-card__body">
        <div class="kpi-card__label">{{ item.label }}</div>
        <div class="kpi-card__value">
          <AnimatedNumber :value="data[item.key]" :decimals="item.decimals" />
          <small>{{ item.unit }}</small>
        </div>
      </div>
      <div class="kpi-card__spark" aria-hidden="true"><i /><i /><i /><i /><i /></div>
    </div>
  </div>
</template>
