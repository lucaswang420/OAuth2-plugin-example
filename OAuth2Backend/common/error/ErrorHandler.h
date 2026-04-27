#pragma once

#include "ErrorTypes.h"
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <functional>

namespace common::error {
using drogon::orm::DrogonDbException;

class ErrorHandler {
public:
    // Generic error handler with try-catch
    template<typename Func>
    static auto handle(Func&& func,
                      std::function<void(const Error&)> callback) -> void {
        try {
            func();
        } catch (const DrogonDbException& e) {
            callback(handleDbException(e));
        } catch (const std::exception& e) {
            callback(Error::fromException(e, ErrorCategory::INTERNAL));
        } catch (...) {
            Error unknown{
                ErrorCode::INTERNAL,
                ErrorCategory::UNKNOWN,
                "Unknown error occurred",
                "",
                generateRequestId()
            };
            callback(unknown);
        }
    }

    // Handle specific exception types
    static Error handleDbException(const DrogonDbException& e);
    static Error handleValidationError(const std::string& field,
                                      const std::string& reason);

    // Utility functions
    static std::string generateRequestId();
    static void logError(const Error& error, const std::string& context = "");
};

} // namespace common::error
