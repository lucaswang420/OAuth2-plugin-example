#include "OAuth2Controller.h"
#include "../services/AuthService.h"
#include <drogon/drogon.h>
#include "../plugins/OAuth2Metrics.h"
#include <drogon/utils/Utilities.h>
#include <algorithm>
#include "../common/error/ErrorHandler.h"

using namespace oauth2;
using namespace services;
using namespace common::error;

void OAuth2Controller::errorResponse(std::function<void(const HttpResponsePtr &)> &&callback,
                                    const std::string &message,
                                    int statusCode) {
    Json::Value error;
    error["error"] = message;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(statusCode));
    callback(resp);
}

void OAuth2Controller::authorize(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    using namespace common::validation;

    // Use ErrorHandler for unified error handling
    ErrorHandler::handle([&]() {
        auto params = req->getParameters();
        std::string responseType = params["response_type"];
        std::string clientId = params["client_id"];
        std::string redirectUri = params["redirect_uri"];
        std::string scope = params["scope"];
        std::string state = params["state"];

        // Validate client_id
        auto result1 = Validator::validateClientId(clientId);
        if (!result1.isValid) {
            throw Error{
                ErrorCode::FORMAT_ERROR,
                ErrorCategory::VALIDATION,
                result1.errorMessage,
                "field: client_id",
                ErrorHandler::generateRequestId()
            };
        }

        // Validate redirect_uri
        auto result2 = Validator::validateRedirectUri(redirectUri);
        if (!result2.isValid) {
            throw Error{
                ErrorCode::FORMAT_ERROR,
                ErrorCategory::VALIDATION,
                result2.errorMessage,
                "field: redirect_uri",
                ErrorHandler::generateRequestId()
            };
        }

        // Validate response_type
        auto result3 = Validator::validateResponseType(responseType);
        if (!result3.isValid) {
            throw Error{
                ErrorCode::FORMAT_ERROR,
                ErrorCategory::VALIDATION,
                result3.errorMessage,
                "field: response_type",
                ErrorHandler::generateRequestId()
            };
        }

        // Validate scope if provided
        if (!scope.empty()) {
            auto result4 = Validator::validateScope(scope);
            if (!result4.isValid) {
                throw Error{
                    ErrorCode::FORMAT_ERROR,
                    ErrorCategory::VALIDATION,
                    result4.errorMessage,
                    "field: scope",
                    ErrorHandler::generateRequestId()
                };
            }
        }

        auto plugin = drogon::app().getPlugin<OAuth2Plugin>();
        if (!plugin) {
            throw Error{
                ErrorCode::INTERNAL,
                ErrorCategory::INTERNAL,
                "OAuth2Plugin not loaded",
                "",
                ErrorHandler::generateRequestId()
            };
        }

        // Validate Client (Async)
        plugin->validateClient(
            clientId,
            "",
            [=, callback = std::move(callback)](bool validClient) mutable {
                if (!validClient)
                {
                    Metrics::incRequest("authorize", 400);
                    Metrics::incLoginFailure("invalid_client_id");

                    Error error{
                        ErrorCode::INVALID_CREDENTIALS,
                        ErrorCategory::AUTHENTICATION,
                        "Invalid client_id",
                        "client_id: " + clientId,
                        ErrorHandler::generateRequestId()
                    };

                    ErrorHandler::logError(error, "OAuth2Controller::authorize");

                    Json::Value errorJson = error.toJson();
                    auto resp = HttpResponse::newHttpJsonResponse(errorJson);
                    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
                    callback(resp);
                    return;
                }

                // Validate Redirect URI (Async)
                plugin->validateRedirectUri(
                    clientId,
                    redirectUri,
                    [=, callback = std::move(callback)](bool validUri) mutable {
                        if (!validUri)
                        {
                            Error error{
                                ErrorCode::INVALID_CREDENTIALS,
                                ErrorCategory::AUTHENTICATION,
                                "Invalid redirect_uri",
                                "redirect_uri: " + redirectUri,
                                ErrorHandler::generateRequestId()
                            };

                            ErrorHandler::logError(error, "OAuth2Controller::authorize");

                            Json::Value errorJson = error.toJson();
                            auto resp = HttpResponse::newHttpJsonResponse(errorJson);
                            resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
                            callback(resp);
                            return;
                        }

                        // Check Session
                        auto userId = req->session()->get<std::string>("userId");
                        if (!userId.empty())
                        {
                            // Generate Code (Async)
                            plugin->generateAuthorizationCode(
                                clientId,
                                userId,
                                scope,
                                [=,
                                 callback = std::move(callback)](std::string code) {
                                    std::string location =
                                        redirectUri + "?code=" + code;
                                    if (!state.empty())
                                        location += "&state=" + state;
                                    auto resp =
                                        HttpResponse::newRedirectionResponse(
                                            location);
                                    Metrics::incRequest("authorize", 302);
                                    callback(resp);
                                });
                            return;
                        }

                        // Render Login Page
                        HttpViewData data;
                        data.insert("client_id", clientId);
                        data.insert("redirect_uri", redirectUri);
                        data.insert("scope", scope);
                        data.insert("state", state);
                        data.insert("response_type", responseType);
                        auto resp =
                            HttpResponse::newHttpViewResponse("login.csp", data);
                        callback(resp);
                    });
            });
    }, [&](const Error& error) {
        // Error callback - unified error response
        ErrorHandler::logError(error, "OAuth2Controller::authorize");

        Json::Value errorJson = error.toJson();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorJson);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
        callback(resp);
    });
}

