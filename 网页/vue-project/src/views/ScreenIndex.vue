<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import TopBar from '@/components/screen/TopBar.vue'
import LeftColumn from '@/components/screen/LeftColumn.vue'
import CenterColumn from '@/components/screen/CenterColumn.vue'
import RightColumn from '@/components/screen/RightColumn.vue'
import { useRouter } from 'vue-router'

type ScreenMode = 'scene' | 'charts'

const router = useRouter()
const mode = ref<ScreenMode>('scene')
const showLeft = ref(false)
const showRight = ref(false)
const scale = ref(1)
const canvasRef = ref<HTMLDivElement | null>(null)

function syncPanelDefaults() {
  if (mode.value === 'scene') {
    showLeft.value = false
    showRight.value = false
  } else {
    showLeft.value = true
    showRight.value = true
  }
}

function toggleMode() {
  mode.value = mode.value === 'scene' ? 'charts' : 'scene'
  syncPanelDefaults()
}

function toggleLeft() {
  showLeft.value = !showLeft.value
}

function toggleRight() {
  showRight.value = !showRight.value
}

function goBack() {
  if (window.history.length > 1) {
    router.back()
    return
  }
  void router.push({ name: 'dashboard' })
}

function updateScale() {
  const canvas = canvasRef.value
  if (!canvas) {
    return
  }

  const width = canvas.offsetWidth
  const height = canvas.offsetHeight
  if (width <= 0 || height <= 0) {
    return
  }

  const padding = 10
  const availableW = window.innerWidth - padding * 2
  const availableH = window.innerHeight - padding * 2
  const nextScale = Math.min(1, availableW / width, availableH / height)
  scale.value = Number.isFinite(nextScale) ? nextScale : 1
}

const canvasStyle = computed(() => ({
  transform: `translate(-50%, 0) scale(${scale.value})`,
}))

onMounted(() => {
  syncPanelDefaults()
  nextTick(() => {
    updateScale()
  })
  window.addEventListener('resize', updateScale)
})

onBeforeUnmount(() => {
  window.removeEventListener('resize', updateScale)
})

watch(
  () => mode.value,
  async () => {
    await nextTick()
    updateScale()
  },
)
</script>

<template>
  <div class="screen-root" :data-mode="mode">
    <div class="screen-wrapper">
      <div ref="canvasRef" class="screen-canvas" :style="canvasStyle">
        <TopBar
          :mode="mode"
          :show-left="showLeft"
          :show-right="showRight"
          @back="goBack"
          @toggle-mode="toggleMode"
          @toggle-left="toggleLeft"
          @toggle-right="toggleRight"
        />
        <section class="screen-content">
          <LeftColumn v-if="showLeft" />
          <CenterColumn :mode="mode" />
          <RightColumn v-if="showRight" />
        </section>
      </div>
    </div>
  </div>
</template>

<style scoped>
.screen-root {
  position: relative;
  height: 100vh;
  overflow: hidden;
  padding: 0;
  display: flex;
  justify-content: center;
  align-items: flex-start;
}

.screen-root[data-mode='scene'] {
  background: radial-gradient(circle at 20% 20%, rgba(83, 199, 255, 0.08), transparent 55%),
    linear-gradient(180deg, rgba(3, 10, 21, 0.86), rgba(2, 7, 16, 0.92));
}

.screen-root[data-mode='charts'] {
  background: radial-gradient(circle at 20% 20%, rgba(83, 199, 255, 0.14), transparent 52%),
    radial-gradient(circle at 80% 25%, rgba(120, 255, 218, 0.08), transparent 55%),
    linear-gradient(180deg, rgba(2, 8, 18, 0.98), rgba(1, 5, 12, 0.98));
}

.screen-root::after {
  content: '';
  position: absolute;
  inset: 0;
  background: radial-gradient(circle at 50% 30%, rgba(255, 255, 255, 0.06), transparent 55%);
  pointer-events: none;
}

.screen-wrapper {
  width: 100%;
  height: 100%;
  position: relative;
  display: flex;
  justify-content: center;
  align-items: flex-start;
}

.screen-canvas {
  position: absolute;
  left: 50%;
  top: 0;
  width: 1920px;
  max-width: 1920px;
  display: flex;
  flex-direction: column;
  transform-origin: top center;
  z-index: 1;
}

.screen-content {
  display: flex;
  justify-content: space-between;
  align-items: stretch;
  gap: 14px;
  padding: 6px 14px 14px;
  box-sizing: border-box;
}

@media (max-width: 1280px) {
  .screen-content {
    flex-direction: column;
  }
}
</style>
