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
    server["url"] = "http://localhost:8080";
    server["description"] = "Development server";
    servers.append(server);
    spec["servers"] = servers;

    // Paths
    Json::Value paths;
    for (const auto &endpoint : endpoints_)
    {
        std::string pathKey = endpoint.path;
        Json::Value pathItem = generatePathItem(endpoint);

        if (paths.isMember(pathKey))
        {
            paths[pathKey][endpoint.method] = pathItem[endpoint.method];
        }
        else
        {
            paths[pathKey] = pathItem;
        }
    }
    spec["paths"] = paths;

    // Components/schemas
    spec["components"]["schemas"] = generateSchema();

    return spec;
}

Json::Value OpenApiGenerator::generatePathItem(const EndpointInfo &endpoint)
{
    Json::Value pathItem;
    pathItem["summary"] = endpoint.summary;
    pathItem["description"] = endpoint.description;
    pathItem["operationId"] = endpoint.method + "_" + endpoint.path.substr(1);

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
        scheme.append("oauth2");
        security["oauth2"] = scheme;
        pathItem["security"] = security;
    }

    Json::Value result;
    result[endpoint.method] = pathItem;
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

        // Copy swagger-ui files to build directory
        copySwaggerUiFiles(dirPath.parent_path() / "swagger-ui");

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error writing OpenAPI spec: " << e.what() << std::endl;
        return false;
    }

    void OpenApiGenerator::copySwaggerUiFiles(
        const std::filesystem::path &targetDir)
    {
        try
        {
            // Create swagger-ui directory if it doesn't exist
            if (!std::filesystem::exists(targetDir))
            {
                std::filesystem::create_directories(targetDir);
                std::cout << "Created swagger-ui directory: "
                          << targetDir.string() << std::endl;
            }

            // Source swagger-ui files (assuming they're in the source tree)
            std::filesystem::path sourceDir = "docs/api/swagger-ui";

            // Check if source directory exists
            if (!std::filesystem::exists(sourceDir))
            {
                std::cerr << "Warning: Source swagger-ui directory not found: "
                          << sourceDir.string() << std::endl;
                // Create a simple swagger-ui index.html as fallback
                createSimpleSwaggerUi(targetDir / "index.html");
                return;
            }

            // Copy all files from source to target
            for (const auto &entry :
                 std::filesystem::directory_iterator(sourceDir))
            {
                if (entry.is_regular_file())
                {
                    std::filesystem::path targetFile =
                        targetDir / entry.path().filename();
                    std::filesystem::copy_file(
                        entry.path(),
                        targetFile,
                        std::filesystem::copy_options::overwrite_existing);
                    std::cout << "Copied swagger-ui file: "
                              << entry.path().filename().string() << std::endl;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error copying swagger-ui files: " << e.what()
                      << std::endl;
            // Create fallback simple UI
            createSimpleSwaggerUi(targetDir / "index.html");
        }
    }

    void OpenApiGenerator::createSimpleSwaggerUi(
        const std::filesystem::path &outputPath)
    {
        try
        {
            std::ofstream outputFile(outputPath);
            if (!outputFile.is_open())
            {
                std::cerr << "Failed to create swagger-ui index.html: "
                          << outputPath.string() << std::endl;
                return;
            }

            outputFile << R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>OAuth2 API Documentation</title>
    <link rel="stylesheet" type="text/css" href="https://unpkg.com/swagger-ui-dist@5.9.0/swagger-ui-bundle.css">
    <style>
        body { margin: 0; padding: 20px; font-family: Arial, sans-serif; }
        #swagger-ui { max-width: 1200px; margin: 0 auto; }
    </style>
</head>
<body>
    <div id="swagger-ui"></div>
    <script src="https://unpkg.com/swagger-ui-dist@5.9.0/swagger-ui-bundle.js"></script>
    <script>
        window.onload = function() {
            SwaggerUIBundle({
                url: '/docs/api/openapi.json',
                dom_id: '#swagger-ui',
                deepLinking: true,
                presets: [
                    SwaggerUIBundle.presets.apis,
                    SwaggerUIBundle.SwaggerUIStandalonePreset
                ],
                plugins: [
                    SwaggerUIBundle.plugins.DownloadUrl
                ]
            });
        };
    </script>
</body>
</html>
            )" << std::endl;

            outputFile.close();
            std::cout << "Created swagger-ui index.html: "
                      << outputPath.string() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error creating swagger-ui index.html: " << e.what()
                      << std::endl;
        }
    }

}  // namespace common::documentation
