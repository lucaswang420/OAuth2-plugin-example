# Common functions for test scripts

function Get-PostgresContainer {
    # Try to find a running postgres container, suppress Docker errors
    try {
        $result = docker ps --format "{{.Names}}" 2>$null | Select-String -Pattern "postgres"
        return $result
    } catch {
        return $null
    }
}

function Invoke-PsqlQuery {
    param(
        [string]$Query,
        [string]$DbUser,
        [string]$DbName,
        [string]$DbPassword,
        [string]$DbHost
    )
    $env:PGPASSWORD = $DbPassword
    psql -U $DbUser -d $DbName -h $DbHost -c $Query 2>$null | Out-Null
    $exitCode = $LASTEXITCODE
    $env:PGPASSWORD = $null
    return $exitCode
}

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
        $containerName = Get-PostgresContainer
        if ($containerName) {
            docker exec $containerName psql -U $DbUser -d $DbName -c $query 2>$null | Out-Null
            if (-not $Silent) {
                Write-Host "Admin account reset successfully (Docker)" -ForegroundColor Green
            }
        } else {
            $exitCode = Invoke-PsqlQuery -Query $query -DbUser $DbUser -DbName $DbName -DbPassword $DbPassword -DbHost $DbHost
            if ($exitCode -eq 0) {
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
        $containerName = Get-PostgresContainer
        if ($containerName) {
            docker exec $containerName psql -U $DbUser -d $DbName -c $query 2>$null | Out-Null
            if (-not $Silent) {
                Write-Host "Admin lockout reset successfully (Docker)" -ForegroundColor Green
            }
        } else {
            $exitCode = Invoke-PsqlQuery -Query $query -DbUser $DbUser -DbName $DbName -DbPassword $DbPassword -DbHost $DbHost
            if ($exitCode -eq 0) {
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
