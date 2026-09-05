# 手机端充电场景插图

本素材用于用户端首页约 130 px 宽、登录页约 220 px 宽的插图。图形为现代小型电动车、充电桩与实际连接到车辆充电口的电缆，主要使用奶白与森林绿，适配深绿背景。

## 文件

- `charging-hero.png`：透明 PNG，1497 × 826，约 131 KB。由内建图像生成工具输出 RGBA 源图，再完成实色归并、透明边缘清理和裁边。
- `charging-hero.svg`：真实 VTracer 曲线转描结果，62 个 `path`，约 69 KB；包含 `viewBox="0 0 1497 826"`，没有内嵌位图、背景矩形、滤镜或渐变。没有手工拼几何形状代替生成图。
- `charging-hero.preview.png`：素材验收图，深绿 `#143B30` 背景；左列为 PNG，右列为 SVG；上排显示宽度 220 px，下排显示宽度 130 px。此文件仅供比较，不用作应用素材。

## 生成工具与来源

使用内建 `image_gen.imagegen` 生成，再用 VTracer 转描；不依赖第三方汽车照片或运行时图像服务。

生成原图为 1536 × 1024 RGBA；仓库保留准备后的透明PNG和最终SVG，均可独立使用。

## 实色归并与转描

工具版本：Pillow 12.3.0、VTracer Python 0.6.15（SVG 生成器标记为 VTracer 0.6.12）、CairoSVG 2.9.0。

准备步骤：

1. 使用 Pillow 固定调色板量化，不使用抖动，保留以下 8 个实色：`#F6F3E8`、`#FAF9F1`、`#E4EBD9`、`#8FAE9A`、`#285D4B`、`#143B30`、`#567967`、`#B8D76C`。
2. alpha 小于 8 的极低不透明度残留设为 0；alpha 不小于 245 的主体设为 255；其余边缘 alpha 保留。没有删除车、桩、电缆之间真实的透明空隙。
3. 以 alpha ≥ 128 的主体范围外留 24 px 余量，源图裁切框为 `(11, 75, 1508, 901)`，得到 1497 × 826 的最终 PNG。
4. 转描输入使用同一 PNG，alpha ≥ 128 为实体、其他为透明。VTracer 按真实像素边界生成曲线，以较少控制点平滑图形；没有自行描画汽车几何。
5. 在 220 px / 130 px 实际显示尺寸比较三个曲线精度版本，选用中间的温和简化版本：62 条路径、69 KB；比初版 73 条路径、120 KB 更紧凑，同时保留车窗、轮毂和电缆连接。

最终 VTracer 参数：

```python
vtracer.convert_image_to_svg_py(
  image_path=trace_input,
  out_path='charging-hero.svg',
  colormode='color',
  hierarchical='stacked',
  mode='spline',
  filter_speckle=28,
  color_precision=8,
  layer_difference=16,
  corner_threshold=90,
  length_threshold=12.0,
  max_iterations=10,
  splice_threshold=60,
  path_precision=1,
)
```

生成后补充 SVG 根元素的 `viewBox`，维持与 PNG 相同的固有宽高。VTracer 区域归并会计算出一些临近填色，随后按 RGB 距离自动归并回同一固定调色板；仅替换填色，没有重画或改变转描得到的路径。最终 PNG 有 8 个实色，SVG 使用其中 7 个。

## 检查结论

- PNG 使用 RGBA，背景和车桩间空隙有实际透明 alpha。
- SVG 仅使用转描得到的路径，62 个 `path`；无 `image`、`data:image`、背景底板、阴影滤镜或渐变。
- SVG 渲染的四角 alpha 均为 0。
- 已比较 PNG / SVG 在深绿背景上的 220 px 和 130 px 预览：整体轮廓、车窗、两个车轮、充电桩与连接电缆可辨，没有一圈光晕或矩形底板。
- 车窗和轮毂保留有限细节，保证小尺寸仍可识别，不追求产品渲染的全部细节。

## 首次生成提示词

```text
Use case: stylized-concept
Asset type: compact transparent hero illustration for a polished mobile EV charging app, used at only 130px wide on home and 220px wide on login.
Primary request: Draw one refined contemporary compact electric hatchback connected to one charging pedestal. The car is the dominant subject, with an elegant friendly silhouette, believable wheelbase, two visible wheels with simple hubs, crisp windows, a subtle headlight and a few restrained door seams. The pedestal stands just behind the car on the right; one clearly visible pale-sage charging cable runs continuously from the pedestal to a plug visibly inserted into the rear side charging port of the car. Show the port and connected plug; do not let the cable merely touch a tire or disappear without a connection.
Style/medium: premium clean flat editorial vector-style illustration; smooth deliberate contours and broad solid-color planes, restrained detail, no outlines around every part. A shallow front-side three-quarter view facing lower-left; do not use complex isometric geometry or 3D rendering.
Composition: horizontal subject group, about 3:2 footprint, car filling about 75% of group width, pedestal taller than roof but secondary. Keep the entire car, wheels, cable and pedestal visible. Close framing with a small consistent transparent margin. No other objects.
Color palette: use only approximately 8 flat opaque solid fills: warm ivory #F6F3E8, pale cream #FAF9F1, pale sage #E4EBD9, sage #8FAE9A, forest green #285D4B, dark green #143B30, muted glass green #567967, small lime accents #B8D76C. Mainly ivory car and pedestal with forest-green windows, wheel tires and restrained trim. This artwork must remain readable on a dark forest-green app background, so emphasize light body surfaces and a clearly visible light sage cable.
Background: genuinely transparent alpha everywhere outside the actual car, charger and cable. Empty transparent space between objects and through the cable loop; not a white rectangular background and not a checkerboard drawing.
Strict constraints: absolutely no text, letters, digits, logos, charging lightning symbols, badges, decorative circles, scenery, backdrop, gradients, drop shadows, ground shadow, floor slab, platform, ground plane, texture, speckle, noise, photographic detail or watermark. Shapes must be clean enough for actual automatic vector tracing afterwards.
```
