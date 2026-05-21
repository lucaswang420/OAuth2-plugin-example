import { test, expect } from '@playwright/test'
import { setupAuthenticatedMocks, loginAsAdmin } from './helpers/mock-api'

test.describe('Dashboard', () => {
  test.beforeEach(async ({ page }) => {
    await setupAuthenticatedMocks(page)
    await loginAsAdmin(page)
  })

  test('displays system health status', async ({ page }) => {
    await expect(page.locator('text=System Status')).toBeVisible()
    await expect(page.locator('text=Healthy')).toBeVisible()
  })

  test('shows database connection status', async ({ page }) => {
    await expect(page.locator('text=Database')).toBeVisible()
    await expect(page.locator('text=connected').first()).toBeVisible()
  })

  test('shows Redis connection status', async ({ page }) => {
    await expect(page.locator('text=Redis')).toBeVisible()
  })

  test('displays quick action links', async ({ page }) => {
    await expect(page.locator('text=Quick Actions')).toBeVisible()
    // Quick action cards are in main content area
    const main = page.locator('main')
    await expect(main.locator('text=Applications')).toBeVisible()
    await expect(main.locator('text=Users')).toBeVisible()
    await expect(main.locator('text=Audit Logs')).toBeVisible()
    await expect(main.locator('text=Settings')).toBeVisible()
  })

  test('quick action links navigate correctly', async ({ page }) => {
    // Click quick action links (inside main content area, not sidebar)
    await page.locator('main a:has-text("Applications")').click()
    await expect(page).toHaveURL(/\/admin\/applications/)

    await page.goBack()
    await page.locator('main a:has-text("Users")').click()
    await expect(page).toHaveURL(/\/admin\/users/)
  })
})

test.describe('Dashboard - unhealthy state', () => {
  test('shows unhealthy status when backend is down', async ({ page }) => {
    await setupAuthenticatedMocks(page)
    // Override health to return error (must be set AFTER setupAuthenticatedMocks)
    await page.route('**/health/ready', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({ status: 'error', message: 'Database unreachable' }),
      })
    })
    await loginAsAdmin(page)
    await expect(page.locator('text=Unhealthy')).toBeVisible()
  })
})
