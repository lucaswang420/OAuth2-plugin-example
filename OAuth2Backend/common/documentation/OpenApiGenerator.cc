#include "OpenApiGenerator.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <system_error>

namespace common::documentation
{

std::vector<EndpointInfo> OpenApiGenerator::endpoints_;
Json::Value OpenApiGenerator::apiInfo_;
bool OpenApiGenerator::initialized_ = false;

void OpenApiGenerator::setApiInfo(const std::string &title,
                                  const std::string &version,
                                  const std::string &description)
{
    apiInfo_["title"] = title;
    apiInfo_["version"] = version;
    apiInfo_["description"] = description;
    initialized_ = true;
}

void OpenApiGenerator::addEndpoint(const EndpointInfo &endpoint)
{
    endpoints_.push_back(endpoint);
}

Json::Value OpenApiGenerator::generateOpenApiSpec()
{
    Json::Value spec;
    spec["openapi"] = "3.0.0";

    // Info section
    if (!initialized_)
    {
        setApiInfo("OAuth2 Authorization Server API",
                   "1.0.0",
                   "OAuth2.0 authorization server with token management");
    }
    spec["info"] = apiInfo_;

    // Servers
    Json::Value servers(Json::arrayValue);
    Json::Value server;
    server["url"] = "http://localhost:5555";
    server["description"] = "Development server";
    servers.append(server);
    spec["servers"] = servers;

    // Paths
    Json::Value paths;
    for (const auto &endpoint : endpoints_)
    {
        std::string pathKey = endpoint.path;
        Json::Value pathItem = generatePathItem(endpoint);

        std::string mLower = endpoint.method;
        std::transform(mLower.begin(), mLower.end(), mLower.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        if (paths.isMember(pathKey))
        {
            paths[pathKey][mLower] = pathItem[mLower];
        }
        else
        {
            paths[pathKey] = pathItem;
        }
    }
    spec["paths"] = paths;

    // Components/schemas
    spec["components"]["schemas"] = generateSchema();
    
    // Security Schemes
    Json::Value securitySchemes;
    Json::Value bearerAuth;
    bearerAuth["type"] = "http";
    bearerAuth["scheme"] = "bearer";
    securitySchemes["bearerAuth"] = bearerAuth;
    spec["components"]["securitySchemes"] = securitySchemes;

    return spec;
}

Json::Value OpenApiGenerator::generatePathItem(const EndpointInfo &endpoint)
{
    Json::Value pathItem;
    pathItem["summary"] = endpoint.summary;
    pathItem["description"] = endpoint.description;
    
    std::string safePath = endpoint.path.substr(1);
    std::replace(safePath.begin(), safePath.end(), '/', '_');
    
    std::string methodLower = endpoint.method;
    std::transform(methodLower.begin(), methodLower.end(), methodLower.begin(),
                   [](unsigned char c){ return std::tolower(c); });
                   
    pathItem["operationId"] = methodLower + "_" + safePath;

    // Tags
    Json::Value tags(Json::arrayValue);
    for (const auto &tag : endpoint.tags)
    {
        tags.append(tag);
    }
    pathItem["tags"] = tags;

    // Parameters
    if (!endpoint.parameters.empty())
    {
        Json::Value parameters(Json::arrayValue);
        for (const auto &[name, desc] : endpoint.parameters)
        {
            Json::Value param;
            param["name"] = name;
            param["in"] = "query";
            param["description"] = desc;
            param["required"] = true;
            param["schema"]["type"] = "string";
            parameters.append(param);
        }
        pathItem["parameters"] = parameters;
    }

    // Responses
    Json::Value responses;
    for (const auto &[code, desc] : endpoint.responses)
    {
        Json::Value response;
        response["description"] = desc;
        if (code == 200)
        {
            response["content"]["application/json"]["schema"]["type"] =
                "object";
        }
        responses[std::to_string(code)] = response;
    }
    pathItem["responses"] = responses;

    // Security
    if (endpoint.requiresAuth)
    {
        Json::Value security;
        Json::Value scheme(Json::arrayValue);
        // Using bearerAuth based on existing openapi.yaml
        scheme.append("bearerAuth");
        security["bearerAuth"] = scheme;
        pathItem["security"] = security;
    }

    Json::Value result;
    result[methodLower] = pathItem;
    return result;
}

Json::Value OpenApiGenerator::generateSchema()
{
    Json::Value schemas;

    // Error schema
    Json::Value errorSchema;
    errorSchema["type"] = "object";
    Json::Value errorProps;
    errorProps["code"]["type"] = "integer";
    errorProps["category"]["type"] = "string";
    errorProps["message"]["type"] = "string";
    errorProps["details"]["type"] = "string";
    errorProps["request_id"]["type"] = "string";
    errorSchema["properties"] = errorProps;
    schemas["Error"] = errorSchema;

    // Token response schema
    Json::Value tokenSchema;
    tokenSchema["type"] = "object";
    Json::Value tokenProps;
    tokenProps["access_token"]["type"] = "string";
    tokenProps["refresh_token"]["type"] = "string";
    tokenProps["expires_in"]["type"] = "integer";
    tokenProps["token_type"]["type"] = "string";
    tokenSchema["properties"] = tokenProps;
    schemas["TokenResponse"] = tokenSchema;

    return schemas;
}

bool OpenApiGenerator::writeToFile(const std::string &outputPath)
{
    try
    {
        // Create directory if it doesn't exist
        std::filesystem::path filePath(outputPath);
        std::filesystem::path dirPath = filePath.parent_path();

        if (!dirPath.empty() && !std::filesystem::exists(dirPath))
        {
            std::filesystem::create_directories(dirPath);
            std::cout << "Created directory: " << dirPath.string() << std::endl;
        }

        Json::Value spec = generateOpenApiSpec();

        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

        std::ofstream outputFile(outputPath);
        if (!outputFile.is_open())
        {
            std::cerr << "Failed to open file for writing: " << outputPath
                      << std::endl;
            return false;
        }

        writer->write(spec, &outputFile);
        outputFile.close();

        std::cout << "OpenAPI specification written to: " << outputPath
                  << std::endl;
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error writing OpenAPI spec: " << e.what() << std::endl;
        return false;
    }
}

}  // namespace common::documentation
