import { createRouter, createWebHistory } from 'vue-router'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/screen',
      name: 'screen',
      component: () => import('@/views/ScreenIndex.vue'),
    },
    {
      path: '/',
      component: () => import('@/layouts/AppLayout.vue'),
      children: [
        { path: '', redirect: { name: 'dashboard' } },
        { path: 'dashboard', name: 'dashboard', component: () => import('@/views/DashboardView.vue') },
        { path: 'scene', name: 'scene', component: () => import('@/views/SceneView.vue') },
        { path: 'trends', name: 'trends', component: () => import('@/views/TrendsView.vue') },
        { path: 'mqtt', name: 'mqtt', component: () => import('@/views/MqttView.vue') },
      ],
    },
    { path: '/:pathMatch(.*)*', redirect: { name: 'dashboard' } },
  ],
})

export default router
