//
// Created by stuka on 31.10.2026.
//

#pragma once

#include <system_error>

namespace sgf::serialization
{
    enum class Error
    {
        kExistingKey = 1,
        kUnexpectedKey,
        kUnexpectedValue,
        kUnbalancedScope,
        kInvalidNumber,
        kWriteFailed,
        kSettingsLocked,
        kParseError,
        kTypeMismatch
    };

    struct ErrorCategory final : public std::error_category
    {
        [[nodiscard]] const char* name() const noexcept override { return "Serialization"; }

        [[nodiscard]] std::string message(int ev) const override
        {
            switch(static_cast<Error>(ev))
            {
                case Error::kExistingKey: return "Such key is already exists.";
                case Error::kUnexpectedKey: return "A key is not expected here.";
                case Error::kUnexpectedValue: return "A value is not expected here.";
                case Error::kUnbalancedScope: return "Object or array was closed in the wrong scope.";
                case Error::kInvalidNumber: return "Number is not finite.";
                case Error::kWriteFailed: return "JSON writer failed to emit a value.";
                case Error::kSettingsLocked: return "Writer settings cannot be changed after output has started.";
                case Error::kParseError: return "JSON text could not be parsed.";
                case Error::kTypeMismatch: return "JSON value has a different type.";
                default: return "Unknown error";
            }
        }
    };

    inline const std::error_category& error_category()
    {
        static const ErrorCategory cat;
        return cat;
    }

    inline std::error_code make_error_code(Error e) noexcept
    {
        return { static_cast<int>(e), error_category() };
    }
}

namespace std
{
    template<>
    struct is_error_code_enum<sgf::serialization::Error> : true_type {};
}