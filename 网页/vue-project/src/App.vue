<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import mqtt, { type IClientOptions, type MqttClient } from 'mqtt'
import { Line } from 'vue-chartjs'
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler,
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

const selectedHours = ref(24)
const loading = ref(false)
const backendOnline = ref(false)
const health = ref<HealthResponse | null>(null)
const realtime = ref<RealtimeResponse | null>(null)
const history = ref<HistoryResponse | null>(null)
const stats = ref<StatsResponse | null>(null)
const presence = ref<PresenceResponse | null>(null)
const errorMessage = ref('')

const showMqttDialog = ref(false)
const mqttStatus = ref<'idle' | 'connecting' | 'connected' | 'error' | 'disconnected'>('idle')
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
const mqttLogs = ref<MqttLogEntry[]>([])
const mqttLastMessage = ref('')
const mqttLastMessageAt = ref('')
const mqttCurrentUrl = ref('')
const mqttClient = ref<MqttClient | null>(null)
const mqttLogSeed = ref(0)

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

function formatDateTime(value?: string | null) {
  if (!value) {
    return '暂无记录'
  }

  return new Date(value).toLocaleString('zh-CN', { hour12: false })
}

function formatTime(value?: string | null) {
  if (!value) return ''
  return new Date(value).toLocaleTimeString('zh-CN', { hour12: false })
}

function formatNumber(value?: number | null, digits = 1) {
  if (value === undefined || value === null || Number.isNaN(value)) {
    return '--'
  }

  return value.toFixed(digits)
}

function pushMqttLog(message: string, level: MqttLogEntry['level'] = 'info') {
  mqttLogSeed.value += 1
  mqttLogs.value = [
    ...mqttLogs.value,
    {
      id: mqttLogSeed.value,
      time: createTimestamp(),
      level,
      message,
    },
  ].slice(-120)
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
    // 生成北京时间字符串 (YYYY-MM-DD HH:mm:ss)
    const now = new Date()
    const beijingTime = new Date(now.getTime() + 8 * 60 * 60 * 1000)
    const formattedTime = beijingTime.toISOString().slice(0, 19).replace('T', ' ')
    // 构建 TelemetryUpsertRequest 并发送到后端
    const request = {
      nodeId: parsed.node_id as number ?? 1,
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
      source: 'mqtt'
    }
    void (async () => {
      try {
        const response = await fetch('/api/telemetry', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(request)
        })
        if (response.ok) {
          pushMqttLog('数据已存储到数据库', 'success')
          // 延迟刷新仪表板，避免频繁刷新
          setTimeout(() => { void loadDashboard() }, 500)
        }
      } catch (e) {
        pushMqttLog(`存储数据失败: ${e instanceof Error ? e.message : String(e)}`, 'error')
      }
    })()
  } catch {
    pushMqttLog('消息不是 JSON，保留原始文本显示', 'warning')
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
    pushMqttLog(`连接成功 [${mqttForm.value.clientId}] ${mqttForm.value.topic} → ${brokerUrl}`, 'success')
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

const environmentCards = computed(() => {
  if (!realtime.value) {
    return []
  }

  return [
    { label: '温度', value: `${formatNumber(realtime.value.env.temperature)} °C` },
    { label: '湿度', value: `${formatNumber(realtime.value.env.humidity)} %` },
    { label: '土壤湿度', value: `${formatNumber(realtime.value.env.soil)} %` },
    { label: '光照', value: `${realtime.value.env.light}` },
    { label: 'CO2', value: `${realtime.value.env.co2} ppm` },
    { label: '亮度', value: `${formatNumber(realtime.value.vision.brightness)}` },
  ]
})

const statsCards = computed(() => {
  if (!stats.value) {
    return []
  }

  return [
    { label: '温度', data: stats.value.temperature, unit: '°C' },
    { label: '湿度', data: stats.value.humidity, unit: '%' },
    { label: '土壤湿度', data: stats.value.soil, unit: '%' },
    { label: '光照', data: stats.value.light, unit: '' },
    { label: 'CO2', data: stats.value.co2, unit: 'ppm' },
  ]
})

const presenceEntries = computed(() => {
  if (!presence.value) {
    return []
  }

  return [
    { label: '采集节点 1', value: presence.value.sensor1 },
    { label: '采集节点 2', value: presence.value.sensor2 },
    { label: '采集节点 3', value: presence.value.sensor3 },
    { label: '执行节点 4', value: presence.value.executor4 },
    { label: '执行节点 5', value: presence.value.executor5 },
  ]
})

onMounted(() => {
  void loadDashboard()
  pushMqttLog('MQTT 面板已就绪，等待手动连接', 'info')
})

onBeforeUnmount(() => {
  disconnectMqtt(false)
})

defineExpose({ handleIncomingMessage })

const chartLabels = computed(() =>
  (history.value?.records ?? []).map((r) => formatDateTime(r.createdAt)).reverse()
)

const chartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  plugins: { legend: { position: 'top' as const } },
  scales: { y: { beginAtZero: false } },
}

