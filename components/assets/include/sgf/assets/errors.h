//
// Created by stuka on 26.10.2026.
//

#pragma once

#include <system_error>

namespace sgf::assets
{
    enum class Error
    {
        kLoaderNotSpecified
    };

    struct ErrorCategory final : public std::error_category
    {
        [[nodiscard]] const char* name() const noexcept override { return "Assets"; }

        [[nodiscard]] std::string message(int ev) const override
        {
            switch(static_cast<Error>(ev))
            {
                case Error::kLoaderNotSpecified: return "The loader for this type of assets is not specified.";
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
    struct is_error_code_enum<sgf::assets::Error> : true_type {};
}