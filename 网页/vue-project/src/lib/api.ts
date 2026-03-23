export async function fetchJson<T>(url: string, init?: RequestInit): Promise<T | null> {
  const response = await fetch(url, init)
  if (response.status === 204) {
    return null
  }
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}`)
  }
  return (await response.json()) as T
}
