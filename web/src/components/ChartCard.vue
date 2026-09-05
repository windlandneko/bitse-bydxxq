<script setup lang="ts">
defineProps<{
  title: string
  eyebrow?: string
  className?: string
}>()
</script>

<template>
  <article class="chart-card" :class="className">
    <span class="corner corner--tl" />
    <span class="corner corner--tr" />
    <span class="corner corner--bl" />
    <span class="corner corner--br" />
    <header class="chart-card__header">
      <div>
        <span v-if="eyebrow" class="chart-card__eyebrow">{{ eyebrow }}</span>
        <h2>{{ title }}</h2>
      </div>
      <div class="chart-card__tools">
        <slot name="tools" />
      </div>
    </header>
    <div class="chart-card__body">
      <slot />
    </div>
  </article>
</template>
