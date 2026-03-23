<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { RouterLink } from 'vue-router'
import { useGreenhouseStore } from '@/stores/greenhouse'
import { formatDateTime, formatInteger, formatNumber } from '@/lib/format'

const store = useGreenhouseStore()

const currentTime = ref('')
let clockTimer: ReturnType<typeof setInterval> | null = null
let refreshTimer: ReturnType<typeof setInterval> | null = null

function updateClock() {
  currentTime.value = new Date().toLocaleString('zh-CN', { hour12: false })
}

function createMetricCard(label: string, value: string, unit: string, tone: string, progress: number) {
  return { label, value, unit, tone, progress: Math.min(Math.max(progress, 0), 1) }
}

function ratio(value: number | null | undefined, min: number, max: number) {
  if (value === undefined || value === null || Number.isNaN(value) || max <= min) {
    return 0
  }
  return (value - min) / (max - min)
}

const heroStatusCards = computed(() => [
  { label: '后端服务', value: store.backendOnline ? '在线' : '离线', tone: store.backendOnline ? 'ok' : 'danger' },
  { label: '数据存储', value: store.health?.storage ?? '--', tone: 'neutral' },
  { label: '数据来源', value: store.realtime?.dataSource ?? '--', tone: 'accent' },
  { label: '最近更新时间', value: formatDateTime(store.realtime?.updatedAt), tone: 'neutral' },
])

const environmentCards = computed(() => {
  const realtime = store.realtime
  if (!realtime) {
    return [
      createMetricCard('空气温度', '--', '°C', 'warm', 0.55),
      createMetricCard('空气湿度', '--', '%', 'cool', 0.4),
      createMetricCard('土壤湿度', '--', '%', 'green', 0.35),
      createMetricCard('光照强度', '--', 'lx', 'gold', 0.6),
      createMetricCard('CO2 浓度', '--', 'ppm', 'violet', 0.3),
      createMetricCard('画面亮度', '--', '', 'cyan', 0.45),
    ]
  }

  return [
    createMetricCard('空气温度', formatNumber(realtime.env.temperature), '°C', 'warm', ratio(realtime.env.temperature, 0, 40)),
    createMetricCard('空气湿度', formatNumber(realtime.env.humidity), '%', 'cool', ratio(realtime.env.humidity, 0, 100)),
    createMetricCard('土壤湿度', formatNumber(realtime.env.soil), '%', 'green', ratio(realtime.env.soil, 0, 100)),
    createMetricCard('光照强度', formatInteger(realtime.env.light), 'lx', 'gold', ratio(realtime.env.light, 0, 3000)),
    createMetricCard('CO2 浓度', formatInteger(realtime.env.co2), 'ppm', 'violet', ratio(realtime.env.co2, 0, 2000)),
    createMetricCard('画面亮度', formatNumber(realtime.vision.brightness), '', 'cyan', ratio(realtime.vision.brightness, 0, 255)),
  ]
})

const visionEntries = computed(() => [
  { label: '视觉状态', value: store.realtime?.vision.visionOk ? '正常' : '异常', tone: store.realtime?.vision.visionOk ? 'ok' : 'danger' },
  { label: '运动检测', value: store.realtime?.vision.motionFlag ? '检测到运动' : '无明显运动', tone: store.realtime?.vision.motionFlag ? 'warning' : 'neutral' },
  { label: '绿植比例', value: formatNumber(store.realtime?.vision.greenRatio, 3), tone: 'ok' },
])

const presenceEntries = computed(() => {
  const source = store.presence ?? store.realtime?.presence
  if (!source) {
    return []
  }

  return [
    { label: '采集节点 1', value: source.sensor1 },
    { label: '采集节点 2', value: source.sensor2 },
    { label: '采集节点 3', value: source.sensor3 },
    { label: '执行节点 4', value: source.executor4 },
    { label: '执行节点 5', value: source.executor5 },
  ]
})

const actuatorEntries = computed(() => {
  const decision = store.realtime?.decision
  if (!decision) {
    return [
      { label: '风扇', value: '--', tone: 'neutral' },
      { label: '灌溉器', value: '--', tone: 'neutral' },
      { label: '补光灯', value: '--', tone: 'neutral' },
    ]
  }

  const normalize = (value: string) => value.toUpperCase().includes('ON')
  return [
    { label: '风扇', value: decision.fan, tone: normalize(decision.fan) ? 'ok' : 'neutral' },
    { label: '灌溉器', value: decision.pump, tone: normalize(decision.pump) ? 'warning' : 'neutral' },
    { label: '补光灯', value: decision.led, tone: normalize(decision.led) ? 'accent' : 'neutral' },
  ]
})

async function refresh() {
  await store.loadOverview()
}

