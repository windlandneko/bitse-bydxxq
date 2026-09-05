<script setup lang="ts">
import { onBeforeUnmount, ref, watch } from 'vue'
const props = withDefaults(defineProps<{ value: number; decimals?: number }>(), { decimals: 0 })
const displayed = ref(props.value)
let frame = 0
watch(
  () => props.value,
  (value) => {
    cancelAnimationFrame(frame)
    if (matchMedia('(prefers-reduced-motion: reduce)').matches) {
      displayed.value = value
      return
    }
    const start = performance.now()
    const from = displayed.value
    const animate = (now: number) => {
      const progress = Math.min(1, (now - start) / 700)
      displayed.value = from + (value - from) * (1 - (1 - progress) ** 3)
      if (progress < 1) frame = requestAnimationFrame(animate)
    }
    frame = requestAnimationFrame(animate)
  },
)
onBeforeUnmount(() => cancelAnimationFrame(frame))
</script>
<template>
  <span>{{
    displayed.toLocaleString('zh-CN', {
      minimumFractionDigits: decimals,
      maximumFractionDigits: decimals,
    })
  }}</span>
</template>
