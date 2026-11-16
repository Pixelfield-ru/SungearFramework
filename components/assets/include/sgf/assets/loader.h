//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <cctype>
#include <filesystem>
#include <span>
#include <system_error>
#include <expected>

#include "slot.h"
#include "handle.h"
#include "importer.h"

#include "sgf/core/memory.h"
#include "sgf/core/utility/file.h"

#include "errors.h"

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
        using allocator_type = std::pmr::polymorphic_allocator<>;

        explicit IAssetLoader(allocator_type allocator = {}) :
            importers_(allocator),
            extensions_(allocator)
        { }

        ~IAssetLoader() override = default;
        [[nodiscard]] virtual std::error_code loadSource(const std::filesystem::path& source, AssetT& asset)
        {
            auto* importer = findImporter(source);
            if(!importer) return Error::kImporterNotSpecified;
            return importer->importSource(source, asset);
        }
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

        /// Registers a source importer. Call before load. The same extension cannot be registered twice.
        virtual std::error_code addImporter(core::UniquePtr<IAssetImporter<AssetT>> importer)
        {
            if(!importer) return std::make_error_code(std::errc::invalid_argument);

            const auto declared = importer->extensions();
            if(declared.empty()) return std::make_error_code(std::errc::invalid_argument);

            core::Vector<core::String> keys(extensions_.get_allocator());
            keys.reserve(declared.size());
            for(const std::string_view extension : declared)
            {
                if(extension.empty() || extension.front() != '.')
                {
                    return std::make_error_code(std::errc::invalid_argument);
                }

                core::String key = core::normalizedExtension(extension);
                // if this loader already has an importer for such extension
                if(extensions_.contains(key)) return std::make_error_code(std::errc::invalid_argument);
                // repeating the file extension
                for(const core::String& existing : keys)
                {
                    if(existing == key) return std::make_error_code(std::errc::invalid_argument);
                }

                keys.push_back(std::move(key));
            }

            // store the importer
            IAssetImporter<AssetT>* const importer_ptr = importer.get();
            importers_.push_back(std::move(importer));
            for(core::String& key : keys)
            {
                extensions_.emplace(std::move(key), importer_ptr);
            }

            return {};
        }

        /// Removes the importer that serves this extension, and every other extension it serves.
        void removeImporter(std::string_view extension)
        {
            const core::String key = core::normalizedExtension(extension, extensions_.get_allocator());
            const auto found = extensions_.find(key);
            if(found == extensions_.end()) return;

            IAssetImporter<AssetT>* const importer = found->second;
            // remove the importer of 'extension' from his other extensions
            for(const std::string_view declared : importer->extensions())
            {
                extensions_.erase(core::normalizedExtension(declared, extensions_.get_allocator()));
            }

            // remove the importer from the vector
            for(auto it = importers_.begin(); it != importers_.end(); ++it)
            {
                if(it->get() != importer) continue;
                importers_.erase(it);
                return;
            }
        }

    protected:
        /// Importer for this path's extension, or null when none was registered.
        [[nodiscard]] IAssetImporter<AssetT>* findImporter(const std::filesystem::path& source) const
        {
            const auto found = extensions_.find(core::normalizedExtension(source.extension().string()));
            if(found == extensions_.end()) return {};
            return found->second;
        }

    private:
        core::Vector<core::UniquePtr<IAssetImporter<AssetT>>> importers_;
        core::UnorderedMap<core::String, IAssetImporter<AssetT>*> extensions_;
    };
}
