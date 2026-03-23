<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import mqtt, { type IClientOptions, type MqttClient } from 'mqtt'
import { Line } from 'vue-chartjs'
import {
  CategoryScale,
  Chart as ChartJS,
  Filler,
  Legend,
  LineElement,
  LinearScale,
  PointElement,
  Title,
  Tooltip,
} from 'chart.js'

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend, Filler)

type HealthResponse = {
  status: string
  storage: string
  service: string
}

type RealtimeResponse = {
  hasData: boolean
  summary: string
  dataSource: string
  updatedAt: string
  env: {
    temperature: number
    humidity: number
    soil: number
    light: number
    co2: number
  }
  vision: {
    brightness: number
    greenRatio: number
    motionFlag: boolean
    visionOk: boolean
  }
  presence: {
    sensor1: string
    sensor2: string
    sensor3: string
    executor4: string
    executor5: string
  }
}

type HistoryRecord = {
  createdAt: string
  source: string
  temperature: number
  humidity: number
  soil: number
  light: number
  co2: number
  brightness: number
  greenRatio: number
  motionFlag: boolean
  visionOk: boolean
}

type HistoryResponse = {
  start: string
  end: string
  count: number
  records: HistoryRecord[]
}

type MetricStats = {
  average: number
  max: number
  min: number
  latest: number
}

type StatsResponse = {
  sampleCount: number
  temperature: MetricStats
  humidity: MetricStats
  soil: MetricStats
  light: MetricStats
  co2: MetricStats
}

type PresenceResponse = {
  sensor1: string
  sensor2: string
  sensor3: string
  executor4: string
  executor5: string
  latestRecordAt: string | null
}

type MqttForm = {
  name: string
  protocol: 'ws' | 'wss'
  host: string
  port: number
  path: string
  clientId: string
  username: string
  password: string
  useTls: boolean
  topic: string
  qos: 0 | 1 | 2
}

type MqttLogEntry = {
  id: number
  time: string
  level: 'info' | 'success' | 'warning' | 'error'
  message: string
}

ChartJS.defaults.color = '#a7bed3'
ChartJS.defaults.borderColor = 'rgba(84, 120, 153, 0.22)'

const selectedHours = ref(24)
const loading = ref(false)
const backendOnline = ref(false)
const health = ref<HealthResponse | null>(null)
const realtime = ref<RealtimeResponse | null>(null)
const history = ref<HistoryResponse | null>(null)
const stats = ref<StatsResponse | null>(null)
const presence = ref<PresenceResponse | null>(null)
const errorMessage = ref('')
const currentTime = ref('')

const showMqttDialog = ref(false)
const mqttStatus = ref<'idle' | 'connecting' | 'connected' | 'error' | 'disconnected'>('idle')
const mqttLogs = ref<MqttLogEntry[]>([])
const mqttLastMessage = ref('')
const mqttLastMessageAt = ref('')
const mqttCurrentUrl = ref('')
const mqttClient = ref<MqttClient | null>(null)
const mqttLogSeed = ref(0)
let clockTimer: ReturnType<typeof setInterval> | null = null

const mqttForm = ref<MqttForm>({
  name: 'smart-greenhouse',
  protocol: 'ws',
  host: 'broker.emqx.io',
  port: 8083,
  path: '/mqtt',
  clientId: createClientId(),
  username: '',
  password: '',
  useTls: false,
  topic: 'smart-greenhouse/node1/state',
  qos: 0,
})

const rangeOptions = [
  { label: '最近 1 小时', value: 1 },
  { label: '最近 6 小时', value: 6 },
  { label: '最近 24 小时', value: 24 },
  { label: '最近 7 天', value: 168 },
]

const mqttStatusText = computed(() => {
  const labelMap = {
    idle: '未连接',
    connecting: '连接中',
    connected: '已连接',
    error: '连接异常',
    disconnected: '已断开',
  } as const

  return labelMap[mqttStatus.value]
})

const heroStatusCards = computed(() => [
  { label: '后端服务', value: backendOnline.value ? '在线' : '离线', tone: backendOnline.value ? 'ok' : 'danger' },
  { label: '数据存储', value: health.value?.storage ?? '--', tone: 'neutral' },
  { label: '数据来源', value: realtime.value?.dataSource ?? '--', tone: 'accent' },
  { label: '最近更新时间', value: formatDateTime(realtime.value?.updatedAt), tone: 'neutral' },
])

function createMetricCard(label: string, value: string, unit: string, tone: string, progress: number) {
  return { label, value, unit, tone, progress: Math.min(Math.max(progress, 0), 1) }
}

function ratio(value: number | null | undefined, min: number, max: number) {
  if (value === undefined || value === null || Number.isNaN(value) || max <= min) {
    return 0
  }
  return (value - min) / (max - min)
}

const environmentCards = computed(() => {
  if (!realtime.value) {
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
    createMetricCard('空气温度', formatNumber(realtime.value.env.temperature), '°C', 'warm', ratio(realtime.value.env.temperature, 0, 40)),
    createMetricCard('空气湿度', formatNumber(realtime.value.env.humidity), '%', 'cool', ratio(realtime.value.env.humidity, 0, 100)),
    createMetricCard('土壤湿度', formatNumber(realtime.value.env.soil), '%', 'green', ratio(realtime.value.env.soil, 0, 100)),
    createMetricCard('光照强度', formatInteger(realtime.value.env.light), 'lx', 'gold', ratio(realtime.value.env.light, 0, 3000)),
    createMetricCard('CO2 浓度', formatInteger(realtime.value.env.co2), 'ppm', 'violet', ratio(realtime.value.env.co2, 0, 2000)),
    createMetricCard('画面亮度', formatNumber(realtime.value.vision.brightness), '', 'cyan', ratio(realtime.value.vision.brightness, 0, 255)),
  ]
})

