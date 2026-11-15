//
// Created by stuka on 25.10.2026.
//

#pragma once

#include "sgf/core/memory.h"
#include "sgf/core/structures/type_map.h"
#include "sgf/core/utility/file.h"
#include "sgf/core/log.h"
#include "sgf/core/thread_pool.h"
#include "sgf/core/utility/type_index.h"
#include "sgf/core/utility/encoding.h"

#include <atomic>
#include <functional>
#include <shared_mutex>

#include "storage.h"
#include "catalog.h"
#include "handle.h"
#include "errors.h"

namespace sgf::assets
{
    struct AssetServerCreateInfo
    {
        serialization::Serializer serializer;
        size_t page_size = 10;
        const core::LogCallback* debug_output {};
        core::ThreadPool* thread_pool {};
    };

    // todo: use stable type index
    struct AssetServer final
    {
        friend class Baker;

        using allocator_type = std::pmr::polymorphic_allocator<>;

    private:
        struct PostedJobs
        {
            std::atomic<std::size_t> count {};
        };

    public:
        explicit AssetServer(const allocator_type& alloc = {}) :
            storages_map_(alloc),
            catalog_(alloc),
            posted_(core::makeShared<PostedJobs>(alloc))
        { }
        AssetServer(const AssetServer&) = delete;
        AssetServer(AssetServer&& other) noexcept;
        AssetServer(AssetServer&& other, const allocator_type& alloc) noexcept;
        ~AssetServer();

        /**
         * Block until every load posted to the pool has finished.
         * The caller must not be a pool thread, and must not call load while waiting.
         */
        void waitPosted() noexcept;

        /**
         * Mount the catalog in a directory.
         * Do not call while a load is in progress. A loaded asset may already live outside the new root.
         * @param path Path to mount the catalog.
         */
        std::error_code mount(std::filesystem::path path) noexcept;

