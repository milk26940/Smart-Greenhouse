<script setup lang="ts">
import { computed, onMounted, watch } from 'vue'
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
import { useGreenhouseStore } from '@/stores/greenhouse'
import { formatDateTime, formatInteger, formatNumber, formatTime } from '@/lib/format'

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend, Filler)
ChartJS.defaults.color = '#a7bed3'
ChartJS.defaults.borderColor = 'rgba(84, 120, 153, 0.22)'

const store = useGreenhouseStore()

const rangeOptions = [
  { label: '最近 1 小时', value: 1 },
  { label: '最近 6 小时', value: 6 },
  { label: '最近 24 小时', value: 24 },
  { label: '最近 7 天', value: 168 },
]

const chartLabels = computed(() => (store.history?.records ?? []).map((record) => formatTime(record.createdAt)).reverse())

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
    { label: '空气温度 (°C)', data: (store.history?.records ?? []).map((record) => record.temperature).reverse(), borderColor: '#ff8c5c', backgroundColor: 'rgba(255,140,92,0.16)', fill: true, tension: 0.35 },
    { label: '空气湿度 (%)', data: (store.history?.records ?? []).map((record) => record.humidity).reverse(), borderColor: '#5de2ff', backgroundColor: 'rgba(93,226,255,0.12)', fill: true, tension: 0.35 },
    { label: '土壤湿度 (%)', data: (store.history?.records ?? []).map((record) => record.soil).reverse(), borderColor: '#78f7a8', backgroundColor: 'rgba(120,247,168,0.12)', fill: true, tension: 0.35 },
  ],
}))

const lightCo2ChartData = computed(() => ({
  labels: chartLabels.value,
  datasets: [
    { label: '光照强度 (lx)', data: (store.history?.records ?? []).map((record) => record.light).reverse(), borderColor: '#f5de6f', backgroundColor: 'rgba(245,222,111,0.14)', fill: true, tension: 0.35 },
    { label: 'CO2 浓度 (ppm)', data: (store.history?.records ?? []).map((record) => record.co2).reverse(), borderColor: '#b890ff', backgroundColor: 'rgba(184,144,255,0.12)', fill: true, tension: 0.35 },
  ],
}))

const statsCards = computed(() => {
  const stats = store.stats
  if (!stats) {
    return []
  }
  return [
    { label: '空气温度', data: stats.temperature, unit: '°C' },
    { label: '空气湿度', data: stats.humidity, unit: '%' },
    { label: '土壤湿度', data: stats.soil, unit: '%' },
    { label: '光照强度', data: stats.light, unit: 'lx' },
    { label: 'CO2 浓度', data: stats.co2, unit: 'ppm' },
  ]
})

async function refresh() {
  await store.loadTrends()
}

onMounted(() => {
  void refresh()
})

watch(
  () => store.selectedHours,
  () => {
    void refresh()
  },
)
</script>

<template>
  <main class="page-shell">
    <section v-if="store.errorMessage" class="banner error-banner">后端请求失败：{{ store.errorMessage }}</section>

    <section class="history-layout">
      <section class="panel main-panel">
        <header class="panel-header history-header">
          <div>
            <p class="section-tag">TREND</p>
            <h2>趋势分析与历史记录</h2>
          </div>
          <div class="controls">
            <select v-model="store.selectedHours" :disabled="store.trendsLoading">
              <option v-for="option in rangeOptions" :key="option.value" :value="option.value">
                {{ option.label }}
              </option>
            </select>
            <button class="secondary-btn" :disabled="store.trendsLoading" @click="refresh">
              {{ store.trendsLoading ? '加载中...' : '刷新统计' }}
            </button>
          </div>
        </header>

        <div v-if="store.history?.records.length" class="kpi-strip">
          <div class="kpi">
            <span>记录条数</span>
            <strong>{{ store.history.count }}</strong>
          </div>
          <div class="kpi">
            <span>开始时间</span>
            <strong>{{ formatDateTime(store.history.start) }}</strong>
          </div>
          <div class="kpi">
            <span>结束时间</span>
            <strong>{{ formatDateTime(store.history.end) }}</strong>
          </div>
        </div>

        <div v-if="store.history?.records.length" class="charts-wrap">
          <div class="chart-card">
            <div class="chart-head">
              <p class="chart-title">温度 / 湿度 / 土壤湿度</p>
              <span class="chart-note">单位：°C / %</span>
            </div>
            <div class="chart-body">
              <Line :data="tempHumidityChartData" :options="chartOptions" />
            </div>
          </div>
          <div class="chart-card">
            <div class="chart-head">
              <p class="chart-title">光照 / CO2</p>
              <span class="chart-note">单位：lx / ppm</span>
            </div>
            <div class="chart-body">
              <Line :data="lightCo2ChartData" :options="chartOptions" />
            </div>
          </div>
        </div>

        <div v-if="store.history?.records.length" class="table-card">
          <div class="table-head">
            <p class="chart-title">历史明细</p>
            <span class="chart-note">按时间倒序（最新在上）</span>
          </div>
          <div class="table-wrap">
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
                <tr v-for="record in store.history.records" :key="record.createdAt">
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
        </div>

        <div v-else class="empty-state">当前时间范围内暂无历史记录。</div>
      </section>

      <section class="panel stats-panel">
        <header class="panel-header">
          <div>
            <p class="section-tag">STATS</p>
            <h2>统计概览</h2>
          </div>
          <span class="sample-chip">样本数：{{ store.stats?.sampleCount ?? 0 }}</span>
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
  </main>