void OAuth2Controller::login(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Use ErrorHandler for unified error handling
    ErrorHandler::handle([&]() {
        // Prefer POST body (JSON or form data) over URL parameters for security
        std::string username, password;
        std::string clientId, redirectUri, scope, state;

        // Try JSON body first
        if (req->contentType() == CT_APPLICATION_JSON)
        {
            auto json = req->getJsonObject();
            if (json)
            {
                username = json->get("username", "").asString();
                password = json->get("password", "").asString();
                clientId = json->get("client_id", "").asString();
                redirectUri = json->get("redirect_uri", "").asString();
                scope = json->get("scope", "").asString();
                state = json->get("state", "").asString();
            }
        }
        // Fallback to form data (Drogon automatically parses form-urlencoded)
        else
        {
            auto params = req->getParameters();
            username = params["username"];
            password = params["password"];
            clientId = params["client_id"];
            redirectUri = params["redirect_uri"];
            scope = params["scope"];
            state = params["state"];
        }

        // Validate required fields and length limits
        if (username.empty() || password.empty())
        {
            Metrics::incLoginFailure("missing_credentials");
            throw Error{
                ErrorCode::MISSING_REQUIRED_FIELD,
                ErrorCategory::VALIDATION,
                "Username and password required",
                "fields: username, password",
                ErrorHandler::generateRequestId()
            };
        }

        // Add reasonable length limits to prevent DoS
        const size_t MAX_USERNAME_LENGTH = 100;
        const size_t MAX_PASSWORD_LENGTH = 200;
        if (username.length() > MAX_USERNAME_LENGTH)
        {
            Metrics::incLoginFailure("username_too_long");
            throw Error{
                ErrorCode::INVALID_INPUT,
                ErrorCategory::VALIDATION,
                "Username exceeds maximum length",
                "max_length: 100",
                ErrorHandler::generateRequestId()
            };
        }
        if (password.length() > MAX_PASSWORD_LENGTH)
        {
            Metrics::incLoginFailure("password_too_long");
            throw Error{
                ErrorCode::INVALID_INPUT,
                ErrorCategory::VALIDATION,
                "Password exceeds maximum length",
                "max_length: 200",
                ErrorHandler::generateRequestId()
            };
        }

        AuthService::validateUser(
            username,
            password,
            [=, callback = std::move(callback)](std::optional<int> userId) {
                if (userId)
                {
                    req->session()->insert("userId", std::to_string(*userId));
                    auto plugin = drogon::app().getPlugin<OAuth2Plugin>();
                    if (!plugin)
                    {
                        Error error{
                            ErrorCode::INTERNAL,
                            ErrorCategory::INTERNAL,
                            "OAuth2Plugin not loaded during login",
                            "",
                            ErrorHandler::generateRequestId()
                        };

                        ErrorHandler::logError(error, "OAuth2Controller::login");

                        Json::Value errorJson = error.toJson();
                        auto resp = HttpResponse::newHttpJsonResponse(errorJson);
                        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
                        callback(resp);
                        return;
                    }

                    plugin->generateAuthorizationCode(
                        clientId,
                        std::to_string(*userId),
                        scope,
                        [=, callback = std::move(callback)](std::string code) {
                            std::string location = redirectUri + "?code=" + code;
                            if (!state.empty())
                                location += "&state=" + state;
                            if (req->getParameter("json") == "true")
                            {
                                Json::Value ret;
                                ret["code"] = code;
                                ret["location"] = location;
                                auto resp = HttpResponse::newHttpJsonResponse(ret);
                                callback(resp);
                                return;
                            }
                            auto resp =
                                HttpResponse::newRedirectionResponse(location);
                            callback(resp);
                        });
                }
                else
                {
                    // Fail (Bad Password or User Not Found)
                    Metrics::incLoginFailure("bad_credentials");

                    Error error{
                        ErrorCode::INVALID_CREDENTIALS,
                        ErrorCategory::AUTHENTICATION,
                        "Login Failed: Invalid Credentials",
                        "username: " + username,
                        ErrorHandler::generateRequestId()
                    };

                    ErrorHandler::logError(error, "OAuth2Controller::login");

                    Json::Value errorJson = error.toJson();
                    auto resp = HttpResponse::newHttpJsonResponse(errorJson);
                    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
                    callback(resp);
                }
            });
    }, [&](const Error& error) {
        // Error callback - unified error response
        ErrorHandler::logError(error, "OAuth2Controller::login");

        Json::Value errorJson = error.toJson();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorJson);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
        callback(resp);
    });
}

