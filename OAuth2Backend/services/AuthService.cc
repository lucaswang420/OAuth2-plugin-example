#include "AuthService.h"
#include "../models/Users.h"
#include "../models/Roles.h"
#include "../models/UserRoles.h"
#include <drogon/utils/Utilities.h>
#include <algorithm>

using namespace drogon;
using namespace drogon::orm;

namespace services
{

void AuthService::validateUser(
    const std::string &username,
    const std::string &password,
    std::function<void(std::optional<int> userId)> &&callback)
{
    auto sharedCb =
        std::make_shared<std::function<void(std::optional<int> userId)>>(
            std::move(callback));
    try
    {
        auto mapper =
            Mapper<drogon_model::oauth_test::Users>(app().getDbClient());

        // Find user by username
        mapper.findOne(
            {drogon_model::oauth_test::Users::Cols::_username,
             CompareOperator::EQ,
             username},
            [sharedCb, password](const drogon_model::oauth_test::Users &user) {
                // Compute Hash
                std::string salt = user.getValueOfSalt();
                std::string dbHash = user.getValueOfPasswordHash();
                std::string inputHash = utils::getSha256(password + salt);

                // Compare (Case insensitive for Hex)
                bool valid = false;
                if (inputHash.length() == dbHash.length())
                {
                    std::string inputLower = inputHash;
                    std::string dbLower = dbHash;

                    std::transform(inputLower.begin(),
                                   inputLower.end(),
                                   inputLower.begin(),
                                   ::tolower);
                    std::transform(dbLower.begin(),
                                   dbLower.end(),
                                   dbLower.begin(),
                                   ::tolower);

                    if (inputLower == dbLower)
                        valid = true;
                }

                if (valid)
                    (*sharedCb)(user.getValueOfId());
                else
                    (*sharedCb)(std::nullopt);
            },
            [sharedCb](const DrogonDbException &e) {
                LOG_WARN << "Validate User Failed: " << e.base().what();
                (*sharedCb)(std::nullopt);
            });
    }
    catch (const DrogonDbException &e)
    {
        LOG_WARN << "Validate User Init Failed: " << e.base().what();
        (*sharedCb)(std::nullopt);
    }
}

void AuthService::registerUser(
    const std::string &username,
    const std::string &password,
    const std::string &email,
    std::function<void(const std::string &error)> &&callback)
{
    auto sharedCb =
        std::make_shared<std::function<void(const std::string &error)>>(
            std::move(callback));
    // Hash Password
    std::string salt = utils::getUuid();
    std::string passwordHash = utils::getSha256(password + salt);

    drogon_model::oauth_test::Users newUser;
    newUser.setUsername(username);
    newUser.setPasswordHash(passwordHash);
    newUser.setSalt(salt);
    if (!email.empty())
        newUser.setEmail(email);

    try
    {
        auto db = app().getDbClient();
        // Start Transaction? For now, just chain.

        auto mapper = Mapper<drogon_model::oauth_test::Users>(db);

        // Async Insert
        mapper.insert(
            newUser,
            [sharedCb, db](const drogon_model::oauth_test::Users &u) {
                // Assign Default Role "user"
                try
                {
                    auto roleMapper =
                        Mapper<drogon_model::oauth_test::Roles>(db);
                    roleMapper.findOne(
                        Criteria(drogon_model::oauth_test::Roles::Cols::_name,
                                 CompareOperator::EQ,
                                 "user"),
                        [sharedCb, db, userId = u.getValueOfId()](
                            const drogon_model::oauth_test::Roles &role) {
                            try
                            {
                                auto urMapper =
                                    Mapper<drogon_model::oauth_test::UserRoles>(
                                        db);
                                drogon_model::oauth_test::UserRoles ur;
                                ur.setUserId(userId);
                                ur.setRoleId(role.getValueOfId());

                                urMapper.insert(
                                    ur,
                                    [sharedCb](const drogon_model::oauth_test::
                                                   UserRoles &) {
                                        (*sharedCb)("");  // Success
                                    },
                                    [sharedCb](const DrogonDbException &e) {
                                        LOG_ERROR << "Assign Role Failed: "
                                                  << e.base().what();
                                        (*sharedCb)("");  // Treat as success
                                                          // for now (User
                                                          // created), but log
                                                          // error
                                    });
                            }
                            catch (...)
                            {
                                (*sharedCb)("");
                            }
                        },
                        [sharedCb](const DrogonDbException &e) {
                            LOG_ERROR << "Default Role 'user' not found: "
                                      << e.base().what();
                            (*sharedCb)("");  // User created w/o role
                        });
                }
                catch (...)
                {
                    (*sharedCb)("");
                }
            },
            [sharedCb](const DrogonDbException &e) {
                LOG_ERROR << "Register Failed: " << e.base().what();
                // Usually duplicate username
                (*sharedCb)("Registration Failed (Username likely exists)");
            });
    }
    catch (const DrogonDbException &e)
    {
        LOG_ERROR << "Register Init Failed: " << e.base().what();
        (*sharedCb)("Internal Server Error");
    }
}

}  // namespace services
