export function formatDateTime(value?: string | null) {
  if (!value) {
    return '暂无记录'
  }
  return new Date(value).toLocaleString('zh-CN', { hour12: false })
}

export function formatTime(value?: string | null) {
  if (!value) {
    return '--:--:--'
  }
  return new Date(value).toLocaleTimeString('zh-CN', { hour12: false })
}

export function formatNumber(value?: number | null, digits = 1) {
  if (value === undefined || value === null || Number.isNaN(value)) {
    return '--'
  }
  return value.toFixed(digits)
}

export function formatInteger(value?: number | null) {
  if (value === undefined || value === null || Number.isNaN(value)) {
    return '--'
  }
  return Math.round(value).toString()
}
