import { test, expect } from '@playwright/test'
import { setupAuthenticatedMocks, loginAsAdmin, MOCK_SCOPES } from './helpers/mock-api'

test.describe('Settings & Scopes', () => {
  test.beforeEach(async ({ page }) => {
    await setupAuthenticatedMocks(page)
    await loginAsAdmin(page)
    await page.click('nav a:has-text("Settings")')
    await page.waitForURL('**/admin/settings')
  })

  test('displays settings page title', async ({ page }) => {
    await expect(page.locator('h2')).toContainText('Settings & Scopes')
  })

  test('shows OAuth2 Scopes section', async ({ page }) => {
    await expect(page.locator('h3:has-text("OAuth2 Scopes")')).toBeVisible()
  })

  test('displays scope table with correct columns', async ({ page }) => {
    await expect(page.locator('th:has-text("Name")')).toBeVisible()
    await expect(page.locator('th:has-text("Description")')).toBeVisible()
    await expect(page.locator('th:has-text("Mapped Role")')).toBeVisible()
    await expect(page.locator('th:has-text("Default")')).toBeVisible()
    await expect(page.locator('th:has-text("Admin Only")')).toBeVisible()
  })

  test('shows all scopes with correct data', async ({ page }) => {
    for (const scope of MOCK_SCOPES) {
      await expect(page.locator(`text=${scope.name}`).first()).toBeVisible()
    }
  })

  test('displays scope descriptions', async ({ page }) => {
    await expect(page.locator('text=OpenID Connect')).toBeVisible()
    await expect(page.locator('text=User profile')).toBeVisible()
    await expect(page.locator('text=Admin access')).toBeVisible()
  })

  test('shows default scope indicators', async ({ page }) => {
    // openid and profile rows should have a checkmark in the Default column (4th)
    // The openid row's 4th td should not contain "—"
    const openidRow = page.locator('tbody tr').first()
    const defaultCell = openidRow.locator('td').nth(3)
    await expect(defaultCell).not.toContainText('—')
  })

  test('shows admin-only scope indicators', async ({ page }) => {
    // admin scope row (3rd row) should have a checkmark in Admin Only column (5th)
    const adminRow = page.locator('tbody tr').nth(2)
    const adminOnlyCell = adminRow.locator('td').nth(4)
    await expect(adminOnlyCell).not.toContainText('—')
  })

  test('shows mapped role for admin scope', async ({ page }) => {
    // The admin scope row has mapped_role: 'admin' in the third column
    const adminRow = page.locator('tr:has-text("Admin access")')
    // The mapped role cell contains "admin" text
    await expect(adminRow.locator('td').nth(2)).toContainText('admin')
  })
})
