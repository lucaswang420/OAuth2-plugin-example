#include <drogon/drogon_test.h>
#include "common/config/ConfigManager.h"
#include <cstdlib>

DROGON_TEST(ConfigMigrationTest_MainCcConfigLoadWorks)
{
    // Test that main.cc can use ConfigManager
    Json::Value config;
    CHECK(common::config::ConfigManager::load("config.json", config));

    std::string errMsg;
    CHECK(common::config::ConfigManager::validate(config, errMsg));

    // Verify key config sections exist
    CHECK(config.isMember("db_clients"));
    CHECK(config.isMember("redis_clients"));
}

DROGON_TEST(ConfigMigrationTest_EnvOverridesWorkAsBefore)
{
    // Test that environment variable overrides work consistently
#ifdef _WIN32
    _putenv_s("OAUTH2_DB_HOST", "test-host");
    _putenv_s("OAUTH2_DB_PORT", "5433");
#else
    setenv("OAUTH2_DB_HOST", "test-host", 1);
    setenv("OAUTH2_DB_PORT", "5433", 1);
#endif

    Json::Value config;
    common::config::ConfigManager::load("config.json", config);

    std::string host = common::config::ConfigManager::get<std::string>(config, "db_clients.0.host");
    int port = common::config::ConfigManager::get<int>(config, "db_clients.0.port");

    CHECK(host == "test-host");
    CHECK(port == 5433);

#ifdef _WIN32
    _putenv_s("OAUTH2_DB_HOST", "");
    _putenv_s("OAUTH2_DB_PORT", "");
#else
    unsetenv("OAUTH2_DB_HOST");
    unsetenv("OAUTH2_DB_PORT");
#endif
}
