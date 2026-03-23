<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useGreenhouseStore } from '@/stores/greenhouse'
import { formatDateTime, formatNumber } from '@/lib/format'

const store = useGreenhouseStore()

const props = defineProps<{ showPanels?: boolean; embed?: boolean; canvasHeight?: number }>()
const panelsVisible = computed(() => props.showPanels ?? true)
const embedMode = computed(() => props.embed ?? false)
const canvasHeight = computed(() => Math.max(240, props.canvasHeight ?? 520))

type SceneSelection =
  | { type: 'none' }
  | { type: 'sensor'; id: string; label: string }
  | { type: 'device'; id: string; label: string; deviceKind: 'fan' | 'pump' | 'led' }

const emit = defineEmits<{
  (event: 'select', selection: SceneSelection): void
}>()

const showHeatmap = ref(true)
const showSensors = ref(true)
const showDevices = ref(true)

const canvasRef = ref<HTMLCanvasElement | null>(null)
const canvasHostRef = ref<HTMLDivElement | null>(null)

const selectedSensorId = ref<string | null>(null)
const selectedDeviceId = ref<string | null>(null)

let resizeObserver: ResizeObserver | null = null
let refreshTimer: ReturnType<typeof setInterval> | null = null
let rafId = 0

const backgroundUrl = '/scene/greenhouse-top.png'
const sceneBackgroundStyle = computed(() => ({
  backgroundImage: `url("${backgroundUrl}")`,
  backgroundSize: 'cover',
  opacity: '1',
}))

type Point = { id: string; label: string; x: number; y: number }
type DevicePoint = Point & { kind: 'fan' | 'pump' | 'led' }

const sensors: Point[] = [
  { id: 'sensor1', label: '采集节点 1', x: 0.2, y: 0.22 },
  { id: 'sensor2', label: '采集节点 2', x: 0.5, y: 0.22 },
  { id: 'sensor3', label: '采集节点 3', x: 0.8, y: 0.22 },
  { id: 'sensor4', label: '采集节点 4', x: 0.2, y: 0.82 },
  { id: 'sensor5', label: '采集节点 5', x: 0.5, y: 0.82 },
  { id: 'sensor6', label: '采集节点 6', x: 0.8, y: 0.82 },
]

const devices: DevicePoint[] = [
  { id: 'fan', kind: 'fan', label: '风扇', x: 0.08, y: 0.9 },
  { id: 'pump1', kind: 'pump', label: '灌溉器 1', x: 0.42, y: 0.6 },
  { id: 'pump2', kind: 'pump', label: '灌溉器 2', x: 0.58, y: 0.6 },
  { id: 'led1', kind: 'led', label: '补光灯 1', x: 0.42, y: 0.4 },
  { id: 'led2', kind: 'led', label: '补光灯 2', x: 0.58, y: 0.4 },
]

const actuatorStatus = computed(() => {
  const decision = store.realtime?.decision
  const normalize = (value?: string) => Boolean(value && value.toUpperCase().includes('ON'))
  return {
    fan: normalize(decision?.fan),
    pump: normalize(decision?.pump),
    led: normalize(decision?.led),
    text: {
      fan: decision?.fan ?? '--',
      pump: decision?.pump ?? '--',
      led: decision?.led ?? '--',
    },
  }
})

function clamp(value: number, min: number, max: number) {
  return Math.min(Math.max(value, min), max)
}

function lerp(a: number, b: number, t: number) {
  return a + (b - a) * t
}

function toRgb(hex: string) {
  const value = hex.startsWith('#') ? hex.slice(1) : hex
  const num = Number.parseInt(value, 16)
  return { r: (num >> 16) & 255, g: (num >> 8) & 255, b: num & 255 }
}

function mixColor(a: string, b: string, t: number) {
  const ca = toRgb(a)
  const cb = toRgb(b)
  const r = Math.round(lerp(ca.r, cb.r, t))
  const g = Math.round(lerp(ca.g, cb.g, t))
  const b2 = Math.round(lerp(ca.b, cb.b, t))
  return `rgb(${r}, ${g}, ${b2})`
}

