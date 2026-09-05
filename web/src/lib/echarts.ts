// Local wrapper around ECharts. Kept out of git (see root .gitignore `lib/`)
// so the chart components import from a single place. Exposes the small subset
// the dashboard charts rely on: init, the `graphic` namespace (LinearGradient),
// and the ECharts instance type.
import * as echarts from 'echarts'

export type ChartInstance = echarts.ECharts
export const init = echarts.init
export const graphic = echarts.graphic