const visionEntries = computed(() => {
  const isOk = realtime.value?.vision?.visionOk ?? false
  return [
    { label: '视觉状态', value: isOk ? '正常' : '异常', tone: isOk ? 'ok' : 'danger' },
    { label: '运动检测', value: realtime.value?.vision?.motionFlag ? '检测到运动' : '无明显运动', tone: realtime.value?.vision?.motionFlag ? 'warning' : 'neutral' },
    { label: '绿植比例', value: formatNumber(realtime.value?.vision?.greenRatio, 3), tone: 'ok' },
    { label: '画面摘要', value: realtime.value?.summary || '等待实时数据', tone: 'neutral' },
  ]
})

const statsCards = computed(() => {
  if (!stats.value) {
    return []
  }

  return [
    { label: '空气温度', data: stats.value.temperature, unit: '°C' },
    { label: '空气湿度', data: stats.value.humidity, unit: '%' },
    { label: '土壤湿度', data: stats.value.soil, unit: '%' },
    { label: '光照强度', data: stats.value.light, unit: 'lx' },
    { label: 'CO2 浓度', data: stats.value.co2, unit: 'ppm' },
  ]
})

const presenceEntries = computed(() => {
  const source = presence.value ?? realtime.value?.presence
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

const chartLabels = computed(() => (history.value?.records ?? []).map((record) => formatTime(record.createdAt)).reverse())

const chartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  plugins: {
    legend: {
      position: 'top' as const,
      labels: { color: '#cde2f6' },
    },
  },
  scales: {
    x: {
      ticks: { color: '#8aa6c2' },
      grid: { color: 'rgba(91, 122, 150, 0.14)' },
    },
    y: {
      beginAtZero: false,
      ticks: { color: '#8aa6c2' },
      grid: { color: 'rgba(91, 122, 150, 0.14)' },
    },
  },
}

const tempHumidityChartData = computed(() => ({
  labels: chartLabels.value,
  datasets: [
    { label: '空气温度 (°C)', data: (history.value?.records ?? []).map((record) => record.temperature).reverse(), borderColor: '#ff8c5c', backgroundColor: 'rgba(255,140,92,0.16)', fill: true, tension: 0.35 },
    { label: '空气湿度 (%)', data: (history.value?.records ?? []).map((record) => record.humidity).reverse(), borderColor: '#5de2ff', backgroundColor: 'rgba(93,226,255,0.12)', fill: true, tension: 0.35 },
    { label: '土壤湿度 (%)', data: (history.value?.records ?? []).map((record) => record.soil).reverse(), borderColor: '#78f7a8', backgroundColor: 'rgba(120,247,168,0.12)', fill: true, tension: 0.35 },
  ],
}))

const lightCo2ChartData = computed(() => ({
  labels: chartLabels.value,
  datasets: [
    { label: '光照强度 (lx)', data: (history.value?.records ?? []).map((record) => record.light).reverse(), borderColor: '#f5de6f', backgroundColor: 'rgba(245,222,111,0.14)', fill: true, tension: 0.35 },
    { label: 'CO2 浓度 (ppm)', data: (history.value?.records ?? []).map((record) => record.co2).reverse(), borderColor: '#b890ff', backgroundColor: 'rgba(184,144,255,0.12)', fill: true, tension: 0.35 },
  ],
}))

async function fetchJson<T>(url: string): Promise<T | null> {
  const response = await fetch(url)
  if (response.status === 204) {
    return null
  }
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}`)
  }
  return (await response.json()) as T
}

async function loadDashboard() {
  loading.value = true
  errorMessage.value = ''

  try {
    const [healthData, realtimeData, historyData, statsData, presenceData] = await Promise.all([
      fetchJson<HealthResponse>('/api/health'),
      fetchJson<RealtimeResponse>('/api/realtime'),
      fetchJson<HistoryResponse>(`/api/history?hours=${selectedHours.value}`),
      fetchJson<StatsResponse>(`/api/stats?hours=${selectedHours.value}`),
      fetchJson<PresenceResponse>('/api/presence'),
    ])

    health.value = healthData
    realtime.value = realtimeData
    history.value = historyData
    stats.value = statsData
    presence.value = presenceData
    backendOnline.value = true
  } catch (error) {
    backendOnline.value = false
    errorMessage.value = error instanceof Error ? error.message : '请求失败'
  } finally {
    loading.value = false
  }
}

function createClientId() {
  return `mqttx_${Math.random().toString(16).slice(2, 10)}`
}

function createTimestamp() {
  return new Date().toLocaleTimeString('zh-CN', { hour12: false })
}

function updateClock() {
  currentTime.value = new Date().toLocaleString('zh-CN', { hour12: false })
}

function formatDateTime(value?: string | null) {
  if (!value) {
    return '暂无记录'
  }
  return new Date(value).toLocaleString('zh-CN', { hour12: false })
}

function formatTime(value?: string | null) {
  if (!value) {
    return '--:--:--'
  }
  return new Date(value).toLocaleTimeString('zh-CN', { hour12: false })
}

function formatNumber(value?: number | null, digits = 1) {
  if (value === undefined || value === null || Number.isNaN(value)) {
    return '--'
  }
  return value.toFixed(digits)
}

function formatInteger(value?: number | null) {
  if (value === undefined || value === null || Number.isNaN(value)) {
    return '--'
  }
  return Math.round(value).toString()
}

function pushMqttLog(message: string, level: MqttLogEntry['level'] = 'info') {
  mqttLogSeed.value += 1
  mqttLogs.value = [...mqttLogs.value, { id: mqttLogSeed.value, time: createTimestamp(), level, message }].slice(-120)
}

function clearMqttLogs() {
  mqttLogs.value = []
  pushMqttLog('日志已清空', 'info')
}

function regenerateClientId() {
  mqttForm.value.clientId = createClientId()
}

function saveMqttConfigAndConnect() {
  showMqttDialog.value = false
  connectMqtt()
}

function buildBrokerUrl() {
  const protocol = mqttForm.value.useTls ? 'wss' : mqttForm.value.protocol
  const path = mqttForm.value.path.startsWith('/') ? mqttForm.value.path : `/${mqttForm.value.path}`
  return `${protocol}://${mqttForm.value.host}:${mqttForm.value.port}${path}`
}

