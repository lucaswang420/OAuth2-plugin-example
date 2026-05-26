param(
    [string]$BaseUrl = "http://127.0.0.1:5555"
)

$ErrorActionPreference = "Stop"
$passed = 0
$failed = 0
$total = 20

. "$PSScriptRoot\common-test-functions.ps1"

function Test-Endpoint {
    param([string]$Name, [scriptblock]$Block)
    Write-Host "[*] $Name" -ForegroundColor Cyan
    try {
        & $Block
        $script:passed++
        Write-Host "    [PASS]" -ForegroundColor Green
    } catch {
        $script:failed++
        Write-Host "    [FAIL] $($_.Exception.Message)" -ForegroundColor Red
    }
    Write-Host ""
}

# ========================================
# Pre-test Setup
# ========================================
Write-Host "========================================" -ForegroundColor Yellow
Write-Host "Pre-test Setup" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow
Reset-AdminAccount
Write-Host ""

Write-Host "========================================" -ForegroundColor Yellow
Write-Host "Admin New Endpoints Tests ($total tests)" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow
Write-Host "Base URL: $BaseUrl"
Write-Host ""

# Get admin token
$accessToken = $null
Test-Endpoint "Setup: Admin Login" {
    $loginBody = @{
        username = 'admin'; password = 'admin'
        client_id = 'admin-console'
        redirect_uri = 'http://localhost:5174/admin/callback'
        scope = 'openid profile admin'
        state = 'test-state'; json = 'true'
    }
    $login = Invoke-RestMethod -Uri "$BaseUrl/oauth2/login" -Method Post -Body $loginBody
    if (-not $login.code) { throw "no auth code" }
    $tok = Invoke-RestMethod -Uri "$BaseUrl/oauth2/token" -Method Post -Body @{
        grant_type = 'authorization_code'; code = $login.code
        redirect_uri = 'http://localhost:5174/admin/callback'
        client_id = 'admin-console'; client_secret = ''
    }
    if (-not $tok.access_token) { throw "no access_token" }
    $script:accessToken = $tok.access_token
    Write-Host "    Token: $($tok.access_token.Substring(0,16))..."
}

function Get-AuthHeaders { 
    return [hashtable]@{ Authorization = "Bearer $script:accessToken"; "Content-Type" = "application/json" } 
}

# ========================================
# Dashboard Stats
# ========================================
Test-Endpoint "Test 1: GET /api/admin/dashboard/stats" {
    $h = [hashtable]@{ Authorization = "Bearer $script:accessToken"; "Content-Type" = "application/json" }
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/dashboard/stats" -Method Get -Headers $h
    if ($r.status -ne "success") { throw "status != success" }
    if ($null -eq $r.total_users) { throw "missing total_users" }
    if ($null -eq $r.total_clients) { throw "missing total_clients" }
    if ($null -eq $r.active_tokens) { throw "missing active_tokens" }
    if ($null -eq $r.failures_today) { throw "missing failures_today" }
    Write-Host "    users=$($r.total_users), clients=$($r.total_clients), tokens=$($r.active_tokens), failures=$($r.failures_today)"
}

# ========================================
# User Detail
# ========================================
$adminUserId = $null
Test-Endpoint "Test 2: GET /api/admin/users (get admin user id)" {
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/users" -Method Get -Headers (Get-AuthHeaders)
    $adminUser = $r.users | Where-Object { $_.username -eq 'admin' }
    if (-not $adminUser) { throw "admin user not found in list" }
    $script:adminUserId = $adminUser.id
    Write-Host "    admin user id: $($script:adminUserId)"
}

Test-Endpoint "Test 3: GET /api/admin/users/:id - User Detail" {
    if (-not $adminUserId) { throw "skipped: no user id" }
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/users/$adminUserId" -Method Get -Headers (Get-AuthHeaders)
    if ($r.status -ne "success") { throw "status != success" }
    if ($r.username -ne "admin") { throw "username mismatch: $($r.username)" }
    if ($null -eq $r.email) { throw "missing email" }
    if ($null -eq $r.email_verified) { throw "missing email_verified" }
    if ($null -eq $r.mfa_enabled) { throw "missing mfa_enabled" }
    if ($null -eq $r.roles) { throw "missing roles" }
    if ($r.roles -isnot [array]) { throw "roles is not array" }
    if ($null -eq $r.locked) { throw "missing locked field" }
    Write-Host "    username=$($r.username), email=$($r.email), roles=[$($r.roles -join ',')]"
}

Test-Endpoint "Test 4: GET /api/admin/users/:id - Not Found" {
    try {
        Invoke-RestMethod -Uri "$BaseUrl/api/admin/users/99999999" -Method Get -Headers (Get-AuthHeaders) -ErrorAction Stop
        throw "should have returned 404"
    } catch {
        if ($_.Exception.Response.StatusCode -eq "NotFound") {
            Write-Host "    Correctly returned 404"
        } else { throw "expected 404, got: $($_.Exception.Response.StatusCode)" }
    }
}

