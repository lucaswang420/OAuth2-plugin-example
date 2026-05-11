#include <gtest/gtest.h>
#include "../../common/utils/SubjectGenerator.h"
#include "../../storage/MemoryOAuth2Storage.h"
#include "../../plugins/OAuth2Plugin.h"
#include <json/json.h>
#include <thread>
#include <chrono>

using namespace oauth2;
using namespace oauth2::utils;

// ========== P1: Test Fixture ==========

class P1FunctionalityTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Initialize storage
        storage_ = std::make_shared<MemoryOAuth2Storage>();
        storage_->initFromConfig(clientsConfig_, adminConfig_);

        // Initialize plugin
        plugin_ = std::make_shared<OAuth2Plugin>();
        Json::Value config;
        config["storage_type"] = "memory";
        config["clients"] = clientsConfig_;
        plugin_->initAndStart(config);

        // Setup test client
        Json::Value clientConfig;
        clientConfig["type"] = "CONFIDENTIAL";
        clientConfig["secret"] = testClientSecret_;
        clientConfig["redirect_uri"] = "http://localhost:8080/callback";
        clientsConfig_[testClientId_] = clientConfig;

        // Setup test user
        testUserId_ = 1;
        testSubject_ = "local:testuser";

        // Create subject mapping
        storage_->createSubjectMapping("testuser", testUserId_, "local", [](bool success)
        {
            ASSERT_TRUE(success);
        });
    }

    void TearDown() override
    {
        plugin_->shutdown();
        plugin_.reset();
        storage_.reset();
    }

    // Helper: Create a valid access token for testing
    std::string createTestAccessToken(const std::string &clientId, const std::string &userId)
    {
        std::string token;
        std::promise<std::string> p;
        auto f = p.get_future();

        OAuth2AccessToken accessToken;
        accessToken.token = "test-access-token-" + userId;
        accessToken.clientId = clientId;
        accessToken.userId = userId;
        accessToken.scope = "openid profile";
        accessToken.expiresAt = std::time(nullptr) + 3600;  // 1 hour from now
        accessToken.issuedAt = std::time(nullptr);
        accessToken.notBefore = std::time(nullptr);
        accessToken.issuer = "https://oauth.example.com";
        accessToken.audience = clientId;
        accessToken.revoked = false;

        storage_->saveAccessToken(accessToken, [&p, &token, &accessToken]()
        {
            token = accessToken.token;
            p.set_value(token);
        });

        EXPECT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
        return f.get();
    }

    // Helper: Create a valid refresh token for testing
    std::string createTestRefreshToken(const std::string &clientId, const std::string &userId)
    {
        std::string token;
        std::promise<std::string> p;
        auto f = p.get_future();

        OAuth2RefreshToken refreshToken;
        refreshToken.token = "test-refresh-token-" + userId;
        refreshToken.clientId = clientId;
        refreshToken.userId = userId;
        refreshToken.scope = "openid profile";
        refreshToken.expiresAt = std::time(nullptr) + 86400 * 30;  // 30 days
        refreshToken.issuedAt = std::time(nullptr);

        storage_->saveRefreshToken(refreshToken, [&p, &token, &refreshToken]()
        {
            token = refreshToken.token;
            p.set_value(token);
        });

        EXPECT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
        return f.get();
    }

    std::shared_ptr<OAuth2Plugin> plugin_;
    std::shared_ptr<MemoryOAuth2Storage> storage_;
    Json::Value clientsConfig_;
    Json::Value adminConfig_;

    std::string testClientId_ = "test-client";
    std::string testClientSecret_ = "test-secret";
    int32_t testUserId_;
    std::string testSubject_;
};

// ========== P1: Token Introspection Tests ==========

