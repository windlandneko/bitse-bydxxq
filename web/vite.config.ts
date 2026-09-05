import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [vue()],
  optimizeDeps: {
    // ECharts has many internal entry points; excluding it keeps the
    // development server from pre-bundling the whole chart library.
    exclude: ['echarts'],
  },
  server: {
    host: '0.0.0.0',
    port: 5173,
  },
})
