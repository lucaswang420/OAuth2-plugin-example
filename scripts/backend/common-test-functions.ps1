# Common functions for test scripts

function Reset-AdminAccount {
    param(
        [string]$DbUser = "oauth2_user",
        [string]$DbName = "oauth2_db",
        [string]$DbPassword = "123456",
        [string]$DbHost = "localhost",
        [switch]$Silent
    )
    
    if (-not $Silent) {
        Write-Host "Resetting admin account (password + lockout)..." -ForegroundColor Cyan
    }
    
    # Default admin password hash (SHA-256 with salt 'admin_salt')
    # Password: 'admin'
    $defaultHash = "892738161086b314334f88d661aa6e7bab7c825c34bf55222811dad46cdbf724"
    $defaultSalt = "admin_salt"
    
    $query = "UPDATE users SET password_hash = '$defaultHash', salt = '$defaultSalt', failed_login_count = 0, locked_until = 0 WHERE username = 'admin';"
    
    try {
        # Try Docker first
        $containerName = docker ps --format "{{.Names}}" 2>$null | Select-String -Pattern "postgres"
        if ($containerName) {
            docker exec $containerName psql -U $DbUser -d $DbName -c $query 2>$null | Out-Null
            if (-not $Silent) {
                Write-Host "Admin account reset successfully (Docker)" -ForegroundColor Green
            }
        } else {
            # Try local PostgreSQL
            $env:PGPASSWORD = $DbPassword
            psql -U $DbUser -d $DbName -h $DbHost -c $query 2>$null | Out-Null
            $env:PGPASSWORD = $null
            
            if ($LASTEXITCODE -eq 0) {
                if (-not $Silent) {
                    Write-Host "Admin account reset successfully (Local PostgreSQL)" -ForegroundColor Green
                }
            } else {
                if (-not $Silent) {
                    Write-Host "Warning: Could not reset admin account" -ForegroundColor Yellow
                }
            }
        }
    } catch {
        if (-not $Silent) {
            Write-Host "Warning: Failed to reset admin account: $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }
}

function Reset-AdminLockout {
    param(
        [string]$DbUser = "oauth2_user",
        [string]$DbName = "oauth2_db",
        [string]$DbPassword = "123456",
        [string]$DbHost = "localhost",
        [switch]$Silent
    )
    
    if (-not $Silent) {
        Write-Host "Resetting admin account lockout..." -ForegroundColor Cyan
    }
    
    $query = "UPDATE users SET failed_login_count = 0, locked_until = 0 WHERE username = 'admin';"
    
    try {
        # Try Docker first
        $containerName = docker ps --format "{{.Names}}" 2>$null | Select-String -Pattern "postgres"
        if ($containerName) {
            docker exec $containerName psql -U $DbUser -d $DbName -c $query 2>$null | Out-Null
            if (-not $Silent) {
                Write-Host "Admin lockout reset successfully (Docker)" -ForegroundColor Green
            }
        } else {
            # Try local PostgreSQL
            $env:PGPASSWORD = $DbPassword
            psql -U $DbUser -d $DbName -h $DbHost -c $query 2>$null | Out-Null
            $env:PGPASSWORD = $null
            
            if ($LASTEXITCODE -eq 0) {
                if (-not $Silent) {
                    Write-Host "Admin lockout reset successfully (Local PostgreSQL)" -ForegroundColor Green
                }
            } else {
                if (-not $Silent) {
                    Write-Host "Warning: Could not reset admin lockout" -ForegroundColor Yellow
                }
            }
        }
    } catch {
        if (-not $Silent) {
            Write-Host "Warning: Failed to reset admin lockout: $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }
}