void OAuth2Controller::registerUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Use ErrorHandler for unified error handling
    ErrorHandler::handle([&]() {
        auto params = req->getParameters();
        std::string username = params["username"];
        std::string password = params["password"];
        std::string email = params["email"];

        if (username.empty() || password.empty())
        {
            throw Error{
                ErrorCode::MISSING_REQUIRED_FIELD,
                ErrorCategory::VALIDATION,
                "Username and password required",
                "fields: username, password",
                ErrorHandler::generateRequestId()
            };
        }

        AuthService::registerUser(
            username, password, email, [callback](const std::string &error) {
                if (error.empty())
                {
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setBody("User Registered");
                    callback(resp);
                }
                else
                {
                    Error err{
                        ErrorCode::INTERNAL,
                        ErrorCategory::INTERNAL,
                        error,
                        "",
                        ErrorHandler::generateRequestId()
                    };

                    ErrorHandler::logError(err, "OAuth2Controller::registerUser");

                    Json::Value errorJson = err.toJson();
                    auto resp = HttpResponse::newHttpJsonResponse(errorJson);
                    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(err.toHttpStatusCode()));
                    callback(resp);
                }
            });
    }, [&](const Error& error) {
        // Error callback - unified error response
        ErrorHandler::logError(error, "OAuth2Controller::registerUser");

        Json::Value errorJson = error.toJson();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorJson);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
        callback(resp);
    });
}

