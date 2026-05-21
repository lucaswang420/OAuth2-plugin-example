import { test, expect } from '@playwright/test'
import { setupAuthenticatedMocks, loginAsAdmin, MOCK_CLIENTS } from './helpers/mock-api'

test.describe('Applications Management', () => {
  test.beforeEach(async ({ page }) => {
    await setupAuthenticatedMocks(page)
    await loginAsAdmin(page)
    await page.click('nav a:has-text("Applications")')
    await page.waitForURL('**/admin/applications')
  })

  test('displays applications list with correct columns', async ({ page }) => {
    await expect(page.locator('h2')).toContainText('Applications')
    await expect(page.locator('th:has-text("Name")')).toBeVisible()
    await expect(page.locator('th:has-text("Client ID")')).toBeVisible()
    await expect(page.locator('th:has-text("Type")')).toBeVisible()
    await expect(page.locator('th:has-text("Actions")')).toBeVisible()
  })

  test('shows all registered clients', async ({ page }) => {
    for (const client of MOCK_CLIENTS) {
      await expect(page.locator(`text=${client.client_id}`)).toBeVisible()
      await expect(page.locator(`text=${client.name}`)).toBeVisible()
    }
  })

  test('displays client type badges correctly', async ({ page }) => {
    await expect(page.locator('span:has-text("PUBLIC")')).toBeVisible()
    await expect(page.locator('span:has-text("CONFIDENTIAL")')).toBeVisible()
  })

  test('opens create application modal', async ({ page }) => {
    await page.click('button:has-text("Create Application")')
    await expect(page.locator('h3:has-text("Create Application")')).toBeVisible()
    await expect(page.locator('input[placeholder="My App"]')).toBeVisible()
    await expect(page.locator('select')).toBeVisible()
    // Grant types shown as checkboxes
    await expect(page.locator('label:has-text("Authorization Code")')).toBeVisible()
    await expect(page.locator('label:has-text("Refresh Token")')).toBeVisible()
    await expect(page.locator('label:has-text("Client Credentials")')).toBeVisible()
    await expect(page.locator('label:has-text("Device Code")')).toBeVisible()
  })

  test('creates a new application and shows secret', async ({ page }) => {
    await page.click('button:has-text("Create Application")')

    // Fill form
    await page.fill('input[placeholder="My App"]', 'Test Application')
    await page.selectOption('select', 'CONFIDENTIAL')
    await page.fill('input[placeholder*="myapp.com"]', 'https://test.com/callback')
    // Grant types are checkboxes - authorization_code is checked by default, add refresh_token
    await page.locator('label:has-text("Refresh Token") input[type="checkbox"]').check()

    // Submit - use the form submit button inside the modal
    await page.locator('.fixed button[type="submit"]').click()

    // Should show secret modal
    await expect(page.locator('h3:has-text("Client Secret")')).toBeVisible()
    await expect(page.locator('text=Copy this secret now')).toBeVisible()
    await expect(page.locator('.font-mono.select-all')).toContainText('generated-secret-abc123xyz')
  })

  test('closes secret modal with Done button', async ({ page }) => {
    await page.click('button:has-text("Create Application")')
    await page.fill('input[placeholder="My App"]', 'Test')
    await page.locator('.fixed button[type="submit"]').click()

    await expect(page.locator('h3:has-text("Client Secret")')).toBeVisible()
    await page.click('button:has-text("Done")')
    await expect(page.locator('h3:has-text("Client Secret")')).not.toBeVisible()
  })

  test('cancel button closes create modal', async ({ page }) => {
    await page.click('button:has-text("Create Application")')
    await expect(page.locator('h3:has-text("Create Application")')).toBeVisible()
    await page.click('button:has-text("Cancel")')
    await expect(page.locator('h3:has-text("Create Application")')).not.toBeVisible()
  })

  test('delete client with confirmation', async ({ page }) => {
    // Set up dialog handler
    page.on('dialog', (dialog) => dialog.accept())

    await page.click('button:has-text("Delete")')
    // After delete, page should still show (mock returns same list)
    await expect(page.locator('h2')).toContainText('Applications')
  })

  test('reset secret for confidential client', async ({ page }) => {
    page.on('dialog', (dialog) => dialog.accept())

    await page.click('button:has-text("Reset Secret")')

    // Should show new secret
    await expect(page.locator('h3:has-text("Client Secret")')).toBeVisible()
    await expect(page.locator('.font-mono.select-all')).toContainText('new-secret-after-reset-xyz789')
  })

  test('shows empty state when no clients exist', async ({ page }) => {
    await page.route('**/api/admin/clients', async (route) => {
      if (route.request().method() === 'GET') {
        await route.fulfill({
          status: 200,
          contentType: 'application/json',
          body: JSON.stringify({ clients: [] }),
        })
      } else {
        await route.continue()
      }
    })

    // Navigate away and back to trigger a fresh fetch with the new mock
    await page.click('nav a:has-text("Dashboard")')
    await page.click('nav a:has-text("Applications")')
    await page.waitForURL('**/admin/applications')
    await expect(page.locator('text=No applications registered yet')).toBeVisible()
    await expect(page.locator('button:has-text("Create your first application")')).toBeVisible()
  })
})
