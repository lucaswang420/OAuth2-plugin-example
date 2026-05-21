import { Page } from '@playwright/test'

/**
 * Mock API responses for E2E tests.
 * Intercepts backend calls so tests run without a real OAuth2 server.
 */

export const ADMIN_USER = {
  sub: '550e8400-e29b-41d4-a716-446655440000',
  username: 'admin',
  email: 'admin@example.com',
  email_verified: true,
  roles: ['admin', 'user'],
}

export const MOCK_CLIENTS = [
  {
    client_id: 'vue-client',
    name: 'Vue Frontend',
    client_type: 'PUBLIC',
    redirect_uris: 'http://localhost:8080/callback',
    allowed_grant_types: 'authorization_code',
  },
  {
    client_id: 'api-service',
    name: 'API Service',
    client_type: 'CONFIDENTIAL',
    redirect_uris: 'https://api.example.com/callback',
    allowed_grant_types: 'client_credentials',
  },
]

export const MOCK_USERS = [
  {
    id: '550e8400-e29b-41d4-a716-446655440000',
    username: 'admin',
    email: 'admin@example.com',
    email_verified: true,
    mfa_enabled: true,
  },
  {
    id: '660e8400-e29b-41d4-a716-446655440001',
    username: 'testuser',
    email: 'test@example.com',
    email_verified: false,
    mfa_enabled: false,
  },
]

export const MOCK_SCOPES = [
  { id: 1, name: 'openid', description: 'OpenID Connect', mapped_role: null, is_default: true, requires_admin_role: false },
  { id: 2, name: 'profile', description: 'User profile', mapped_role: null, is_default: true, requires_admin_role: false },
  { id: 3, name: 'admin', description: 'Admin access', mapped_role: 'admin', is_default: false, requires_admin_role: true },
]

export const MOCK_LOGS = [
  { id: 1, action: 'login_success', actor_type: 'user', actor_id: '550e8400-e29b-41d4-a716-446655440000', outcome: 'success', ip: '127.0.0.1', timestamp: '2026-05-21T10:00:00Z' },
  { id: 2, action: 'token_issued', actor_type: 'client', actor_id: 'vue-client', outcome: 'success', ip: '127.0.0.1', timestamp: '2026-05-21T10:00:01Z' },
  { id: 3, action: 'login_failure', actor_type: 'user', actor_id: '660e8400-e29b-41d4-a716-446655440001', outcome: 'failure', ip: '192.168.1.100', timestamp: '2026-05-21T09:55:00Z' },
]

/**
 * Set up all API mocks for an authenticated admin session.
 */
export async function setupAuthenticatedMocks(page: Page) {
  // Login endpoint
  await page.route('**/oauth2/login', async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ code: 'mock-auth-code-12345' }),
    })
  })

  // Token exchange
  await page.route('**/oauth2/token', async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({
        access_token: 'mock-access-token',
        refresh_token: 'mock-refresh-token',
        token_type: 'Bearer',
        expires_in: 3600,
      }),
    })
  })

  // UserInfo
  await page.route('**/oauth2/userinfo', async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify(ADMIN_USER),
    })
  })

  // Health
  await page.route('**/health/ready', async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ status: 'ok', database: 'connected', redis: 'connected' }),
    })
  })

  // Admin clients
  await page.route('**/api/admin/clients', async (route) => {
    if (route.request().method() === 'GET') {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({ clients: MOCK_CLIENTS }),
      })
    } else if (route.request().method() === 'POST') {
      await route.fulfill({
        status: 201,
        contentType: 'application/json',
        body: JSON.stringify({
          client_id: 'new-client-' + Date.now(),
          client_secret: 'generated-secret-abc123xyz',
          name: 'New App',
          client_type: 'CONFIDENTIAL',
        }),
      })
    } else {
      await route.continue()
    }
  })

  // Delete client
  await page.route('**/api/admin/clients/*', async (route) => {
    if (route.request().method() === 'DELETE') {
      await route.fulfill({ status: 204 })
    } else {
      await route.continue()
    }
  })

  // Reset secret
  await page.route('**/api/admin/clients/*/reset-secret', async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ client_secret: 'new-secret-after-reset-xyz789' }),
    })
  })

  // Admin users
  await page.route('**/api/admin/users', async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ users: MOCK_USERS }),
    })
  })

  // Assign roles
  await page.route('**/api/admin/users/*/roles', async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ message: 'Roles updated' }),
    })
  })

  // Scopes
  await page.route('**/api/admin/scopes', async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ scopes: MOCK_SCOPES }),
    })
  })

  // Audit logs
  await page.route('**/api/admin/logs**', async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ logs: MOCK_LOGS }),
    })
  })
}

/**
 * Perform login through the UI.
 */
export async function loginAsAdmin(page: Page) {
  await page.goto('/admin/login')
  await page.fill('input[type="text"]', 'admin')
  await page.fill('input[type="password"]', 'admin')
  await page.click('button[type="submit"]')
  // Wait for navigation to dashboard
  await page.waitForURL('**/admin/')
}
