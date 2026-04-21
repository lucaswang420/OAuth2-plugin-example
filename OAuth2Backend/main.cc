#include <drogon/drogon.h>
#include <drogon/plugins/Hodor.h>
#include <drogon/utils/Utilities.h>
#include <vector>
#include <string>
#include <algorithm>
#include <json/json.h>
#include <sstream>

using namespace drogon;

// Helper to parse JSON (replaces deprecated Json::Reader)
static bool parseJsonString(std::istream &stream, Json::Value &json)
{
    Json::CharReaderBuilder builder;
    std::string errs;
    return Json::parseFromStream(builder, stream, &json, &errs);
}

// Helper to serialize JSON to string (replaces deprecated Json::StyledWriter)
static std::string jsonToStyledString(const Json::Value &json)
{
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, json);
}

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>

// Helper to create log directory from config
void createLogDirFromConfig(const std::string &configPath)
{
    std::ifstream configFile(configPath);
    if (!configFile.is_open())
        return;

    Json::Value root;
    if (parseJsonString(configFile, root))
    {
        const auto &logConfig = root["app"]["log"];
        if (!logConfig.isNull())
        {
            std::string logPath = logConfig.get("log_path", "").asString();
            if (!logPath.empty())
            {
                try
                {
                    if (!std::filesystem::exists(logPath))
                    {
                        std::filesystem::create_directories(logPath);
                        std::cout << "Created log directory: " << logPath
                                  << std::endl;
                    }
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Failed to create log directory: " << e.what()
                              << std::endl;
                }
            }
        }
    }
}

void setupCors()
{
    // Define the whitelist check logic - STRICT MODE: no wildcards
    auto isAllowed = [](const std::string &origin) -> bool {
        if (origin.empty())
            return false;

        const auto &customConfig = drogon::app().getCustomConfig();
        const auto &allowOrigins = customConfig["cors"]["allow_origins"];

        if (allowOrigins.isArray())
        {
            for (const auto &allowed : allowOrigins)
            {
                auto allowedStr = allowed.asString();
                // SECURITY: Only exact match allowed, no wildcards
                // This prevents CSRF attacks from arbitrary origins
                if (allowedStr == origin)
                    return true;
            }
        }
        return false;
    };

    // Register sync advice to handle CORS preflight (OPTIONS) requests
    drogon::app().registerSyncAdvice(
        [isAllowed](
            const drogon::HttpRequestPtr &req) -> drogon::HttpResponsePtr {
            if (req->method() == drogon::HttpMethod::Options)
            {
                const auto &origin = req->getHeader("Origin");
                if (isAllowed(origin))
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->addHeader("Access-Control-Allow-Origin", origin);

                    const auto &requestMethod =
                        req->getHeader("Access-Control-Request-Method");
                    if (!requestMethod.empty())
                    {
                        resp->addHeader("Access-Control-Allow-Methods",
                                        requestMethod);
                    }

                    resp->addHeader("Access-Control-Allow-Credentials", "true");

                    const auto &requestHeaders =
                        req->getHeader("Access-Control-Request-Headers");
                    if (!requestHeaders.empty())
                    {
                        resp->addHeader("Access-Control-Allow-Headers",
                                        requestHeaders);
                    }
                    return resp;
                }
                // SECURITY: Reject unauthorized preflight requests with 403
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k403Forbidden);
                return resp;
            }
            return {};
        });

    // Register post-handling advice to add CORS headers to all responses
    drogon::app().registerPostHandlingAdvice(
        [isAllowed](const drogon::HttpRequestPtr &req,
                    const drogon::HttpResponsePtr &resp) {
            const auto &origin = req->getHeader("Origin");
            if (isAllowed(origin))
            {
                resp->addHeader("Access-Control-Allow-Origin", origin);
                resp->addHeader("Access-Control-Allow-Methods",
                                "GET, POST, OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers",
                                "Content-Type, Authorization");
                resp->addHeader("Access-Control-Allow-Credentials", "true");
            }
        });
}

