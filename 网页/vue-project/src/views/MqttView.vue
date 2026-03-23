<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import mqtt, { type IClientOptions, type MqttClient } from 'mqtt'
import { useGreenhouseStore } from '@/stores/greenhouse'
import { formatTime } from '@/lib/format'
import type { MqttForm, MqttLogEntry } from '@/lib/types'

const store = useGreenhouseStore()

const showMqttDialog = ref(false)
const mqttStatus = ref<'idle' | 'connecting' | 'connected' | 'error' | 'disconnected'>('idle')
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

const mqttBrokerPreview = computed(() => {
  const protocol = mqttForm.value.useTls ? 'wss' : mqttForm.value.protocol
  const path = mqttForm.value.path.startsWith('/') ? mqttForm.value.path : `/${mqttForm.value.path}`
  return `${protocol}://${mqttForm.value.host}:${mqttForm.value.port}${path}`
})

function createClientId() {
  return `mqttx_${Math.random().toString(16).slice(2, 10)}`
}

function createTimestamp() {
  return new Date().toLocaleTimeString('zh-CN', { hour12: false })
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

function buildTelemetryRequest(parsed: Record<string, unknown>) {
  const env = parsed.env as Record<string, unknown> | undefined
  const vision = parsed.vision as Record<string, unknown> | undefined

  const now = new Date()
  const beijingTime = new Date(now.getTime() + 8 * 60 * 60 * 1000)
  const formattedTime = beijingTime.toISOString().slice(0, 19).replace('T', ' ')

  return {
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
}

function handleIncomingMessage(topic: string, payload: Uint8Array) {
  const text = new TextDecoder().decode(payload)
  mqttLastMessage.value = text
  mqttLastMessageAt.value = new Date().toISOString()
  pushMqttLog(`收到消息 ${topic}: ${text}`, 'success')

  try {
    const parsed = JSON.parse(text) as Record<string, unknown>
    const request = buildTelemetryRequest(parsed)

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
            void store.loadOverview()
          }, 500)
        } else {
          const errorText = await response.text().catch(() => '')
          pushMqttLog(`数据写入失败: ${response.status} ${response.statusText} ${errorText}`.trim(), 'error')
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
  pushMqttLog('MQTT 面板已就绪，等待手动连接', 'info')
  void store.loadOverview()
})

onBeforeUnmount(() => {
  disconnectMqtt(false)
})

defineExpose({ handleIncomingMessage })
</script>

<template>
  <main class="page-shell">
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
        <div class="span-2">
          <span class="info-label">Broker</span>
          <strong class="mono">{{ mqttCurrentUrl || mqttBrokerPreview }}</strong>
        </div>
        <div class="span-2">
          <span class="info-label">Topic</span>
          <strong class="mono">{{ mqttForm.topic }}</strong>
        </div>
      </div>

      <div class="mqtt-grid">
        <section class="log-card mqtt-log-card">
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

        <div class="mqtt-side">
          <section class="log-card mqtt-message-card">
            <header class="subpanel-header">
              <span>最近消息</span>
              <small>{{ mqttForm.topic }}</small>
            </header>
            <pre class="message-preview">{{ mqttLastMessage || '尚未收到消息' }}</pre>
            <p v-if="mqttLastMessageAt" class="message-time">到达 {{ formatTime(mqttLastMessageAt) }}</p>
          </section>

          <section class="panel mqtt-hint-panel">
            <header class="panel-header">
              <div>
                <p class="section-tag">SYNC</p>
                <h2>数据同步说明</h2>
              </div>
              <span class="sample-chip">
                {{ store.realtime?.updatedAt ? new Date(store.realtime.updatedAt).toLocaleString('zh-CN', { hour12: false }) : '暂无刷新' }}
              </span>
            </header>
            <p class="mqtt-hint">
              当前页面会把收到的 MQTT JSON 消息映射为 `/api/telemetry` 请求写入数据库；总览页与大屏总览会自动拉取 `/api/realtime` 展示一致的执行器状态。
            </p>
            <p class="mqtt-hint muted">
              当前后端对入库字段有必填校验，消息缺字段会写入失败；失败原因会记录在日志里。
            </p>
          </section>
        </div>
      </div>
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

<style scoped>
.mqtt-panel {
  padding: 18px;
}

.mqtt-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 10px;
}

.mqtt-info-strip {
  margin-top: 12px;
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10px;
}

.mqtt-info-strip .span-2 {
  grid-column: span 2;
}

.mqtt-info-strip strong.mono {
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
  font-size: 12px;
  letter-spacing: 0.02em;
}

.mqtt-grid {
  margin-top: 14px;
  display: grid;
  grid-template-columns: minmax(0, 1fr) 420px;
  gap: 14px;
  align-items: start;
}

.mqtt-log-card {
  min-height: 520px;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
}

.log-list {
  min-height: 0;
  max-height: 520px;
  overflow: auto;
}

.mqtt-side {
  display: grid;
  gap: 14px;
}

.mqtt-message-card .message-preview {
  max-height: 240px;
  overflow: auto;
}

.mqtt-hint-panel {
  padding: 20px;
}

.mqtt-hint {
  margin: 0 0 10px;
  color: #cde2f6;
  line-height: 1.7;
}

.mqtt-hint.muted {
  color: #86abc9;
}

@media (max-width: 1180px) {
  .mqtt-grid {
    grid-template-columns: 1fr;
  }

  .mqtt-log-card {
    min-height: 420px;
  }
}
</style>
