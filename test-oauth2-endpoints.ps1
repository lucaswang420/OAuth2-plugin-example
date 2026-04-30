# OAuth2 Endpoints Testing Script for PowerShell
# Usage: Run in PowerShell: .\test-oauth2-endpoints.ps1

$baseUrl = "http://127.0.0.1:5555"

# Error handling to pause on error
trap {
    Write-Host ""
    Write-Host "[-] Script terminated with error" -ForegroundColor Red
    Write-Host "Press any key to exit..." -ForegroundColor Gray
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    exit 1
}

Write-Host "=== OAuth2 Endpoints Testing ===" -ForegroundColor Cyan
Write-Host ""

# Test 1: Health Check
Write-Host "[*] Test 1: Health Check" -ForegroundColor Yellow
try {
    $response = Invoke-RestMethod -Uri "$baseUrl/health" -Method Get
    Write-Host "[+] Health check successful" -ForegroundColor Green
    Write-Host "   Status: $($response.status)" -ForegroundColor Gray
    Write-Host "   Service: $($response.service)" -ForegroundColor Gray

    if ($response.storage_type) {
        Write-Host "   Storage: $($response.storage_type)" -ForegroundColor Gray
    }
} catch {
    Write-Host "[-] Health check failed: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Write-Host "[!] Make sure the OAuth2 server is running:" -ForegroundColor Yellow
    Write-Host "   cd OAuth2Backend/build" -ForegroundColor Gray
    Write-Host "   ./OAuth2Backend -c ../config.json" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Press any key to exit..." -ForegroundColor Gray
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    exit 1
}
Write-Host ""

# Test 2: Login
Write-Host "[*] Test 2: OAuth2 Login" -ForegroundColor Yellow
try {
    $body = @{
        username = "admin"
        password = "admin"
        client_id = "vue-client"
        redirect_uri = "http://127.0.0.1:5173/callback"
        json = "true"
    }

    $response = Invoke-RestMethod -Uri "$baseUrl/oauth2/login" -Method Post -Body $body
    Write-Host "[+] Login successful" -ForegroundColor Green
    Write-Host "   Code: $($response.code)" -ForegroundColor Gray
    Write-Host "   Location: $($response.location)" -ForegroundColor Gray

    # Save authorization code for next test
    $authCode = $response.code
} catch {
    Write-Host "[-] Login failed: $($_.Exception.Message)" -ForegroundColor Red
    if ($_.ErrorDetails) {
        Write-Host "   Details: $($_.ErrorDetails.Message)" -ForegroundColor Red
    }
}
Write-Host ""

# Test 3: Exchange Code for Token (if login succeeded)
if ($authCode) {
    Write-Host "[*] Test 3: Exchange Authorization Code for Token" -ForegroundColor Yellow
    try {
        $tokenBody = @{
            grant_type = "authorization_code"
            code = $authCode
            redirect_uri = "http://127.0.0.1:5173/callback"
            client_id = "vue-client"
            client_secret = "123456"
        }

        $tokenResponse = Invoke-RestMethod -Uri "$baseUrl/oauth2/token" -Method Post -Body $tokenBody
        Write-Host "[+] Token exchange successful" -ForegroundColor Green
        Write-Host "   Access Token: $($tokenResponse.access_token.Substring(0, 20))..." -ForegroundColor Gray
        Write-Host "   Token Type: $($tokenResponse.token_type)" -ForegroundColor Gray
        Write-Host "   Expires In: $($tokenResponse.expires_in)s" -ForegroundColor Gray
        Write-Host "   Refresh Token: $($tokenResponse.refresh_token.Substring(0, 20))..." -ForegroundColor Gray

        # Save access token for next test
        $accessToken = $tokenResponse.access_token
    } catch {
        Write-Host "[-] Token exchange failed: $($_.Exception.Message)" -ForegroundColor Red
        if ($_.ErrorDetails) {
            Write-Host "   Details: $($_.ErrorDetails.Message)" -ForegroundColor Red
        }
    }
    Write-Host ""
}

# Test 4: Access Protected Resource (if token obtained)
if ($accessToken) {
    Write-Host "[*] Test 4: Access Protected Resource (UserInfo)" -ForegroundColor Yellow
    try {
        $headers = @{
            Authorization = "Bearer $accessToken"
        }

        $userInfo = Invoke-RestMethod -Uri "$baseUrl/oauth2/userinfo" -Method Get -Headers $headers
        Write-Host "[+] UserInfo access successful" -ForegroundColor Green
        Write-Host "   User ID: $($userInfo.sub)" -ForegroundColor Gray
        Write-Host "   Name: $($userInfo.name)" -ForegroundColor Gray
        Write-Host "   Email: $($userInfo.email)" -ForegroundColor Gray
    } catch {
        Write-Host "[-] UserInfo access failed: $($_.Exception.Message)" -ForegroundColor Red
        if ($_.ErrorDetails) {
            Write-Host "   Details: $($_.ErrorDetails.Message)" -ForegroundColor Red
        }
    }
    Write-Host ""
}

# Test 5: Test Admin Dashboard (should require admin role)
if ($accessToken) {
    Write-Host "[*] Test 5: Access Admin Dashboard" -ForegroundColor Yellow
    try {
        $headers = @{
            Authorization = "Bearer $accessToken"
        }

        $adminData = Invoke-RestMethod -Uri "$baseUrl/api/admin/dashboard" -Method Get -Headers $headers
        Write-Host "[+] Admin dashboard access successful" -ForegroundColor Green
        Write-Host "   Message: $($adminData.message)" -ForegroundColor Gray
        Write-Host "   Status: $($adminData.status)" -ForegroundColor Gray
    } catch {
        Write-Host "[-] Admin dashboard access failed: $($_.Exception.Message)" -ForegroundColor Red
        if ($_.ErrorDetails) {
            Write-Host "   Details: $($_.ErrorDetails.Message)" -ForegroundColor Red
        }
    }
    Write-Host ""
}

Write-Host "=== Testing Complete ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Press any key to exit..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