void OAuth2Controller::token(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    using namespace common::validation;

    // Use ErrorHandler for unified error handling
    ErrorHandler::handle([&]() {
        auto plugin = drogon::app().getPlugin<OAuth2Plugin>();
        if (!plugin) {
            throw Error{
                ErrorCode::INTERNAL,
                ErrorCategory::INTERNAL,
                "OAuth2Plugin not loaded",
                "",
                ErrorHandler::generateRequestId()
            };
        }

        // OAuth2 spec requires POST body (form-urlencoded) for token endpoint
        // Do NOT use URL parameters for sensitive data like client_secret
        std::string grantType, code, redirectUri, clientId, clientSecret;
        std::string refreshToken;

        // Prefer POST body (form-urlencoded) - Drogon auto-parses to
        // getParameters()
        if (req->method() == Post)
        {
            auto params = req->getParameters();
            grantType = params["grant_type"];
            code = params["code"];
            redirectUri = params["redirect_uri"];
            clientId = params["client_id"];
            clientSecret = params["client_secret"];
            refreshToken = params["refresh_token"];
        }
        else
        {
            // Fallback to query parameters (not recommended, but for compatibility)
            grantType = req->getParameter("grant_type");
            code = req->getParameter("code");
            redirectUri = req->getParameter("redirect_uri");
            clientId = req->getParameter("client_id");
            clientSecret = req->getParameter("client_secret");
            refreshToken = req->getParameter("refresh_token");
        }

        // Validate grant_type
        if (grantType.empty()) {
            Error error{
                ErrorCode::MISSING_REQUIRED_FIELD,
                ErrorCategory::VALIDATION,
                "grant_type is required",
                "field: grant_type",
                ErrorHandler::generateRequestId()
            };
            throw error;
        }

        // Validate grant_type format
        auto result1 = Validator::validateGrantType(grantType);
        if (!result1.isValid) {
            throw Error{
                ErrorCode::FORMAT_ERROR,
                ErrorCategory::VALIDATION,
                result1.errorMessage,
                "field: grant_type",
                ErrorHandler::generateRequestId()
            };
        }

        // Validate client_id
        auto result2 = Validator::validateClientId(clientId);
        if (!result2.isValid) {
            throw Error{
                ErrorCode::FORMAT_ERROR,
                ErrorCategory::VALIDATION,
                result2.errorMessage,
                "field: client_id",
                ErrorHandler::generateRequestId()
            };
        }

        // Validate client_secret if provided
        if (!clientSecret.empty()) {
            auto result3 = Validator::validateClientSecret(clientSecret);
            if (!result3.isValid) {
                throw Error{
                    ErrorCode::FORMAT_ERROR,
                    ErrorCategory::VALIDATION,
                    result3.errorMessage,
                    "field: client_secret",
                    ErrorHandler::generateRequestId()
                };
            }
        }

        // Grant type specific validation
        if (grantType == "authorization_code") {
            auto result4 = Validator::validateToken(code);
            if (!result4.isValid) {
                throw Error{
                    ErrorCode::TOKEN_INVALID,
                    ErrorCategory::VALIDATION,
                    "Invalid authorization code",
                    "field: code",
                    ErrorHandler::generateRequestId()
                };
            }

            if (!redirectUri.empty()) {
                auto result5 = Validator::validateRedirectUri(redirectUri);
                if (!result5.isValid) {
                    throw Error{
                        ErrorCode::FORMAT_ERROR,
                        ErrorCategory::VALIDATION,
                        result5.errorMessage,
                        "field: redirect_uri",
                        ErrorHandler::generateRequestId()
                    };
                }
            }
        } else if (grantType == "refresh_token") {
            auto result6 = Validator::validateToken(refreshToken);
            if (!result6.isValid) {
                throw Error{
                    ErrorCode::TOKEN_INVALID,
                    ErrorCategory::VALIDATION,
                    "Invalid refresh token",
                    "field: refresh_token",
                    ErrorHandler::generateRequestId()
                };
            }
        }

        // Process grant types
        if (grantType == "authorization_code")
        {
            plugin->exchangeCodeForToken(
                code,
                clientId,
                [callback = std::move(callback)](const Json::Value &result) {
                    if (result.isMember("error"))
                    {
                        // Convert legacy error format to new Error structure
                        Error error{
                            ErrorCode::TOKEN_INVALID,
                            ErrorCategory::AUTHENTICATION,
                            result.get("error", "Unknown error").asString(),
                            result.get("error_description", "").asString(),
                            ErrorHandler::generateRequestId()
                        };

                        ErrorHandler::logError(error, "OAuth2Controller::token");

                        Json::Value errorJson = error.toJson();
                        auto resp = HttpResponse::newHttpJsonResponse(errorJson);
                        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
                        callback(resp);
                        return;
                    }

                    auto resp = HttpResponse::newHttpJsonResponse(result);
                    Metrics::incRequest("token", 200);
                    Metrics::updateActiveTokens(1);
                    callback(resp);
                });
        }
        else if (grantType == "refresh_token")
        {
            std::string refreshTokenStr = req->getParameter("refresh_token");
            plugin->refreshAccessToken(
                refreshTokenStr,
                clientId,
                [callback = std::move(callback)](const Json::Value &result) {
                    if (result.isMember("error"))
                    {
                        // Convert legacy error format to new Error structure
                        Error error{
                            ErrorCode::TOKEN_EXPIRED,
                            ErrorCategory::AUTHENTICATION,
                            result.get("error", "Unknown error").asString(),
                            result.get("error_description", "").asString(),
                            ErrorHandler::generateRequestId()
                        };

                        ErrorHandler::logError(error, "OAuth2Controller::token");

                        Json::Value errorJson = error.toJson();
                        auto resp = HttpResponse::newHttpJsonResponse(errorJson);
                        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
                        callback(resp);
                        return;
                    }

                    auto resp = HttpResponse::newHttpJsonResponse(result);
                    Metrics::incRequest("token", 200);
                    callback(resp);
                });
        }
        else
        {
            Error error{
                ErrorCode::INVALID_INPUT,
                ErrorCategory::VALIDATION,
                "unsupported_grant_type",
                "Supported types: authorization_code, refresh_token",
                ErrorHandler::generateRequestId()
            };
            throw error;
        }
    }, [&](const Error& error) {
        // Error callback - unified error response
        ErrorHandler::logError(error, "OAuth2Controller::token");

        Json::Value errorJson = error.toJson();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorJson);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
        Metrics::incRequest("token", error.toHttpStatusCode());
        callback(resp);
    });
}

