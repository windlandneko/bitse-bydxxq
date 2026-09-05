<script setup lang="ts">
defineProps<{
  title: string
  eyebrow?: string
  className?: string
}>()
</script>

<template>
  <article class="chart-card" :class="className">
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
