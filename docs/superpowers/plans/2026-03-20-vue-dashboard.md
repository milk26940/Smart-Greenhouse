# Vue 监控大屏改版 Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 `网页/vue-project` 的首页改造成科技监控风数据大屏，并让实时环境指标成为首屏主视觉中心。

**Architecture:** 保持单页结构和现有接口不变，主要在 `src/App.vue` 内重组模板、修正文案、补充展示型计算属性并重写样式。通过更新 `src/__tests__/App.spec.ts` 验证新大屏的关键结构、MQTT 区域和核心文案仍然可见，最终用 Vitest 和构建命令做回归确认。

**Tech Stack:** Vue 3、TypeScript、Vite、Vitest、Vue Test Utils、Chart.js、MQTT.js

---

### Task 1: 更新测试以描述新大屏结构

**Files:**
- Modify: `网页/vue-project/src/__tests__/App.spec.ts`
- Test: `网页/vue-project/src/__tests__/App.spec.ts`

- [ ] **Step 1: 写出失败测试，约束新首页主结构**

```ts
it('renders the command center hero and realtime metric cards', async () => {
  const wrapper = mount(App)
  await Promise.resolve()
  await Promise.resolve()

  expect(wrapper.text()).toContain('智慧农业大棚监控中心')
  expect(wrapper.text()).toContain('实时环境核心指标')
  expect(wrapper.findAll('.metric-card')).toHaveLength(6)
})
```

- [ ] **Step 2: 运行测试并确认失败**

Run: `npm run test:unit -- src/__tests__/App.spec.ts --run`
Expected: FAIL，因为当前页面标题和结构仍是旧版文案与布局。

- [ ] **Step 3: 补充 MQTT 与状态区结构测试**

```ts
it('renders mqtt link monitor and device status section', async () => {
  const wrapper = mount(App)
  await Promise.resolve()
  await Promise.resolve()

  expect(wrapper.text()).toContain('通信链路监控')
  expect(wrapper.text()).toContain('设备在线状态')
})
```

- [ ] **Step 4: 再次运行测试，确认失败原因仍符合预期**

Run: `npm run test:unit -- src/__tests__/App.spec.ts --run`
Expected: FAIL，且失败集中在新文案与新结构未实现。

### Task 2: 实现首页科技监控风重构

**Files:**
- Modify: `网页/vue-project/src/App.vue`

- [ ] **Step 1: 修正中文文案和展示标签**

将页面内出现乱码的中文文本统一恢复为正常中文，包括：

- 标题、副标题、按钮文案
- 区块标题
- 状态标签
- MQTT 日志提示

- [ ] **Step 2: 调整展示层数据结构**

在 `App.vue` 中增加最小展示辅助逻辑，例如：

```ts
const currentTime = ref('')

const environmentCards = computed(() => [
  { label: '空气温度', value: formatNumber(...), unit: '°C', tone: 'warm' },
])
```

包含：

- 当前时间展示
- 实时指标卡单位和高亮色映射
- 设备状态条目映射
- MQTT 状态说明文案映射

- [ ] **Step 3: 重组模板**

将模板改为以下分区：

- 顶部总控区
- 实时环境核心指标区
- 视觉与设备状态区
- 趋势分析区
- MQTT 通信链路监控区

- [ ] **Step 4: 重写样式**

重点实现：

- 深色科技监控风背景
- 发光指标卡
- 深色图表容器
- 终端感 MQTT 消息区
- 响应式单列降级

- [ ] **Step 5: 运行单测并修正实现**

Run: `npm run test:unit -- src/__tests__/App.spec.ts --run`
Expected: PASS

### Task 3: 全量回归验证

**Files:**
- Modify: `网页/vue-project/src/App.vue`
- Modify: `网页/vue-project/src/__tests__/App.spec.ts`

- [ ] **Step 1: 运行单元测试**

Run: `npm run test:unit -- --run`
Expected: PASS

- [ ] **Step 2: 运行构建**

Run: `npm run build`
Expected: PASS

- [ ] **Step 3: 记录验证结果并准备交付说明**

在最终说明中明确：

- 修改了哪些文件
- 测试和构建结果
- 是否存在未覆盖的残余风险