const tempHumidityChartData = computed(() => ({
  labels: chartLabels.value,
  datasets: [
    {
      label: '温度 (°C)',
      data: (history.value?.records ?? []).map((r) => r.temperature).reverse(),
      borderColor: '#f97316',
      backgroundColor: 'rgba(249,115,22,0.1)',
      fill: true,
      tension: 0.4,
    },
    {
      label: '湿度 (%)',
      data: (history.value?.records ?? []).map((r) => r.humidity).reverse(),
      borderColor: '#3b82f6',
      backgroundColor: 'rgba(59,130,246,0.1)',
      fill: true,
      tension: 0.4,
    },
    {
      label: '土壤湿度 (%)',
      data: (history.value?.records ?? []).map((r) => r.soil).reverse(),
      borderColor: '#22c55e',
      backgroundColor: 'rgba(34,197,94,0.1)',
      fill: true,
      tension: 0.4,
    },
  ],
}))

const lightCo2ChartData = computed(() => ({
  labels: chartLabels.value,
  datasets: [
    {
      label: '光照',
      data: (history.value?.records ?? []).map((r) => r.light).reverse(),
      borderColor: '#eab308',
      backgroundColor: 'rgba(234,179,8,0.1)',
      fill: true,
      tension: 0.4,
    },
    {
      label: 'CO2 (ppm)',
      data: (history.value?.records ?? []).map((r) => r.co2).reverse(),
      borderColor: '#a855f7',
      backgroundColor: 'rgba(168,85,247,0.1)',
      fill: true,
      tension: 0.4,
    },
  ],
}))
</script>

