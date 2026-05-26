<script setup lang="ts">
import { ref } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useAuthStore } from '../stores/auth'

const auth = useAuthStore()
const router = useRouter()
const route = useRoute()

const username = ref('')
const password = ref('')
const mfaCode = ref('')
const mfaToken = ref('')
const showMfa = ref(false)

async function handleLogin() {
  const result = await auth.login(username.value, password.value)
  if (result.mfaRequired) {
    mfaToken.value = result.mfaToken!
    showMfa.value = true
  } else if (result.success) {
    router.push((route.query.redirect as string) || '/')
  }
}

async function handleMfa() {
  const result = await auth.verifyMfa(mfaToken.value, mfaCode.value)
  if (result.success) {
    router.push((route.query.redirect as string) || '/')
  }
}
</script>

<template>
  <div class="min-h-screen flex items-center justify-center bg-gradient-to-br from-indigo-50 to-blue-100 px-4">
    <div class="w-full max-w-md">
      <div class="bg-white rounded-2xl shadow-xl p-8">
        <div class="text-center mb-8">
          <h1 class="text-3xl font-bold text-gray-900">Welcome Back</h1>
          <p class="mt-2 text-gray-500">Sign in to your account</p>
        </div>

        <!-- Error -->
        <div v-if="auth.error" class="mb-4 p-3 bg-red-50 border border-red-200 text-red-700 rounded-lg text-sm">
          {{ auth.error }}
        </div>

        <!-- MFA Step -->
        <form v-if="showMfa" @submit.prevent="handleMfa" class="space-y-5">
          <div class="text-center mb-4">
            <p class="text-sm text-gray-600">Enter the 6-digit code from your authenticator app</p>
          </div>
          <div>
            <input v-model="mfaCode" type="text" inputmode="numeric" maxlength="6" autocomplete="one-time-code"
              class="block w-full px-4 py-3 text-center text-2xl tracking-widest border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
              placeholder="000000" />
          </div>
          <button type="submit" :disabled="auth.loading || mfaCode.length !== 6"
            class="w-full py-3 px-4 bg-indigo-600 text-white font-medium rounded-lg hover:bg-indigo-700 disabled:opacity-50 transition-colors">
            {{ auth.loading ? 'Verifying...' : 'Verify Code' }}
          </button>
        </form>

        <!-- Login Form -->
        <form v-else @submit.prevent="handleLogin" class="space-y-5">
          <div>
            <label class="block text-sm font-medium text-gray-700 mb-1">Username</label>
            <input v-model="username" type="text" required autocomplete="username"
              class="block w-full px-4 py-3 border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
              placeholder="Enter your username" />
          </div>
          <div>
            <label class="block text-sm font-medium text-gray-700 mb-1">Password</label>
            <input v-model="password" type="password" required autocomplete="current-password"
              class="block w-full px-4 py-3 border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
              placeholder="Enter your password" />
          </div>
          <div class="flex justify-end">
            <router-link to="/forgot-password" class="text-sm text-indigo-600 hover:text-indigo-800">Forgot password?</router-link>
          </div>
          <button type="submit" :disabled="auth.loading"
            class="w-full py-3 px-4 bg-indigo-600 text-white font-medium rounded-lg hover:bg-indigo-700 disabled:opacity-50 transition-colors">
            {{ auth.loading ? 'Signing in...' : 'Sign In' }}
          </button>
        </form>

        <p class="mt-6 text-center text-sm text-gray-500">
          Don't have an account?
          <router-link to="/register" class="text-indigo-600 font-medium hover:text-indigo-800">Create one</router-link>
        </p>
      </div>
    </div>
  </div>
</template>