function soilColor(value: number) {
  const v = clamp(value, 0, 100)
  if (v <= 50) {
    return mixColor('#ff5c5c', '#f5de6f', v / 50)
  }
  return mixColor('#f5de6f', '#5de2ff', (v - 50) / 50)
}

function roundRectPath(ctx: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, r: number) {
  const radius = Math.max(0, Math.min(r, w / 2, h / 2))
  const maybe = ctx as unknown as { roundRect?: (x: number, y: number, w: number, h: number, r: number) => void }

  ctx.beginPath()
  if (typeof maybe.roundRect === 'function') {
    maybe.roundRect(x, y, w, h, radius)
    return
  }

  ctx.moveTo(x + radius, y)
  ctx.lineTo(x + w - radius, y)
  ctx.arcTo(x + w, y, x + w, y + radius, radius)
  ctx.lineTo(x + w, y + h - radius)
  ctx.arcTo(x + w, y + h, x + w - radius, y + h, radius)
  ctx.lineTo(x + radius, y + h)
  ctx.arcTo(x, y + h, x, y + h - radius, radius)
  ctx.lineTo(x, y + radius)
  ctx.arcTo(x, y, x + radius, y, radius)
  ctx.closePath()
}

function setupCanvasSize() {
  const host = canvasHostRef.value
  const canvas = canvasRef.value
  if (!host || !canvas) {
    return
  }

  const width = host.clientWidth
  const height = host.clientHeight
  if (width <= 0 || height <= 0) {
    return
  }

  const dpr = Math.max(1, Math.min(window.devicePixelRatio || 1, 2))
  canvas.width = Math.floor(width * dpr)
  canvas.height = Math.floor(height * dpr)
  canvas.style.width = `${width}px`
  canvas.style.height = `${height}px`

  const ctx = canvas.getContext('2d')
  if (ctx) {
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0)
  }
}