<template>
  <main class="page-shell">
    <section class="hero-panel">
      <div class="hero-copy">
        <p class="eyebrow">GREENHOUSE COMMAND DECK</p>
        <h1>智慧农业大棚监控舱</h1>
        <p class="hero-text">
          当前页面同时保留后端环境总览，并新增浏览器直连 MQTT Broker 的监测面板，便于直接判断连接状态、订阅情况和原始消息。
        </p>
        <div class="hero-actions">
          <button class="primary-btn" :disabled="loading" @click="loadDashboard">
            {{ loading ? '刷新中...' : '刷新总览' }}
          </button>
          <button class="secondary-btn" type="button" @click="showMqttDialog = true">编辑配置</button>
        </div>
      </div>

      <div class="hero-status">
        <article class="status-card">
          <span>后端服务</span>
          <strong>{{ backendOnline ? '在线' : '离线' }}</strong>
        </article>
        <article class="status-card">
          <span>数据存储</span>
          <strong>{{ health?.storage ?? '--' }}</strong>
        </article>
        <article class="status-card">
          <span>最近更新时间</span>
          <strong>{{ formatDateTime(realtime?.updatedAt) }}</strong>
        </article>
        <article class="status-card">
          <span>数据来源</span>
          <strong>{{ realtime?.dataSource ?? '--' }}</strong>
        </article>
      </div>
    </section>

    <section v-if="errorMessage" class="banner error-banner">后端请求失败：{{ errorMessage }}</section>

    <section v-if="!realtime && !loading && !errorMessage" class="banner info-banner">
      当前数据库暂无记录。你可以先向 `/api/telemetry` 写入数据，或等待后续接入 MQTT 落库。
    </section>

    <section class="content-grid">
      <section class="panel">
        <header class="panel-header">
          <div>
            <p class="section-tag">总览</p>
            <h2>实时环境主读数</h2>
          </div>
        </header>
        <div class="metric-grid">
          <article v-for="card in environmentCards" :key="card.label" class="metric-card">
            <span>{{ card.label }}</span>
            <strong>{{ card.value }}</strong>
          </article>
        </div>
      </section>

      <section class="panel side-panel">
        <header class="panel-header">
          <div>
            <p class="section-tag">视觉</p>
            <h2>视觉状态</h2>
          </div>
        </header>
        <dl class="decision-list">
          <div>
            <dt>视觉状态</dt>
            <dd>{{ realtime?.vision.visionOk ? '正常' : '异常' }}</dd>
          </div>
          <div>
            <dt>运动检测</dt>
            <dd>{{ realtime?.vision.motionFlag ? '有运动' : '无明显运动' }}</dd>
          </div>
          <div>
            <dt>亮度</dt>
            <dd>{{ formatNumber(realtime?.vision.brightness) }}</dd>
          </div>
          <div>
            <dt>绿植比例</dt>
            <dd>{{ formatNumber(realtime?.vision.greenRatio, 3) }}</dd>
          </div>
        </dl>
      </section>
    </section>

    <section class="content-grid history-layout">
      <section class="panel">
        <header class="panel-header history-header">
          <div>
            <p class="section-tag">历史</p>
            <h2>环境趋势分析</h2>
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
              <tr v-for="record in history?.records" :key="record.createdAt">
                <td>{{ formatDateTime(record.createdAt) }}</td>
                <td>{{ record.source }}</td>
                <td>{{ formatNumber(record.temperature) }}</td>
                <td>{{ formatNumber(record.humidity) }}</td>
                <td>{{ formatNumber(record.soil) }}</td>
                <td>{{ record.light }}</td>
                <td>{{ record.co2 }}</td>
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
            <p class="section-tag">统计</p>
            <h2>平均值与区间概览</h2>
          </div>
          <span class="sample-count">样本数：{{ stats?.sampleCount ?? 0 }}</span>
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

    <section class="content-grid">
      <section class="panel mqtt-panel">
        <header class="panel-header">
          <div>
            <p class="section-tag">MQTT</p>
            <h2>MQTT 连接</h2>
          </div>
          <span class="mqtt-badge" :data-state="mqttStatus">{{ mqttStatusText }}</span>
        </header>

        <div class="mqtt-actions">
          <button class="primary-btn" type="button" :disabled="mqttStatus === 'connecting'" @click="connectMqtt">
            {{ mqttStatus === 'connected' ? '重新连接' : '连接 Broker' }}
          </button>
          <button class="secondary-btn" type="button" @click="disconnectMqtt()">断开连接</button>
          <button class="ghost-btn" type="button" @click="showMqttDialog = true">编辑配置</button>
          <button class="ghost-btn" type="button" @click="clearMqttLogs">清空日志</button>
        </div>

        <div class="mqtt-info-strip">
          <div>
            <span class="info-label">名称</span>
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
          <p v-if="mqttLastMessageAt" class="message-time">
            到达 {{ formatTime(mqttLastMessageAt) }}
          </p>
        </section>
      </section>

      <section class="panel side-panel">
        <header class="panel-header">
          <div>
            <p class="section-tag">在线状态</p>
            <h2>节点状态</h2>
          </div>
        </header>
        <ul class="presence-list">
          <li v-for="entry in presenceEntries" :key="entry.label">
            <span>{{ entry.label }}</span>
            <strong>{{ entry.value }}</strong>
          </li>
        </ul>
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
              <input v-model="mqttForm.host" type="text" placeholder="test.mosquitto.org" />
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
              <button class="ghost-btn small-btn" type="button" @click="regenerateClientId">
                重新生成
              </button>
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
            <span>SSL/TLS</span>
            <input v-model="mqttForm.useTls" type="checkbox" />
          </label>
        </div>

        <div class="dialog-note">
          浏览器只能直接使用 WebSocket 连接 MQTT，因此这里填写的是 `ws://` 或 `wss://` 的 Broker 地址。
        </div>

        <footer class="dialog-footer">
          <button class="ghost-btn" type="button" @click="showMqttDialog = false">取消</button>
          <button class="primary-btn" type="button" @click="saveMqttConfigAndConnect">保存并连接</button>
        </footer>
      </section>
    </div>
  </main>
</template>

<style scoped>
:global(*) {
  box-sizing: border-box;
}

