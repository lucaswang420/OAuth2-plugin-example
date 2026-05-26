import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import axios from 'axios'

const CLIENT_ID = 'vue-client'
const REDIRECT_URI = window.location.origin + '/callback'

export const useAuthStore = defineStore('auth', () => {
  const accessToken = ref<string | null>(localStorage.getItem('access_token'))
  const refreshToken = ref<string | null>(localStorage.getItem('refresh_token'))
  const user = ref<any>(null)
  const loading = ref(false)
  const error = ref('')

  const isAuthenticated = computed(() => !!accessToken.value)

  function setTokens(access: string, refresh: string) {
    accessToken.value = access
    refreshToken.value = refresh
    localStorage.setItem('access_token', access)
    localStorage.setItem('refresh_token', refresh)
  }

  function clearTokens() {
    accessToken.value = null
    refreshToken.value = null
    user.value = null
    localStorage.removeItem('access_token')
    localStorage.removeItem('refresh_token')
  }

  async function login(username: string, password: string, scope = 'openid profile email') {
    error.value = ''
    loading.value = true
    try {
      const loginResp = await axios.post('/oauth2/login', new URLSearchParams({
        username, password,
        client_id: CLIENT_ID,
        redirect_uri: REDIRECT_URI,
        scope,
        state: crypto.randomUUID(),
        json: 'true',
      }))

      if (loginResp.data.mfa_required) {
        return { mfaRequired: true, mfaToken: loginResp.data.mfa_token }
      }

      const code = loginResp.data.code
      if (!code) throw new Error('No authorization code received')

      const tokenResp = await axios.post('/oauth2/token', new URLSearchParams({
        grant_type: 'authorization_code',
        code,
        redirect_uri: REDIRECT_URI,
        client_id: CLIENT_ID,
        client_secret: '123456',
      }))

      setTokens(tokenResp.data.access_token, tokenResp.data.refresh_token)
      await fetchUser()
      return { success: true }
    } catch (e: any) {
      error.value = e.response?.data?.error_description || e.response?.data?.error || e.message || 'Login failed'
      return { error: error.value }
    } finally {
      loading.value = false
    }
  }

  async function verifyMfa(mfaToken: string, code: string) {
    error.value = ''
    loading.value = true
    try {
      const resp = await axios.post('/oauth2/mfa/verify', new URLSearchParams({
        mfa_token: mfaToken,
        code,
        client_id: CLIENT_ID,
        redirect_uri: REDIRECT_URI,
      }))
      if (resp.data.access_token) {
        setTokens(resp.data.access_token, resp.data.refresh_token)
        await fetchUser()
        return { success: true }
      }
      return { error: 'MFA verification failed' }
    } catch (e: any) {
      error.value = e.response?.data?.error_description || 'MFA verification failed'
      return { error: error.value }
    } finally {
      loading.value = false
    }
  }

  async function fetchUser() {
    if (!accessToken.value) return
    try {
      const resp = await axios.get('/oauth2/userinfo', {
        headers: { Authorization: `Bearer ${accessToken.value}` },
      })
      user.value = resp.data
    } catch {
      clearTokens()
    }
  }

  async function refreshAccessToken() {
    if (!refreshToken.value) return false
    try {
      const resp = await axios.post('/oauth2/token', new URLSearchParams({
        grant_type: 'refresh_token',
        refresh_token: refreshToken.value,
        client_id: CLIENT_ID,
        client_secret: '123456',
      }))
      setTokens(resp.data.access_token, resp.data.refresh_token)
      return true
    } catch {
      clearTokens()
      return false
    }
  }

  async function logout() {
    if (accessToken.value) {
      try {
        await axios.post('/oauth2/revoke', new URLSearchParams({
          token: accessToken.value,
          client_id: CLIENT_ID,
          client_secret: '123456',
        }), { headers: { Authorization: `Bearer ${accessToken.value}` } })
      } catch {}
    }
    clearTokens()
  }

  // Axios interceptors
  axios.interceptors.request.use((config) => {
    if (accessToken.value && !config.headers.Authorization) {
      config.headers.Authorization = `Bearer ${accessToken.value}`
    }
    return config
  })

  axios.interceptors.response.use(
    (response) => response,
    async (err) => {
      if (err.response?.status === 401 && refreshToken.value && !err.config._retry) {
        err.config._retry = true
        const refreshed = await refreshAccessToken()
        if (refreshed) {
          err.config.headers.Authorization = `Bearer ${accessToken.value}`
          return axios(err.config)
        }
      }
      return Promise.reject(err)
    }
  )

  // Initialize: fetch user if token exists
  if (accessToken.value) {
    fetchUser()
  }

  return {
    accessToken, user, loading, error, isAuthenticated,
    login, verifyMfa, fetchUser, refreshAccessToken, logout, clearTokens, setTokens,
  }
})