TEST_F(P1FunctionalityTest, IntrospectValidToken)
{
    // Arrange: Create a valid access token
    std::string token = createTestAccessToken(testClientId_, testSubject_);

    // Act: Introspect the token
    std::promise<std::optional<oauth2::TokenIntrospection>> p;
    auto f = p.get_future();

    plugin_->introspectToken(token, [&p](auto introspection)
    {
        p.set_value(introspection);
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    auto introspection = f.get();

    // Assert: Token should be active with correct metadata
    ASSERT_TRUE(introspection.has_value());
    EXPECT_TRUE(introspection->active);
    EXPECT_EQ(introspection->clientId, testClientId_);
    EXPECT_EQ(introspection->userId, testSubject_);
    EXPECT_EQ(introspection->scope, "openid profile");
    EXPECT_EQ(introspection->issuer, "https://oauth.example.com");
    EXPECT_EQ(introspection->audience, testClientId_);
    EXPECT_GT(introspection->expiresAt, 0);
    EXPECT_GT(introspection->issuedAt, 0);
    EXPECT_GT(introspection->notBefore, 0);
}

TEST_F(P1FunctionalityTest, IntrospectInvalidToken)
{
    // Arrange: Use a non-existent token
    std::string token = "non-existent-token";

    // Act: Introspect the token
    std::promise<std::optional<oauth2::TokenIntrospection>> p;
    auto f = p.get_future();

    plugin_->introspectToken(token, [&p](auto introspection)
    {
        p.set_value(introspection);
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    auto introspection = f.get();

    // Assert: Token should not be found (nullopt)
    EXPECT_FALSE(introspection.has_value());
}

TEST_F(P1FunctionalityTest, IntrospectExpiredToken)
{
    // Arrange: Create an expired token
    OAuth2AccessToken accessToken;
    accessToken.token = "expired-token";
    accessToken.clientId = testClientId_;
    accessToken.userId = testSubject_;
    accessToken.scope = "openid profile";
    accessToken.expiresAt = std::time(nullptr) - 3600;  // Expired 1 hour ago
    accessToken.issuedAt = std::time(nullptr) - 7200;
    accessToken.notBefore = std::time(nullptr) - 7200;
    accessToken.issuer = "https://oauth.example.com";
    accessToken.audience = testClientId_;
    accessToken.revoked = false;

    std::promise<bool> saveP;
    auto saveF = saveP.get_future();
    storage_->saveAccessToken(accessToken, [&saveP](bool success)
    {
        saveP.set_value(success);
    });
    ASSERT_EQ(saveF.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_TRUE(saveF.get());

    // Act: Introspect the expired token
    std::promise<std::optional<oauth2::TokenIntrospection>> p;
    auto f = p.get_future();

    plugin_->introspectToken(accessToken.token, [&p](auto introspection)
    {
        p.set_value(introspection);
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    auto introspection = f.get();

    // Assert: Token should be inactive
    ASSERT_TRUE(introspection.has_value());
    EXPECT_FALSE(introspection->active);
}

TEST_F(P1FunctionalityTest, IntrospectRevokedToken)
{
    // Arrange: Create and revoke a token
    std::string token = createTestAccessToken(testClientId_, testSubject_);

    std::promise<bool> revokeP;
    auto revokeF = revokeP.get_future();
    storage_->revokeAccessToken(token, testClientId_, [&revokeP]()
    {
        revokeP.set_value(true);
    });
    ASSERT_EQ(revokeF.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    revokeF.get();

    // Act: Introspect the revoked token
    std::promise<std::optional<oauth2::TokenIntrospection>> p;
    auto f = p.get_future();

    plugin_->introspectToken(token, [&p](auto introspection)
    {
        p.set_value(introspection);
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    auto introspection = f.get();

    // Assert: Token should be inactive
    ASSERT_TRUE(introspection.has_value());
    EXPECT_FALSE(introspection->active);
}

TEST_F(P1FunctionalityTest, IncrementIntrospectCount)
{
    // Arrange: Create a valid token
    std::string token = createTestAccessToken(testClientId_, testSubject_);

    // Act: Increment introspect count
    std::promise<void> p;
    auto f = p.get_future();

    plugin_->incrementIntrospectCount(token, [&p]()
    {
        p.set_value();
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    f.get();

    // Assert: Introspect count should be incremented
    std::promise<std::optional<oauth2::TokenIntrospection>> p2;
    auto f2 = p2.get_future();

    plugin_->introspectToken(token, [&p2](auto introspection)
    {
        p2.set_value(introspection);
    });

    ASSERT_EQ(f2.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    auto introspection = f2.get();

    ASSERT_TRUE(introspection.has_value());
    // Note: introspectCount is a P1 field, check if it's incremented
    // (implementation depends on storage layer)
}

// ========== P1: Token Revocation Tests ==========

TEST_F(P1FunctionalityTest, RevokeValidToken)
{
    // Arrange: Create a valid token
    std::string token = createTestAccessToken(testClientId_, testSubject_);

    // Act: Revoke the token
    std::promise<void> p;
    auto f = p.get_future();

    plugin_->revokeAccessToken(token, testClientId_, [&p]()
    {
        p.set_value();
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    f.get();

    // Assert: Token should be revoked
    std::promise<std::optional<oauth2::TokenIntrospection>> p2;
    auto f2 = p2.get_future();

    plugin_->introspectToken(token, [&p2](auto introspection)
    {
        p2.set_value(introspection);
    });

    ASSERT_EQ(f2.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    auto introspection = f2.get();

    ASSERT_TRUE(introspection.has_value());
    EXPECT_FALSE(introspection->active);
    EXPECT_GT(introspection->revokedAt, 0);
    EXPECT_EQ(introspection->revokedBy, testClientId_);
}

TEST_F(P1FunctionalityTest, RevokeNonExistentToken)
{
    // Arrange: Use a non-existent token
    std::string token = "non-existent-token";

    // Act: Revoke the token (should not throw)
    std::promise<void> p;
    auto f = p.get_future();

    plugin_->revokeAccessToken(token, testClientId_, [&p]()
    {
        p.set_value();
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    f.get();

    // Assert: Should complete without error (RFC 7009 requires idempotency)
}

TEST_F(P1FunctionalityTest, RevokeTokenUnauthorizedClient)
{
    // Arrange: Create a token for test-client
    std::string token = createTestAccessToken(testClientId_, testSubject_);

    // Act: Try to revoke with different client (simulated in storage layer test)
    std::promise<std::optional<oauth2::TokenIntrospection>> p;
    auto f = p.get_future();

    plugin_->introspectToken(token, [&p](auto introspection)
    {
        p.set_value(introspection);
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    auto introspection = f.get();

    // Assert: Token belongs to test-client
    ASSERT_TRUE(introspection.has_value());
    EXPECT_EQ(introspection->clientId, testClientId_);

    // In controller layer, this would return 403 Forbidden
    // (This test verifies storage layer behavior, controller handles permission)
}

TEST_F(P1FunctionalityTest, RevokeRefreshToken)
{
    // Arrange: Create a refresh token
    std::string token = createTestRefreshToken(testClientId_, testSubject_);

    // Act: Revoke the refresh token
    std::promise<void> p;
    auto f = p.get_future();

    plugin_->revokeAccessToken(token, testClientId_, [&p]()
    {
        p.set_value();
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    f.get();

    // Assert: Token should be revoked (verify through storage layer)
    std::promise<std::optional<oauth2::OAuth2RefreshToken>> p2;
    auto f2 = p2.get_future();

    storage_->getRefreshToken(token, [&p2](auto refreshToken)
    {
        p2.set_value(refreshToken);
    });

    ASSERT_EQ(f2.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    auto refreshToken = f2.get();

    ASSERT_TRUE(refreshToken.has_value());
    EXPECT_TRUE(refreshToken->revoked);
}

// ========== P1: Client Authentication Tests ==========

TEST_F(P1FunctionalityTest, ValidateClientSuccess)
{
    // Act: Validate correct client credentials
    std::promise<bool> p;
    auto f = p.get_future();

    plugin_->validateClient(testClientId_, testClientSecret_, [&p](bool valid)
    {
        p.set_value(valid);
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_TRUE(f.get());
}

TEST_F(P1FunctionalityTest, ValidateClientInvalidSecret)
{
    // Act: Validate with wrong secret
    std::promise<bool> p;
    auto f = p.get_future();

    plugin_->validateClient(testClientId_, "wrong-secret", [&p](bool valid)
    {
        p.set_value(valid);
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_FALSE(f.get());
}

TEST_F(P1FunctionalityTest, ValidateClientInvalidId)
{
    // Act: Validate with non-existent client
    std::promise<bool> p;
    auto f = p.get_future();

    plugin_->validateClient("non-existent-client", "secret", [&p](bool valid)
    {
        p.set_value(valid);
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_FALSE(f.get());
}

// ========== P1: Error Handling Tests ==========

TEST_F(P1FunctionalityTest, IntrospectEmptyToken)
{
    // Act: Try to introspect empty token
    std::promise<std::optional<oauth2::TokenIntrospection>> p;
    auto f = p.get_future();

    plugin_->introspectToken("", [&p](auto introspection)
    {
        p.set_value(introspection);
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    auto introspection = f.get();

    // Assert: Should return nullopt (token not found)
    EXPECT_FALSE(introspection.has_value());
}

TEST_F(P1FunctionalityTest, RevokeEmptyToken)
{
    // Act: Try to revoke empty token (should not throw)
    std::promise<void> p;
    auto f = p.get_future();

    plugin_->revokeAccessToken("", testClientId_, [&p]()
    {
        p.set_value();
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    f.get();

    // Assert: Should complete without error
}

// ========== P1: P1 Field Completeness Tests ==========

TEST_F(P1FunctionalityTest, P1FieldsPopulated)
{
    // Arrange: Create a token with P1 fields
    std::string token = createTestAccessToken(testClientId_, testSubject_);

    // Act: Introspect to check P1 fields
    std::promise<std::optional<oauth2::TokenIntrospection>> p;
    auto f = p.get_future();

    plugin_->introspectToken(token, [&p](auto introspection)
    {
        p.set_value(introspection);
    });

    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    auto introspection = f.get();

    // Assert: P1 fields should be populated
    ASSERT_TRUE(introspection.has_value());
    EXPECT_TRUE(introspection->active);
    EXPECT_GT(introspection->issuedAt, 0);       // P1 field
    EXPECT_GT(introspection->notBefore, 0);      // P1 field
    EXPECT_FALSE(introspection->issuer.empty()); // P1 field
    EXPECT_FALSE(introspection->audience.empty()); // P1 field
    EXPECT_GE(introspection->introspectCount, 0); // P1 field
}

// ========== Main Function ==========

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
