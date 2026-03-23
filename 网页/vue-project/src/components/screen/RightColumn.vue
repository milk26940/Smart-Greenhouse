<script setup lang="ts">
import { computed } from 'vue'
import { useGreenhouseStore } from '@/stores/greenhouse'
import { formatNumber } from '@/lib/format'

const store = useGreenhouseStore()

const statsCards = computed(() => {
  const stats = store.stats
  if (!stats) {
    return []
  }
  return [
    { label: '空气温度', unit: '°C', data: stats.temperature },
    { label: '空气湿度', unit: '%', data: stats.humidity },
    { label: '土壤湿度', unit: '%', data: stats.soil },
    { label: '光照强度', unit: 'lx', data: stats.light },
    { label: 'CO2 浓度', unit: 'ppm', data: stats.co2 },
  ]
})

const backendStatus = computed(() => (store.backendOnline ? '在线' : '离线'))
</script>

<template>
  <aside class="right-root">
    <section class="panel-block">
      <header class="panel-header">
        <p class="panel-eyebrow">SERVICE</p>
        <h2 class="panel-title">后端与数据库</h2>
      </header>
      <dl class="kv-list">
        <div>
          <dt>服务状态</dt>
          <dd :data-tone="store.backendOnline ? 'ok' : 'danger'">{{ backendStatus }}</dd>
        </div>
        <div>
          <dt>存储类型</dt>
          <dd>{{ store.health?.storage ?? '--' }}</dd>
        </div>
        <div>
          <dt>服务标识</dt>
          <dd>{{ store.health?.service ?? '--' }}</dd>
        </div>
      </dl>
    </section>

    <section class="panel-block">
      <header class="panel-header">
        <p class="panel-eyebrow">STATS</p>
        <h2 class="panel-title">统计概览</h2>
      </header>
      <div class="stats-grid">
        <article v-for="item in statsCards" :key="item.label" class="stats-card">
          <span>{{ item.label }}</span>
          <strong>{{ formatNumber(item.data.latest) }} {{ item.unit }}</strong>
          <p class="stats-line">
            区间 {{ formatNumber(item.data.min) }} - {{ formatNumber(item.data.max) }} {{ item.unit }}
          </p>
          <p class="stats-line">
            平均 {{ formatNumber(item.data.average) }} {{ item.unit }}
          </p>
        </article>
      </div>
    </section>
  </aside>
</template>

<style scoped>
.right-root {
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

.kv-list {
  margin: 0;
  padding: 0;
}

.kv-list div {
  display: flex;
  justify-content: space-between;
  padding: 8px 0;
  border-bottom: 1px solid rgba(74, 112, 146, 0.16);
}

.kv-list div:last-child {
  border-bottom: none;
}

.kv-list dt {
  color: #7da2c3;
  font-size: 13px;
}

.kv-list dd {
  margin: 0;
  color: #eaf7ff;
  font-weight: 600;
}

.kv-list dd[data-tone='ok'] {
  color: #83ffbf;
}

.kv-list dd[data-tone='danger'] {
  color: #ff8f8f;
}

.stats-grid {
  display: grid;
  gap: 10px;
}

.stats-card {
  border-radius: 16px;
  padding: 8px 10px;
  border: 1px solid rgba(103, 152, 190, 0.16);
  background: rgba(6, 14, 27, 0.86);
}

.stats-card span {
  display: block;
  color: #82a8c6;
  font-size: 13px;
}

.stats-card strong {
  display: block;
  margin-top: 6px;
  font-size: 18px;
}

.stats-line {
  margin: 2px 0;
  color: #7da2c3;
  font-size: 12px;
}

@media (max-width: 1280px) {
  .right-root {
    width: auto;
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}
</style>
