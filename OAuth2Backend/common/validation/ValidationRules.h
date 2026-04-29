#pragma once

#include <string>

namespace common::validation
{

// OAuth2 validation patterns and constants
inline const char *CLIENT_ID_PATTERN = "^[a-zA-Z0-9._-]{1,128}$";
inline const size_t CLIENT_ID_MIN_LEN = 1;
inline const size_t CLIENT_ID_MAX_LEN = 128;

inline const char *REDIRECT_URI_PATTERN = "^https?://[^\\s/$.?#].[^\\s]*$";
inline const size_t REDIRECT_URI_MIN_LEN = 10;
inline const size_t REDIRECT_URI_MAX_LEN = 2048;

inline const char *SCOPE_PATTERN = "^[a-zA-Z0-9: ]+$";
inline const size_t SCOPE_MIN_LEN = 1;
inline const size_t SCOPE_MAX_LEN = 256;

inline const char *TOKEN_PATTERN = "^[a-zA-Z0-9._-]+$";
inline const size_t TOKEN_MIN_LEN = 32;

inline const char *RESPONSE_TYPE_PATTERN = "^[a-zA-Z0-9_]+$";
inline const char *GRANT_TYPE_PATTERN = "^[a-zA-Z0-9_]+$";

enum class ValidationRule
{
    NOT_EMPTY,
    LENGTH_LIMIT,
    REGEX_PATTERN,
    NUMERIC_RANGE,
    URL_FORMAT,
    EMAIL_FORMAT
};

struct ValidationResult
{
    bool isValid;
    std::string fieldName;
    std::string errorMessage;

    static ValidationResult success();
    static ValidationResult failure(const std::string &field,
                                    const std::string &message);
};

}  // namespace common::validation
