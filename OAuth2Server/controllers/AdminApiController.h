#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class AdminApiController : public drogon::HttpController<AdminApiController>
{
  public:
    METHOD_LIST_BEGIN
    // Client Management
    ADD_METHOD_TO(
      AdminApiController::listClients,
      "/api/admin/clients",
      Get,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::createClient,
      "/api/admin/clients",
      Post,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::getClient,
      "/api/admin/clients/{clientId}",
      Get,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::updateClient,
      "/api/admin/clients/{clientId}",
      Put,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::deleteClient,
      "/api/admin/clients/{clientId}",
      Delete,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::resetClientSecret,
      "/api/admin/clients/{clientId}/reset-secret",
      Post,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::getClientScopes,
      "/api/admin/clients/{clientId}/scopes",
      Get,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::updateClientScopes,
      "/api/admin/clients/{clientId}/scopes",
      Put,
      "AuthorizationFilter"
    );

    // User Management
    ADD_METHOD_TO(AdminApiController::listUsers, "/api/admin/users", Get, "AuthorizationFilter");
    ADD_METHOD_TO(
      AdminApiController::disableUser,
      "/api/admin/users/{userId}/disable",
      Put,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::assignUserRoles,
      "/api/admin/users/{userId}/roles",
      Put,
      "AuthorizationFilter"
    );

    // Scope Management
    ADD_METHOD_TO(AdminApiController::listScopes, "/api/admin/scopes", Get, "AuthorizationFilter");

    // Audit Logs
    ADD_METHOD_TO(AdminApiController::listLogs, "/api/admin/logs", Get, "AuthorizationFilter");

    // Token Management
    ADD_METHOD_TO(AdminApiController::listTokens, "/api/admin/tokens", Get, "AuthorizationFilter");
    ADD_METHOD_TO(
      AdminApiController::revokeTokensByClient,
      "/api/admin/tokens/revoke-by-client",
      Post,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::revokeTokensByUser,
      "/api/admin/tokens/revoke-by-user",
      Post,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::revokeToken,
      "/api/admin/tokens/{tokenPrefix}",
      Delete,
      "AuthorizationFilter"
    );

    // User Detail & Management
    ADD_METHOD_TO(
      AdminApiController::getUser,
      "/api/admin/users/{userId}",
      Get,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::updateUser,
      "/api/admin/users/{userId}",
      Put,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::enableUser,
      "/api/admin/users/{userId}/enable",
      Post,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::getUserRoles,
      "/api/admin/users/{userId}/roles",
      Get,
      "AuthorizationFilter"
    );

    // Role Management
    ADD_METHOD_TO(AdminApiController::listRoles, "/api/admin/roles", Get, "AuthorizationFilter");
    ADD_METHOD_TO(AdminApiController::createRole, "/api/admin/roles", Post, "AuthorizationFilter");
    ADD_METHOD_TO(
      AdminApiController::updateRole,
      "/api/admin/roles/{roleId}",
      Put,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::deleteRole,
      "/api/admin/roles/{roleId}",
      Delete,
      "AuthorizationFilter"
    );

    // Scope Management (CRUD)
    ADD_METHOD_TO(
      AdminApiController::createScope,
      "/api/admin/scopes",
      Post,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::updateScope,
      "/api/admin/scopes/{scopeId}",
      Put,
      "AuthorizationFilter"
    );
    ADD_METHOD_TO(
      AdminApiController::deleteScope,
      "/api/admin/scopes/{scopeId}",
      Delete,
      "AuthorizationFilter"
    );

    // Dashboard Stats
    ADD_METHOD_TO(
      AdminApiController::getDashboardStats,
      "/api/admin/dashboard/stats",
      Get,
      "AuthorizationFilter"
    );

    // OIDC Key Info
    ADD_METHOD_TO(
      AdminApiController::getOidcKeys,
      "/api/admin/oidc/keys",
      Get,
      "AuthorizationFilter"
    );
    METHOD_LIST_END

    void listClients(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void createClient(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void deleteClient(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &clientId
    );

    void getClient(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &clientId
    );

    void updateClient(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &clientId
    );

    void getClientScopes(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &clientId
    );

    void updateClientScopes(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &clientId
    );

    void resetClientSecret(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &clientId
    );

    void listUsers(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void disableUser(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &userId
    );

    void assignUserRoles(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &userId
    );

    void listScopes(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void listLogs(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void listTokens(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void revokeToken(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &tokenPrefix
    );

    void revokeTokensByClient(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void revokeTokensByUser(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void getOidcKeys(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    // User Detail & Management
    void getUser(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &userId
    );

    void updateUser(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &userId
    );

    void enableUser(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &userId
    );

    void getUserRoles(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &userId
    );

    // Role Management
    void listRoles(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void createRole(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void updateRole(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &roleId
    );

    void deleteRole(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &roleId
    );

    // Scope Management (CRUD)
    void createScope(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void updateScope(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &scopeId
    );

    void deleteScope(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback,
      const std::string &scopeId
    );

    // Dashboard Stats
    void getDashboardStats(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback
    );
};