</template>

<style scoped>
.history-layout {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 360px;
  gap: 18px;
  align-items: start;
}

.main-panel {
  padding: 18px;
}

.stats-panel {
  position: sticky;
  top: 18px;
  align-self: start;
  max-height: calc(100vh - 120px);
  overflow: auto;
  padding: 18px;
}

.controls {
  display: flex;
  align-items: center;
  gap: 10px;
}

.kpi-strip {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
  margin-top: 12px;
}

.kpi {
  border-radius: 16px;
  padding: 10px 12px;
  border: 1px solid rgba(103, 152, 190, 0.16);
  background: rgba(6, 14, 27, 0.72);
}

.kpi span {
  display: block;
  color: #86abc9;
  font-size: 12px;
  letter-spacing: 0.12em;
}

.kpi strong {
  display: block;
  margin-top: 6px;
  color: #eaf7ff;
  font-size: 14px;
}

.charts-wrap {
  margin-top: 14px;
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.chart-card {
  border-radius: 18px;
  border: 1px solid rgba(103, 152, 190, 0.16);
  background: rgba(6, 14, 27, 0.72);
  padding: 12px 12px 10px;
  display: grid;
  gap: 10px;
}

.chart-head {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 10px;
}

.chart-title {
  margin: 0;
  color: #d9f0ff;
  font-weight: 700;
  letter-spacing: 0.06em;
}

.chart-note {
  color: #7da2c3;
  font-size: 12px;
}

.chart-body {
  height: 240px;
}

.table-card {
  margin-top: 14px;
  border-radius: 18px;
  border: 1px solid rgba(103, 152, 190, 0.16);
  background: rgba(6, 14, 27, 0.72);
  overflow: hidden;
}

.table-head {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 10px;
  padding: 12px 12px 10px;
  border-bottom: 1px solid rgba(103, 152, 190, 0.12);
}

.table-wrap {
  max-height: 360px;
  overflow: auto;
}

table {
  width: 100%;
  border-collapse: collapse;
}

thead th {
  position: sticky;
  top: 0;
  background: rgba(8, 17, 31, 0.92);
  color: #86abc9;
  font-size: 12px;
  letter-spacing: 0.08em;
  text-align: left;
  padding: 10px 12px;
  border-bottom: 1px solid rgba(103, 152, 190, 0.12);
  z-index: 1;
}

tbody td {
  padding: 10px 12px;
  border-bottom: 1px solid rgba(103, 152, 190, 0.08);
  color: #d9f0ff;
  font-size: 13px;
}

tbody tr:nth-child(2n) td {
  background: rgba(8, 17, 31, 0.38);
}

@media (max-width: 1180px) {
  .history-layout {
    grid-template-columns: 1fr;
  }

  .charts-wrap {
    grid-template-columns: 1fr;
  }

  .kpi-strip {
    grid-template-columns: 1fr;
  }

  .stats-panel {
    position: static;
    max-height: none;
  }
}
</style>
