# 手机端充电场景插图

用户端登录页和首页的电动车充电插图，使用奶白与森林绿配色。
应用打包并加载 SVG；PNG 保留为转描源图和预览素材。

## 文件

- `charging-hero.png`：透明 PNG，1497 × 826，约 131 KB。由内建图像生成工具输出 RGBA 源图，再完成实色归并、透明边缘清理和裁边。
- `charging-hero.svg`：VTracer转描结果，62条路径、7个实色，约69KB，viewBox为1497×826。
- `charging-hero.preview.png`：深绿背景的尺寸对照图，左列PNG、右列SVG；上排宽220px、下排宽130px。

## 生成工具与来源

使用内建 `image_gen.imagegen` 生成，再用 VTracer 转描。

生成原图为 1536 × 1024 RGBA；仓库保留准备后的透明PNG和最终SVG，均可独立使用。

## 实色归并与转描

工具版本：Pillow 12.3.0、VTracer Python 0.6.15（SVG 生成器标记为 VTracer 0.6.12）、CairoSVG 2.9.0。

准备步骤：

1. 使用 Pillow 固定调色板量化，不使用抖动，保留以下 8 个实色：`#F6F3E8`、`#FAF9F1`、`#E4EBD9`、`#8FAE9A`、`#285D4B`、`#143B30`、`#567967`、`#B8D76C`。
2. alpha小于8设为0，不小于245设为255，其余边缘alpha保留。
3. 以 alpha ≥ 128 的主体范围外留 24 px 余量，源图裁切框为 `(11, 75, 1508, 901)`，得到 1497 × 826 的最终 PNG。
4. 转描输入使用同一PNG，alpha ≥ 128为实体、其他为透明，按以下参数生成曲线。

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

生成后补充SVG的viewBox，填色按RGB距离归并回原调色板。最终PNG有8个实色，SVG使用其中7个。

## 检查结论

PNG背景透明；SVG由路径组成，渲染后四角alpha为0。130px和220px宽度下，
车窗、车轮、充电桩与电缆连接均可辨认，预览见尺寸对照图。

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
