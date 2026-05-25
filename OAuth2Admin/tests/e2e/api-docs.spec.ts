import { test, expect } from '@playwright/test';

test.describe('API Documentation (Swagger UI)', () => {
  const API_DOCS_URL = 'http://localhost:5555/docs/api/';

  test('should load Swagger UI successfully', async ({ page }) => {
    await page.goto(API_DOCS_URL);
    
    // Check if the title is correct
    await expect(page).toHaveTitle(/OAuth2 Authorization Server API Documentation/);
    
    // Wait for the spec to load (it renders the info section)
    const infoTitle = page.locator('.info .title');
    await expect(infoTitle).toBeVisible({ timeout: 10000 });
    await expect(infoTitle).toContainText('OAuth2 Authorization Server API');
  });

  test('should execute /health endpoint successfully via Try it out', async ({ page }) => {
    await page.goto(API_DOCS_URL);

    // Find the health endpoint
    const healthEndpoint = page.locator('.opblock-summary-path').filter({ hasText: '/health' }).first();
    await healthEndpoint.click();

    // Click "Try it out"
    await page.getByRole('button', { name: 'Try it out' }).click();

    // Click "Execute"
    await page.getByRole('button', { name: 'Execute' }).click();

    // Wait for response and check status code 200
    const responseStatus = page.locator('.responses-table .response-col_status').filter({ hasText: /200/ }).first();
    await expect(responseStatus).toBeVisible({ timeout: 15000 });

    // Check response body - specifically the one containing the JSON response
    const responseBody = page.locator('.live-responses-table .microlight').first();
    await expect(responseBody).toContainText('"status": "ok"', { timeout: 15000 });
  });

  test('should list all major tags (System, OAuth2, Admin, User Profile, etc.)', async ({ page }) => {
    await page.goto(API_DOCS_URL);

    const tags = ['System', 'OAuth2', 'Admin', 'User Profile', 'MFA', 'WebAuthn'];
    for (const tag of tags) {
      const tagElement = page.locator('.opblock-tag').filter({ hasText: tag });
      await expect(tagElement.first()).toBeVisible();
    }
  });
  test('should find /oauth2/token endpoint and show its parameters', async ({ page }) => {
    await page.goto(API_DOCS_URL);

    // Find and expand the OAuth2 tag
    const oauth2Tag = page.locator('.opblock-tag').filter({ hasText: 'OAuth2' }).first();
    await oauth2Tag.scrollIntoViewIfNeeded();
    
    // Check if expanded (aria-expanded or checking if following sibling is visible)
    // A simple way is to click it if the endpoint isn't visible yet
    const tokenEndpoint = page.locator('.opblock-summary-path').filter({ hasText: '/oauth2/token' }).first();
    if (!await tokenEndpoint.isVisible()) {
      await oauth2Tag.click();
    }

    await tokenEndpoint.scrollIntoViewIfNeeded();
    await expect(tokenEndpoint).toBeVisible();
    await tokenEndpoint.click();

    // Check for some parameters
    const paramNames = ['grant_type', 'client_id', 'client_secret'];
    for (const name of paramNames) {
      const paramElement = page.locator('.parameter__name').filter({ hasText: name }).first();
      await expect(paramElement).toBeVisible({ timeout: 10000 });
    }
  });
});
