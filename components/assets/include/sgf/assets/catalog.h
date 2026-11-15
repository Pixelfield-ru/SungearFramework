//
// Created by stuka on 28.10.2026.
//

#pragma once

#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <span>

#include "slot.h"

#include "sgf/core/memory.h"
#include "sgf/serialization/macroses.h"
#include "sgf/serialization/serializer.h"
#include "sgf/core/utility/mapped_file.h"

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

        /// Result of acquiring a catalog row by path. `created` is false when that path already had a row.
        struct AcquiredFile final
        {
            File file;
            bool created = false;
        };

        /// Baked file location without the archive path. One bake shares a single archive.
        struct BakedAsset final
        {
            AssetId asset_id {};
            std::size_t offset {};
            std::size_t size {};
        };

        using allocator_type = std::pmr::polymorphic_allocator<>;

        explicit AssetCatalog(const allocator_type& alloc = {}) :
            name_("assets_catalog", alloc),
            files_(alloc),
            archives_(alloc),
            archives_mapping_(alloc),
            files_mapping_(alloc),
            files_mapping_ids_(alloc)
        { }

        explicit AssetCatalog(serialization::Serializer serializer, const allocator_type& alloc = {}) :
            name_("assets_catalog", alloc),
            files_(alloc),
            archives_(alloc),
            archives_mapping_(alloc),
            files_mapping_(alloc),
            files_mapping_ids_(alloc),
            serializer_(std::move(serializer))
        { }
        AssetCatalog(const AssetCatalog& other);
        AssetCatalog(const AssetCatalog& other, const allocator_type& alloc);
        AssetCatalog(AssetCatalog&& other) noexcept;
        AssetCatalog(AssetCatalog&& other, const allocator_type& alloc) noexcept;

        /**
         * Copy of the row for this path. Empty when the path is not in the catalog.
         * The copy stays valid after the catalog lock is released.
         */
        [[nodiscard]] std::optional<File> findFile(const std::filesystem::path& file_path) const;
        /**
         * Copy of the row for this id. Empty when the id is not in the catalog.
         * The copy stays valid after the catalog lock is released.
         */
        [[nodiscard]] std::optional<File> findFile(AssetId id) const;

        [[nodiscard]] bool hasFile(AssetId id) const noexcept;

        /// Archive object for this path. Stays allocated until the catalog is reloaded or destroyed.
        core::MappedFile* findArchive(const std::filesystem::path& archive_path);

        /// Finds the row for this path or creates one. Creation is serialized with other catalog writes.
        AcquiredFile acquireFile(std::filesystem::path file_path);
        /// Creates a row without a path.
        File acquireFile();
        /// Finds the archive for this path or creates a closed one.
        /// Stays valid until mount. Do not call mount while a load is using the archive.
        core::MappedFile& claimArchive(std::filesystem::path archive_path);

        void setTypeId(AssetId id, std::int64_t type_id);
        /// Writes the baked location of an existing row. False when the id is missing.
        bool setBaked(AssetId id, File::Baked baked);
        /// Writes every location under one lock. False when any id is missing, and then no row is changed.
        bool setBaked(const std::filesystem::path& archive, std::span<const BakedAsset> assets);

        [[nodiscard]] std::filesystem::path root() const;
        void setRoot(std::filesystem::path root);

        /**
         * Writes catalog information in file by AssetCatalog::path.
         * @return Catalog save result.
         */
        std::error_code save();
        std::error_code load();

        [[nodiscard]] bool exists() const;

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return files_.get_allocator();
        }

        AssetCatalog& operator=(const AssetCatalog& other);
        AssetCatalog& operator=(AssetCatalog&& other) noexcept;

    private:
        std::atomic<AssetId> next_asset_id_ = 1;
        core::Vector<File> files_;
        core::Vector<core::UniquePtr<core::MappedFile>> archives_;
        core::UnorderedMap<std::filesystem::path, std::size_t> archives_mapping_;
        core::UnorderedMap<std::filesystem::path, std::size_t> files_mapping_;
        core::UnorderedMap<AssetId, std::size_t> files_mapping_ids_;
        serialization::Serializer serializer_;

        core::String name_;
        std::filesystem::path path_;

        mutable std::shared_mutex mutex_;
    };
}
