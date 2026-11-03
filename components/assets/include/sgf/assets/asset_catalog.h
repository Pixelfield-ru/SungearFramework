//
// Created by stuka on 28.10.2026.
//

#pragma once

#include <filesystem>
#include <optional>

#include "asset_slot.h"

#include "sgf/core/memory.h"
#include "sgf/serialization/macroses.h"
#include "sgf/serialization/serializer.h"

namespace sgf::assets
{
    /**
     * Manages assets IDs.
     */
    struct AssetCatalog final
    {
        SGF_SERIALIZER_AS_FRIEND();

        /// File can be baked or saved in filesystem or both.
        struct File final
        {
            struct Baked final
            {
                std::filesystem::path archive;
                std::size_t offset {};
                std::size_t size {};
            };

            AssetId id {};
            std::int64_t type_id {};
            std::optional<std::filesystem::path> path;
            std::optional<Baked> baked;
        };

        using allocator_type = std::pmr::polymorphic_allocator<>;

        std::filesystem::path path;

        AssetCatalog(serialization::Serializer serializer) :
            serializer_(std::move(serializer))
        { }

        AssetCatalog(serialization::Serializer serializer, const allocator_type& alloc) :
            files_(alloc),
            files_mapping_(alloc),
            serializer_(std::move(serializer))
        { }
        AssetCatalog(const AssetCatalog& other) :
            next_asset_id_(other.next_asset_id_.load()),
            path(other.path),
            files_(other.files_),
            files_mapping_(other.files_mapping_),
            serializer_(other.serializer_)
        { }
        AssetCatalog(const AssetCatalog& other, const allocator_type& alloc) :
            next_asset_id_(other.next_asset_id_.load()),
            path(other.path),
            files_(other.files_, alloc),
            files_mapping_(other.files_mapping_, alloc),
            serializer_(other.serializer_, alloc)
        { }
        AssetCatalog(AssetCatalog&& other) noexcept :
            next_asset_id_(other.next_asset_id_.load()),
            path(std::move(other.path)),
            files_(std::move(other.files_)),
            files_mapping_(std::move(other.files_mapping_)),
            serializer_(std::move(other.serializer_))
        { }
        AssetCatalog(AssetCatalog&& other, const allocator_type& alloc) noexcept :
            next_asset_id_(other.next_asset_id_.load()),
            path(std::move(other.path)),
            files_(std::move(other.files_), alloc),
            files_mapping_(std::move(other.files_mapping_), alloc),
            serializer_(std::move(other.serializer_), alloc)
        { }

        /**
         * Get file by path.
         * @param file_path Path of file to get.
         * @return File. Do not store this pointer.
         */
        File const* find(const std::filesystem::path& file_path) const noexcept;
        /**
        * Get file by path.
        * @param file_path Path of file to get.
        * @return File. Do not store this pointer.
        */
        File* find(const std::filesystem::path& file_path) noexcept;

        File& acquireFile(std::filesystem::path file_path);

        /**
         * Writes catalog information in file by AssetCatalog::path.
         * @return Catalog save result.
         */
        std::error_code save();

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return files_.get_allocator();
        }

        AssetCatalog& operator=(const AssetCatalog& other)
        {
            if(this == std::addressof(other)) return *this;

            next_asset_id_ = other.next_asset_id_.load();
            path = other.path;
            files_ = other.files_;
            files_mapping_ = other.files_mapping_;
            serializer_ = other.serializer_;

            return *this;
        }

        AssetCatalog& operator=(AssetCatalog&& other) noexcept
        {
            if(this == std::addressof(other)) return *this;

            next_asset_id_ = other.next_asset_id_.load();
            path = std::move(other.path);
            files_ = std::move(other.files_);
            files_mapping_ = std::move(other.files_mapping_);
            serializer_ = std::move(other.serializer_);

            return *this;
        }

    private:
        std::atomic<AssetId> next_asset_id_ = 1;
        core::Vector<File> files_;
        core::UnorderedMap<std::filesystem::path, std::size_t> files_mapping_;
        serialization::Serializer serializer_;
    };
}
