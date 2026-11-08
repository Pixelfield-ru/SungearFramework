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
#include "asset_archive.h"

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

        core::String name;
        std::filesystem::path path;

        AssetCatalog() :
            name("assets_catalog")
        { }

        AssetCatalog(const allocator_type& alloc) :
            name("assets_catalog", alloc)
        { }

        AssetCatalog(serialization::Serializer serializer) :
            name("assets_catalog"),
            serializer_(std::move(serializer))
        { }

        AssetCatalog(serialization::Serializer serializer, const allocator_type& alloc) :
            name("assets_catalog", alloc),
            files_(alloc),
            archives_mapping_(alloc),
            files_mapping_(alloc),
            files_mapping_ids_(alloc),
            serializer_(std::move(serializer))
        { }
        AssetCatalog(const AssetCatalog& other) :
            name(other.name),
            next_asset_id_(other.next_asset_id_.load()),
            path(other.path),
            files_(other.files_),
            archives_mapping_(other.archives_mapping_),
            files_mapping_(other.files_mapping_),
            files_mapping_ids_(other.files_mapping_ids_),
            serializer_(other.serializer_)
        { }
        AssetCatalog(const AssetCatalog& other, const allocator_type& alloc) :
            name(other.name, alloc),
            next_asset_id_(other.next_asset_id_.load()),
            path(other.path),
            files_(other.files_, alloc),
            archives_mapping_(other.archives_mapping_, alloc),
            files_mapping_(other.files_mapping_, alloc),
            files_mapping_ids_(other.files_mapping_ids_, alloc),
            serializer_(other.serializer_, alloc)
        { }
        AssetCatalog(AssetCatalog&& other) noexcept :
            name(std::move(other.name)),
            next_asset_id_(other.next_asset_id_.load()),
            path(std::move(other.path)),
            files_(std::move(other.files_)),
            archives_mapping_(std::move(other.archives_mapping_)),
            files_mapping_(std::move(other.files_mapping_)),
            files_mapping_ids_(std::move(other.files_mapping_ids_)),
            serializer_(std::move(other.serializer_))
        {
            other.next_asset_id_ = 0;
        }
        AssetCatalog(AssetCatalog&& other, const allocator_type& alloc) noexcept :
            name(std::move(other.name), alloc),
            next_asset_id_(other.next_asset_id_.load()),
            path(std::move(other.path)),
            files_(std::move(other.files_), alloc),
            archives_mapping_(std::move(other.archives_mapping_), alloc),
            files_mapping_(std::move(other.files_mapping_), alloc),
            files_mapping_ids_(std::move(other.files_mapping_ids_), alloc),
            serializer_(std::move(other.serializer_), alloc)
        {
            other.next_asset_id_ = 0;
        }

        /**
         * Get file by path.
         * @param file_path Path of file to get.
         * @return File. Do not store this pointer.
         */
        File const* findFile(const std::filesystem::path& file_path) const noexcept;
        /**
        * Get file by path.
        * @param file_path Path of file to get.
        * @return File. Do not store this pointer.
        */
        File* findFile(const std::filesystem::path& file_path) noexcept;

        /**
        * Get file by asset ID.
        * @param id ID of asset.
        * @return File. Do not store this pointer.
        */
        File* findFile(AssetId id) noexcept;
        /**
       * Get file by asset ID.
       * @param id ID of asset.
       * @return File. Do not store this pointer.
       */
        File const* findFile(AssetId id) const noexcept;

        Archive* findArchive(const std::filesystem::path& archive_path) noexcept;

        File& acquireFile(std::filesystem::path file_path);
        Archive& acquireArchive(std::filesystem::path archive_path);

        AssetId acquireId();

        /**
         * Writes catalog information in file by AssetCatalog::path.
         * @return Catalog save result.
         */
        std::error_code save();
        std::error_code load();

        [[nodiscard]] bool exists() const noexcept;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return files_.get_allocator();
        }

        AssetCatalog& operator=(const AssetCatalog& other)
        {
            if(this == std::addressof(other)) return *this;

            name = other.name;
            next_asset_id_ = other.next_asset_id_.load();
            path = other.path;
            files_ = other.files_;
            archives_mapping_ = other.archives_mapping_;
            files_mapping_ = other.files_mapping_;
            files_mapping_ids_ = other.files_mapping_ids_;
            serializer_ = other.serializer_;

            return *this;
        }

        AssetCatalog& operator=(AssetCatalog&& other) noexcept
        {
            if(this == std::addressof(other)) return *this;

            name = std::move(other.name);
            next_asset_id_ = other.next_asset_id_.load();
            other.next_asset_id_ = 0;
            path = std::move(other.path);
            files_ = std::move(other.files_);
            archives_mapping_ = std::move(other.archives_mapping_);
            files_mapping_ = std::move(other.files_mapping_);
            files_mapping_ids_ = std::move(other.files_mapping_ids_);
            serializer_ = std::move(other.serializer_);

            return *this;
        }

    private:
        std::atomic<AssetId> next_asset_id_ = 1;
        core::Vector<File> files_;
        core::Vector<Archive> archives_;
        core::UnorderedMap<std::filesystem::path, std::size_t> archives_mapping_;
        core::UnorderedMap<std::filesystem::path, std::size_t> files_mapping_;
        core::UnorderedMap<AssetId, std::size_t> files_mapping_ids_;
        serialization::Serializer serializer_;
    };
}