void OAuth2Controller::userInfo(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    if (req->method() == Options)
    {
        auto resp = HttpResponse::newHttpResponse();
        callback(resp);
        return;
    }

    // Use ErrorHandler for unified error handling
    ErrorHandler::handle([&]() {
        // This endpoint is protected by OAuth2Middleware.
        // If we are here, we have a valid token.

        // Attributes set by OAuth2Middleware
        std::string userId;
        auto attrs = req->getAttributes();
        if (!attrs->find("userId")) {
            throw Error{
                ErrorCode::TOKEN_INVALID,
                ErrorCategory::AUTHENTICATION,
                "User ID not found in request attributes",
                "This endpoint requires valid authentication",
                ErrorHandler::generateRequestId()
            };
        }
        userId = attrs->get<std::string>("userId");

        // TODO: Replace with actual user data from database
        // This is placeholder data for demonstration
        // Ideally should query users table and return real email, name, etc.
        Json::Value json;
        json["sub"] = userId;
        json["name"] = userId;
        json["email"] = userId + "@local";

        auto resp = HttpResponse::newHttpJsonResponse(json);
        callback(resp);
    }, [&](const Error& error) {
        // Error callback - unified error response
        ErrorHandler::logError(error, "OAuth2Controller::userInfo");

        Json::Value errorJson = error.toJson();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorJson);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
        callback(resp);
    });
}

void OAuth2Controller::health(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Use ErrorHandler for unified error handling
    ErrorHandler::handle([&]() {
        // Health check endpoint for monitoring/orchestration systems
        // Returns 200 OK if service is healthy
        Json::Value json;
        json["status"] = "ok";
        json["service"] = "OAuth2Server";
        json["timestamp"] = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());

        // Check database connectivity (optional - can be expensive)
        try
        {
            auto plugin = drogon::app().getPlugin<OAuth2Plugin>();
            if (plugin)
            {
                json["storage_type"] = plugin->getStorageType();
                json["database"] = "connected";
            }
            else
            {
                json["database"] = "unknown";
            }
        }
        catch (...)
        {
            json["database"] = "disconnected";
        }

        auto resp = HttpResponse::newHttpJsonResponse(json);
        resp->setStatusCode(k200OK);
        callback(resp);
    }, [&](const Error& error) {
        // Error callback - unified error response
        ErrorHandler::logError(error, "OAuth2Controller::health");

        Json::Value errorJson = error.toJson();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorJson);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
        callback(resp);
    });
}
