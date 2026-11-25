//
// Created by stuka on 25.10.2026.
//

#pragma once

#include "sgf/core/memory.h"
#include "sgf/core/execution/domain.h"
#include "sgf/core/structures/type_map.h"
#include "sgf/core/utility/file.h"
#include "sgf/core/log.h"
#include "sgf/core/execution/thread_pool.h"
#include "sgf/core/utility/type_index.h"
#include "sgf/core/utility/encoding.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

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

    // todo: maybe use std::future
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
         * Copy an engine resource tree into engine/resources under the mounted catalog.
         * Call after mount. Catalog rows are left untouched.
         * An existing file stays in place when skip_existing is true, and is replaced when it is false.
         * Files copied before a failure stay on disk. A later call with skip_existing continues from them.
         * @param source Directory whose contents are copied.
         * @param skip_existing Leave files that are already in the catalog tree.
         * @return Error.
         */
        std::error_code importEngineResources(const std::filesystem::path& source, bool skip_existing) const noexcept;

        /**
         * Load asset by path.
         * Do not call while a caller still uses the pointer from get() for this asset.
         * kAny returns while the slot may still be kLoading. Do not call wait() on that handle from a pool thread.
         * kCurrent reads on this thread and returns after the slot is kLoaded or the read has failed.
         * @tparam AssetT Type of asset.
         * @param path Path of asset.
         * @param domain Where the file is read.
         * @return Handle to the asset or an error.
         */
        template<typename AssetT>
        std::expected<Handle<AssetT>, std::error_code> load(
            const std::filesystem::path& path,
            core::ExecutionDomain domain = core::ExecutionDomain::kAny) noexcept
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

            // the catalog row and its slot claim stay together, so remove cannot drop this id in between
            std::uint32_t file_version = 1;
            AssetId file_id {};
            ClaimedSlot claimed;
            {
                std::unique_lock lock(identity_mutex_);

                const auto acquired = catalog_.ensureFile(*key, type);
                file_id = acquired.file.id;
                file_version = acquired.file.version;
                if(acquired.created)
                {
                    if(const auto err = catalog_.save()) return std::unexpected(err);

                    if(const auto err = loader->writeMeta(actual_path, file_id)) return std::unexpected(err);
                }
                else
                {
                    if(const auto err = loader->loadMeta(actual_path, file_id)) return std::unexpected(err);
                }

                claimed = storage.claimSlot(file_id);
            }
            auto* slot = static_cast<AssetSlot<AssetT>*>(claimed.slot);

            Handle<AssetT> handle;
            handle.slot_ = slot;
            handle.asset_id_ = file_id;
            handle.type_id_ = type;

            // the slot is already kLoading by other caller
            if(!claimed.load) return handle;

            // first read of this slot. set version from the catalog
            slot->version = file_version;

            // capture the handle to increase lifetime of the slot
            if(const auto err = postTracked([this, loader, actual_path, handle] {
                auto* slot = static_cast<AssetSlot<AssetT>*>(handle.slot_);

                if(const auto err = loader->loadSource(actual_path, *slot->value, *this))
                {
                    slot->error = err;
                    slot->setState(AssetState::kFailed);
                    return;
                }

                slot->error.clear();
                slot->setState(AssetState::kLoaded);
            }, domain))
            {
                slot->error = err;
                slot->setState(AssetState::kFailed);
                return std::unexpected(err);
            }

            return handle;
        }

        /**
         * Find the catalog row for this path or create one, then fill the asset on this thread.
         * The path is a catalog key. The file does not need to exist, and no meta file is written.
         * A row that is already loading or loaded returns that handle and does not call fill.
         * Do not call wait() on the returned handle from a pool thread while another caller still fills it.
         * @tparam AssetT Type of asset.
         * @param path Catalog key of the asset.
         * @param fill Writes the asset. Runs only when this call owns the slot.
         * @return Handle to the asset or an error.
         */
        template<typename AssetT, typename FillFunc>
        requires std::is_invocable_r_v<std::error_code, FillFunc, AssetT&>
        std::expected<Handle<AssetT>, std::error_code> ensure(const std::filesystem::path& path, FillFunc&& fill)
        {
            const auto root = catalog_.root();
            const auto key = core::normalizedRelativePath(root / path, root);
            if(!key) return std::unexpected(key.error());

            const auto type = core::registerStableType<AssetStorage<AssetT>>();
            auto& storage = this->storage<AssetT>();

            // the catalog row and its slot claim stay together, so remove cannot drop this id in between
            std::uint32_t file_version = 1;
            AssetId file_id {};
            ClaimedSlot claimed;
            {
                std::unique_lock lock(identity_mutex_);

                const auto acquired = catalog_.ensureFile(*key, type);
                file_id = acquired.file.id;
                file_version = acquired.file.version;
                if(acquired.created)
                {
                    if(const auto err = catalog_.save()) return std::unexpected(err);
                }
                else if(acquired.file.type_id != static_cast<std::int64_t>(type))
                {
                    core::LogCallback::error(debug_output_,
                                             "Catalog path is already another asset type. Path: '{}', asset type: '{}'",
                                             core::pathToUtf8(path), core::typeName<AssetT>());
                    return std::unexpected(std::make_error_code(std::errc::invalid_argument));
                }

                claimed = storage.claimSlot(file_id);
            }
            auto* slot = static_cast<AssetSlot<AssetT>*>(claimed.slot);

            Handle<AssetT> handle;
            handle.slot_ = slot;
            handle.asset_id_ = file_id;
            handle.type_id_ = type;

            // the slot is already kLoaded, or another caller is filling it
            if(!claimed.load) return handle;

            // set slot version from catalog
            slot->version = file_version;

            if(const auto err = std::invoke(std::forward<FillFunc>(fill), *slot->value))
            {
                slot->error = err;
                slot->setState(AssetState::kFailed);
                return std::unexpected(err);
            }

            slot->error.clear();
            slot->setState(AssetState::kLoaded);
            return handle;
        }

        /**
         * Load asset that exists in catalog.
         * Asset loads only if it was not loaded earlier and was saved by path or in an archive.
         * Do not call while a caller still uses the pointer from get() for this asset.
         * kAny returns while the slot may still be kLoading. Do not call wait() on that handle from a pool thread.
         * kCurrent reads on this thread and returns after the slot is kLoaded or the read has failed.
         * @param asset_id Asset ID that needs to be loaded.
         * @param type_id Type ID of asset.
         * @param domain Where the asset is read.
         * @return Handle to the asset.
         */
        std::expected<AnyHandle, std::error_code> load(
            AssetId asset_id, std::uint32_t type_id,
            core::ExecutionDomain domain = core::ExecutionDomain::kAny) noexcept;

        /**
         * Reload asset by path.
         * Do not call while a caller still uses the pointer from get().
         * kAny returns while the reload may still be running. Do not call wait() on that handle from a pool thread.
         * kCurrent reads on this thread and returns after the slot is kLoaded or the read has failed.
         * @param handle Asset that needs to be reloaded.
         * @param domain Where the file is read.
         * @return Error.
         */
        std::error_code reload(
            const AnyHandle& handle,
            core::ExecutionDomain domain = core::ExecutionDomain::kAny) noexcept;

        /**
         * Remove the catalog row for this id.
         * The id is not reused. The source file and the baked bytes stay in place.
         * A slot that is still in memory keeps the row and returns kInUse.
         * @param asset_id Asset to remove.
         * @return Error.
         */
        std::error_code remove(AssetId asset_id);

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

            // the new row and its slot stay together, so remove cannot drop the id before the slot exists
            AssetSlot<AssetT>* slot = nullptr;
            {
                std::unique_lock lock(identity_mutex_);
                const auto new_file = catalog_.createFile(type_id);
                slot = storage.createSlot(new_file.id);
            }
            // creating new asset using copy constructor with allocator
            // or without allocator if T does not have such constructor
            auto new_asset = std::make_obj_using_allocator<AssetT>(get_allocator(), *asset);
            slot->insert(std::move(new_asset));
            // the source is loaded so we put kLoaded state in the copy
            slot->setState(AssetState::kLoaded);

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
        /// kCurrent runs the task before this call returns. kAny posts it to the pool.
        [[nodiscard]] std::error_code postTracked(
            std::move_only_function<void()> task, core::ExecutionDomain domain) noexcept;

        static void finishPosted(const core::SharedPtr<PostedJobs>& posted) noexcept;

        mutable core::TypeMap storages_map_;
        mutable std::shared_mutex storages_mutex_;
        /// Catalog identity and the slot claim. remove cannot drop a row that load or ensure is about to claim.
        std::mutex identity_mutex_;
        AssetCatalog catalog_;
        size_t page_size_ {};

        const core::LogCallback* debug_output_ {};
        core::ThreadPool* thread_pool_ {};
        core::SharedPtr<PostedJobs> posted_ {};
    };
}
