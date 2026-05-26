import { createRouter, createWebHistory } from 'vue-router'
import { useAuthStore } from '../stores/auth'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    // Public routes
    { path: '/login', name: 'login', component: () => import('../pages/LoginPage.vue'), meta: { guest: true } },
    { path: '/register', name: 'register', component: () => import('../pages/RegisterPage.vue'), meta: { guest: true } },
    { path: '/callback', name: 'callback', component: () => import('../pages/CallbackPage.vue') },
    { path: '/forgot-password', name: 'forgot-password', component: () => import('../pages/ForgotPasswordPage.vue'), meta: { guest: true } },
    { path: '/reset-password', name: 'reset-password', component: () => import('../pages/ResetPasswordPage.vue'), meta: { guest: true } },
    { path: '/verify-email', name: 'verify-email', component: () => import('../pages/VerifyEmailPage.vue') },
    { path: '/device/verify', name: 'device-verify', component: () => import('../pages/DeviceVerifyPage.vue') },
    { path: '/consent', name: 'consent', component: () => import('../pages/ConsentPage.vue') },

    // Protected routes (require auth)
    {
      path: '/',
      component: () => import('../layouts/AppLayout.vue'),
      meta: { auth: true },
      children: [
        { path: '', name: 'dashboard', component: () => import('../pages/DashboardPage.vue') },
        { path: 'profile', name: 'profile', component: () => import('../pages/ProfilePage.vue') },
        { path: 'security', name: 'security', component: () => import('../pages/SecurityPage.vue') },
        { path: 'authorized-apps', name: 'authorized-apps', component: () => import('../pages/AuthorizedAppsPage.vue') },
      ],
    },
  ],
})

router.beforeEach((to, _from, next) => {
  const auth = useAuthStore()
  if (to.meta.auth && !auth.isAuthenticated) {
    next({ name: 'login', query: { redirect: to.fullPath } })
  } else if (to.meta.guest && auth.isAuthenticated) {
    next({ name: 'dashboard' })
  } else {
    next()
  }
})

export default router
