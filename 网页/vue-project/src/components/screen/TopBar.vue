<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'

type ScreenMode = 'scene' | 'charts'

const props = defineProps<{
  mode: ScreenMode
  showLeft: boolean
  showRight: boolean
}>()

const emit = defineEmits<{
  (event: 'back'): void
  (event: 'toggle-mode'): void
  (event: 'toggle-left'): void
  (event: 'toggle-right'): void
}>()

const currentTime = ref('')
let clockTimer: ReturnType<typeof setInterval> | null = null

function updateClock() {
  currentTime.value = new Date().toLocaleString('zh-CN', { hour12: false })
}

function toggleFullscreen() {
  const root = document.documentElement
  if (!document.fullscreenElement) {
    void root.requestFullscreen?.()
  } else {
    void document.exitFullscreen?.()
  }
}

const modeButtonLabel = computed(() => (props.mode === 'charts' ? '返回场景' : '查看图表'))

onMounted(() => {
  updateClock()
  clockTimer = setInterval(updateClock, 1000)
})

onBeforeUnmount(() => {
  if (clockTimer) {
    clearInterval(clockTimer)
    clockTimer = null
  }
})
</script>

<template>
  <header class="topbar-root">
    <div class="topbar-bg">
      <div class="topbar-left">
        <h1 class="topbar-title">智慧大棚数字孪生大屏</h1>
        <p class="topbar-subtitle">SMART GREENHOUSE COMMAND CENTER</p>
      </div>
      <div class="topbar-right">
        <div class="topbar-clock">
          <span class="clock-label">当前时间</span>
          <span class="clock-value">{{ currentTime }}</span>
        </div>
        <div class="topbar-controls">
          <button class="topbar-btn ghost" type="button" @click="emit('back')">
            返回
          </button>
          <button class="topbar-btn secondary" type="button" @click="emit('toggle-mode')">
            {{ modeButtonLabel }}
          </button>
          <button v-if="props.mode === 'charts'" class="topbar-btn ghost" type="button" @click="emit('toggle-left')">
            {{ props.showLeft ? '隐藏左侧' : '显示左侧' }}
          </button>
          <button v-if="props.mode === 'charts'" class="topbar-btn ghost" type="button" @click="emit('toggle-right')">
            {{ props.showRight ? '隐藏右侧' : '显示右侧' }}
          </button>
        </div>
        <button class="topbar-btn" type="button" @click="toggleFullscreen">
          全屏切换
        </button>
      </div>
    </div>
  </header>
</template>

<style scoped>
.topbar-root {
  position: relative;
  padding: 0 16px;
  margin-bottom: 10px;
}

.topbar-bg {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 14px 20px;
  border-radius: 18px;
  border: 1px solid rgba(83, 199, 255, 0.55);
  background:
    radial-gradient(circle at left, rgba(83, 199, 255, 0.18), transparent 60%),
    radial-gradient(circle at right, rgba(120, 255, 218, 0.12), transparent 55%),
    linear-gradient(90deg, rgba(5, 16, 35, 0.96), rgba(8, 22, 48, 0.96));
  box-shadow:
    0 0 0 1px rgba(41, 107, 201, 0.5) inset,
    0 18px 40px rgba(0, 0, 0, 0.6);
}

.topbar-left {
  display: grid;
  gap: 4px;
}

.topbar-title {
  margin: 0;
  font-size: 26px;
  letter-spacing: 0.16em;
}

.topbar-subtitle {
  margin: 0;
  color: #78ceff;
  font-size: 13px;
  letter-spacing: 0.24em;
}

.topbar-right {
  display: flex;
  align-items: center;
  gap: 18px;
}

.topbar-controls {
  display: flex;
  align-items: center;
  gap: 10px;
}

.topbar-clock {
  text-align: right;
}

.clock-label {
  display: block;
  color: #7da2c3;
  font-size: 12px;
  letter-spacing: 0.16em;
}

.clock-value {
  display: block;
  margin-top: 4px;
  font-size: 16px;
}

.topbar-btn {
  padding: 8px 16px;
  border-radius: 999px;
  border: 1px solid rgba(83, 199, 255, 0.7);
  background: rgba(7, 21, 46, 0.9);
  color: #e4f1ff;
  font-size: 13px;
  letter-spacing: 0.12em;
  cursor: pointer;
}

.topbar-btn.secondary {
  border-color: rgba(120, 255, 218, 0.7);
}

.topbar-btn.ghost {
  padding: 8px 12px;
  border-color: rgba(83, 199, 255, 0.24);
  background: rgba(7, 21, 46, 0.55);
  color: #cde2f6;
  font-size: 12px;
}
</style>
