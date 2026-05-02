# Client Secret Validation Design

**Date:** 2026-05-02
**Author:** OAuth2 Plugin Development Team
**Status:** Design Approved

## Problem Statement

Current OAuth2 token endpoint has a critical security vulnerability: `client_secret` parameter is read but not validated during token exchange. This allows unauthorized access to confidential client resources, violating OAuth2 RFC 6749 security requirements.

**Security Impact:** Confidential clients can be impersonated if authorization code is intercepted, as the system doesn't verify the client's secret.

## Solution Overview

Implement proper client authentication according to OAuth2 RFC 6749 by adding `client_secret` validation for confidential clients while maintaining backward compatibility with public clients.

## Architecture

### Core Components

**1. Type System Enhancement**
- Create `common/types/OAuth2Types.h` with centralized type definitions
- Add `ClientType` enum (PUBLIC, CONFIDENTIAL)
- Add `GrantType` enum for type safety

**2. Data Model Extension**
- Add `clientType` field to `OAuth2Client` structure
- Update SQL schema to include client type
- Migration strategy: drop and regenerate database

**3. Validation Flow**
```
Token Request → Controller → Plugin → Storage
                           ↓
                    validateClient() → Check clientType
                           ↓
                    Confidential → Verify secret
                    Public → Skip secret validation
```

## Implementation Details

### Type System

**File:** `common/types/OAuth2Types.h`

```cpp
namespace oauth2 {
enum class ClientType {
    PUBLIC,           // SPA, mobile apps
    CONFIDENTIAL      // Backend services
};

enum class GrantType {
    AUTHORIZATION_CODE,
    REFRESH_TOKEN,
    CLIENT_CREDENTIALS,
    IMPLICIT
};

// Helper functions
std::string clientTypeToString(ClientType type);
ClientType stringToClientType(const std::string& str);
}
```

### Data Model Changes

**Updated OAuth2Client Structure:**
```cpp
struct OAuth2Client {
    std::string clientId;
    ClientType clientType;  // New field
    std::string clientSecretHash;
    std::string salt;
    std::vector<std::string> redirectUris;
    std::vector<std::string> allowedScopes;
};
```

### Storage Layer Validation

**Enhanced validateClient() Logic:**
- Check client type first
- For CONFIDENTIAL clients: verify secret hash
- For PUBLIC clients: accept without secret validation
- Log warnings for inconsistent requests

### Plugin Layer Changes

**Updated exchangeCodeForToken Signature:**
```cpp
void exchangeCodeForToken(
    const std::string &code,
    const std::string &clientId,
    const std::string &clientSecret,  // New parameter
    std::function<void(const Json::Value &)> &&callback
);
```

### Controller Layer Integration

**Token Endpoint Validation Flow:**
```cpp
// 1. Extract parameters
std::string clientId, clientSecret, grantType, code;

// 2. Validate client first
plugin->validateClient(clientId, clientSecret, [this, code, clientId, grantType, callback](bool isValid) {
    if (!isValid) {
        return error("invalid_client", "Client authentication failed");
    }
    
    // 3. Proceed with token exchange
    plugin->exchangeCodeForToken(code, clientId, clientSecret, callback);
});
```

## Error Handling

### OAuth2 Compliant Error Responses

**invalid_client (400 Bad Request):**
```json
{
  "error": "invalid_client",
  "error_description": "Client authentication failed"
}
```

### Edge Cases

**1. Missing client_secret for confidential client:**
- Reject with `invalid_client` error
- Log security event

**2. Public client provides secret:**
- Accept but log warning (compatibility)
- Ignore secret in validation

**3. Missing clientType in database:**
- Default to CONFIDENTIAL (safer)
- Log warning for data consistency

## Security Considerations

**Logging Security:**
- Never log secret values
- Log validation failures for monitoring
- Implement rate limiting for failed attempts

**Timing Attack Protection:**
- Use constant-time comparison for secret hashes
- Avoid early returns in secret validation

**Monitoring:**
- Alert on repeated validation failures
- Track unusual client authentication patterns

## Testing Strategy

### Unit Tests (DROGON_TEST)

**Type System Tests:**
- ClientType string conversion
- Enum validation

**Client Validation Tests:**
- Confidential client with valid secret → Success
- Confidential client with invalid secret → Failure
- Public client without secret → Success
- Public client with secret → Success (with warning)

### Integration Tests

**Token Endpoint Tests:**
- Full authorization code flow with confidential client
- Full authorization code flow with public client
- Error response validation
- Backward compatibility verification

## Implementation Steps

1. **Create type system** - `common/types/OAuth2Types.h`
2. **Update data model** - Modify `IOAuth2Storage.h` and SQL schema
3. **Enhance storage validation** - Update `validateClient()` implementations
4. **Modify plugin interface** - Update `exchangeCodeForToken()` signature
5. **Integrate controller validation** - Add client auth to token endpoint
6. **Write comprehensive tests** - DROGON_TEST coverage
7. **Verify backward compatibility** - Test existing clients

## Impact Analysis

**Modified Files:**
- `common/types/OAuth2Types.h` (new)
- `storage/IOAuth2Storage.h`
- `storage/PostgresOAuth2Storage.cc`
- `storage/MemoryOAuth2Storage.cc`
- `plugins/OAuth2Plugin.h/.cc`
- `controllers/OAuth2Controller.cc`
- `test/ClientSecretValidationTest.cc` (new)
- `sql/001_oauth2_core.sql`

**Backward Compatibility:**
- ✅ Public clients (vue-client) continue working
- ✅ Existing confidential clients need proper secret
- ⚠️  Database regeneration required

## Success Criteria

- ✅ Confidential clients must provide valid client_secret
- ✅ Public clients work without client_secret
- ✅ OAuth2 compliant error responses
- ✅ All tests pass (unit + integration)
- ✅ No regression in existing functionality
- ✅ Security audit confirms vulnerability fixed

## Future Enhancements

- Add explicit client registration endpoint
- Implement client credentials grant type
- Add PKCE support for public clients
- Implement client rotation/rekeying