function draw(nowMs: number) {
  const canvas = canvasRef.value
  const host = canvasHostRef.value
  if (!canvas || !host) {
    return
  }

  const ctx = canvas.getContext('2d') as CanvasRenderingContext2D

  const width = host.clientWidth
  const height = host.clientHeight
  if (width <= 0 || height <= 0) {
    return
  }

  ctx.clearRect(0, 0, width, height)

  const backgroundEnabled = backgroundUrl.trim().length > 0

  const padding = 18
  const sceneX = padding
  const sceneY = padding
  const sceneW = width - padding * 2
  const sceneH = height - padding * 2

  ctx.save()
  ctx.strokeStyle = 'rgba(103, 152, 190, 0.16)'
  ctx.lineWidth = 1
  roundRectPath(ctx, sceneX, sceneY, sceneW, sceneH, 22)
    if (!backgroundEnabled) {
    ctx.fillStyle = 'rgba(6, 14, 27, 0.82)'
    ctx.fill()
  }
  ctx.stroke()
  ctx.restore()

  const greenhouseInset = 34
  const gx = sceneX + greenhouseInset
  const gy = sceneY + greenhouseInset
  const gw = sceneW - greenhouseInset * 2
  const gh = sceneH - greenhouseInset * 2

  if (!backgroundEnabled) {
    ctx.save()
    ctx.strokeStyle = 'rgba(120, 206, 255, 0.22)'
    ctx.lineWidth = 2
    roundRectPath(ctx, gx, gy, gw, gh, 18)
    ctx.stroke()
    ctx.restore()

    ctx.save()
    ctx.strokeStyle = 'rgba(91, 122, 150, 0.14)'
    ctx.lineWidth = 1
    const gridX = 12
    const gridY = 8
    for (let i = 1; i < gridX; i += 1) {
      const x = gx + (gw * i) / gridX
      ctx.beginPath()
      ctx.moveTo(x, gy)
      ctx.lineTo(x, gy + gh)
      ctx.stroke()
    }
    for (let j = 1; j < gridY; j += 1) {
      const y = gy + (gh * j) / gridY
      ctx.beginPath()
      ctx.moveTo(gx, y)
      ctx.lineTo(gx + gw, y)
      ctx.stroke()
    }
    ctx.restore()

    ctx.save()
    const walkwayW = Math.max(32, gw * 0.09)
    const walkwayX = gx + (gw - walkwayW) / 2
    ctx.fillStyle = 'rgba(6, 14, 27, 0.62)'
    ctx.fillRect(walkwayX, gy + 4, walkwayW, gh - 8)
    ctx.restore()

    ctx.save()
    const bedCount = 6
    const bedGap = Math.max(10, gw * 0.03)
    const bedW = (gw - bedGap * (bedCount + 1)) / bedCount
    const bedY = gy + gh * 0.12
    const bedH = gh * 0.76
    for (let i = 0; i < bedCount; i += 1) {
      const x = gx + bedGap + i * (bedW + bedGap)
      const isWalkway = x < walkwayX + walkwayW && x + bedW > walkwayX
      if (isWalkway) {
        continue
      }
      ctx.save()
      ctx.fillStyle = 'rgba(9, 22, 36, 0.74)'
      ctx.strokeStyle = 'rgba(120, 206, 255, 0.12)'
      ctx.lineWidth = 1
      roundRectPath(ctx, x, bedY, bedW, bedH, 14)
      ctx.fill()
      ctx.stroke()
      ctx.globalAlpha = 0.18
      ctx.strokeStyle = '#78f7a8'
      for (let s = 0; s < 10; s += 1) {
        const sx = x + 10 + (bedW - 20) * (s / 9)
        ctx.beginPath()
        ctx.moveTo(sx, bedY + 8)
        ctx.lineTo(sx, bedY + bedH - 8)
        ctx.stroke()
      }
      ctx.restore()
    }
    ctx.restore()
  }

  if (showHeatmap.value) {
    const base = store.realtime?.env.soil ?? 50
    const temp = store.realtime?.env.temperature ?? 25
    const t = (nowMs / 1000) * 0.35
    const cols = 30
    const rows = 18
    const cellW = gw / cols
    const cellH = gh / rows

    ctx.save()
    ctx.globalAlpha = backgroundEnabled ? 0.42 : 0.72
    for (let y = 0; y < rows; y += 1) {
      for (let x = 0; x < cols; x += 1) {
        const nx = x / (cols - 1)
        const ny = y / (rows - 1)
        const wave1 = Math.sin(nx * 8 + ny * 6 + t)
        const wave2 = Math.cos(nx * 4 - ny * 5 + temp * 0.12)
        const value = clamp(base + wave1 * 10 + wave2 * 6, 0, 100)
        ctx.fillStyle = soilColor(value)
        ctx.fillRect(gx + x * cellW, gy + y * cellH, cellW + 0.6, cellH + 0.6)
      }
    }
    ctx.restore()
  }

  function toWorld(p: Point) {
    return { x: gx + p.x * gw, y: gy + p.y * gh }
  }

  function drawSensor(p: Point) {
    const pos = toWorld(p)
    const isSelected = selectedSensorId.value === p.id
    ctx.save()
    ctx.fillStyle = isSelected ? 'rgba(131, 255, 191, 0.95)' : 'rgba(120, 206, 255, 0.9)'
    ctx.strokeStyle = isSelected ? 'rgba(32, 128, 96, 0.9)' : 'rgba(12, 30, 52, 0.8)'
    ctx.lineWidth = isSelected ? 3 : 2
    ctx.beginPath()
    ctx.arc(pos.x, pos.y, isSelected ? 12 : 10, 0, Math.PI * 2)
    ctx.fill()
    ctx.stroke()
    ctx.fillStyle = '#d9f0ff'
    ctx.font = '12px Microsoft YaHei, PingFang SC, Segoe UI, sans-serif'
    ctx.textAlign = 'left'
    ctx.textBaseline = 'middle'
    ctx.fillText(p.label, pos.x + 14, pos.y)
    ctx.restore()
  }

  function drawFan(p: Point) {
    const pos = toWorld(p)
    const on = actuatorStatus.value.fan
    const angle = on ? nowMs / 180 : 0
    ctx.save()
    ctx.translate(pos.x, pos.y)
    ctx.fillStyle = on ? 'rgba(131, 255, 191, 0.9)' : 'rgba(156, 180, 201, 0.75)'
    ctx.strokeStyle = 'rgba(12, 30, 52, 0.8)'
    ctx.lineWidth = 2

    ctx.beginPath()
    ctx.arc(0, 0, 16, 0, Math.PI * 2)
    ctx.fill()
    ctx.stroke()

    ctx.rotate(angle)
    ctx.fillStyle = 'rgba(6, 14, 27, 0.76)'
    for (let i = 0; i < 3; i += 1) {
      ctx.rotate((Math.PI * 2) / 3)
      ctx.beginPath()
      ctx.moveTo(0, 0)
      ctx.quadraticCurveTo(18, -4, 24, 0)
      ctx.quadraticCurveTo(18, 4, 0, 0)
      ctx.fill()
    }

    ctx.resetTransform()
    ctx.fillStyle = '#d9f0ff'
    ctx.font = '12px Microsoft YaHei, PingFang SC, Segoe UI, sans-serif'
    ctx.fillText(p.label, pos.x - 16, pos.y + 30)
    ctx.restore()
  }

  function drawPump(p: Point) {
    const pos = toWorld(p)
    const on = actuatorStatus.value.pump
    const pulse = on ? (Math.sin(nowMs / 180) + 1) / 2 : 0
    ctx.save()
    ctx.fillStyle = on ? 'rgba(255, 213, 127, 0.9)' : 'rgba(156, 180, 201, 0.75)'
    ctx.strokeStyle = 'rgba(12, 30, 52, 0.8)'
    ctx.lineWidth = 2
    roundRectPath(ctx, pos.x - 18, pos.y - 12, 36, 24, 8)
    ctx.fill()
    ctx.stroke()

    if (on) {
      ctx.strokeStyle = `rgba(93, 226, 255, ${0.25 + pulse * 0.45})`
      ctx.lineWidth = 2
      for (let i = -2; i <= 2; i += 1) {
        ctx.beginPath()
        ctx.moveTo(pos.x + i * 6, pos.y + 14)
        ctx.lineTo(pos.x + i * 6, pos.y + 14 + 18 + pulse * 8)
        ctx.stroke()
      }
    }

    ctx.fillStyle = '#d9f0ff'
    ctx.font = '12px Microsoft YaHei, PingFang SC, Segoe UI, sans-serif'
    ctx.textAlign = 'center'
    ctx.textBaseline = 'top'
    ctx.fillText(p.label, pos.x, pos.y + 26)
    ctx.restore()
  }

  function drawLed(p: Point) {
    const pos = toWorld(p)
    const on = actuatorStatus.value.led
    const glow = on ? (Math.sin(nowMs / 240) + 1) / 2 : 0
    ctx.save()
    ctx.fillStyle = on ? `rgba(120, 206, 255, ${0.5 + glow * 0.4})` : 'rgba(156, 180, 201, 0.75)'
    ctx.strokeStyle = 'rgba(12, 30, 52, 0.8)'
    ctx.lineWidth = 2
    roundRectPath(ctx, pos.x - 20, pos.y - 10, 40, 20, 10)
    ctx.fill()
    ctx.stroke()

    if (on) {
      ctx.strokeStyle = `rgba(120, 206, 255, ${0.18 + glow * 0.25})`
      ctx.lineWidth = 2
      for (let i = -2; i <= 2; i += 1) {
        ctx.beginPath()
        ctx.moveTo(pos.x + i * 8, pos.y + 14)
        ctx.lineTo(pos.x + i * 12, pos.y + 14 + 22)
        ctx.stroke()
      }
    }

    ctx.fillStyle = '#d9f0ff'
    ctx.font = '12px Microsoft YaHei, PingFang SC, Segoe UI, sans-serif'
    ctx.textAlign = 'center'
    ctx.textBaseline = 'top'
    ctx.fillText(p.label, pos.x, pos.y + 26)
    ctx.restore()
  }

  if (showSensors.value) {
    sensors.forEach(drawSensor)
  }

  if (showDevices.value) {
    devices.forEach((device) => {
      if (device.kind === 'fan') {
        drawFan(device)
      } else if (device.kind === 'pump') {
        drawPump(device)
      } else {
        drawLed(device)
      }
    })
  }

  if (showHeatmap.value && panelsVisible.value) {
    ctx.save()
    ctx.fillStyle = 'rgba(6, 14, 27, 0.92)'
    ctx.strokeStyle = 'rgba(103, 152, 190, 0.16)'
    ctx.lineWidth = 1
    roundRectPath(ctx, sceneX + 16, sceneY + 16, 210, 104, 16)
    ctx.fill()
    ctx.stroke()
    ctx.fillStyle = '#cde2f6'
    ctx.font = '12px Microsoft YaHei, PingFang SC, Segoe UI, sans-serif'
    ctx.fillText('土壤湿度热图', sceneX + 30, sceneY + 40)
    ctx.fillStyle = '#86abc9'
    ctx.fillText('低', sceneX + 30, sceneY + 92)
    ctx.fillText('高', sceneX + 190, sceneY + 92)
    const barX = sceneX + 52
    const barY = sceneY + 76
    const barW = 130
    const barH = 12
    const grad = ctx.createLinearGradient(barX, 0, barX + barW, 0)
    grad.addColorStop(0, '#ff5c5c')
    grad.addColorStop(0.5, '#f5de6f')
    grad.addColorStop(1, '#5de2ff')
    ctx.fillStyle = grad
    ctx.fillRect(barX, barY, barW, barH)
    ctx.restore()

    ctx.save()
    ctx.fillStyle = '#86abc9'
    ctx.font = '12px Microsoft YaHei, PingFang SC, Segoe UI, sans-serif'
    ctx.fillText(`soil=${formatNumber(store.realtime?.env.soil)}`, sceneX + 30, sceneY + 56)
    ctx.restore()
  }
}

