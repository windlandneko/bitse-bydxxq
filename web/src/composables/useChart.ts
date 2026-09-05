import { onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { init, type ChartInstance, type ChartOption } from '../lib/echarts'

export function useChart(createOption: () => ChartOption, onInit?: (chart: ChartInstance) => void) {
  const element = ref<HTMLDivElement>()
  let chart: ChartInstance | undefined
  let observer: ResizeObserver | undefined

  onMounted(() => {
    if (!element.value) return
    chart = init(element.value)
    onInit?.(chart)
    chart.setOption(createOption(), { notMerge: true })
    observer = new ResizeObserver(() => chart?.resize())
    observer.observe(element.value)
  })

  watch(createOption, (option) => chart?.setOption(option, { notMerge: true }), { deep: true })

  onBeforeUnmount(() => {
    observer?.disconnect()
    chart?.dispose()
  })

  return element
}
