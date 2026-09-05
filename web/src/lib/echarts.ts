import * as echarts from 'echarts/core'
import {
  BarChart,
  EffectScatterChart,
  HeatmapChart,
  LineChart,
  PieChart,
  type BarSeriesOption,
  type EffectScatterSeriesOption,
  type HeatmapSeriesOption,
  type LineSeriesOption,
  type PieSeriesOption,
} from 'echarts/charts'
import {
  GraphicComponent,
  GridComponent,
  LegendComponent,
  TooltipComponent,
  VisualMapComponent,
  type GraphicComponentOption,
  type GridComponentOption,
  type LegendComponentOption,
  type TooltipComponentOption,
  type VisualMapComponentOption,
} from 'echarts/components'
import { CanvasRenderer } from 'echarts/renderers'

echarts.use([
  BarChart,
  EffectScatterChart,
  HeatmapChart,
  LineChart,
  PieChart,
  GraphicComponent,
  GridComponent,
  LegendComponent,
  TooltipComponent,
  VisualMapComponent,
  CanvasRenderer,
])
export const { init, graphic } = echarts
export type ChartInstance = ReturnType<typeof init>
export type { ECElementEvent as ChartEvent } from 'echarts/core'
export type ChartOption = echarts.ComposeOption<
  | BarSeriesOption
  | EffectScatterSeriesOption
  | HeatmapSeriesOption
  | LineSeriesOption
  | PieSeriesOption
  | GraphicComponentOption
  | GridComponentOption
  | LegendComponentOption
  | TooltipComponentOption
  | VisualMapComponentOption
>
type TooltipParams = Parameters<Exclude<TooltipComponentOption['formatter'], string | undefined>>[0]

export function firstTooltipItem(params: TooltipParams) {
  return Array.isArray(params) ? params[0] : params
}