function getMqttOptions(): IClientOptions {
  return {
    clientId: mqttForm.value.clientId.trim(),
    username: mqttForm.value.username.trim() || undefined,
    password: mqttForm.value.password || undefined,
    reconnectPeriod: 0,
    connectTimeout: 8_000,
    clean: true,
  }
}

function handleIncomingMessage(topic: string, payload: Uint8Array) {
  const text = new TextDecoder().decode(payload)
  mqttLastMessage.value = text
  mqttLastMessageAt.value = new Date().toISOString()
  pushMqttLog(`收到消息 ${topic}: ${text}`, 'success')

  try {
    const parsed = JSON.parse(text) as Record<string, unknown>
    const env = parsed.env as Record<string, unknown> | undefined
    const vision = parsed.vision as Record<string, unknown> | undefined
    const now = new Date()
    const beijingTime = new Date(now.getTime() + 8 * 60 * 60 * 1000)
    const formattedTime = beijingTime.toISOString().slice(0, 19).replace('T', ' ')
    const request = {
      nodeId: (parsed.node_id as number) ?? 1,
      createdAt: formattedTime,
      temperature: env?.temp ?? env?.temperature ?? null,
      humidity: env?.hum ?? env?.humidity ?? null,
      soil: env?.soil ?? env?.soilMoisture ?? null,
      light: env?.light ?? null,
      co2: env?.co2 ?? null,
      brightness: vision?.brightness ?? null,
      greenRatio: vision?.green_ratio ?? vision?.greenRatio ?? null,
      motionFlag: vision?.motion_flag ?? vision?.motionFlag ?? null,
      visionOk: vision?.vision_ok ?? vision?.visionOk ?? null,
      source: 'mqtt',
    }

    void (async () => {
      try {
        const response = await fetch('/api/telemetry', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(request),
        })
        if (response.ok) {
          pushMqttLog('数据已同步写入数据库', 'success')
          setTimeout(() => {
            void loadDashboard()
          }, 500)
        }
      } catch (error) {
        pushMqttLog(`数据写入失败: ${error instanceof Error ? error.message : String(error)}`, 'error')
      }
    })()
  } catch {
    pushMqttLog('消息不是 JSON，保留原始内容展示', 'warning')
  }
}

function disconnectMqtt(manual = true) {
  const client = mqttClient.value
  if (!client) {
    if (manual) {
      mqttStatus.value = 'disconnected'
      pushMqttLog('当前没有活动中的 MQTT 连接', 'warning')
    }
    return
  }

  client.removeAllListeners()
  client.end(true)
  mqttClient.value = null
  mqttStatus.value = 'disconnected'
  if (manual) {
    pushMqttLog('已手动断开 MQTT 连接', 'warning')
  }
}

function connectMqtt() {
  disconnectMqtt(false)
  const brokerUrl = buildBrokerUrl()
  mqttCurrentUrl.value = brokerUrl
  mqttStatus.value = 'connecting'
  pushMqttLog(`开始连接 ${brokerUrl}`, 'info')

  const client = mqtt.connect(brokerUrl, getMqttOptions())
  mqttClient.value = client

  client.on('connect', () => {
    mqttStatus.value = 'connected'
    pushMqttLog(`连接成功 [${mqttForm.value.clientId}] ${mqttForm.value.topic} -> ${brokerUrl}`, 'success')
    client.subscribe(mqttForm.value.topic, { qos: mqttForm.value.qos }, (error) => {
      if (error) {
        mqttStatus.value = 'error'
        pushMqttLog(`订阅失败: ${error.message}`, 'error')
        return
      }
      pushMqttLog(`订阅成功 [QoS ${mqttForm.value.qos}]`, 'success')
    })
  })

  client.on('message', (topic, payload) => {
    handleIncomingMessage(topic, payload)
  })

  client.on('reconnect', () => {
    mqttStatus.value = 'connecting'
    pushMqttLog('正在尝试重连 MQTT Broker', 'warning')
  })

  client.on('close', () => {
    if (mqttStatus.value !== 'disconnected') {
      mqttStatus.value = 'disconnected'
      pushMqttLog('连接已关闭', 'warning')
    }
  })

  client.on('error', (error) => {
    mqttStatus.value = 'error'
    pushMqttLog(`连接异常: ${error.message}`, 'error')
  })
}

