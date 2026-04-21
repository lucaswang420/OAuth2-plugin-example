#include "OAuth2Controller.h"
#include "../services/AuthService.h"
#include <drogon/drogon.h>
#include "../plugins/OAuth2Metrics.h"
#include <drogon/utils/Utilities.h>
#include <algorithm>

using namespace oauth2;
using namespace services;

void OAuth2Controller::authorize(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto params = req->getParameters();
    std::string responseType = params["response_type"];
    std::string clientId = params["client_id"];
    std::string redirectUri = params["redirect_uri"];
    std::string scope = params["scope"];
    std::string state = params["state"];
    auto plugin = drogon::app().getPlugin<OAuth2Plugin>();
    if (!plugin)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("OAuth2Plugin not loaded");
        callback(resp);
        return;
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

                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Invalid client_id");
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
                        auto resp = HttpResponse::newHttpResponse();
                        resp->setStatusCode(k400BadRequest);
                        resp->setBody("Invalid redirect_uri");
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
}

void OAuth2Controller::login(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
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
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Username and password required");
        callback(resp);
        return;
    }

    // Add reasonable length limits to prevent DoS
    const size_t MAX_USERNAME_LENGTH = 100;
    const size_t MAX_PASSWORD_LENGTH = 200;
    if (username.length() > MAX_USERNAME_LENGTH)
    {
        Metrics::incLoginFailure("username_too_long");
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Username exceeds maximum length");
        callback(resp);
        return;
    }
    if (password.length() > MAX_PASSWORD_LENGTH)
    {
        Metrics::incLoginFailure("password_too_long");
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Password exceeds maximum length");
        callback(resp);
        return;
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
                    LOG_ERROR << "OAuth2Plugin not loaded during login";
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k500InternalServerError);
                    resp->setBody("Internal Server Error: Plugin not loaded");
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
                auto resp = HttpResponse::newHttpResponse();
                resp->setBody("Login Failed: Invalid Credentials");
                callback(resp);
            }
        });
}

void OAuth2Controller::registerUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto params = req->getParameters();
    std::string username = params["username"];
    std::string password = params["password"];
    std::string email = params["email"];

    if (username.empty() || password.empty())
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Username and password required");
        callback(resp);
        return;
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
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(error);
                callback(resp);
            }
        });
}

void OAuth2Controller::token(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto plugin = drogon::app().getPlugin<OAuth2Plugin>();
    // OAuth2 spec requires POST body (form-urlencoded) for token endpoint
    // Do NOT use URL parameters for sensitive data like client_secret
    std::string grantType, code, redirectUri, clientId, clientSecret;

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
    }
    else
    {
        // Fallback to query parameters (not recommended, but for compatibility)
        grantType = req->getParameter("grant_type");
        code = req->getParameter("code");
        redirectUri = req->getParameter("redirect_uri");
        clientId = req->getParameter("client_id");
        clientSecret = req->getParameter("client_secret");
    }

    // Validate grant_type
    if (grantType.empty())
    {
        Metrics::incRequest("token", 400);
        Json::Value json;
        json["error"] = "invalid_request";
        json["error_description"] = "grant_type is required";
        auto resp = HttpResponse::newHttpJsonResponse(json);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    if (grantType == "authorization_code")
    {
        plugin->exchangeCodeForToken(
            code,
            clientId,
            [callback = std::move(callback)](const Json::Value &result) {
                if (result.isMember("error"))
                {
                    auto resp = HttpResponse::newHttpJsonResponse(result);
                    resp->setStatusCode(k400BadRequest);
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
        std::string refreshToken = req->getParameter("refresh_token");
        plugin->refreshAccessToken(
            refreshToken,
            clientId,
            [callback = std::move(callback)](const Json::Value &result) {
                if (result.isMember("error"))
                {
                    auto resp = HttpResponse::newHttpJsonResponse(result);
                    resp->setStatusCode(k400BadRequest);
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
        Metrics::incRequest("token", 400);
        Json::Value json;
        json["error"] = "unsupported_grant_type";
        auto resp = HttpResponse::newHttpJsonResponse(json);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }
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

    // This endpoint is protected by OAuth2Middleware.
    // If we are here, we have a valid token.

    // Attributes set by OAuth2Middleware
    std::string userId;
    auto attrs = req->getAttributes();
    if (attrs->find("userId"))
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
}

void OAuth2Controller::health(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
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
}
