<script setup lang="ts">
import { ref, watch } from 'vue'

const props = withDefaults(defineProps<{ value: number; decimals?: number; duration?: number }>(), {
  decimals: 0,
  duration: 1200,
})

const display = ref(0)
let raf = 0

watch(() => props.value, () => {
  const from = display.value
  const to = props.value
  const start = performance.now()
  cancelAnimationFrame(raf)
  const tick = (now: number) => {
    const progress = Math.min(1, (now - start) / props.duration)
    const eased = 1 - Math.pow(1 - progress, 3)
    display.value = from + (to - from) * eased
    if (progress < 1) raf = requestAnimationFrame(tick)
  }
  raf = requestAnimationFrame(tick)
}, { immediate: true })

function format() {
  return display.value.toLocaleString('zh-CN', {
    minimumFractionDigits: props.decimals,
    maximumFractionDigits: props.decimals,
  })
}
</script>

<template><span>{{ format() }}</span></template>