function handleCanvasClick(event: MouseEvent) {
  const host = canvasHostRef.value
  if (!host) {
    return
  }
  const rect = host.getBoundingClientRect()
  const px = event.clientX - rect.left
  const py = event.clientY - rect.top

  const padding = 18
  const sceneX = padding
  const sceneY = padding
  const sceneW = rect.width - padding * 2
  const sceneH = rect.height - padding * 2

  const greenhouseInset = 34
  const gx = sceneX + greenhouseInset
  const gy = sceneY + greenhouseInset
  const gw = sceneW - greenhouseInset * 2
  const gh = sceneH - greenhouseInset * 2

  const distance = (p: Point) => {
    const x = gx + p.x * gw
    const y = gy + p.y * gh
    const dx = px - x
    const dy = py - y
    return Math.hypot(dx, dy)
  }

  let nearestSensor: Point | null = null
  let sensorDist = Infinity
  sensors.forEach((sensor) => {
    const d = distance(sensor)
    if (d < sensorDist) {
      sensorDist = d
      nearestSensor = sensor
    }
  })

  let nearestDevice: DevicePoint | null = null
  let deviceDist = Infinity
  devices.forEach((device) => {
    const d = distance(device)
    if (d < deviceDist) {
      deviceDist = d
      nearestDevice = device
    }
  })

  const sensorThreshold = 24
  const deviceThreshold = 28

  if (nearestSensor !== null && sensorDist < sensorThreshold && sensorDist <= deviceDist) {
    const sensor = nearestSensor as Point
    selectedSensorId.value = sensor.id
    selectedDeviceId.value = null
    emit('select', { type: 'sensor', id: sensor.id, label: sensor.label })
    return
  }

  if (nearestDevice !== null && deviceDist < deviceThreshold) {
    const device = nearestDevice as DevicePoint
    selectedDeviceId.value = device.id
    selectedSensorId.value = null
    emit('select', {
      type: 'device',
      id: device.id,
      label: device.label,
      deviceKind: device.kind,
    })
    return
  }

  selectedSensorId.value = null
  selectedDeviceId.value = null
  emit('select', { type: 'none' })
}

