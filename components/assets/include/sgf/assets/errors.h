//
// Created by stuka on 26.10.2026.
//

#pragma once

#include <system_error>

namespace sgf::assets
{
    enum class Error
    {
        kLoaderNotSpecified,
        kInvalidStorage,
        kUnknownAsset,
        kNoSource,
        kLoading,
        kInvalidHandle,
        kUnknownStorageType
    };

    struct ErrorCategory final : public std::error_category
    {
        [[nodiscard]] const char* name() const noexcept override { return "Assets"; }

        [[nodiscard]] std::string message(int ev) const override
        {
            switch(static_cast<Error>(ev))
            {
                case Error::kLoaderNotSpecified: return "The loader for this type of assets is not specified.";
                case Error::kInvalidStorage: return "The storage for the type is invalid.";
                case Error::kUnknownAsset: return "Catalog does not have asset with this ID";
                case Error::kNoSource: return "This asset does not have a source. Maybe the asset is baked?";
                case Error::kLoading: return "This asset is loading now.";
                case Error::kInvalidHandle: return "Invalid asset handle.";
                case Error::kUnknownStorageType: return "Asset was not saved by path and was not baked. Storage type is unknown.";
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