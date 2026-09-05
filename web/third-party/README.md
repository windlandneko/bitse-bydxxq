# 第三方来源

`src/components/vendor/BorderBox.vue` 的 SVG 边框路径与结构直接改编自
[daidaibg/IofTV-Screen-Vue3](https://github.com/daidaibg/IofTV-Screen-Vue3/blob/f1383bcf078d66329dc7fa6ca6a4e2befaf83215/src/components/datav/border-box-13/border-box-13.vue)，
提交 `f1383bcf078d66329dc7fa6ca6a4e2befaf83215`，MIT 许可见同目录完整许可文件。
修改：移除 lodash/VueUse/SCSS 依赖，使用原生 ResizeObserver，修正 stroke 绑定，
调整颜色和透明度，加入无障碍标记。项目的三栏指挥中心布局与标题装饰也参考该项目。

本仓库 PR #8 提供的数字动画思路已重新实现，加入取消动画和减少动态效果支持。
ECharts 与 Vue 通过 package.json/pnpm-lock.yaml 锁定，图表、图标和边框本地渲染，
没有第三方 CDN、在线字体、地图瓦片或来源不明的图片。中央空间分布图直接使用后端
站点经纬度绘制散点，供查看站点分布和选择站点。