function tick(nowMs: number) {
  draw(nowMs)
  rafId = window.requestAnimationFrame(tick)
}

async function refresh() {
  await store.loadOverview()
}

onMounted(() => {
  void refresh()
  refreshTimer = setInterval(() => {
    void refresh()
  }, 5000)

  setupCanvasSize()
  resizeObserver = new ResizeObserver(() => {
    setupCanvasSize()
  })
  if (canvasHostRef.value) {
    resizeObserver.observe(canvasHostRef.value)
  }
  rafId = window.requestAnimationFrame(tick)
})

onBeforeUnmount(() => {
  if (refreshTimer) {
    clearInterval(refreshTimer)
    refreshTimer = null
  }
  if (resizeObserver && canvasHostRef.value) {
    resizeObserver.unobserve(canvasHostRef.value)
  }
  resizeObserver = null
  if (rafId) {
    window.cancelAnimationFrame(rafId)
  }
})

watch(
  () => [store.realtime?.updatedAt, showHeatmap.value, showSensors.value, showDevices.value],
  () => {
    draw(Date.now())
  },
)
</script>

<template>
  <main v-if="!embedMode" class="page-shell">
    <section v-if="store.errorMessage" class="banner error-banner">后端请求失败：{{ store.errorMessage }}</section>

    <section class="panel scene-panel">
      <header class="panel-header">
        <div>
          <p class="section-tag">SCENE</p>
          <h2>俯视大棚 2D 演示</h2>
        </div>
        <span class="sample-chip">更新时间：{{ formatDateTime(store.realtime?.updatedAt) }}</span>
      </header>

      <div class="scene-layout">
        <aside v-if="panelsVisible" class="scene-controls">
          <div class="control-group">
            <p class="control-title">图层</p>
            <label class="switch">
              <input v-model="showHeatmap" type="checkbox" />
              <span>土壤湿度热图</span>
            </label>
            <label class="switch">
              <input v-model="showSensors" type="checkbox" />
              <span>传感器分布</span>
            </label>
            <label class="switch">
              <input v-model="showDevices" type="checkbox" />
              <span>设备分布</span>
            </label>
          </div>

          <div class="control-group">
            <p class="control-title">执行器状态</p>
            <ul class="status-list">
              <li>
                <span>风扇</span>
                <strong :data-on="actuatorStatus.fan">{{ actuatorStatus.text.fan }}</strong>
              </li>
              <li>
                <span>灌溉器 1</span>
                <strong :data-on="actuatorStatus.pump">{{ actuatorStatus.text.pump }}</strong>
              </li>
              <li>
                <span>灌溉器 2</span>
                <strong :data-on="actuatorStatus.pump">{{ actuatorStatus.text.pump }}</strong>
              </li>
              <li>
                <span>补光灯 1</span>
                <strong :data-on="actuatorStatus.led">{{ actuatorStatus.text.led }}</strong>
              </li>
              <li>
                <span>补光灯 2</span>
                <strong :data-on="actuatorStatus.led">{{ actuatorStatus.text.led }}</strong>
              </li>
            </ul>
          </div>

          <div class="control-group">
            <p class="control-title">说明</p>
            <p class="control-text">2D 场景会根据 `/api/realtime` 的执行器状态驱动动画。</p>
            <p class="control-text muted">土壤热图为演示用插值效果，后续可替换为真实网格数据。</p>
          </div>
        </aside>

        <div ref="canvasHostRef" class="canvas-host">
          <div class="scene-bg" :style="sceneBackgroundStyle" />
          <canvas ref="canvasRef" class="scene-canvas" @click="handleCanvasClick" />
        </div>
      </div>
    </section>
  </main>

  <div v-else class="embed-root">
    <div class="scene-layout" :data-panels="panelsVisible ? 'on' : 'off'">
      <aside v-if="panelsVisible" class="scene-controls">
        <div class="control-group">
          <p class="control-title">图层</p>
          <label class="switch">
            <input v-model="showHeatmap" type="checkbox" />
            <span>土壤湿度热图</span>
          </label>
          <label class="switch">
            <input v-model="showSensors" type="checkbox" />
            <span>传感器分布</span>
          </label>
          <label class="switch">
            <input v-model="showDevices" type="checkbox" />
            <span>设备分布</span>
          </label>
        </div>

        <div class="control-group">
          <p class="control-title">执行器状态</p>
          <ul class="status-list">
            <li>
              <span>风扇</span>
              <strong :data-on="actuatorStatus.fan">{{ actuatorStatus.text.fan }}</strong>
            </li>
            <li>
              <span>灌溉器 1</span>
              <strong :data-on="actuatorStatus.pump">{{ actuatorStatus.text.pump }}</strong>
            </li>
            <li>
              <span>灌溉器 2</span>
              <strong :data-on="actuatorStatus.pump">{{ actuatorStatus.text.pump }}</strong>
            </li>
            <li>
              <span>补光灯 1</span>
              <strong :data-on="actuatorStatus.led">{{ actuatorStatus.text.led }}</strong>
            </li>
            <li>
              <span>补光灯 2</span>
              <strong :data-on="actuatorStatus.led">{{ actuatorStatus.text.led }}</strong>
            </li>
          </ul>
        </div>
      </aside>

      <div ref="canvasHostRef" class="canvas-host" :style="{ height: `${canvasHeight}px` }">
        <div class="scene-bg" :style="sceneBackgroundStyle" />
        <canvas ref="canvasRef" class="scene-canvas" @click="handleCanvasClick" />
      </div>
    </div>
  </div>
