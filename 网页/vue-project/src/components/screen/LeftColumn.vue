<script setup lang="ts">
import { computed } from 'vue'
import { useGreenhouseStore } from '@/stores/greenhouse'
import { formatDateTime, formatInteger, formatNumber } from '@/lib/format'

const store = useGreenhouseStore()

const environmentCards = computed(() => {
  const realtime = store.realtime
  if (!realtime) {
    return [
      { label: '空气温度', value: '-- °C' },
      { label: '空气湿度', value: '-- %' },
      { label: '土壤湿度', value: '-- %' },
      { label: '光照强度', value: '-- lx' },
      { label: 'CO2 浓度', value: '-- ppm' },
    ]
  }

  return [
    { label: '空气温度', value: `${formatNumber(realtime.env.temperature)} °C` },
    { label: '空气湿度', value: `${formatNumber(realtime.env.humidity)} %` },
    { label: '土壤湿度', value: `${formatNumber(realtime.env.soil)} %` },
    { label: '光照强度', value: `${formatInteger(realtime.env.light)} lx` },
    { label: 'CO2 浓度', value: `${formatInteger(realtime.env.co2)} ppm` },
  ]
})

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
</script>

<template>
  <aside class="left-root">
    <section class="panel-block">
      <header class="panel-header">
        <p class="panel-eyebrow">ENVIRONMENT</p>
        <h2 class="panel-title">环境核心指标</h2>
        <span class="panel-subtitle">{{ formatDateTime(store.realtime?.updatedAt) }}</span>
      </header>
      <ul class="env-list">
        <li v-for="item in environmentCards" :key="item.label">
          <span>{{ item.label }}</span>
          <strong>{{ item.value }}</strong>
        </li>
      </ul>
    </section>

    <section class="panel-block">
      <header class="panel-header">
        <p class="panel-eyebrow">PRESENCE</p>
        <h2 class="panel-title">节点在线状态</h2>
        <span class="panel-subtitle">{{ formatDateTime(store.presence?.latestRecordAt) }}</span>
      </header>
      <ul class="presence-list">
        <li v-for="entry in presenceEntries" :key="entry.label">
          <span>{{ entry.label }}</span>
          <strong>{{ entry.value }}</strong>
        </li>
      </ul>
    </section>
  </aside>
</template>

<style scoped>
.left-root {
  width: 320px;
  min-width: 0;
  display: grid;
  gap: 16px;
}

.panel-block {
  border-radius: 22px;
  padding: 14px 16px;
  border: 1px solid rgba(103, 152, 190, 0.18);
  background: rgba(8, 17, 31, 0.7);
}

.panel-header {
  display: flex;
  flex-direction: column;
  gap: 6px;
  margin-bottom: 12px;
}

.panel-eyebrow {
  margin: 0;
  color: #78ceff;
  font-size: 11px;
  letter-spacing: 0.24em;
}

.panel-title {
  margin: 0;
  font-size: 18px;
}

.panel-subtitle {
  margin: 0;
  color: #7da2c3;
  font-size: 12px;
}

.env-list,
.presence-list {
  margin: 0;
  padding: 0;
  list-style: none;
  display: grid;
  gap: 10px;
}

.env-list li,
.presence-list li {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 8px 10px;
  border-radius: 14px;
  border: 1px solid rgba(103, 152, 190, 0.16);
  background: rgba(6, 14, 27, 0.86);
}

.env-list span,
.presence-list span {
  color: #86abc9;
  font-size: 13px;
}

.env-list strong,
.presence-list strong {
  color: #eaf7ff;
}

@media (max-width: 1280px) {
  .left-root {
    width: auto;
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}
</style>