        /**
         * Load asset by path.
         * Do not call while a caller still uses the pointer from get() for this asset.
         * Do not use the asset until the load has finished.
         * @tparam AssetT Type of asset.
         * @param path Path of asset.
         * @return Handle to the asset or an error.
         */
        template<typename AssetT>
        std::expected<Handle<AssetT>, std::error_code> load(const std::filesystem::path& path) noexcept
        {
            const auto root = catalog_.root();
            const auto key = core::normalizedRelativePath(root / path, root);
            if(!key) return std::unexpected(key.error());

            const auto actual_path = root / *key;

            if(!std::filesystem::exists(actual_path))
            {
                core::LogCallback::error(debug_output_, "Cannot find file. Path: '{}', asset type: '{}'",
                                         core::pathToUtf8(path), core::typeName<AssetT>());
                return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
            }

            const auto type = core::registerStableType<AssetStorage<AssetT>>();
            auto& storage = this->storage<AssetT>();

            auto* loader = storage.loader();
            if(!loader)
            {
                core::LogCallback::error(debug_output_, "Cannot find loader. Path: '{}', asset type: '{}'",
                                         core::pathToUtf8(path), core::typeName<AssetT>());
                return std::unexpected(Error::kLoaderNotSpecified);
            }

            // one catalog lock covers the find-or-create, so two loads of the same path share one row
            const auto acquired = catalog_.acquireFile(*key);
            const auto file_id = acquired.file.id;
            // if new file was created in the catalog
            if(acquired.created)
            {
                // set the type id for new file
                catalog_.setTypeId(file_id, type);

                // saving catalog to file
                if(const auto err = catalog_.save()) return std::unexpected(err);

                // writing meta of the file near the file
                if(const auto err = loader->writeMeta(actual_path, file_id)) return std::unexpected(err);
            }
            else
            {
                // load meta of the file near the file
                if(const auto err = loader->loadMeta(actual_path, file_id)) return std::unexpected(err);
            }

            const auto claimed = storage.claimSlot(file_id);
            auto* slot = static_cast<AssetSlot<AssetT>*>(claimed.slot);

            Handle<AssetT> handle;
            handle.slot_ = slot;
            handle.asset_id_ = file_id;
            handle.type_id_ = type;

            // the slot is already kLoading by other caller
            if(!claimed.load) return handle;

            // first read of this slot. set version from the catalog
            slot->version = acquired.file.version;

            // capture the handle to increase lifetime of the slot
            if(const auto err = postTracked([loader, actual_path, handle] {
                auto* slot = static_cast<AssetSlot<AssetT>*>(handle.slot_);

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

            return handle;
        }

        /**
         * Load asset that exists in catalog.
         * Asset loads only if it was not loaded earlier and was saved by path or in an archive.
         * Do not call while a caller still uses the pointer from get() for this asset.
         * Do not use the asset until the load has finished.
         * @param asset_id Asset ID that needs to be loaded.
         * @param type_id Type ID of asset.
         * @return Handle to the asset.
         */
        std::expected<AnyHandle, std::error_code> load(AssetId asset_id, std::uint32_t type_id) noexcept;

        /**
         * Reload asset by path.
         * Do not call while a caller still uses the pointer from get().
         * Do not use the asset until this reload has finished.
         * @param handle Asset that needs to be reloaded.
         * @return Error.
         */
        std::error_code reload(const AnyHandle& handle) noexcept;

        std::error_code bake(const AnyHandle& handle, const std::filesystem::path& archive, std::int32_t compression_level);

        template<typename AssetT>
        Handle<AssetT> find(AssetId asset_id) noexcept
        {
            auto& st = storage<AssetT>();
            auto* slot = st.retainSlot(asset_id);
            if(!slot) return {};

            Handle<AssetT> handle;
            handle.slot_ = slot;
            handle.asset_id_ = asset_id;
            handle.type_id_ = core::registerStableType<AssetStorage<AssetT>>();

            return handle;
        }

        AnyHandle find(AssetId asset_id, std::uint32_t type_id) noexcept
        {
            auto* st = storage(core::typeIndexFromStableHash(type_id));
            if(!st) return {};

            auto* control = st->retainErasedSlot(asset_id);
            if(!control) return {};

            AnyHandle handle;
            handle.slot_ = control;
            handle.asset_id_ = asset_id;
            handle.type_id_ = type_id;

            return handle;
        }

        template<typename AssetT>
        requires(std::is_copy_constructible_v<AssetT>)
        Handle<AssetT> copy(Handle<AssetT> source) noexcept
        {
            const auto* asset = source.get();
            // kLoading and kFailed have no finished value to copy
            if(!asset) return {};

            auto& storage = this->storage<AssetT>();
            static const auto type_id = core::registerStableType<AssetStorage<AssetT>>();

            const auto new_file = catalog_.acquireFile();
            catalog_.setTypeId(new_file.id, type_id);
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
        AssetStorage<AssetT>& storage()
        {
            using Storage = AssetStorage<AssetT>;

            core::registerStableType<Storage>();

            // hasType and emplace stay one critical section: two callers must not both insert Storage
            std::unique_lock lock(storages_mutex_);
            if(storages_map_.hasType<Storage>())
            {
                return storages_map_.write<Storage>();
            }

            auto& storage = storages_map_.emplace<Storage>(page_size_, storages_map_.get_allocator());
            return storage;
        }

        IAssetStorage* storage(std::uint32_t type_id)
        {
            std::shared_lock lock(storages_mutex_);
            if(!storages_map_.hasType(type_id))
            {
                return {};
            }

            // the storage object stays on the heap, so the pointer outlives this lock
            return static_cast<IAssetStorage*>(storages_map_.erased(type_id));
        }

        template<typename AssetT>
        const AssetStorage<AssetT>& storage() const
        {
            return const_cast<AssetServer*>(this)->storage<AssetT>();
        }

        [[nodiscard]] allocator_type get_allocator() const
        {
            std::shared_lock lock(storages_mutex_);
            return storages_map_.get_allocator();
        }

        [[nodiscard]] static std::error_code create(AssetServerCreateInfo create_info, AssetServer& output) noexcept;

        AssetServer& operator=(const AssetServer&) = delete;
        AssetServer& operator=(AssetServer&& other) noexcept;

    private:
        [[nodiscard]] std::error_code postTracked(std::move_only_function<void()> task) noexcept;

        static void finishPosted(const core::SharedPtr<PostedJobs>& posted) noexcept;

        mutable core::TypeMap storages_map_;
        mutable std::shared_mutex storages_mutex_;
        AssetCatalog catalog_;
        size_t page_size_ {};

        const core::LogCallback* debug_output_ {};
        core::ThreadPool* thread_pool_ {};
        core::SharedPtr<PostedJobs> posted_ {};
    };
}