onMounted(() => {
  updateClock()
  clockTimer = setInterval(updateClock, 1000)
  void loadDashboard()
  pushMqttLog('MQTT 面板已就绪，等待手动连接', 'info')
})

onBeforeUnmount(() => {
  disconnectMqtt(false)
  if (clockTimer) {
    clearInterval(clockTimer)
    clockTimer = null
  }
})

defineExpose({ handleIncomingMessage })
</script>

<template>
  <main class="page-shell">
    <section class="hero-panel">
      <div class="hero-copy">
        <p class="eyebrow">GREENHOUSE COMMAND CENTER</p>
        <h1>智慧农业大棚监控中心</h1>
        <p class="hero-text">
          聚焦实时环境指标、设备在线状态与 MQTT 通信链路，让大棚运行状态在一个页面里完成监看与判断。
        </p>

        <div class="hero-actions">
          <button class="primary-btn" :disabled="loading" @click="loadDashboard">
            {{ loading ? '正在刷新...' : '刷新总览' }}
          </button>
          <button class="secondary-btn" type="button" @click="showMqttDialog = true">编辑 MQTT 配置</button>
        </div>
      </div>

      <div class="hero-aside">
        <div class="time-card">
          <span class="time-label">指挥舱时间</span>
          <strong>{{ currentTime }}</strong>
          <small>{{ mqttCurrentUrl || '链路待配置' }}</small>
        </div>

        <div class="hero-status">
          <article
            v-for="card in heroStatusCards"
            :key="card.label"
            class="status-card"
            :data-tone="card.tone"
          >
            <span>{{ card.label }}</span>
            <strong>{{ card.value }}</strong>
          </article>
        </div>
      </div>
    </section>

    <section v-if="errorMessage" class="banner error-banner">后端请求失败：{{ errorMessage }}</section>

    <section v-if="!realtime && !loading && !errorMessage" class="banner info-banner">
      当前数据库暂无记录。你可以先向 `/api/telemetry` 写入数据，或连接 MQTT Broker 等待消息入库。
    </section>

    <section class="panel metrics-panel">
      <header class="panel-header">
        <div>
          <p class="section-tag">REALTIME</p>
          <h2>实时环境核心指标</h2>
        </div>
        <span class="sample-chip">窗口范围：{{ selectedHours }} 小时</span>
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
            <p class="section-tag">DEVICE</p>
            <h2>设备在线状态</h2>
          </div>
          <span class="sample-chip">{{ formatDateTime(presence?.latestRecordAt) }}</span>
        </header>

        <ul class="presence-list">
          <li v-for="entry in presenceEntries" :key="entry.label">
            <span>{{ entry.label }}</span>
            <strong>{{ entry.value }}</strong>
          </li>
        </ul>
      </section>
    </section>

    <section class="content-grid history-layout">
      <section class="panel">
        <header class="panel-header history-header">
          <div>
            <p class="section-tag">TREND</p>
            <h2>趋势分析与历史记录</h2>
          </div>
          <div class="controls">
            <select v-model="selectedHours" :disabled="loading">
              <option v-for="option in rangeOptions" :key="option.value" :value="option.value">
                {{ option.label }}
              </option>
            </select>
            <button class="secondary-btn" :disabled="loading" @click="loadDashboard">刷新统计</button>
          </div>
        </header>

        <div v-if="history?.records.length" class="table-wrap">
          <table>
            <thead>
              <tr>
                <th>时间</th>
                <th>来源</th>
                <th>温度</th>
                <th>湿度</th>
                <th>土壤</th>
                <th>光照</th>
                <th>CO2</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="record in history.records" :key="record.createdAt">
                <td>{{ formatDateTime(record.createdAt) }}</td>
                <td>{{ record.source }}</td>
                <td>{{ formatNumber(record.temperature) }}</td>
                <td>{{ formatNumber(record.humidity) }}</td>
                <td>{{ formatNumber(record.soil) }}</td>
                <td>{{ formatInteger(record.light) }}</td>
                <td>{{ formatInteger(record.co2) }}</td>
              </tr>
            </tbody>
          </table>
        </div>

        <div v-if="history?.records.length" class="charts-wrap">
          <div class="chart-block">
            <p class="chart-title">温度 / 湿度 / 土壤湿度</p>
            <Line :data="tempHumidityChartData" :options="chartOptions" />
          </div>
          <div class="chart-block">
            <p class="chart-title">光照 / CO2</p>
            <Line :data="lightCo2ChartData" :options="chartOptions" />
          </div>
        </div>
        <div v-else class="empty-state">当前时间范围内暂无历史记录。</div>
      </section>

      <section class="panel">
        <header class="panel-header">
          <div>
            <p class="section-tag">STATS</p>
            <h2>统计概览</h2>
          </div>
          <span class="sample-chip">样本数：{{ stats?.sampleCount ?? 0 }}</span>
        </header>

        <div class="stats-grid">
          <article v-for="item in statsCards" :key="item.label" class="stats-card">
            <span>{{ item.label }}</span>
            <strong>{{ formatNumber(item.data.latest) }} {{ item.unit }}</strong>
            <dl>
              <div>
                <dt>平均值</dt>
                <dd>{{ formatNumber(item.data.average) }} {{ item.unit }}</dd>
              </div>
              <div>
                <dt>最大值</dt>
                <dd>{{ formatNumber(item.data.max) }} {{ item.unit }}</dd>
              </div>
              <div>
                <dt>最小值</dt>
                <dd>{{ formatNumber(item.data.min) }} {{ item.unit }}</dd>
              </div>
            </dl>
          </article>
        </div>
      </section>
    </section>

    <section class="content-grid mqtt-layout">
      <section class="panel mqtt-panel">
        <header class="panel-header">
          <div>
            <p class="section-tag">MQTT</p>
            <h2>通信链路监控</h2>
          </div>
          <span class="mqtt-badge" :data-state="mqttStatus">{{ mqttStatusText }}</span>
        </header>

        <div class="mqtt-actions">
          <button class="primary-btn" type="button" :disabled="mqttStatus === 'connecting'" @click="connectMqtt">
            {{ mqttStatus === 'connected' ? '重新连接 Broker' : '连接 Broker' }}
          </button>
          <button class="secondary-btn" type="button" @click="disconnectMqtt()">断开连接</button>
          <button class="ghost-btn" type="button" @click="showMqttDialog = true">编辑 MQTT 配置</button>
          <button class="ghost-btn" type="button" @click="clearMqttLogs">清空日志</button>
        </div>

        <div class="mqtt-info-strip">
          <div>
            <span class="info-label">连接名称</span>
            <strong>{{ mqttForm.name }}</strong>
          </div>
          <div>
            <span class="info-label">QoS</span>
            <strong>{{ mqttForm.qos }}</strong>
          </div>
          <div>
            <span class="info-label">SSL/TLS</span>
            <strong>{{ mqttForm.useTls ? '开启' : '关闭' }}</strong>
          </div>
        </div>

        <div class="mqtt-log-layout">
          <section class="log-card">
            <header class="subpanel-header">
              <span>连接日志</span>
              <small>{{ mqttLogs.length }} 条</small>
            </header>
            <div class="log-list" data-testid="mqtt-log-list">
              <div v-for="item in mqttLogs" :key="item.id" class="log-item" :data-level="item.level">
                <span>[{{ item.time }}]</span>
                <p>{{ item.message }}</p>
              </div>
            </div>
          </section>
        </div>

        <section class="log-card mqtt-message-card">
          <header class="subpanel-header">
            <span>最近消息</span>
            <small>{{ mqttForm.topic }}</small>
          </header>
          <pre class="message-preview">{{ mqttLastMessage || '尚未收到消息' }}</pre>
          <p v-if="mqttLastMessageAt" class="message-time">到达 {{ formatTime(mqttLastMessageAt) }}</p>
        </section>
      </section>
    </section>

    <div v-if="showMqttDialog" class="dialog-backdrop" @click.self="showMqttDialog = false">
      <section class="dialog-card" aria-label="MQTT 连接配置">
        <header class="dialog-header">
          <div>
            <p class="section-tag">MQTT</p>
            <h2>MQTT 连接配置</h2>
          </div>
          <button class="icon-btn" type="button" @click="showMqttDialog = false">×</button>
        </header>

        <div class="dialog-grid">
          <label class="field">
            <span>名称</span>
            <input v-model="mqttForm.name" type="text" />
          </label>

          <div class="field span-2">
            <span>服务器地址</span>
            <div class="inline-grid protocol-grid">
              <select v-model="mqttForm.protocol">
                <option value="ws">ws://</option>
                <option value="wss">wss://</option>
              </select>
              <input v-model="mqttForm.host" type="text" placeholder="broker.emqx.io" />
            </div>
          </div>

          <label class="field">
            <span>端口</span>
            <input v-model.number="mqttForm.port" type="number" min="1" max="65535" />
          </label>

          <label class="field">
            <span>路径</span>
            <input v-model="mqttForm.path" type="text" placeholder="/mqtt" />
          </label>

          <div class="field span-2">
            <span>Client ID</span>
            <div class="inline-grid client-grid">
              <input v-model="mqttForm.clientId" type="text" />
              <button class="ghost-btn small-btn" type="button" @click="regenerateClientId">重新生成</button>
            </div>
          </div>

          <label class="field">
            <span>用户名</span>
            <input v-model="mqttForm.username" type="text" />
          </label>

          <label class="field">
            <span>密码</span>
            <input v-model="mqttForm.password" type="password" />
          </label>

          <label class="field">
            <span>Topic</span>
            <input v-model="mqttForm.topic" type="text" />
          </label>

          <label class="field">
            <span>QoS</span>
            <select v-model.number="mqttForm.qos">
              <option :value="0">0</option>
              <option :value="1">1</option>
              <option :value="2">2</option>
            </select>
          </label>

          <label class="field switch-field span-2">
            <span>启用 SSL/TLS</span>
            <input v-model="mqttForm.useTls" type="checkbox" />
          </label>
        </div>

        <div class="dialog-note">
          浏览器只能使用 WebSocket 方式连接 MQTT，因此这里填写的是 `ws://` 或 `wss://` 地址。
        </div>

        <footer class="dialog-footer">
          <button class="ghost-btn" type="button" @click="showMqttDialog = false">取消</button>
          <button class="primary-btn" type="button" @click="saveMqttConfigAndConnect">保存并连接</button>
        </footer>
      </section>
    </div>
  </main>