Test-Endpoint "Test 5: PUT /api/admin/users/:id - Update User" {
    if (-not $adminUserId) { throw "skipped: no user id" }
    $body = @{ email_verified = $true } | ConvertTo-Json
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/users/$adminUserId" -Method Put -Headers (Get-AuthHeaders) -Body $body
    if ($r.status -ne "success") { throw "status != success: $($r.status)" }
    Write-Host "    $($r.message)"
}

Test-Endpoint "Test 6: GET /api/admin/users/:id/roles" {
    if (-not $adminUserId) { throw "skipped: no user id" }
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/users/$adminUserId/roles" -Method Get -Headers (Get-AuthHeaders)
    if ($r.status -ne "success") { throw "status != success" }
    if ($null -eq $r.roles) { throw "missing roles" }
    if ($r.roles -isnot [array]) { throw "roles is not array" }
    Write-Host "    roles: [$($r.roles.name -join ', ')]"
}

# ========================================
# Enable/Disable User
# ========================================
$testUserId = $null
Test-Endpoint "Test 7: Create test user for enable/disable" {
    $ts = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    $body = @{ username = "testuser_$ts"; password = "TestPass123"; email = "test_$ts@example.com" }
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/register" -Method Post -Body $body
    if (-not $r.message) { throw "registration failed" }
    # Get the user id
    $users = Invoke-RestMethod -Uri "$BaseUrl/api/admin/users" -Method Get -Headers (Get-AuthHeaders)
    $testUser = $users.users | Where-Object { $_.username -eq "testuser_$ts" }
    if (-not $testUser) { throw "test user not found after creation" }
    $script:testUserId = $testUser.id
    Write-Host "    Created test user id: $($script:testUserId)"
}

Test-Endpoint "Test 8: PUT /api/admin/users/:id/disable" {
    if (-not $testUserId) { throw "skipped: no test user" }
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/users/$testUserId/disable" -Method Put -Headers (Get-AuthHeaders)
    if ($r.status -ne "success") { throw "status != success: $($r.status)" }
    # Verify user is locked
    $user = Invoke-RestMethod -Uri "$BaseUrl/api/admin/users/$testUserId" -Method Get -Headers (Get-AuthHeaders)
    if (-not $user.locked) { throw "user should be locked after disable" }
    Write-Host "    User disabled and verified locked=true"
}

Test-Endpoint "Test 9: POST /api/admin/users/:id/enable" {
    if (-not $testUserId) { throw "skipped: no test user" }
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/users/$testUserId/enable" -Method Post -Headers (Get-AuthHeaders)
    if ($r.status -ne "success") { throw "status != success: $($r.status)" }
    # Verify user is unlocked
    $user = Invoke-RestMethod -Uri "$BaseUrl/api/admin/users/$testUserId" -Method Get -Headers (Get-AuthHeaders)
    if ($user.locked) { throw "user should be unlocked after enable" }
    Write-Host "    User enabled and verified locked=false"
}

# ========================================
# Role Management
# ========================================
$testRoleId = $null
Test-Endpoint "Test 10: GET /api/admin/roles" {
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/roles" -Method Get -Headers (Get-AuthHeaders)
    if ($r.status -ne "success") { throw "status != success" }
    if ($null -eq $r.roles) { throw "missing roles" }
    if ($r.roles -isnot [array]) { throw "roles is not array" }
    $adminRole = $r.roles | Where-Object { $_.name -eq 'admin' }
    if (-not $adminRole) { throw "admin role not found" }
    if ($null -eq $adminRole.user_count) { throw "missing user_count" }
    Write-Host "    Total roles: $($r.total), admin users: $($adminRole.user_count)"
}

Test-Endpoint "Test 11: POST /api/admin/roles - Create Role" {
    $ts = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    $body = @{ name = "testrole_$ts"; description = "Test role for API testing" } | ConvertTo-Json
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/roles" -Method Post -Headers (Get-AuthHeaders) -Body $body
    if ($r.status -ne "success") { throw "status != success: $($r.status)" }
    if (-not $r.id) { throw "missing id in response" }
    if (-not $r.name) { throw "missing name in response" }
    $script:testRoleId = $r.id
    Write-Host "    Created role id=$($r.id), name=$($r.name)"
}

Test-Endpoint "Test 12: POST /api/admin/roles - Duplicate Name" {
    try {
        $body = @{ name = "admin" } | ConvertTo-Json
        Invoke-RestMethod -Uri "$BaseUrl/api/admin/roles" -Method Post -Headers (Get-AuthHeaders) -Body $body -ErrorAction Stop
        throw "should have returned 409"
    } catch {
        if ($_.Exception.Response.StatusCode -eq "Conflict") {
            Write-Host "    Correctly returned 409 for duplicate role name"
        } else { throw "expected 409, got: $($_.Exception.Response.StatusCode)" }
    }
}

Test-Endpoint "Test 13: PUT /api/admin/roles/:id - Update Role" {
    if (-not $testRoleId) { throw "skipped: no test role" }
    $body = @{ description = "Updated description" } | ConvertTo-Json
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/roles/$testRoleId" -Method Put -Headers (Get-AuthHeaders) -Body $body
    if ($r.status -ne "success") { throw "status != success: $($r.status)" }
    Write-Host "    $($r.message)"
}

