<script setup lang="ts">
// Adapted from daidaibg/IofTV-Screen-Vue3 BorderBox13 (MIT).
// Upstream commit and complete license: web/third-party/README.md.
// Native ResizeObserver replaces VueUse; corrected upstream stroke binding.
import { onBeforeUnmount, onMounted, ref } from 'vue'
const element = ref<HTMLElement>()
const width = ref(0)
const height = ref(0)
let observer: ResizeObserver | undefined
onMounted(() => {
  observer = new ResizeObserver(([entry]) => {
    width.value = entry?.contentRect.width ?? 0
    height.value = entry?.contentRect.height ?? 0
  })
  if (element.value) observer.observe(element.value)
})
onBeforeUnmount(() => observer?.disconnect())
</script>
<template>
  <div ref="element" class="border-box">
    <svg :width="width" :height="height" class="border-box__svg" aria-hidden="true">
      <path
        fill="rgba(7,26,49,.76)"
        stroke="#21466a"
        :d="`M 5 20 L 5 10 L 12 3 L 60 3 L 68 10 L ${width - 20} 10 L ${width - 5} 25 L ${width - 5} ${height - 5} L 20 ${height - 5} L 5 ${height - 20} L 5 20`"
      />
      <path
        fill="transparent"
        stroke="#548fd4"
        stroke-width="3"
        stroke-linecap="round"
        stroke-dasharray="10, 5"
        d="M 16 9 L 61 9"
      />
      <path fill="transparent" stroke="#2cf7fe" d="M 5 20 L 5 10 L 12 3 L 60 3 L 68 10" />
      <path
        fill="transparent"
        stroke="#2cf7fe"
        :d="`M ${width - 5} ${height - 30} L ${width - 5} ${height - 5} L ${width - 30} ${height - 5}`"
      />
    </svg>
    <div class="border-box__content"><slot /></div>
  </div>
</template>
<style scoped>
.border-box {
  position: relative;
  width: 100%;
  height: 100%;
  min-height: 0;
}
.border-box__svg {
  position: absolute;
  inset: 0;
  pointer-events: none;
}
.border-box__content {
  position: relative;
  height: 100%;
  min-height: 0;
}
</style>