</template>

<style>
* {
  box-sizing: border-box;
}

body {
  margin: 0;
  font-family: 'Microsoft YaHei', 'PingFang SC', 'Segoe UI', sans-serif;
  color: #e4f1ff;
  background:
    radial-gradient(circle at top left, rgba(24, 189, 255, 0.22), transparent 26%),
    radial-gradient(circle at top right, rgba(49, 255, 169, 0.12), transparent 20%),
    linear-gradient(180deg, #050b14 0%, #07111d 38%, #091724 100%);
}

body::before {
  content: '';
  position: fixed;
  inset: 0;
  pointer-events: none;
  background-image:
    linear-gradient(rgba(69, 105, 136, 0.08) 1px, transparent 1px),
    linear-gradient(90deg, rgba(69, 105, 136, 0.08) 1px, transparent 1px);
  background-size: 48px 48px;
  mask-image: linear-gradient(180deg, rgba(0, 0, 0, 0.58), transparent);
}

#app {
  min-height: 100vh;
}
</style>

<style scoped>

.page-shell {
  width: min(1440px, calc(100% - 32px));
  margin: 0 auto;
  padding: 24px 0 40px;
  display: grid;
  gap: 18px;
}

.hero-panel,
.panel,
.banner,
.dialog-card {
  border: 1px solid rgba(102, 154, 198, 0.16);
  background: linear-gradient(180deg, rgba(9, 23, 40, 0.92), rgba(7, 16, 31, 0.92));
  box-shadow:
    0 0 0 1px rgba(42, 95, 142, 0.08) inset,
    0 20px 50px rgba(0, 0, 0, 0.34);
  backdrop-filter: blur(14px);
}

