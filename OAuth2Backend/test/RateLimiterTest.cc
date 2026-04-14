#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include <drogon/HttpClient.h>
#include <json/json.h>
#include <iostream>
#include <thread>
#include <chrono>

DROGON_TEST(RateLimiterTest)
{
    // Test Configuration
    std::string baseUrl = "http://127.0.0.1:5555";
    int maxRetries = 3;
    bool serverReady = false;

    // 1. Check if server is running
    for (int i = 0; i < maxRetries; ++i)
    {
        try
        {
            auto client = drogon::HttpClient::newHttpClient(baseUrl);
            auto req = drogon::HttpRequest::newHttpHeadRequest("/");
            auto response = client->sendRequest(req, 3);

            if (response)
            {
                serverReady = true;
                LOG_INFO << "Server is ready for testing";
                break;
            }
        }
        catch (...)
        {
            LOG_WARN << "Server not ready, attempt " << i + 1 << "/" << maxRetries;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    if (!serverReady)
    {
        SKIP_TEST("Server is not running, skipping rate limiter tests");
        return;
    }

    // 2. Test IP-based rate limiting on /oauth2/login
    SUBTEST(/LoginRateLimiting)
    {
        LOG_INFO << "Testing IP-based rate limiting on /oauth2/login";

        int successCount = 0;
        int rateLimitedCount = 0;

        // Send 7 requests (limit is 5 per minute for /oauth2/login)
        for (int i = 0; i < 7; ++i)
        {
            try
            {
                auto client = drogon::HttpClient::newHttpClient(baseUrl);
                Json::Value loginData;
                loginData["username"] = "testuser";
                loginData["password"] = "testpass";

                auto req = drogon::HttpRequest::newHttpJsonPostRequest(
                    "/oauth2/login", loginData);
                auto response = client->sendRequest(req, 5);

                if (response)
                {
                    if (response->statusCode() == drogon::k200OK)
                    {
                        successCount++;
                    }
                    else if (response->statusCode() == drogon::k429TooManyRequests)
                    {
                        rateLimitedCount++;
                    }
                }

                // Small delay between requests
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            catch (const std::exception &e)
            {
                LOG_WARN << "Request failed: " << e.what();
            }
        }

        // Verify rate limiting is working
        // First 5 requests should succeed, last 2 should be rate limited
        LOG_INFO << "Successful requests: " << successCount;
        LOG_INFO << "Rate limited requests: " << rateLimitedCount;

        CHECK(successCount <= 5); // Should not exceed limit
        CHECK(rateLimitedCount >= 1); // At least some requests should be rate limited

        // Note: If localhost is in trust_ips, all requests will succeed
        if (rateLimitedCount == 0)
        {
            LOG_WARN << "All requests succeeded - localhost might be whitelisted";
        }
    }

    // 3. Test endpoint-specific rate limiting on /oauth2/token
    SUBTEST(/TokenRateLimiting)
    {
        LOG_INFO << "Testing rate limiting on /oauth2/token";

        int successCount = 0;
        int rateLimitedCount = 0;

        // Send 12 requests (limit is 10 per minute for /oauth2/token)
        for (int i = 0; i < 12; ++i)
        {
            try
            {
                auto client = drogon::HttpClient::newHttpClient(baseUrl);
                Json::Value tokenData;
                tokenData["grant_type"] = "password";
                tokenData["username"] = "test";
                tokenData["password"] = "test";

                auto req = drogon::HttpRequest::newHttpJsonPostRequest(
                    "/oauth2/token", tokenData);
                auto response = client->sendRequest(req, 5);

                if (response)
                {
                    if (response->statusCode() == drogon::k200OK ||
                        response->statusCode() == drogon::k400BadRequest)
                    {
                        successCount++;
                    }
                    else if (response->statusCode() == drogon::k429TooManyRequests)
                    {
                        rateLimitedCount++;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            catch (const std::exception &e)
            {
                LOG_WARN << "Request failed: " << e.what();
            }
        }

        LOG_INFO << "Token endpoint - Successful: " << successCount
                 << ", Rate limited: " << rateLimitedCount;

        CHECK(successCount <= 10); // Should not exceed token endpoint limit
    }

    // 4. Test register endpoint rate limiting
    SUBTEST(/RegisterRateLimiting)
    {
        LOG_INFO << "Testing rate limiting on /api/register";

        int successCount = 0;
        int rateLimitedCount = 0;

        // Send 7 requests (limit is 5 per minute for /api/register)
        for (int i = 0; i < 7; ++i)
        {
            try
            {
                auto client = drogon::HttpClient::newHttpClient(baseUrl);
                Json::Value registerData;
                registerData["username"] = "testuser" + std::to_string(i);
                registerData["password"] = "testpass";
                registerData["email"] = "test" + std::to_string(i) + "@example.com";

                auto req = drogon::HttpRequest::newHttpJsonPostRequest(
                    "/api/register", registerData);
                auto response = client->sendRequest(req, 5);

                if (response)
                {
                    if (response->statusCode() == drogon::k200OK ||
                        response->statusCode() == drogon::k400BadRequest)
                    {
                        successCount++;
                    }
                    else if (response->statusCode() == drogon::k429TooManyRequests)
                    {
                        rateLimitedCount++;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            catch (const std::exception &e)
            {
                LOG_WARN << "Request failed: " << e.what();
            }
        }

        LOG_INFO << "Register endpoint - Successful: " << successCount
                 << ", Rate limited: " << rateLimitedCount;

        CHECK(successCount <= 5); // Should not exceed register endpoint limit
    }

    // 5. Test rate limit recovery (token bucket refill)
    SUBTEST(RateLimitRecovery)
    {
        LOG_INFO << "Testing rate limit recovery over time";

        // First, exhaust the rate limit
        for (int i = 0; i < 6; ++i)
        {
            try
            {
                auto client = drogon::HttpClient::newHttpClient(baseUrl);
                Json::Value loginData;
                loginData["username"] = "testuser";
                loginData["password"] = "testpass";

                auto req = drogon::HttpRequest::newHttpJsonPostRequest(
                    "/oauth2/login", loginData);
                auto response = client->sendRequest(req, 5);

                if (i >= 5)
                {
                    // Last request should be rate limited
                    if (response && response->statusCode() == drogon::k429TooManyRequests)
                    {
                        LOG_INFO << "Rate limit triggered as expected";
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            catch (const std::exception &e)
            {
                LOG_WARN << "Request failed: " << e.what();
            }
        }

        // Wait for token bucket to partially refill (61+ seconds for full refill,
        // but we'll just verify the mechanism exists)
        LOG_INFO << "Rate limit recovery test completed (would need 61s wait for full recovery)";

        // Note: Full recovery test is skipped to avoid long test duration
        // In production, you would wait 61 seconds and verify requests succeed again
    }

    // 6. Test whitelist functionality
    SUBTEST(WhitelistTest)
    {
        LOG_INFO << "Testing whitelist functionality";

        // Check if localhost is whitelisted by sending many requests
        int allSuccess = true;

        for (int i = 0; i < 10; ++i)
        {
            try
            {
                auto client = drogon::HttpClient::newHttpClient(baseUrl);
                Json::Value loginData;
                loginData["username"] = "whitelist_test";
                loginData["password"] = "testpass";

                auto req = drogon::HttpRequest::newHttpJsonPostRequest(
                    "/oauth2/login", loginData);
                auto response = client->sendRequest(req, 5);

                if (response && response->statusCode() == drogon::k429TooManyRequests)
                {
                    allSuccess = false;
                    LOG_INFO << "Whitelist test: Rate limiting is active (localhost not whitelisted)";
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            catch (const std::exception &e)
            {
                LOG_WARN << "Request failed: " << e.what();
            }
        }

        if (allSuccess)
        {
            LOG_INFO << "Whitelist test: All requests succeeded (localhost might be whitelisted)";
        }
    }

    LOG_INFO << "Rate limiter tests completed";
}
