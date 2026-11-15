//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <filesystem>
#include <span>
#include <system_error>
#include <expected>

#include "sgf/core/memory.h"
#include "slot.h"
#include "handle.h"

namespace sgf::assets
{
    struct IErasedAssetLoader
    {
        virtual ~IErasedAssetLoader() = default;
        [[nodiscard]] virtual std::error_code loadSource(const std::filesystem::path& source, std::byte* asset) = 0;
        [[nodiscard]] virtual std::error_code loadBaked(std::span<const std::byte> source,
                                                              std::span<const AnyHandle> dependencies,
                                                              std::byte* asset) = 0;
        [[nodiscard]] virtual std::expected<core::Vector<std::byte>, std::error_code> saveBaked(std::byte const* asset) = 0;

        [[nodiscard]] virtual std::error_code loadMeta(const std::filesystem::path& source, AssetId id) = 0;
        [[nodiscard]] virtual std::error_code writeMeta(const std::filesystem::path& source, AssetId id) = 0;

        [[nodiscard]] virtual core::Vector<AnyHandle> dependencies(std::byte const* asset) = 0;
    };

    template<typename AssetT>
    struct IAssetLoader : IErasedAssetLoader
    {
        ~IAssetLoader() override = default;
        [[nodiscard]] virtual std::error_code loadSource(const std::filesystem::path& source, AssetT& asset) = 0;
        [[nodiscard]] std::error_code loadSource(const std::filesystem::path& source, std::byte* asset) final
        {
            return loadSource(source, *reinterpret_cast<AssetT*>(asset));
        }
        [[nodiscard]] virtual std::error_code loadBaked(std::span<const std::byte> bytes,
                                                        std::span<const AnyHandle> dependencies,
                                                        AssetT& asset) = 0;
        [[nodiscard]] std::error_code loadBaked(std::span<const std::byte> source,
                                                      std::span<const AnyHandle> dependencies,
                                                      std::byte* asset) final
        {
            return loadBaked(source, dependencies, *reinterpret_cast<AssetT*>(asset));
        }
        [[nodiscard]] virtual std::expected<core::Vector<std::byte>, std::error_code> saveBaked(const AssetT& asset) = 0;
        [[nodiscard]] std::expected<core::Vector<std::byte>, std::error_code> saveBaked(std::byte const* asset) final
        {
            return saveBaked(*reinterpret_cast<AssetT const*>(asset));
        }

        [[nodiscard]] virtual core::Vector<AnyHandle> dependencies(const AssetT& asset) = 0;
        [[nodiscard]] core::Vector<AnyHandle> dependencies(std::byte const* asset) override
        {
            return dependencies(*reinterpret_cast<AssetT const*>(asset));
        }
    };
}