.hero-panel {
  border-radius: 32px;
  padding: 30px;
  display: grid;
  grid-template-columns: 1.15fr 0.85fr;
  gap: 24px;
  align-items: start;
}

.hero-copy,
.hero-aside,
.mqtt-panel,
.time-card,
.status-card,
.metric-card,
.stats-card,
.log-card {
  position: relative;
  overflow: hidden;
}

.hero-copy::after,
.panel::before,
.status-card::before,
.metric-card::before,
.log-card::before {
  content: '';
  position: absolute;
  inset: 0;
  pointer-events: none;
  background: linear-gradient(
    135deg,
    rgba(119, 214, 255, 0.08),
    transparent 34%,
    transparent 66%,
    rgba(120, 255, 218, 0.05)
  );
}

.eyebrow,
.section-tag {
  margin: 0 0 10px;
  letter-spacing: 0.24em;
  font-size: 12px;
  font-weight: 700;
  color: #78ceff;
}

h1,
h2 {
  margin: 0;
  font-weight: 700;
}

h1 {
  font-size: clamp(40px, 5vw, 76px);
  line-height: 0.98;
  text-shadow: 0 0 24px rgba(82, 193, 255, 0.18);
}

h2 {
  font-size: clamp(24px, 2vw, 34px);
}

.hero-text {
  max-width: 720px;
  margin: 16px 0 26px;
  color: #90aeca;
  line-height: 1.8;
  font-size: 15px;
}

.hero-actions,
.controls,
.mqtt-actions,
.dialog-footer,
.inline-grid {
  display: flex;
  gap: 12px;
  align-items: center;
}

button,
select,
input {
  border-radius: 16px;
  border: 1px solid rgba(102, 154, 198, 0.18);
  padding: 12px 16px;
  font: inherit;
}

button {
  cursor: pointer;
}

select,
input {
  color: #dbeeff;
  background: rgba(8, 17, 31, 0.88);
}

button:disabled {
  cursor: not-allowed;
  opacity: 0.68;
}

.primary-btn,
.secondary-btn,
.ghost-btn,
.icon-btn {
  transition:
    transform 0.2s ease,
    box-shadow 0.2s ease,
    border-color 0.2s ease,
    background 0.2s ease;
}

.primary-btn:hover,
.secondary-btn:hover,
.ghost-btn:hover,
.icon-btn:hover {
  transform: translateY(-1px);
}

