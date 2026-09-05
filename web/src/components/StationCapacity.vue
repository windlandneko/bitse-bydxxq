<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import type { Station } from '../types/dashboard'

const props = defineProps<{ stations: Station[]; modelValue?: number }>()
const emit = defineEmits<{ 'update:modelValue': [id: number] }>()
const list = ref<HTMLDivElement>()
const firstVisible = ref(0)
const rows = computed(() =>
  props.stations.map((station) => {
    const total = station.totalChargers
    const idle = station.idleChargers
    // The API's onlineRate is calculated from an integer count without rounding.
    // Reserved and charging are only available together at station level.
    const online = Math.round((total * station.onlineRate) / 100)
    return { ...station, total, idle, occupied: online - idle, unavailable: total - online }
  }),
)
const selectedIndex = computed(() =>
  props.stations.findIndex((station) => station.id === props.modelValue),
)
const lastVisible = computed(() => Math.min(firstVisible.value + 5, rows.value.length))
function description(station: (typeof rows.value)[number]) {
  return `${station.name}：空闲 ${station.idle} 台，在用或预约 ${station.occupied} 台，暂不可用 ${station.unavailable} 台，共 ${station.total} 台`
}
function syncScroll() {
  const viewport = list.value
  firstVisible.value = viewport
    ? Math.round((viewport.scrollLeft * 5) / (viewport.clientWidth + 8))
    : 0
}
function move(direction: number) {
  if (!list.value) return
  list.value.scrollBy({
    left: direction * (list.value.clientWidth + 8),
    behavior: matchMedia('(prefers-reduced-motion: reduce)').matches ? 'instant' : 'smooth',
  })
}
watch(
  [selectedIndex, list],
  ([index, viewport]) => {
    const item = viewport?.children[index] as HTMLElement | undefined
    if (viewport && item) {
      const left = item.offsetLeft - viewport.offsetLeft
      const right = left + item.offsetWidth
      if (left < viewport.scrollLeft) viewport.scrollLeft = left
      else if (right > viewport.scrollLeft + viewport.clientWidth)
        viewport.scrollLeft = right - viewport.clientWidth
    }
    syncScroll()
  },
  { flush: 'post' },
)
</script>

<template>
  <section class="capacity-card" aria-label="各站可用电桩">
    <header class="capacity-header">
      <h2>各站可用电桩</h2>
      <div class="capacity-legend" aria-label="状态图例">
        <span><i class="capacity-idle" />空闲</span>
        <span><i class="capacity-occupied" />在用 / 预约</span>
        <span><i class="capacity-unavailable" />暂不可用</span>
      </div>
      <div v-if="rows.length > 5" class="capacity-pager">
        <button aria-label="上一组电站" :disabled="firstVisible === 0" @click="move(-1)">
          <svg viewBox="0 0 24 24" aria-hidden="true"><path d="m14 6-6 6 6 6" /></svg>
        </button>
        <span>{{ firstVisible + 1 }}–{{ lastVisible }} / {{ rows.length }}</span>
        <button aria-label="下一组电站" :disabled="lastVisible === rows.length" @click="move(1)">
          <svg viewBox="0 0 24 24" aria-hidden="true"><path d="m10 6 6 6-6 6" /></svg>
        </button>
      </div>
    </header>
    <div v-if="rows.length" ref="list" class="capacity-list" role="list" @scroll="syncScroll">
      <div v-for="station in rows" :key="station.id" class="capacity-item" role="listitem">
        <button
          :data-station-id="station.id"
          :class="{ 'is-selected': modelValue === station.id }"
          :aria-pressed="modelValue === station.id"
          :aria-label="description(station)"
          :title="description(station)"
          @click="emit('update:modelValue', station.id)"
        >
          <span class="capacity-name">{{ station.name.replace(/充电站$/, '') }}</span>
          <span class="capacity-value-row">
            <span class="capacity-count"
              ><strong>{{ station.idle }}</strong
              ><span>/{{ station.total }}</span></span
            >
            <span class="capacity-bar" aria-hidden="true">
              <i class="capacity-idle" :style="{ flexGrow: station.idle }" />
              <i class="capacity-occupied" :style="{ flexGrow: station.occupied }" />
              <i class="capacity-unavailable" :style="{ flexGrow: station.unavailable }" />
            </span>
          </span>
        </button>
      </div>
    </div>
    <p v-else class="capacity-empty">暂无电站</p>
  </section>
</template>
