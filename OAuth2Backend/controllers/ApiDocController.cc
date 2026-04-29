#include "ApiDocController.h"
#include <drogon/utils/Utilities.h>
#include <fstream>
#include <sstream>

using namespace api;

void ApiDocController::openApiSpec(const drogon::HttpRequestPtr &req,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    try {
        // Read the OpenAPI specification file
        std::ifstream file("docs/api/openapi.json");
        if (!file.is_open()) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k404NotFound);
            resp->setContentTypeString("application/json");
            resp->setBody("{\"error\": \"OpenAPI specification not found\"}");
            callback(resp);
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        resp->setContentTypeString("application/json");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->setBody(content);
        callback(resp);
    } catch (const std::exception &e) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody(std::string("Error reading OpenAPI spec: ") + e.what());
        callback(resp);
    }
}

void ApiDocController::swaggerUi(const drogon::HttpRequestPtr &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    try {
        // Read the Swagger UI HTML file
        std::ifstream file("docs/api/swagger-ui/index.html");
        if (!file.is_open()) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k404NotFound);
            resp->setContentTypeString("text/html");
            resp->setBody("<h1>Swagger UI not found</h1><p>Please run the application from the build directory</p>");
            callback(resp);
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        resp->setContentTypeString("text/html");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->setBody(content);
        callback(resp);
    } catch (const std::exception &e) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody(std::string("Error loading Swagger UI: ") + e.what());
        callback(resp);
    }
}