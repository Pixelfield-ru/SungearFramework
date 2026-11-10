//
// Created by stuka on 25.10.2026.
//

#pragma once

#include "sgf/core/memory.h"
#include "sgf/core/structures/type_map.h"
#include "sgf/core/utility/file.h"
#include "sgf/core/log.h"
#include "sgf/core/thread_pool.h"

#include <atomic>
#include <functional>

#include "asset_storage.h"
#include "asset_catalog.h"
#include "asset_handle.h"
#include "errors.h"

namespace sgf::assets
{
    struct AssetServerCreateInfo
    {
        serialization::Serializer serializer;
        size_t page_size {};
        const core::LogCallback* debug_output {};
        core::ThreadPool* thread_pool {};
    };

    // todo: use stable type index
    struct AssetServer final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

    private:
        struct PostedJobs
        {
            std::atomic<std::size_t> count {};
        };

    public:
        AssetServer() :
            posted_(core::makeShared<PostedJobs>(allocator_type{}))
        { }
        AssetServer(const allocator_type& alloc) :
            storages_map_(alloc),
            catalog_(alloc),
            posted_(core::makeShared<PostedJobs>(alloc))
        { }
        AssetServer(const AssetServer&) = delete;
        AssetServer(AssetServer&&) = default;
        AssetServer(AssetServer&& other, const allocator_type& alloc) noexcept :
            storages_map_(std::move(other.storages_map_), alloc),
            catalog_(std::move(other.catalog_), alloc),
            page_size_(other.page_size_),
            debug_output_(other.debug_output_),
            thread_pool_(other.thread_pool_),
            posted_(std::move(other.posted_))
        { }
        ~AssetServer();

        /**
         * Block until every load posted to the pool has finished.
         * The caller must not be a pool thread, and must not call load while waiting.
         */
        void waitPosted() noexcept;

        /***
         * Mount the catalog in a directory.
         * @param path Path to mount the catalog.
         */
        std::error_code mount(std::filesystem::path path) noexcept;

        /**
         * Load asset by path.
         * @tparam AssetT Type of asset.
         * @param path Path of asset.
         * @return Handle to the asset or an error.
         */
        template<typename AssetT>
        std::expected<Handle<AssetT>, std::error_code> load(const std::filesystem::path& path) noexcept
        {
            const auto key = core::normalizedRelativePath(catalog_.path / path, catalog_.path);
            if(!key) return std::unexpected(key.error());

            const auto actual_path = catalog_.path / *key;

            if(!std::filesystem::exists(actual_path)) return std::unexpected(std::errc::no_such_file_or_directory);

            const auto type = core::typeIndex<AssetT>();
            auto& storage = this->storage<AssetT>();

            auto* loader = storage.loader();
            if(!loader)
            {
                return Error::kLoaderNotSpecified;
            }

            // trying to find existing asset
            auto* file = catalog_.findFile(*key);

            // if file was not found then creating new file in catalog
            if(!file)
            {
                // acquire new id for new asset
                file = &catalog_.acquireFile(*key);
                file->type_id = type;

                // saving catalog to file
                if(const auto err = catalog_.save()) return std::unexpected(err);

                // writing meta of the file near the file
                if(const auto err = loader->writeMeta(actual_path, file->id)) return std::unexpected(err);
            }
            else
            {
                // load meta of the file near the file
                if(const auto err = loader->loadMeta(actual_path, file->id)) return std::unexpected(err);
            }

            auto* slot = storage.findSlot(file->id);
            // if slot is already exists then leave
            if(slot && (slot->loaded() || slot->state == AssetState::kLoading))
            {
                return Handle<AssetT>(slot, type);
            }

            // get new slot for asset
            if(!slot) slot = storage.acquireSlot(file->id);

            slot->state = AssetState::kLoading;

            if(const auto err = postTracked([loader, actual_path, slot] {
                if(const auto err = loader->loadSource(actual_path, *slot->value))
                {
                    slot->error = err;
                    slot->state = AssetState::kFailed;
                    return;
                }

                slot->error.clear();
                slot->state = AssetState::kLoaded;
            }))
            {
                slot->error = err;
                slot->state = AssetState::kFailed;
                return std::unexpected(err);
            }

            return Handle<AssetT>(slot, type);
        }