:global(body) {
  margin: 0;
  font-family: 'Segoe UI', 'PingFang SC', 'Microsoft YaHei', sans-serif;
  background:
    radial-gradient(circle at top left, rgba(132, 181, 112, 0.28), transparent 24%),
    radial-gradient(circle at top right, rgba(204, 160, 78, 0.2), transparent 22%),
    linear-gradient(135deg, #edf3e2 0%, #f8f6ee 48%, #e7efe1 100%);
  color: #213126;
}

:global(#app) {
  min-height: 100vh;
}

.page-shell {
  width: min(1180px, calc(100% - 48px));
  margin: 32px auto 56px;
  display: grid;
  gap: 24px;
}

.hero-panel,
.panel,
.banner,
.dialog-card {
  background: rgba(255, 252, 244, 0.88);
  border: 1px solid rgba(71, 107, 82, 0.12);
  border-radius: 28px;
  box-shadow: 0 24px 80px rgba(69, 89, 58, 0.12);
  backdrop-filter: blur(18px);
}

.hero-panel {
  padding: 32px;
  display: grid;
  grid-template-columns: 1.2fr 0.8fr;
  gap: 24px;
}

.eyebrow,
.section-tag {
  margin: 0 0 12px;
  color: #39684c;
  letter-spacing: 0.18em;
  font-size: 12px;
  font-weight: 700;
}

h1,
h2 {
  margin: 0;
  font-family: Georgia, 'Times New Roman', serif;
}

h1 {
  font-size: clamp(42px, 5vw, 70px);
  line-height: 0.95;
}

h2 {
  font-size: clamp(30px, 3vw, 42px);
}

.hero-text {
  max-width: 680px;
  margin: 18px 0 24px;
  color: #5c6d62;
  font-size: 17px;
  line-height: 1.7;
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
  border-radius: 18px;
  border: 1px solid rgba(54, 103, 71, 0.16);
  padding: 14px 18px;
  font: inherit;
}

button {
  cursor: pointer;
}

.primary-btn,
.secondary-btn,
.ghost-btn,
.icon-btn {
  transition:
    transform 0.18s ease,
    box-shadow 0.18s ease,
    background 0.18s ease;
}

.primary-btn:hover,
.secondary-btn:hover,
.ghost-btn:hover,
.icon-btn:hover {
  transform: translateY(-1px);
}

.primary-btn {
  background: linear-gradient(135deg, #3b7651, #285c3c);
  color: #f7f8f1;
  border: none;
  box-shadow: 0 12px 24px rgba(45, 89, 60, 0.18);
}

.secondary-btn {
  background: rgba(236, 244, 230, 0.96);
  color: #284b36;
}

.ghost-btn,
.icon-btn {
  background: transparent;
  color: #284b36;
}

.small-btn {
  padding: 12px 16px;
}

.hero-status,
.content-grid,
.metric-grid,
.stats-grid,
.mqtt-summary-grid {
  display: grid;
  gap: 18px;
}

.hero-status {
  grid-template-columns: repeat(2, minmax(0, 1fr));
  align-content: start;
}

.status-card,
.summary-card,
.metric-card,
.stats-card,
.log-card {
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.72);
  border: 1px solid rgba(63, 101, 75, 0.08);
}

.status-card {
  padding: 22px;
  min-height: 110px;
}

.compact-card {
  min-height: auto;
}

.status-card span,
.summary-card span,
.metric-card span,
.stats-card span,
.sample-count,
.info-label {
  display: block;
  color: #6e7b72;
}

.status-card strong {
  display: block;
  margin-top: 12px;
  font-size: 24px;
  line-height: 1.35;
  overflow-wrap: anywhere;
}

.summary-card {
  grid-column: 1 / -1;
  padding: 28px;
}

.summary-card h2 {
  margin: 12px 0 18px;
  color: #2e6947;
}

.summary-card p {
  margin: 6px 0;
  color: #6a785f;
}

.banner {
  padding: 18px 22px;
}

.error-banner {
  color: #8f3c2a;
}

.info-banner {
  color: #52685e;
}

.content-grid {
  grid-template-columns: 1.35fr 0.65fr;
  align-items: start;
}

.history-layout {
  grid-template-columns: 1fr 1fr;
}

.panel {
  padding: 28px;
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

.metric-grid,
.stats-grid,
.mqtt-summary-grid {
  grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
}

.metric-card {
  padding: 18px;
}

.metric-card strong {
  display: block;
  margin-top: 12px;
  font-size: 32px;
  line-height: 1.1;
  color: #274d34;
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
  border-bottom: 1px solid rgba(55, 79, 61, 0.08);
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
  color: #68756c;
}

.decision-list dd,
.presence-list strong,
.stats-card dd {
  margin: 0;
  font-weight: 700;
  color: #254830;
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
  padding: 14px 12px;
  border-bottom: 1px solid rgba(52, 78, 60, 0.08);
  text-align: left;
  white-space: nowrap;
}

th {
  color: #5d6e62;
  font-size: 13px;
}

.empty-state {
  padding: 36px 0 12px;
  color: #6f7b73;
  text-align: center;
}

.stats-card {
  padding: 20px;
}

.stats-card strong {
  display: block;
  margin: 12px 0 18px;
  font-size: 30px;
  color: #244b31;
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
  background: rgba(170, 180, 173, 0.18);
  color: #5f6964;
}

.mqtt-badge[data-state='connecting'] {
  background: rgba(255, 205, 112, 0.28);
  color: #8b5b10;
}

.mqtt-badge[data-state='connected'] {
  background: rgba(94, 184, 114, 0.2);
  color: #246e37;
}

.mqtt-badge[data-state='error'] {
  background: rgba(213, 104, 74, 0.18);
  color: #97331a;
}

.mqtt-info-strip {
  border-radius: 22px;
  padding: 4px 20px;
  background: rgba(255, 255, 255, 0.65);
  border: 1px solid rgba(63, 101, 75, 0.08);
}

.mqtt-info-strip strong {
  color: #254830;
}

.mqtt-log-layout {
  display: grid;
  grid-template-columns: minmax(0, 1fr);
  gap: 18px;
}

.log-card {
  padding: 18px;
}

.mqtt-message-card {
  border: 1px solid rgba(26, 64, 44, 0.1);
  background:
    linear-gradient(180deg, rgba(248, 252, 246, 0.96), rgba(238, 246, 239, 0.92)),
    rgba(255, 255, 255, 0.9);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.58);
}

.subpanel-header {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 14px;
}

.subpanel-header span {
  font-weight: 700;
  color: #284b36;
}

.subpanel-header small {
  color: #6e7b72;
}

.log-list {
  height: 280px;
  overflow: auto;
  display: grid;
  gap: 10px;
}

.log-item {
  padding: 12px 14px;
  border-radius: 18px;
  background: rgba(241, 245, 238, 0.92);
  border: 1px solid rgba(63, 101, 75, 0.08);
}

.log-item[data-level='success'] {
  background: rgba(225, 245, 229, 0.96);
}

.log-item[data-level='warning'] {
  background: rgba(255, 243, 219, 0.96);
}

.log-item[data-level='error'] {
  background: rgba(252, 231, 225, 0.96);
}

.log-item span {
  display: block;
  margin-bottom: 6px;
  color: #6c766f;
  font-size: 13px;
}

.log-item p {
  margin: 0;
  color: #244b31;
  line-height: 1.5;
  overflow-wrap: anywhere;
}

.message-preview {
  margin: 0 0 10px;
  min-height: 160px;
  max-height: 220px;
  overflow: auto;
  padding: 16px 18px;
  border-radius: 18px;
  background:
    linear-gradient(180deg, rgba(22, 38, 31, 0.98), rgba(30, 49, 40, 0.98));
  color: #dff7df;
  font-family: 'Consolas', 'SFMono-Regular', monospace;
  white-space: pre-wrap;
  word-break: break-word;
  border: 1px solid rgba(125, 180, 139, 0.16);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.05);
}

