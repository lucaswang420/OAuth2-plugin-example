<script setup lang="ts">
import { ref, onMounted } from 'vue'
import axios from 'axios'

const health = ref<any>(null)
const stats = ref<any>(null)
const loading = ref(true)

onMounted(async () => {
  try {
    const [healthResp, statsResp] = await Promise.all([
      axios.get('/health/ready'),
      axios.get('/api/admin/dashboard/stats'),
    ])
    health.value = healthResp.data
    stats.value = statsResp.data
  } catch (e) {
    health.value = { status: 'error' }
  } finally {
    loading.value = false
  }
})
</script>

<template>
  <div>
    <h2 class="text-2xl font-bold text-gray-900 mb-6">Dashboard</h2>

    <!-- Stats Grid -->
    <div class="grid grid-cols-2 md:grid-cols-4 gap-4 mb-8">
      <div class="bg-white rounded-lg shadow p-5">
        <p class="text-xs font-medium text-gray-500 uppercase">Total Users</p>
        <p class="mt-2 text-3xl font-bold text-gray-900">{{ loading ? '—' : (stats?.total_users ?? '—') }}</p>
      </div>
      <div class="bg-white rounded-lg shadow p-5">
        <p class="text-xs font-medium text-gray-500 uppercase">Applications</p>
        <p class="mt-2 text-3xl font-bold text-gray-900">{{ loading ? '—' : (stats?.total_clients ?? '—') }}</p>
      </div>
      <div class="bg-white rounded-lg shadow p-5">
        <p class="text-xs font-medium text-gray-500 uppercase">Active Tokens</p>
        <p class="mt-2 text-3xl font-bold text-indigo-600">{{ loading ? '—' : (stats?.active_tokens ?? '—') }}</p>
      </div>
      <div class="bg-white rounded-lg shadow p-5">
        <p class="text-xs font-medium text-gray-500 uppercase">Failures Today</p>
        <p class="mt-2 text-3xl font-bold" :class="(stats?.failures_today || 0) > 0 ? 'text-red-600' : 'text-gray-900'">
          {{ loading ? '—' : (stats?.failures_today ?? '—') }}
        </p>
      </div>
    </div>

    <!-- System Health -->
    <div class="grid grid-cols-1 md:grid-cols-3 gap-4 mb-8">
      <div class="bg-white rounded-lg shadow p-5">
        <div class="flex items-center gap-2">
          <div class="w-2.5 h-2.5 rounded-full" :class="health?.status === 'ok' ? 'bg-green-500' : 'bg-red-500'"></div>
          <h3 class="text-sm font-medium text-gray-500">System Status</h3>
        </div>
        <p class="mt-2 text-xl font-semibold" :class="health?.status === 'ok' ? 'text-green-600' : 'text-red-600'">
          {{ loading ? '...' : (health?.status === 'ok' ? 'Healthy' : 'Unhealthy') }}
        </p>
      </div>
      <div class="bg-white rounded-lg shadow p-5">
        <h3 class="text-sm font-medium text-gray-500">Database</h3>
        <p class="mt-2 text-xl font-semibold text-gray-900">{{ loading ? '...' : (health?.database || 'N/A') }}</p>
      </div>
      <div class="bg-white rounded-lg shadow p-5">
        <h3 class="text-sm font-medium text-gray-500">Redis</h3>
        <p class="mt-2 text-xl font-semibold text-gray-900">{{ loading ? '...' : (health?.redis || 'N/A') }}</p>
      </div>
    </div>

    <!-- Quick Links -->
    <div class="bg-white rounded-lg shadow p-6">
      <h3 class="text-lg font-medium text-gray-900 mb-4">Quick Actions</h3>
      <div class="grid grid-cols-2 md:grid-cols-4 gap-4">
        <router-link to="/applications" class="p-4 border rounded-lg hover:border-indigo-500 hover:bg-indigo-50 transition-colors text-center">
          <span class="text-2xl">📱</span>
          <p class="mt-2 text-sm font-medium text-gray-700">Applications</p>
        </router-link>
        <router-link to="/users" class="p-4 border rounded-lg hover:border-indigo-500 hover:bg-indigo-50 transition-colors text-center">
          <span class="text-2xl">👥</span>
          <p class="mt-2 text-sm font-medium text-gray-700">Users</p>
        </router-link>
        <router-link to="/roles" class="p-4 border rounded-lg hover:border-indigo-500 hover:bg-indigo-50 transition-colors text-center">
          <span class="text-2xl">🛡️</span>
          <p class="mt-2 text-sm font-medium text-gray-700">Roles</p>
        </router-link>
        <router-link to="/scopes" class="p-4 border rounded-lg hover:border-indigo-500 hover:bg-indigo-50 transition-colors text-center">
          <span class="text-2xl">🔐</span>
          <p class="mt-2 text-sm font-medium text-gray-700">Scopes</p>
        </router-link>
      </div>
    </div>
  </div>
</template>
