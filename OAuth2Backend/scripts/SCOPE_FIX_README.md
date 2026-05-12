# OAuth2 Scope Configuration Fix Guide

## Problem

If you see this error when clicking "Sign in with Drogon":

```json
{"error":"invalid_scope","error_description":"Scopes not allowed for this client: openid, profile"}
```

This means the `vue-client` is not configured to allow `openid` and `profile` scopes.

## Root Cause

The OAuth2 backend requires three-tier scope validation per design doc v5.1:
1. **Client scope validation**: Check `oauth2_client_scopes` table
2. **User role validation**: Check user roles against scope requirements  
3. **User consent validation**: Check if user has granted consent

The error occurs because step 1 fails - `vue-client` has no allowed scopes configured.

## Solution

### Option 1: Quick Fix (Recommended)

Run the existing scope configuration script:

```bash
# Linux/macOS
cd OAuth2Backend/sql
psql -U test -d oauth_test -f 004_oauth2_scopes.sql

# Windows (Git Bash or WSL)
cd OAuth2Backend/sql
psql -U test -d oauth_test -f 004_oauth2_scopes.sql

# Or use the setup script (includes all SQL files)
cd OAuth2Backend/scripts
./setup_database.bat
```

### Option 2: Manual Fix

Connect to your PostgreSQL database and run the relevant part from `sql/004_oauth2_scopes.sql`:

```sql
-- Ensure oauth2_scopes table has default scopes
INSERT INTO oauth2_scopes (name, description, mapped_role, is_default, requires_admin_role) VALUES
('openid', 'OpenID Connect身份认证 - 允许客户端验证用户身份', 'user', TRUE, FALSE),
('profile', '访问用户基本信息 (用户名、邮箱等)', 'user', TRUE, FALSE),
('email', '访问用户邮箱地址', 'user', FALSE, FALSE)
ON CONFLICT (name) DO NOTHING;

-- Grant default scopes to vue-client
INSERT INTO oauth2_client_scopes (client_id, scope_name)
SELECT 'vue-client', name
FROM oauth2_scopes
WHERE is_default = TRUE
ON CONFLICT DO NOTHING;
```

### Option 3: Verify Current State

Check current scope configuration:

```sql
SELECT cs.client_id, cs.scope_name, s.description, s.mapped_role
FROM oauth2_client_scopes cs
JOIN oauth2_scopes s ON cs.scope_name = s.name
WHERE cs.client_id = 'vue-client'
ORDER BY s.name;
```

Expected output:
```
client_id  | scope_name | description                                  | mapped_role
-----------+------------+---------------------------------------------+-------------
vue-client | openid     | OpenID Connect身份认证 - 允许客户端验证用户身份 | user
vue-client | profile    | 访问用户基本信息 (用户名、邮箱等)       | user
```

## Code Changes Applied

The following fixes have been applied to the codebase:

### 1. PostgreSQL Storage (`PostgresOAuth2Storage.cc`)
- Modified `getClient()` to query `oauth2_client_scopes` table
- Uses ORM relationship method `getScope()` to fetch allowed scopes
- Populates `client.allowedScopes` from database

### 2. Memory Storage (`MemoryOAuth2Storage.cc`)  
- Updated `initFromConfig()` to read `allowed_scopes` from config
- Supports both array and string formats for allowed_scopes
- Adds default scopes for vue-client for backward compatibility

### 3. Configuration (`config.json`)
- Added `allowed_scopes: ["openid", "profile", "email"]` to vue-client
- Ensures public client has proper scope permissions

## Compliance

This fix ensures compliance with:
- **RFC 6749**: OAuth2.0 core specification (scope validation)
- **Design Doc v5.1**: Three-tier scope validation architecture
- **P0-3**: Scope permission control implementation

## Testing

After applying the fix:

1. **Restart the OAuth2 backend**
2. **Click "Sign in with Drogon"**
3. **Expected result**: Authorization flow should complete successfully
4. **Token should include**: `openid`, `profile`, `email` scopes

## Prevention

To prevent this issue in future:

1. **Always run `004_oauth2_scopes.sql`** during database setup
2. **Use the provided scripts** in `scripts/` directory
3. **Verify scope configuration** when adding new clients
4. **Test scope validation** before deploying to production

## Additional Resources

- Design Document: `docs/superpowers/specs/2026-05-06-oauth2-security-compliance-design-v5.1.md`
- SQL Schema: `sql/004_oauth2_scopes.sql`
- Setup Script: `scripts/setup_database.bat`
