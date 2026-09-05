<script setup lang="ts">
import BorderBox from './vendor/BorderBox.vue'
defineProps<{ title: string; eyebrow?: string }>()
</script>
<template>
  <article class="chart-card">
    <BorderBox>
      <div class="chart-card__inner">
        <header class="chart-card__header">
          <div>
            <span class="chart-card__eyebrow">{{ eyebrow }}</span>
            <h2>{{ title }}</h2>
          </div>
          <div class="chart-card__tools"><slot name="tools" /></div>
        </header>
        <div class="chart-card__body"><slot /></div>
      </div>
    </BorderBox>
  </article>
</template>