</template>

<style scoped>
.embed-root {
  width: 100%;
  min-width: 0;
}

.scene-panel {
  padding: 22px;
}

.scene-layout {
  display: grid;
  grid-template-columns: minmax(0, 1fr);
  gap: 14px;
  align-items: stretch;
}

.scene-layout[data-panels='on'] {
  grid-template-columns: minmax(0, 300px) minmax(0, 1fr);
}

.scene-controls {
  min-width: 0;
  border-radius: 22px;
  padding: 14px;
  border: 1px solid rgba(103, 152, 190, 0.12);
  background: rgba(8, 17, 31, 0.7);
  display: grid;
  gap: 18px;
}

.control-group {
  display: grid;
  gap: 10px;
}

.control-title {
  margin: 0;
  font-weight: 700;
  letter-spacing: 0.12em;
  color: #78ceff;
  font-size: 12px;
}

.switch {
  display: flex;
  align-items: center;
  justify-content: flex-start;
  gap: 12px;
  padding: 12px 14px;
  border-radius: 16px;
  border: 1px solid rgba(103, 152, 190, 0.12);
  background: rgba(6, 14, 27, 0.84);
  color: #d9f0ff;
}

.switch span {
  flex: 1;
}

.switch input {
  width: 18px;
  height: 18px;
}