// Helper to load config with Environment Variable overrides and write to a temp
// file
std::string loadConfigWithEnv(const std::string &configPath)
{
    Json::Value root;
    std::ifstream configFile(configPath);
    if (!configFile.is_open())
    {
        std::cerr << "Error: Config file not found: " << configPath
                  << std::endl;
        return configPath;  // Fallback to original
    }

    if (!parseJsonString(configFile, root))
    {
        std::cerr << "Error: Failed to parse config file: " << configPath
                  << std::endl;
        return configPath;
    }

    // Override DB Settings
    if (const char *env = std::getenv("OAUTH2_DB_HOST"))
        root["db_clients"][0]["host"] = env;
    if (const char *env = std::getenv("OAUTH2_DB_NAME"))
        root["db_clients"][0]["dbname"] = env;
    if (const char *env = std::getenv("OAUTH2_DB_PASSWORD"))
        root["db_clients"][0]["passwd"] = env;
    if (const char *env = std::getenv("OAUTH2_DB_PORT"))
    {
        try
        {
            root["db_clients"][0]["port"] = std::stoi(env);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: Invalid OAUTH2_DB_PORT value: " << env
                      << ", error: " << e.what() << std::endl;
            // Keep default value from config file
        }
    }

    // Override Redis Settings
    if (const char *env = std::getenv("OAUTH2_REDIS_HOST"))
        root["redis_clients"][0]["host"] = env;
    if (const char *env = std::getenv("OAUTH2_REDIS_PASSWORD"))
        root["redis_clients"][0]["passwd"] = env;
    if (const char *env = std::getenv("OAUTH2_REDIS_PORT"))
    {
        try
        {
            root["redis_clients"][0]["port"] = std::stoi(env);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: Invalid OAUTH2_REDIS_PORT value: " << env
                      << ", error: " << e.what() << std::endl;
            // Keep default value from config file
        }
    }

    // Override Client Secret in OAuth2Plugin
    if (const char *env = std::getenv("OAUTH2_VUE_CLIENT_SECRET"))
    {
        if (root["plugins"].isArray())
        {
            for (auto &plugin : root["plugins"])
            {
                if (plugin.get("name", "").asString() == "OAuth2Plugin")
                {
                    // Ensure config structure exists
                    if (!plugin.isMember("config"))
                        plugin["config"] = Json::objectValue;
                    if (!plugin["config"].isMember("clients"))
                        plugin["config"]["clients"] = Json::objectValue;
                    if (!plugin["config"]["clients"].isMember("vue-client"))
                        plugin["config"]["clients"]["vue-client"] =
                            Json::objectValue;

                    plugin["config"]["clients"]["vue-client"]["secret"] = env;
                    std::cout << "Overridden vue-client secret from ENV"
                              << std::endl;
                    break;
                }
            }
        }
    }

    // Write runtime config
    std::string runtimePath = "config_env_runtime.json";
    std::ofstream runtimeFile(runtimePath);
    runtimeFile << jsonToStyledString(root);
    runtimeFile.close();

    std::cout << "Loaded config with ENV overrides from: " << configPath
              << " -> " << runtimePath << std::endl;
    return runtimePath;
}

int main()
{
    // Set HTTP listener address and port
    // drogon::app().addListener("0.0.0.0", 5555);

    // Search for config.json
    std::string configPath = "./config.json";
    if (!std::filesystem::exists(configPath))
        configPath = "../config.json";
    if (!std::filesystem::exists(configPath))
        configPath = "../../../config.json";

    // Ensure log directory exists
    createLogDirFromConfig(configPath);

    // Load config from file + ENV
    auto runtimeConfigPath = loadConfigWithEnv(configPath);
    drogon::app().loadConfigFile(runtimeConfigPath);

    // Setup CORS support
    setupCors();

    // Global Security Headers
    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr
                                                    &req,
                                                const drogon::HttpResponsePtr
                                                    &resp) {
        resp->addHeader("X-Content-Type-Options", "nosniff");
        resp->addHeader("X-Frame-Options", "SAMEORIGIN");
        resp->addHeader(
            "Content-Security-Policy",
            "default-src 'self'; "
            "script-src 'self' 'unsafe-inline' 'unsafe-eval'; "
            "style-src 'self' 'unsafe-inline' https://fonts.googleapis.com; "
            "font-src 'self' https://fonts.gstatic.com; "
            "img-src 'self' data: https:; "
            "frame-ancestors 'self';");

        // Only set HSTS header on HTTPS connections
        // Check X-Forwarded-Proto header for reverse proxy scenarios
        auto forwardedProto = req->getHeader("X-Forwarded-Proto");
        if (forwardedProto == "https")
        {
            resp->addHeader("Strict-Transport-Security",
                            "max-age=31536000; includeSubDomains");
        }
    });

    // Configure Hodor rate limiter with user identification callback
    // Use registerBeginningAdvice to ensure Hodor plugin is initialized first
    // TODO: Temporarily disabled to debug crash
    // drogon::app().registerBeginningAdvice([]() {
    //     try {
    //         auto hodor = drogon::app().getPlugin<drogon::plugin::Hodor>();
    //         std::cout << "Hodor plugin loaded successfully" << std::endl;
    //         std::cout << "Hodor rate limiter user ID getter configured
    //         successfully" << std::endl;
    //     } catch (const std::exception& e) {
    //         std::cerr << "Warning: Failed to configure Hodor plugin: " <<
    //         e.what() << std::endl;
    //     }
    // });

    drogon::app().run();
    return 0;
}
