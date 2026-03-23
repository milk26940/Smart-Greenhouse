import { computed, ref } from 'vue'
import { defineStore } from 'pinia'
import { fetchJson } from '@/lib/api'
import type { HealthResponse, HistoryResponse, PresenceResponse, RealtimeResponse, StatsResponse } from '@/lib/types'

export const useGreenhouseStore = defineStore('greenhouse', () => {
  const selectedHours = ref(24)

  const overviewLoading = ref(false)
  const trendsLoading = ref(false)
  const backendOnline = ref(false)
  const errorMessage = ref('')

  const health = ref<HealthResponse | null>(null)
  const realtime = ref<RealtimeResponse | null>(null)
  const presence = ref<PresenceResponse | null>(null)
  const history = ref<HistoryResponse | null>(null)
  const stats = ref<StatsResponse | null>(null)

  const hasRealtimeData = computed(() => Boolean(realtime.value && realtime.value.hasData))

  async function loadOverview() {
    overviewLoading.value = true
    errorMessage.value = ''

    try {
      const [healthData, realtimeData, presenceData] = await Promise.all([
        fetchJson<HealthResponse>('/api/health'),
        fetchJson<RealtimeResponse>('/api/realtime'),
        fetchJson<PresenceResponse>('/api/presence'),
      ])

      health.value = healthData
      realtime.value = realtimeData
      presence.value = presenceData
      backendOnline.value = true
    } catch (error) {
      backendOnline.value = false
      errorMessage.value = error instanceof Error ? error.message : '请求失败'
    } finally {
      overviewLoading.value = false
    }
  }

  async function loadTrends() {
    trendsLoading.value = true
    errorMessage.value = ''

    try {
      const hours = selectedHours.value
      const [historyData, statsData] = await Promise.all([
        fetchJson<HistoryResponse>(`/api/history?hours=${hours}`),
        fetchJson<StatsResponse>(`/api/stats?hours=${hours}`),
      ])

      history.value = historyData
      stats.value = statsData
      backendOnline.value = true
    } catch (error) {
      backendOnline.value = false
      errorMessage.value = error instanceof Error ? error.message : '请求失败'
    } finally {
      trendsLoading.value = false
    }
  }

  async function loadAll() {
    await Promise.all([loadOverview(), loadTrends()])
  }

  return {
    selectedHours,
    overviewLoading,
    trendsLoading,
    backendOnline,
    errorMessage,
    health,
    realtime,
    presence,
    history,
    stats,
    hasRealtimeData,
    loadOverview,
    loadTrends,
    loadAll,
  }
})