.primary-btn {
  color: #051118;
  border: none;
  background: linear-gradient(135deg, #52d8ff, #73ffbf);
  box-shadow: 0 10px 30px rgba(83, 216, 255, 0.28);
}

.secondary-btn {
  color: #d4efff;
  background: rgba(18, 35, 57, 0.94);
}

.ghost-btn,
.icon-btn {
  color: #9ecdf6;
  background: transparent;
}

.small-btn {
  padding: 10px 14px;
}

.hero-aside {
  display: grid;
  gap: 16px;
}

.time-card {
  padding: 20px 22px;
  border-radius: 24px;
  border: 1px solid rgba(97, 167, 214, 0.18);
  background: linear-gradient(180deg, rgba(8, 19, 35, 0.98), rgba(8, 19, 33, 0.9));
}

.time-label,
.sample-chip,
.info-label {
  display: block;
  color: #7da2c3;
  font-size: 13px;
  letter-spacing: 0.08em;
}

.time-card strong {
  display: block;
  margin-top: 10px;
  font-size: clamp(24px, 2.4vw, 38px);
  color: #ecf8ff;
}

.time-card small {
  display: block;
  margin-top: 10px;
  color: #72b8e4;
  overflow-wrap: anywhere;
}

.hero-status,
.content-grid,
.metric-grid,
.stats-grid {
  display: grid;
  gap: 16px;
}

.hero-status {
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

.status-card,
.metric-card,
.stats-card,
.log-card {
  border-radius: 22px;
  border: 1px solid rgba(103, 152, 190, 0.14);
  background: rgba(8, 17, 31, 0.92);
}

.status-card {
  padding: 18px;
  min-height: 108px;
}

.status-card span,
.metric-label,
.stats-card span {
  display: block;
  color: #82a8c6;
}

.status-card strong {
  display: block;
  margin-top: 12px;
  font-size: 24px;
  line-height: 1.3;
}

.status-card[data-tone='ok'] strong {
  color: #83ffbf;
}

.status-card[data-tone='danger'] strong {
  color: #ff8f8f;
}

.banner {
  border-radius: 22px;
  padding: 16px 20px;
}

.error-banner {
  color: #ffb5b5;
  background: rgba(62, 16, 21, 0.88);
}

.info-banner {
  color: #a8c9e6;
}

.panel {
  border-radius: 28px;
  padding: 24px;
}

.panel-header,
.subpanel-header,
.dialog-header {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  align-items: end;
}

.panel-header {
  margin-bottom: 18px;
}

.metrics-panel {
  animation: rise 0.45s ease;
}

.metric-grid {
  grid-template-columns: repeat(6, minmax(0, 1fr));
}

.metric-card {
  padding: 18px;
  min-height: 176px;
}

.metric-card strong {
  display: block;
  margin-top: 18px;
  font-size: clamp(28px, 2vw, 42px);
  line-height: 1;
  color: #f4fbff;
}

.metric-card small {
  display: block;
  margin-top: 8px;
  color: #6d9fc5;
}

.metric-track {
  height: 8px;
  margin-top: 22px;
  border-radius: 999px;
  overflow: hidden;
  background: rgba(91, 122, 150, 0.18);
}

.metric-fill {
  height: 100%;
  border-radius: inherit;
  background: linear-gradient(90deg, rgba(82, 216, 255, 0.4), #52d8ff);
  box-shadow: 0 0 18px rgba(82, 216, 255, 0.22);
}

.metric-card[data-tone='warm'] strong,
.metric-card[data-tone='warm'] .metric-fill {
  color: #ffb27a;
  background-image: linear-gradient(90deg, rgba(255, 140, 92, 0.32), #ff8c5c);
}

.metric-card[data-tone='cool'] strong,
.metric-card[data-tone='cool'] .metric-fill {
  color: #6de6ff;
  background-image: linear-gradient(90deg, rgba(93, 226, 255, 0.3), #5de2ff);
}

.metric-card[data-tone='green'] strong,
.metric-card[data-tone='green'] .metric-fill {
  color: #78f7a8;
  background-image: linear-gradient(90deg, rgba(120, 247, 168, 0.3), #78f7a8);
}

.metric-card[data-tone='gold'] strong,
.metric-card[data-tone='gold'] .metric-fill {
  color: #f5de6f;
  background-image: linear-gradient(90deg, rgba(245, 222, 111, 0.3), #f5de6f);
}

.metric-card[data-tone='violet'] strong,
.metric-card[data-tone='violet'] .metric-fill {
  color: #c29bff;
  background-image: linear-gradient(90deg, rgba(184, 144, 255, 0.3), #b890ff);
}

.metric-card[data-tone='cyan'] strong,
.metric-card[data-tone='cyan'] .metric-fill {
  color: #7de8e6;
  background-image: linear-gradient(90deg, rgba(125, 232, 230, 0.3), #7de8e6);
}

.content-grid {
  grid-template-columns: 1fr 1fr;
  align-items: start;
}

.history-layout {
  grid-template-columns: 1.2fr 0.8fr;
}

.mqtt-layout {
  grid-template-columns: 1fr;
}

.decision-list,
.presence-list {
  margin: 0;
  padding: 0;
  list-style: none;
}

.decision-list div,
.presence-list li,
.stats-card dl div,
.mqtt-info-strip > div {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  padding: 14px 0;
  border-bottom: 1px solid rgba(74, 112, 146, 0.14);
}

.decision-list div:last-child,
.presence-list li:last-child,
.stats-card dl div:last-child,
.mqtt-info-strip > div:last-child {
  border-bottom: none;
}

.decision-list dt,
.presence-list span,
.stats-card dt {
  color: #7da2c3;
}

.decision-list dd,
.presence-list strong,
.stats-card dd {
  margin: 0;
  color: #eff8ff;
  font-weight: 700;
  text-align: right;
}

.table-wrap {
  overflow-x: auto;
}

table {
  width: 100%;
  border-collapse: collapse;
}

th,
td {
  padding: 12px 10px;
  border-bottom: 1px solid rgba(74, 112, 146, 0.14);
  text-align: left;
  white-space: nowrap;
}

th {
  color: #7da2c3;
  font-size: 13px;
}

.empty-state {
  padding: 34px 0 8px;
  text-align: center;
  color: #7d9dbb;
}

.stats-grid {
  grid-template-columns: 1fr;
}

.stats-card {
  padding: 18px;
}

.stats-card strong {
  display: block;
  margin: 10px 0 16px;
  font-size: 26px;
  color: #f3f8ff;
}

.stats-card dl {
  margin: 0;
}

.mqtt-panel {
  display: grid;
  gap: 18px;
}

.mqtt-badge {
  padding: 8px 14px;
  border-radius: 999px;
  font-size: 13px;
  font-weight: 700;
  letter-spacing: 0.08em;
}

.mqtt-badge[data-state='idle'],
.mqtt-badge[data-state='disconnected'] {
  color: #9cb4c9;
  background: rgba(66, 86, 104, 0.34);
}

.mqtt-badge[data-state='connecting'] {
  color: #ffd57f;
  background: rgba(142, 110, 32, 0.3);
}

.mqtt-badge[data-state='connected'] {
  color: #87ffc2;
  background: rgba(21, 108, 74, 0.36);
  box-shadow: 0 0 18px rgba(91, 255, 179, 0.18);
}

.mqtt-badge[data-state='error'] {
  color: #ff9d9d;
  background: rgba(115, 31, 45, 0.36);
}

.mqtt-info-strip {
  border-radius: 20px;
  padding: 4px 18px;
  background: rgba(6, 14, 27, 0.84);
  border: 1px solid rgba(103, 152, 190, 0.12);
}

.mqtt-info-strip strong {
  color: #e8f6ff;
}

.mqtt-log-layout {
  display: grid;
  grid-template-columns: 1fr;
  gap: 18px;
}

.log-card {
  padding: 18px;
  background:
    linear-gradient(180deg, rgba(8, 19, 35, 0.98), rgba(7, 17, 31, 0.94)),
    rgba(8, 19, 35, 0.92);
}

.subpanel-header {
  margin-bottom: 14px;
}

.subpanel-header span {
  font-weight: 700;
  color: #d9f0ff;
}

.subpanel-header small {
  color: #81a4c4;
}

.log-list {
  height: 280px;
  overflow: auto;
  display: grid;
  gap: 10px;
}

.log-item {
  padding: 12px 14px;
  border-radius: 16px;
  border: 1px solid rgba(103, 152, 190, 0.12);
  background: rgba(10, 20, 34, 0.94);
}

.log-item[data-level='success'] {
  border-color: rgba(120, 247, 168, 0.18);
  background: rgba(10, 34, 28, 0.8);
}

.log-item[data-level='warning'] {
  border-color: rgba(245, 222, 111, 0.18);
  background: rgba(36, 30, 10, 0.78);
}

.log-item[data-level='error'] {
  border-color: rgba(255, 122, 122, 0.18);
  background: rgba(38, 14, 18, 0.8);
}

.log-item span {
  display: block;
  margin-bottom: 6px;
  color: #80a3c0;
  font-size: 13px;
}

.log-item p {
  margin: 0;
  line-height: 1.5;
  color: #eaf7ff;
  overflow-wrap: anywhere;
}

.message-preview {
  margin: 0 0 10px;
  min-height: 160px;
  max-height: 220px;
  overflow: auto;
  padding: 16px 18px;
  border-radius: 18px;
  background: linear-gradient(180deg, rgba(3, 9, 18, 0.98), rgba(6, 15, 26, 0.98));
  color: #dff7df;
  font-family: Consolas, 'SFMono-Regular', monospace;
  white-space: pre-wrap;
  word-break: break-word;
  border: 1px solid rgba(90, 145, 182, 0.14);
}

.message-time {
  margin: 0;
  color: #7fb8d6;
  font-size: 13px;
}

.dialog-backdrop {
  position: fixed;
  inset: 0;
  z-index: 20;
  display: grid;
  place-items: center;
  padding: 24px;
  background: rgba(1, 6, 12, 0.62);
}

.dialog-card {
  width: min(760px, 100%);
  border-radius: 28px;
  padding: 26px;
}

.dialog-header {
  margin-bottom: 20px;
}

.dialog-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.field {
  display: grid;
  gap: 8px;
}

.field span {
  color: #8eb3d1;
  font-size: 14px;
  font-weight: 600;
}

.span-2 {
  grid-column: 1 / -1;
}

.protocol-grid select {
  width: 140px;
}

.protocol-grid input,
.client-grid input {
  flex: 1;
}

.switch-field {
  display: flex;
  align-items: center;
  justify-content: space-between;
  border-radius: 18px;
  padding: 16px 18px;
  border: 1px solid rgba(102, 154, 198, 0.18);
  background: rgba(8, 17, 31, 0.88);
}

.switch-field input {
  width: 20px;
  height: 20px;
  padding: 0;
}

.dialog-note {
  margin-top: 16px;
  color: #86abc9;
  line-height: 1.7;
}

.dialog-footer {
  justify-content: flex-end;
  margin-top: 20px;
}

.icon-btn {
  width: 42px;
  height: 42px;
  padding: 0;
  border-radius: 50%;
  font-size: 28px;
  line-height: 1;
}

.charts-wrap {
  display: grid;
  gap: 18px;
  margin-top: 20px;
}

.chart-block {
  height: 260px;
  padding: 16px;
  border-radius: 22px;
  border: 1px solid rgba(103, 152, 190, 0.12);
  background: rgba(6, 14, 27, 0.82);
}

.chart-title {
  margin: 0 0 8px;
  color: #80a8c8;
  font-size: 13px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

@keyframes rise {
  from {
    opacity: 0;
    transform: translateY(10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@media (max-width: 1200px) {
  .metric-grid {
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }

  .history-layout {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 960px) {
  .hero-panel,
  .content-grid,
  .dialog-grid {
    grid-template-columns: 1fr;
  }

  .hero-status {
    grid-template-columns: 1fr;
  }

  .metric-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .page-shell {
    width: min(100% - 20px, 1440px);
  }

  .hero-panel,
  .panel,
  .dialog-card {
    padding: 20px;
  }

  .hero-actions,
  .mqtt-actions,
  .dialog-footer,
  .controls {
    flex-wrap: wrap;
  }
}

@media (max-width: 640px) {
  .metric-grid {
    grid-template-columns: 1fr;
  }

  h1 {
    font-size: 34px;
  }
}
</style>