onMounted(() => {
  updateClock()
  clockTimer = setInterval(updateClock, 1000)
  void refresh()
  refreshTimer = setInterval(() => {
    void refresh()
  }, 5000)
})

onBeforeUnmount(() => {
  if (clockTimer) {
    clearInterval(clockTimer)
    clockTimer = null
  }
  if (refreshTimer) {
    clearInterval(refreshTimer)
    refreshTimer = null
  }
})
</script>

<template>
  <main class="page-shell">
    <section class="hero-panel">
      <div class="hero-copy">
        <p class="eyebrow">GREENHOUSE COMMAND CENTER</p>
        <h1>智慧农业大棚监控中心</h1>
        <p class="hero-text">聚焦实时环境指标、设备在线与执行器状态，形成分页面的运行态指挥舱。</p>

        <div class="hero-actions">
          <button class="primary-btn" :disabled="store.overviewLoading" @click="refresh">
            {{ store.overviewLoading ? '正在刷新...' : '刷新总览' }}
          </button>
          <RouterLink class="secondary-btn link-btn" :to="{ name: 'screen' }">大屏总览</RouterLink>
          <RouterLink class="ghost-btn link-btn" :to="{ name: 'trends' }">趋势与历史</RouterLink>
          <RouterLink class="ghost-btn link-btn" :to="{ name: 'mqtt' }">通信监控</RouterLink>
        </div>
      </div>

      <div class="hero-aside">
        <div class="time-card">
          <span class="time-label">指挥舱时间</span>
          <strong>{{ currentTime }}</strong>
          <small>{{ store.health?.service ?? 'greenhouse-web' }}</small>
        </div>

        <div class="hero-status">
          <article v-for="card in heroStatusCards" :key="card.label" class="status-card" :data-tone="card.tone">
            <span>{{ card.label }}</span>
            <strong>{{ card.value }}</strong>
          </article>
        </div>
      </div>
    </section>

    <section v-if="store.errorMessage" class="banner error-banner">后端请求失败：{{ store.errorMessage }}</section>

    <section v-if="!store.realtime && !store.overviewLoading && !store.errorMessage" class="banner info-banner">
      当前数据库暂无记录。你可以先向 `/api/telemetry` 写入数据，或到“通信监控”页连接 MQTT 等待消息入库。
    </section>

    <section class="panel metrics-panel">
      <header class="panel-header">
        <div>
          <p class="section-tag">REALTIME</p>
          <h2>实时环境核心指标</h2>
        </div>
      </header>

      <div class="metric-grid">
        <article v-for="card in environmentCards" :key="card.label" class="metric-card" :data-tone="card.tone">
          <span class="metric-label">{{ card.label }}</span>
          <strong>{{ card.value }}</strong>
          <small>{{ card.unit || '实时值' }}</small>
          <div class="metric-track">
            <div class="metric-fill" :style="{ width: `${card.progress * 100}%` }"></div>
          </div>
        </article>
      </div>
    </section>

    <section class="content-grid status-grid">
      <section class="panel side-panel">
        <header class="panel-header">
          <div>
            <p class="section-tag">VISION</p>
            <h2>视觉识别状态</h2>
          </div>
        </header>

        <dl class="decision-list">
          <div v-for="item in visionEntries" :key="item.label" :data-tone="item.tone">
            <dt>{{ item.label }}</dt>
            <dd>{{ item.value }}</dd>
          </div>
        </dl>
      </section>

      <section class="panel side-panel">
        <header class="panel-header">
          <div>
            <p class="section-tag">PRESENCE</p>
            <h2>设备在线状态</h2>
          </div>
          <span class="sample-chip">{{ formatDateTime(store.presence?.latestRecordAt) }}</span>
        </header>

        <ul class="presence-list">
          <li v-for="entry in presenceEntries" :key="entry.label">
            <span>{{ entry.label }}</span>
            <strong>{{ entry.value }}</strong>
          </li>
        </ul>
      </section>
    </section>

    <section class="panel">
      <header class="panel-header">
        <div>
          <p class="section-tag">ACTUATOR</p>
          <h2>执行器状态（与上传数据一致）</h2>
        </div>
        <span class="sample-chip">{{ formatDateTime(store.realtime?.updatedAt) }}</span>
      </header>

      <ul class="presence-list">
        <li v-for="entry in actuatorEntries" :key="entry.label" :data-tone="entry.tone">
          <span>{{ entry.label }}</span>
          <strong>{{ entry.value }}</strong>
        </li>
      </ul>
    </section>
  </main>
</template>

<style scoped>
.hero-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.link-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  text-decoration: none;
}

.presence-list li[data-tone='ok'] strong {
  color: #83ffbf;
}

.presence-list li[data-tone='warning'] strong {
  color: #ffd57f;
}

.presence-list li[data-tone='accent'] strong {
  color: #78ceff;
}
</style>