.status-list {
  margin: 0;
  padding: 0;
  list-style: none;
  display: grid;
  gap: 10px;
}

.status-list li {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  padding: 12px 14px;
  border-radius: 16px;
  border: 1px solid rgba(103, 152, 190, 0.12);
  background: rgba(6, 14, 27, 0.84);
}

.status-list span {
  color: #86abc9;
}

.status-list strong {
  color: #eaf7ff;
}

.status-list strong[data-on='true'] {
  color: #83ffbf;
}

.control-text {
  margin: 0;
  line-height: 1.7;
  color: #cde2f6;
}

.control-text.muted {
  color: #86abc9;
}

.canvas-host {
  position: relative;
  height: 520px;
  min-height: 420px;
  min-width: 0;
  border-radius: 22px;
  border: 1px solid rgba(103, 152, 190, 0.12);
  background: rgba(8, 17, 31, 0.7);
  overflow: hidden;
}

.scene-bg {
  position: absolute;
  inset: 0;
  background-repeat: no-repeat;
  background-position: center;
  filter: saturate(1.12) contrast(1.06);
  pointer-events: none;
}

.scene-canvas {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
  display: block;
}

@media (max-width: 1080px) {
  .scene-layout {
    grid-template-columns: 1fr;
  }
}
</style>
