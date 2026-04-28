#include "AdminController.h"
#include "../common/error/ErrorHandler.h"

using namespace common::error;

void AdminController::dashboard(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Use ErrorHandler for unified error handling
    ErrorHandler::handle([&]() {
        // This endpoint is protected by AuthorizationFilter
        // If we are here, the user has admin permissions

        Json::Value json;
        json["message"] = "Welcome to Admin Dashboard";
        json["status"] = "success";

        auto resp = HttpResponse::newHttpJsonResponse(json);
        callback(resp);
    }, [&](const Error& error) {
        // Error callback - unified error response
        ErrorHandler::logError(error, "AdminController::dashboard");

        Json::Value errorJson = error.toJson();
        auto resp = HttpResponse::newHttpJsonResponse(errorJson);
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(error.toHttpStatusCode()));
        callback(resp);
    });
}