Test-Endpoint "Test 14: DELETE /api/admin/roles/:id - Delete Role" {
    if (-not $testRoleId) { throw "skipped: no test role" }
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/roles/$testRoleId" -Method Delete -Headers (Get-AuthHeaders)
    if ($r.status -ne "success") { throw "status != success: $($r.status)" }
    Write-Host "    $($r.message)"
}

Test-Endpoint "Test 15: DELETE /api/admin/roles - Cannot delete built-in" {
    # Get admin role id
    $roles = Invoke-RestMethod -Uri "$BaseUrl/api/admin/roles" -Method Get -Headers (Get-AuthHeaders)
    $adminRole = $roles.roles | Where-Object { $_.name -eq 'admin' }
    if (-not $adminRole) { throw "admin role not found" }
    try {
        Invoke-RestMethod -Uri "$BaseUrl/api/admin/roles/$($adminRole.id)" -Method Delete -Headers (Get-AuthHeaders) -ErrorAction Stop
        throw "should have returned 404 for built-in role"
    } catch {
        if ($_.Exception.Response.StatusCode -eq "NotFound") {
            Write-Host "    Correctly prevented deletion of built-in role"
        } else { throw "expected 404, got: $($_.Exception.Response.StatusCode)" }
    }
}

# ========================================
# Scope Management
# ========================================
$testScopeId = $null
Test-Endpoint "Test 16: POST /api/admin/scopes - Create Scope" {
    $ts = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    $body = @{ name = "testscope_$ts"; description = "Test scope"; mapped_role = "user"; is_default = $false; requires_admin_role = $false } | ConvertTo-Json
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/scopes" -Method Post -Headers (Get-AuthHeaders) -Body $body
    if ($r.status -ne "success") { throw "status != success: $($r.status)" }
    if (-not $r.id) { throw "missing id" }
    $script:testScopeId = $r.id
    Write-Host "    Created scope id=$($r.id), name=$($r.name)"
}

Test-Endpoint "Test 17: PUT /api/admin/scopes/:id - Update Scope" {
    if (-not $testScopeId) { throw "skipped: no test scope" }
    $body = @{ description = "Updated description"; is_default = $true } | ConvertTo-Json
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/scopes/$testScopeId" -Method Put -Headers (Get-AuthHeaders) -Body $body
    if ($r.status -ne "success") { throw "status != success: $($r.status)" }
    Write-Host "    $($r.message)"
}

Test-Endpoint "Test 18: DELETE /api/admin/scopes/:id - Delete Scope" {
    if (-not $testScopeId) { throw "skipped: no test scope" }
    $r = Invoke-RestMethod -Uri "$BaseUrl/api/admin/scopes/$testScopeId" -Method Delete -Headers (Get-AuthHeaders)
    if ($r.status -ne "success") { throw "status != success: $($r.status)" }
    Write-Host "    $($r.message)"
}

Test-Endpoint "Test 19: DELETE /api/admin/scopes - Cannot delete built-in" {
    $scopes = Invoke-RestMethod -Uri "$BaseUrl/api/admin/scopes" -Method Get -Headers (Get-AuthHeaders)
    $openidScope = $scopes.scopes | Where-Object { $_.name -eq 'openid' }
    if (-not $openidScope) { throw "openid scope not found" }
    try {
        Invoke-RestMethod -Uri "$BaseUrl/api/admin/scopes/$($openidScope.id)" -Method Delete -Headers (Get-AuthHeaders) -ErrorAction Stop
        throw "should have returned 404 for built-in scope"
    } catch {
        if ($_.Exception.Response.StatusCode -eq "NotFound") {
            Write-Host "    Correctly prevented deletion of built-in scope"
        } else { throw "expected 404, got: $($_.Exception.Response.StatusCode)" }
    }
}

Test-Endpoint "Test 20: POST /api/admin/scopes - Duplicate Name" {
    try {
        $body = @{ name = "openid" } | ConvertTo-Json
        Invoke-RestMethod -Uri "$BaseUrl/api/admin/scopes" -Method Post -Headers (Get-AuthHeaders) -Body $body -ErrorAction Stop
        throw "should have returned 409"
    } catch {
        if ($_.Exception.Response.StatusCode -eq "Conflict") {
            Write-Host "    Correctly returned 409 for duplicate scope name"
        } else { throw "expected 409, got: $($_.Exception.Response.StatusCode)" }
    }
}

# ========================================
# Post-test Cleanup
# ========================================
Write-Host ""
Write-Host "========================================" -ForegroundColor Yellow
Write-Host "Post-test Cleanup" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow
Reset-AdminAccount

Write-Host ""
Write-Host "========================================"
Write-Host "Admin New Endpoints Tests: $passed/$total passed, $failed failed"
Write-Host "========================================"

if ($failed -gt 0) {
    Write-Host "FAILED" -ForegroundColor Red
    exit 1
} else {
    Write-Host "ALL PASSED" -ForegroundColor Green
    exit 0
}
