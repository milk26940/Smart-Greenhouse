export type HealthResponse = {
  status: string
  storage: string
  service: string
}

export type RealtimeResponse = {
  hasData: boolean
  summary: string
  dataSource: string
  updatedAt: string
  env: {
    temperature: number
    humidity: number
    soil: number
    light: number
    co2: number
  }
  decision?: {
    fan: string
    pump: string
    led: string
    summary: string
  }
  vision: {
    brightness: number
    greenRatio: number
    motionFlag: boolean
    visionOk: boolean
  }
  presence: {
    sensor1: string
    sensor2: string
    sensor3: string
    executor4: string
    executor5: string
  }
}

export type HistoryRecord = {
  createdAt: string
  source: string
  temperature: number
  humidity: number
  soil: number
  light: number
  co2: number
  brightness: number
  greenRatio: number
  motionFlag: boolean
  visionOk: boolean
}

export type HistoryResponse = {
  start: string
  end: string
  count: number
  records: HistoryRecord[]
}

export type MetricStats = {
  average: number
  max: number
  min: number
  latest: number
}

export type StatsResponse = {
  start?: string
  end?: string
  sampleCount: number
  temperature: MetricStats
  humidity: MetricStats
  soil: MetricStats
  light: MetricStats
  co2: MetricStats
}

export type PresenceResponse = {
  sensor1: string
  sensor2: string
  sensor3: string
  executor4: string
  executor5: string
  latestRecordAt: string | null
}

export type MqttLogLevel = 'info' | 'success' | 'warning' | 'error'

export type MqttForm = {
  name: string
  protocol: 'ws' | 'wss'
  host: string
  port: number
  path: string
  clientId: string
  username: string
  password: string
  useTls: boolean
  topic: string
  qos: 0 | 1 | 2
}

export type MqttLogEntry = {
  id: number
  time: string
  level: MqttLogLevel
  message: string
}
