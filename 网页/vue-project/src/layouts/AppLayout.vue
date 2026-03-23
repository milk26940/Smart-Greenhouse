<script setup lang="ts">
import { computed } from 'vue'
import { RouterLink, RouterView, useRoute } from 'vue-router'
import { useGreenhouseStore } from '@/stores/greenhouse'

const route = useRoute()
const store = useGreenhouseStore()

type NavItem = {
  label: string
  to: { name: string }
}

const navItems: NavItem[] = [
  { label: '大屏总览', to: { name: 'screen' } },
  { label: '总览', to: { name: 'dashboard' } },
  { label: '趋势与历史', to: { name: 'trends' } },
  { label: '通信监控', to: { name: 'mqtt' } },
]

const currentTitle = computed(() => {
  const map: Record<string, string> = {
    screen: '大屏总览',
    dashboard: '总览',
    trends: '趋势与历史',
    mqtt: '通信链路监控',
  }
  return map[String(route.name ?? '')] ?? '智慧农业大棚'
})
</script>

<template>
  <div class="app-layout">
    <aside class="sidebar">
      <div class="brand">
        <strong class="brand-title">Smart Greenhouse</strong>
        <span class="brand-subtitle">网页端指挥舱</span>
      </div>

      <nav class="nav">
        <RouterLink
          v-for="item in navItems"
          :key="item.label"
          class="nav-link"
          :to="item.to"
        >
          {{ item.label }}
        </RouterLink>
      </nav>

      <div class="sidebar-footer">
        <div class="status-row">
          <span>后端</span>
          <strong :data-tone="store.backendOnline ? 'ok' : 'danger'">
            {{ store.backendOnline ? '在线' : '离线' }}
          </strong>
        </div>
        <div class="status-row">
          <span>存储</span>
          <strong>{{ store.health?.storage ?? '--' }}</strong>
        </div>
      </div>
    </aside>

    <main class="main">
      <header class="topbar">
        <h1>{{ currentTitle }}</h1>
        <div class="topbar-actions">
          <span class="topbar-hint">左侧切换页面</span>
        </div>
      </header>

      <section class="main-content">
        <RouterView />
      </section>
    </main>
  </div>
</template>

<style scoped>
.app-layout {
  min-height: 100vh;
  display: grid;
  grid-template-columns: 280px minmax(0, 1fr);
}

.sidebar {
  position: sticky;
  top: 0;
  height: 100vh;
  padding: 22px 18px;
  display: grid;
  grid-template-rows: auto 1fr auto;
  gap: 18px;
  border-right: 1px solid rgba(102, 154, 198, 0.16);
  background: linear-gradient(180deg, rgba(7, 16, 31, 0.92), rgba(6, 14, 27, 0.94));
  backdrop-filter: blur(16px);
}

.brand {
  display: grid;
  gap: 6px;
  padding: 14px 14px;
  border-radius: 18px;
  border: 1px solid rgba(103, 152, 190, 0.14);
  background: rgba(8, 17, 31, 0.78);
}

.brand-title {
  font-size: 18px;
  letter-spacing: 0.06em;
}

.brand-subtitle {
  color: #86abc9;
  font-size: 13px;
  letter-spacing: 0.12em;
}

.nav {
  display: grid;
  gap: 10px;
  align-content: start;
  overflow: auto;
  padding-right: 4px;
}

.nav-link {
  display: block;
  padding: 12px 14px;
  border-radius: 14px;
  border: 1px solid rgba(103, 152, 190, 0.12);
  color: #d9f0ff;
  text-decoration: none;
  background: rgba(8, 17, 31, 0.72);
}

.nav-link.router-link-active {
  border-color: rgba(120, 247, 168, 0.22);
  box-shadow: 0 0 0 1px rgba(120, 247, 168, 0.12) inset;
  color: #83ffbf;
}

.sidebar-footer {
  margin-top: auto;
  display: grid;
  gap: 10px;
  padding: 14px 14px;
  border-radius: 18px;
  border: 1px solid rgba(103, 152, 190, 0.12);
  background: rgba(8, 17, 31, 0.72);
}

.status-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  color: #86abc9;
  font-size: 13px;
  letter-spacing: 0.08em;
}

.status-row strong {
  color: #eaf7ff;
  letter-spacing: normal;
}

.status-row strong[data-tone='ok'] {
  color: #83ffbf;
}

.status-row strong[data-tone='danger'] {
  color: #ff8f8f;
}

.main {
  min-width: 0;
  padding: 18px 18px 34px;
}

.topbar {
  display: flex;
  align-items: end;
  justify-content: space-between;
  gap: 12px;
  padding: 14px 16px;
  border-radius: 18px;
  border: 1px solid rgba(102, 154, 198, 0.16);
  background: linear-gradient(180deg, rgba(9, 23, 40, 0.72), rgba(7, 16, 31, 0.72));
  backdrop-filter: blur(14px);
}

.topbar h1 {
  margin: 0;
  font-size: 22px;
  letter-spacing: 0.08em;
}

.topbar-hint {
  color: #7da2c3;
  font-size: 13px;
  letter-spacing: 0.12em;
}

.main-content {
  min-width: 0;
}

@media (max-width: 980px) {
  .app-layout {
    grid-template-columns: 1fr;
  }

  .sidebar {
    position: relative;
    height: auto;
  }
}
</style>
