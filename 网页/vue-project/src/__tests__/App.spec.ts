import { beforeEach, describe, expect, it, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'

const mqttMock = vi.hoisted(() => {
  const handlers = new Map<string, (...args: unknown[]) => void>()
  const client = {
    on: vi.fn((event: string, handler: (...args: unknown[]) => void) => {
      handlers.set(event, handler)
      return client
    }),
    subscribe: vi.fn(),
    removeAllListeners: vi.fn(() => {
      handlers.clear()
    }),
    end: vi.fn(),
  }

  return {
    handlers,
    client,
    connect: vi.fn(() => client),
  }
})

vi.mock('mqtt', () => ({
  default: {
    connect: mqttMock.connect,
  },
}))

import DashboardView from '../views/DashboardView.vue'
import MqttView from '../views/MqttView.vue'

async function flushView() {
  await Promise.resolve()
  await Promise.resolve()
}

describe('App', () => {
  beforeEach(() => {
    mqttMock.handlers.clear()
    vi.unstubAllGlobals()
    vi.stubGlobal(
      'fetch',
      vi.fn((url: string) => {
        if (url === '/api/health') {
          return Promise.resolve(
            new Response(JSON.stringify({ status: 'online', storage: 'mysql', service: 'greenhouse-web' }), {
              status: 200,
              headers: { 'Content-Type': 'application/json' },
            }),
          )
        }
        if (url === '/api/realtime') {
          return Promise.resolve(
            new Response(
              JSON.stringify({
                hasData: true,
                summary: '土壤偏干，建议启动灌溉',
                dataSource: 'seed',
                updatedAt: '2026-03-19T20:00:00',
                env: { temperature: 25, humidity: 60, soil: 35, light: 1400, co2: 360 },
                decision: { fan: 'OFF', pump: 'ON', led: 'OFF', summary: '土壤偏干，建议启动灌溉' },
                vision: { brightness: 120, greenRatio: 0.02, motionFlag: false, visionOk: true },
                presence: {
                  sensor1: 'ONLINE',
                  sensor2: 'UNKNOWN',
                  sensor3: 'UNKNOWN',
                  executor4: 'UNKNOWN',
                  executor5: 'UNKNOWN',
                },
              }),
              {
                status: 200,
                headers: { 'Content-Type': 'application/json' },
              },
            ),
          )
        }
        if (url === '/api/presence') {
          return Promise.resolve(
            new Response(
              JSON.stringify({
                sensor1: 'ONLINE',
                sensor2: 'UNKNOWN',
                sensor3: 'UNKNOWN',
                executor4: 'UNKNOWN',
                executor5: 'UNKNOWN',
                latestRecordAt: '2026-03-19T20:00:00',
              }),
              {
                status: 200,
                headers: { 'Content-Type': 'application/json' },
              },
            ),
          )
        }
        if (url === '/api/telemetry') {
          return Promise.resolve(new Response(JSON.stringify({ ok: true }), { status: 200 }))
        }

        return Promise.resolve(
          new Response(JSON.stringify({ message: 'not found' }), {
            status: 404,
            headers: { 'Content-Type': 'application/json' },
          }),
        )
      }),
    )
  })

  it('renders the dashboard hero and realtime metric cards', async () => {
    const wrapper = mount(DashboardView, {
      global: {
        plugins: [createPinia()],
        stubs: {
          RouterLink: { template: '<a><slot /></a>' },
        },
      },
    })
    await flushView()

    expect(wrapper.text()).toContain('智慧农业大棚监控中心')
    expect(wrapper.text()).toContain('实时环境核心指标')
    expect(wrapper.findAll('.metric-card')).toHaveLength(6)
  })

  it('shows mqtt panel and opens config dialog', async () => {
    const wrapper = mount(MqttView, {
      global: {
        plugins: [createPinia()],
      },
    })
    await flushView()

    expect(wrapper.text()).toContain('通信链路监控')
    expect(wrapper.text()).toContain('连接日志')

    const editButton = wrapper.findAll('button').find((button) => button.text().includes('编辑 MQTT 配置'))

    expect(editButton).toBeTruthy()

    await editButton!.trigger('click')
    await flushView()

    expect(wrapper.text()).toContain('MQTT 连接配置')
    expect(wrapper.text()).toContain('Client ID')
    expect(wrapper.text()).toContain('Topic')
  })

  it('renders latest mqtt message card below the log card', async () => {
    const wrapper = mount(MqttView, {
      global: {
        plugins: [createPinia()],
      },
    })
    await flushView()

    const mqttPanel = wrapper.find('.mqtt-panel')
    const cardTitles = mqttPanel.findAll('.subpanel-header span').map((node) => node.text())

    expect(cardTitles).toEqual(['连接日志', '最近消息'])

    const cards = mqttPanel.findAll('.log-card')
    expect(cards).toHaveLength(2)
    expect(cards[0]!.text()).toContain('连接日志')
    expect(cards[1]!.text()).toContain('最近消息')
  })

  it('shows arrival timestamp after receiving mqtt message', async () => {
    const wrapper = mount(MqttView, {
      global: {
        plugins: [createPinia()],
      },
    })
    await flushView()

    const messageCard = wrapper.find('.mqtt-message-card')
    expect(messageCard.text()).toContain('尚未收到消息')

    const vm = wrapper.vm as { handleIncomingMessage: (topic: string, payload: Uint8Array) => void }
    vm.handleIncomingMessage('smart-greenhouse/node1/state', new TextEncoder().encode('{"node_id":1}'))
    await flushView()

    const updatedCard = wrapper.find('.mqtt-message-card')
    const text = updatedCard.text()
    expect(text).not.toContain('尚未收到消息')
    expect(text).toMatch(/\d{1,2}:\d{2}:\d{2}/)
    expect(text).toContain('到达')
  })
})