.message-time {
  margin: 0;
  padding: 0 4px;
  color: #6e8c7a;
  font-size: 13px;
  letter-spacing: 0.02em;
}

.dialog-backdrop {
  position: fixed;
  inset: 0;
  background: rgba(30, 39, 30, 0.38);
  display: grid;
  place-items: center;
  padding: 24px;
  z-index: 20;
}

.dialog-card {
  width: min(760px, 100%);
  padding: 28px;
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
  color: #476151;
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
  border: 1px solid rgba(54, 103, 71, 0.16);
  background: rgba(255, 255, 255, 0.76);
}

.switch-field input {
  width: 20px;
  height: 20px;
  padding: 0;
}

.dialog-note {
  margin: 18px 0 0;
  color: #607165;
  line-height: 1.6;
}

.dialog-footer {
  justify-content: flex-end;
  margin-top: 20px;
}

.icon-btn {
  width: 44px;
  height: 44px;
  padding: 0;
  border-radius: 50%;
  font-size: 30px;
  line-height: 1;
}

@media (max-width: 960px) {
  .hero-panel,
  .content-grid,
  .history-layout,
  .mqtt-log-layout,
  .dialog-grid {
    grid-template-columns: 1fr;
  }

  .hero-status {
    grid-template-columns: 1fr;
  }

  .page-shell {
    width: min(100% - 24px, 1180px);
    margin: 16px auto 32px;
  }

  .panel,
  .hero-panel,
  .dialog-card {
    padding: 22px;
  }

  .metric-card strong,
  .stats-card strong {
    font-size: 26px;
  }

  .dialog-footer,
  .mqtt-actions,
  .hero-actions {
    flex-wrap: wrap;
  }
}

.charts-wrap {
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
  margin-top: 1.5rem;
}

.chart-block {
  height: 260px;
}

.chart-title {
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--text-muted, #888);
  margin-bottom: 0.5rem;
  text-transform: uppercase;
  letter-spacing: 0.05em;
}
</style>