        /**
         * Load asset that exists in catalog.
         * Asset loads only if it was not loaded earlier and was saved by path or in an archive.
         * @param asset_id Asset ID that needs to be loaded.
         * @param type_id Type ID of asset.
         * @return Handle to the asset.
         */
        std::expected<AnyHandle, std::error_code> load(AssetId asset_id, std::uint32_t type_id) noexcept;

        /**
         * Reload asset by path.
         * @param handle Asset that needs to be reloaded.
         * @return Error.
         */
        std::error_code reload(const AnyHandle& handle) noexcept;


        std::error_code bake(const AnyHandle& handle, std::filesystem::path archive, std::int32_t compression_level);

        template<typename AssetT>
        Handle<AssetT> find(AssetId asset_id) noexcept
        {
            auto& st = storage<AssetT>();
            auto* slot = st.findSlot(asset_id);

            return { slot, core::typeIndex<AssetT>() };
        }

        AnyHandle find(AssetId asset_id, std::uint32_t type_id) noexcept
        {
            auto* st = storage(type_id);
            if(!st) return {};

            auto* control = st->slotControl(asset_id);
            if(!control) return {};

            return { control, type_id };
        }

        template<typename AssetT>
        requires(std::is_copy_constructible_v<AssetT>)
        Handle<AssetT> copy(const Handle<AssetT>& source) noexcept
        {
            const auto* asset = source.get();
            // kLoading and kFailed have no finished value to copy
            if(!asset) return {};

            auto& storage = this->storage<AssetT>();
            static const auto type_id = core::typeIndex<AssetT>();

            auto& new_file = catalog_.acquireFile();
            new_file.type_id = type_id;
            // get new slot for asset
            auto* slot = storage.acquireSlot(new_file.id);
            // creating new asset using copy constructor with allocator
            // or without allocator if T does not have such constructor
            auto new_asset = std::make_obj_using_allocator<AssetT>(get_allocator(), *asset);
            slot->insert(std::move(new_asset));
            // the source is loaded so we put kLoaded state in the copy
            slot->state = AssetState::kLoaded;

            return Handle<AssetT>(slot, type_id);
        }

        template<typename AssetT>
        AssetStorage<AssetT>& storage() noexcept
        {
            using Storage = AssetStorage<AssetT>;

            if(storages_map_.hasType<Storage>())
            {
                return storages_map_.write<Storage>();
            }

            auto& storage = storages_map_.emplace<Storage>(page_size_, storages_map_.get_allocator());
            return storage;
        }

        IAssetStorage* storage(std::uint32_t type_id) noexcept
        {
            if(!storages_map_.hasType(type_id))
            {
                return {};
            }

            return static_cast<IAssetStorage*>(storages_map_.erased(type_id));
        }

        template<typename AssetT>
        const AssetStorage<AssetT>& storage() const noexcept
        {
            return storage<AssetT>();
        }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return storages_map_.get_allocator();
        }

        [[nodiscard]] static std::error_code create(AssetServerCreateInfo create_info, AssetServer& output) noexcept;

        AssetServer& operator=(const AssetServer&) = delete;
        AssetServer& operator=(AssetServer&& other) noexcept;

    private:
        struct BakeContext final
        {
            core::Vector<std::byte> output_bytes;
            core::UnorderedSet<AssetId> baked_assets;
            std::filesystem::path archive_path;
            std::int32_t compression_level {};
        };

        [[nodiscard]] std::error_code postTracked(std::move_only_function<void()> task) noexcept;
        std::error_code bakeImpl(BakeContext& context, const AnyHandle& handle);

        static void finishPosted(const core::SharedPtr<PostedJobs>& posted) noexcept;

        mutable core::TypeMap storages_map_;
        AssetCatalog catalog_;
        size_t page_size_ {};

        const core::LogCallback* debug_output_ {};
        core::ThreadPool* thread_pool_ {};
        core::SharedPtr<PostedJobs> posted_ {};
    };
}
