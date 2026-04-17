// #define DROGON_TEST_MAIN
#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <json/json.h>
#include <sstream>

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
                // Handle relative paths in tests (relative to build dir
                // usually, or CWD)
                try
                {
                    std::filesystem::path path(logPath);
                    // Verify if we need to resolve it relative to config file
                    // location or CWD For simplicity, we assume CWD or relative
                    // to it, just like drogon does for the most part
                    if (!std::filesystem::exists(path))
                    {
                        std::filesystem::create_directories(path);
                        std::cout << "Created log directory: " << path
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
        return configPath;
    }

    if (!parseJsonString(configFile, root))
    {
        std::cerr << "Error: Failed to parse config file: " << configPath
                  << std::endl;
        return configPath;
    }

    // Override DB Settings
    if (const char *env = std::getenv("OAUTH2_DB_HOST"))
    {
        if (env[0] != '\0')
            root["db_clients"][0]["host"] = env;
    }
    if (const char *env = std::getenv("OAUTH2_DB_NAME"))
    {
        if (env[0] != '\0')
            root["db_clients"][0]["dbname"] = env;
    }
    if (const char *env = std::getenv("OAUTH2_DB_PASSWORD"))
    {
        if (env[0] != '\0')
            root["db_clients"][0]["passwd"] = env;
    }

    // Override Redis Settings
    if (const char *env = std::getenv("OAUTH2_REDIS_HOST"))
    {
        if (env[0] != '\0')
            root["redis_clients"][0]["host"] = env;
    }
    if (const char *env = std::getenv("OAUTH2_REDIS_PASSWORD"))
    {
        if (env[0] != '\0')
            root["redis_clients"][0]["passwd"] = env;
    }

    // Override Client Secret in OAuth2Plugin
    if (const char *env = std::getenv("OAUTH2_VUE_CLIENT_SECRET"))
    {
        if (env[0] != '\0' && root["plugins"].isArray())
        {
            for (auto &plugin : root["plugins"])
            {
                if (plugin.get("name", "").asString() == "OAuth2Plugin")
                {
                    if (plugin.isMember("config") &&
                        plugin["config"].isMember("clients") &&
                        plugin["config"]["clients"].isMember("vue-client"))
                    {
                        plugin["config"]["clients"]["vue-client"]["secret"] =
                            env;
                    }
                    break;
                }
            }
        }
    }

    // Disable daemonizing and relaunch features in tests, because fork() breaks
    // the std::promise cross-thread sync
    if (root.isMember("app") && root["app"].isObject())
    {
        root["app"]["relaunch_on_error"] = false;
        root["app"]["run_as_daemon"] = false;
    }

    // Write runtime config (use specific name for test to avoid conflict?)
    std::string runtimePath = "test_config_env_runtime.json";
    std::ofstream runtimeFile(runtimePath);
    runtimeFile << jsonToStyledString(root);
    runtimeFile.close();

    std::cout << "Loaded config with ENV overrides from: " << configPath
              << " -> " << runtimePath << std::endl;
    return runtimePath;
}

int main(int argc, char **argv)
{
    using namespace drogon;

    std::promise<void> p1;
    std::future<void> f1 = p1.get_future();

    // Start the main loop on another thread
    std::thread thr([&]() {
        try
        {
            // Load Config for Integration Tests BEFORE app().run()
            std::string configPath = "./config.json";
            if (!std::filesystem::exists(configPath))
                configPath = "../config.json";
            if (!std::filesystem::exists(configPath))
                configPath = "../../config.json";
            if (!std::filesystem::exists(configPath))
                configPath = "../../../config.json";

            if (std::filesystem::exists(configPath))
            {
                std::cout << "Loading config from: " << configPath << std::endl;
                createLogDirFromConfig(configPath);
                auto runtimePath = loadConfigWithEnv(configPath);
                drogon::app().loadConfigFile(runtimePath);
            }
            else
            {
                std::cerr << "WARNING: config.json not found. Integration "
                             "tests might fail."
                          << std::endl;
            }

            drogon::app().registerBeginningAdvice([&p1]() {
                std::cout << "Drogon app ready, signaling tests to start..."
                          << std::endl;
                p1.set_value();
            });

            drogon::app().run();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Exception in app().run(): " << e.what() << std::endl;
            try
            {
                p1.set_value();
            }
            catch (...)
            {
            }
        }
        catch (...)
        {
            std::cerr << "Unknown exception in app().run()" << std::endl;
            try
            {
                p1.set_value();
            }
            catch (...)
            {
            }
        }
    });

    // The future is only satisfied after the event loop started
    if (f1.wait_for(std::chrono::seconds(30)) != std::future_status::ready)
    {
        std::cerr << "TIMEOUT: drogon app failed to start within 30s!"
                  << std::endl;
        // Use std::_Exit() to force-terminate without running atexit handlers,
        // which would deadlock trying to shut down Drogon's event loop.
        std::_Exit(1);
    }
    f1.get();
    int status = test::run(argc, argv);

    // Ask the event loop to shutdown and wait
    drogon::app().getLoop()->queueInLoop([]() { drogon::app().quit(); });
    thr.join();
    return status;
}
