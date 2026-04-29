#pragma once

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <json/json.h>

namespace common::documentation
{

struct EndpointInfo
{
    std::string path;
    std::string method;
    std::string summary;
    std::string description;
    std::vector<std::string> tags;
    std::map<std::string, std::string> parameters;
    std::map<int, std::string> responses;
    bool requiresAuth;
};

class OpenApiGenerator
{
  public:
    static void addEndpoint(const EndpointInfo &endpoint);
    static Json::Value generateOpenApiSpec();
    static bool writeToFile(const std::string &outputPath);
    static void setApiInfo(const std::string &title,
                           const std::string &version,
                           const std::string &description);

  private:
    static std::vector<EndpointInfo> endpoints_;
    static Json::Value apiInfo_;
    static bool initialized_;

    static Json::Value generatePathItem(const EndpointInfo &endpoint);
    static Json::Value generateSchema();
    static void copySwaggerUiFiles(const std::filesystem::path &targetDir);
    static void createSimpleSwaggerUi(const std::filesystem::path &outputPath);
};

}  // namespace common::documentation
