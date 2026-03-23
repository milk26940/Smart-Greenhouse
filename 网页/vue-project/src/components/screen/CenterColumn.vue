<script setup lang="ts">
import SceneView from '@/views/SceneView.vue'
import { useGreenhouseStore } from '@/stores/greenhouse'
import { computed, ref } from 'vue'
import { formatDateTime, formatInteger, formatNumber } from '@/lib/format'

type ScreenMode = 'scene' | 'charts'

const props = defineProps<{ mode: ScreenMode }>()

type SceneSelection =
  | { type: 'none' }
  | { type: 'sensor'; id: string; label: string }
  | { type: 'device'; id: string; label: string; deviceKind: 'fan' | 'pump' | 'led' }

const store = useGreenhouseStore()

const headline = computed(() => {
  if (!store.realtime) {
    return '等待实时数据接入'
  }
  return store.realtime.summary || '环境稳定'
})

const showPanels = ref(false)
const selection = ref<SceneSelection>({ type: 'none' })

function handleSelect(nextSelection: SceneSelection) {
  selection.value = nextSelection
}

const selectionTitle = computed(() => {
  if (selection.value.type === 'none') {
    return '点击节点或设备查看详情'
  }
  return selection.value.label
})

const selectionRows = computed(() => {
  const realtime = store.realtime
  type PresenceLike = {
    sensor1?: string
    sensor2?: string
    sensor3?: string
    executor4?: string
    executor5?: string
    latestRecordAt?: string | null
  }

  const presenceSource = (store.presence ?? store.realtime?.presence) as PresenceLike | null
  const updatedAt = realtime?.updatedAt ?? null

  if (selection.value.type === 'sensor') {
    const online =
      selection.value.id === 'sensor1'
        ? presenceSource?.sensor1 ?? 'UNKNOWN'
        : selection.value.id === 'sensor2'
          ? presenceSource?.sensor2 ?? 'UNKNOWN'
          : selection.value.id === 'sensor3'
            ? presenceSource?.sensor3 ?? 'UNKNOWN'
            : 'UNKNOWN'

    return [
      { k: '类型', v: '采集节点' },
      { k: '节点编号', v: selection.value.id },
      { k: '在线状态', v: online },
      { k: '更新时间', v: formatDateTime(updatedAt) },
      { k: '温度', v: realtime ? `${formatNumber(realtime.env.temperature)} °C` : '--' },
      { k: '湿度', v: realtime ? `${formatNumber(realtime.env.humidity)} %` : '--' },
      { k: '土壤', v: realtime ? `${formatNumber(realtime.env.soil)} %` : '--' },
      { k: '光照', v: realtime ? `${formatInteger(realtime.env.light)} lx` : '--' },
      { k: 'CO2', v: realtime ? `${formatInteger(realtime.env.co2)} ppm` : '--' },
    ]
  }

  if (selection.value.type === 'device') {
    const decision = realtime?.decision
    const statusText =
      selection.value.deviceKind === 'fan'
        ? decision?.fan
        : selection.value.deviceKind === 'pump'
          ? decision?.pump
          : decision?.led

    return [
      { k: '类型', v: selection.value.deviceKind === 'fan' ? '风扇' : selection.value.deviceKind === 'pump' ? '灌溉器' : '补光灯' },
      { k: '设备编号', v: selection.value.id },
      { k: '状态', v: statusText ?? '--' },
      { k: '更新时间', v: formatDateTime(updatedAt) },
      { k: '策略摘要', v: decision?.summary ?? '--' },
    ]
  }

  return []
})
</script>

<template>
  <section class="center-root" :data-mode="props.mode">
    <header v-if="props.mode === 'charts'" class="center-header">
      <div class="center-title-block">
        <p class="eyebrow">VISION SCENE</p>
        <h2>大棚场景缩略图</h2>
      </div>
      <div class="center-toolbar">
        <p class="headline">{{ headline }}</p>
        <button class="toolbar-btn" type="button" @click="showPanels = !showPanels">
          {{ showPanels ? '隐藏场景面板' : '显示场景面板' }}
        </button>
      </div>
    </header>

    <div class="center-body">
      <div class="scene-stage">
        <SceneView
          :embed="true"
          :show-panels="props.mode === 'charts' ? showPanels : false"
          :canvas-height="props.mode === 'charts' ? 520 : 900"
          @select="handleSelect"
        />

        <section v-if="props.mode === 'charts'" class="detail-card">
          <header class="detail-header">
            <p class="detail-title">{{ selectionTitle }}</p>
            <span class="detail-badge">{{ selection.type === 'none' ? '未选择' : '已选择' }}</span>
          </header>
          <div v-if="selection.type === 'none'" class="detail-empty">
            点击缩略图中的采集节点或设备，底部会显示对应数据。
          </div>
          <dl v-else class="detail-grid">
            <div v-for="row in selectionRows" :key="row.k" class="detail-row">
              <dt>{{ row.k }}</dt>
              <dd>{{ row.v }}</dd>
            </div>
          </dl>
        </section>
      </div>
    </div>
  </section>
</template>

<style scoped>
.center-root {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.center-header {
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  gap: 16px;
}

.center-title-block {
  display: grid;
  gap: 4px;
}

.eyebrow {
  margin: 0;
  color: #78ceff;
  font-size: 11px;
  letter-spacing: 0.24em;
}

.center-header h2 {
  margin: 0;
  font-size: 20px;
}

.headline {
  margin: 0 0 4px;
  color: #9ecdf6;
  font-size: 13px;
  max-width: 360px;
  text-align: right;
}

.center-body {
  border-radius: 22px;
  border: 1px solid rgba(103, 152, 190, 0.18);
  background: rgba(8, 17, 31, 0.55);
  padding: 10px;
}

.center-toolbar {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  gap: 4px;
}

.toolbar-btn {
  padding: 4px 10px;
  border-radius: 999px;
  border: 1px solid rgba(83, 199, 255, 0.6);
  background: rgba(7, 21, 46, 0.85);
  color: #e4f1ff;
  font-size: 12px;
  letter-spacing: 0.08em;
}

.scene-stage {
  display: grid;
  justify-items: center;
  align-items: start;
  gap: 12px;
  min-height: 560px;
}

.center-root[data-mode='scene'] .center-body {
  border-color: rgba(103, 152, 190, 0.12);
  background: rgba(8, 17, 31, 0.32);
  padding: 0;
}

.center-root[data-mode='scene'] .scene-stage {
  min-height: 0;
}

.detail-card {
  width: 100%;
  border-radius: 18px;
  border: 1px solid rgba(103, 152, 190, 0.16);
  background: rgba(6, 14, 27, 0.64);
  padding: 12px 14px;
}

.detail-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  margin-bottom: 10px;
}

.detail-title {
  margin: 0;
  color: #d9f0ff;
  font-weight: 700;
  letter-spacing: 0.06em;
}

.detail-badge {
  color: #86abc9;
  font-size: 12px;
  letter-spacing: 0.12em;
}

.detail-empty {
  color: #86abc9;
  line-height: 1.7;
  font-size: 13px;
}

.detail-grid {
  margin: 0;
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10px 14px;
}

.detail-row {
  margin: 0;
  display: grid;
  gap: 4px;
}

.detail-row dt {
  color: #86abc9;
  font-size: 12px;
  letter-spacing: 0.1em;
}

.detail-row dd {
  margin: 0;
  color: #eaf7ff;
  font-size: 13px;
}

@media (max-width: 1280px) {
  .detail-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}
</style>
