<script setup lang="ts">
import { useAuthStore } from '../stores/auth'
import { useRouter } from 'vue-router'

const auth = useAuthStore()
const router = useRouter()

async function handleLogout() {
  await auth.logout()
  router.push('/login')
}

const navItems = [
  { name: 'Dashboard', path: '/', icon: '🏠' },
  { name: 'Profile', path: '/profile', icon: '👤' },
  { name: 'Security', path: '/security', icon: '🔒' },
  { name: 'Authorized Apps', path: '/authorized-apps', icon: '📱' },
]
</script>

<template>
  <div class="min-h-screen bg-gray-50">
    <!-- Top Navigation -->
    <nav class="bg-white border-b border-gray-200 shadow-sm">
      <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div class="flex justify-between h-16">
          <div class="flex items-center gap-8">
            <router-link to="/" class="text-xl font-bold text-indigo-600">OAuth2 App</router-link>
            <div class="hidden md:flex gap-1">
              <router-link v-for="item in navItems" :key="item.path" :to="item.path"
                class="px-3 py-2 rounded-md text-sm font-medium transition-colors"
                :class="$route.path === item.path ? 'bg-indigo-50 text-indigo-700' : 'text-gray-600 hover:text-gray-900 hover:bg-gray-50'">
                {{ item.icon }} {{ item.name }}
              </router-link>
            </div>
          </div>
          <div class="flex items-center gap-4">
            <span class="text-sm text-gray-600">{{ auth.user?.name || auth.user?.sub }}</span>
            <button @click="handleLogout" class="px-3 py-1.5 text-sm text-gray-600 hover:text-red-600 border border-gray-300 rounded-md hover:border-red-300 transition-colors">
              Sign Out
            </button>
          </div>
        </div>
      </div>
    </nav>

    <!-- Page Content -->
    <main class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
      <router-view />
    </main>
  </div>
</template>
