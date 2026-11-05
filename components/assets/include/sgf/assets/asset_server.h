//
// Created by stuka on 25.10.2026.
//

#pragma once

#include "asset_handle.h"
#include "sgf/core/memory.h"
#include "sgf/core/structures/type_map.h"
#include "sgf/core/utility/file.h"

#include "asset_storage.h"
#include "asset_catalog.h"
#include "errors.h"

namespace sgf::assets
{
    // todo: use log callback
    // todo: use stable type index
    // todo: accept threadpool
    // todo: use AssetServerCreateInfo
    struct AssetServer final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        AssetServer(serialization::Serializer serializer, size_t page_size) :
            catalog_(std::move(serializer)),
            page_size_(page_size)
        { }
        AssetServer(serialization::Serializer serializer, size_t page_size, const allocator_type& alloc) :
            storages_map_(alloc),
            catalog_(std::move(serializer)),
            page_size_(page_size)
        { }
        AssetServer(const AssetServer&) = delete;
        AssetServer(AssetServer&&) = default;
        AssetServer(AssetServer&& other, const allocator_type& alloc) noexcept :
            storages_map_(std::move(other.storages_map_), alloc),
            catalog_(std::move(other.catalog_), alloc),
            page_size_(other.page_size_)
        { }

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

            // todo: load async
            if(const auto err = loader->loadSource(actual_path, *slot->value))
            {
                slot->state = AssetState::kFailed;
                return std::unexpected(err);
            }

            slot->state = AssetState::kLoaded;

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

        template<typename AssetT>
        std::error_code bake(const Handle<AssetT>& handle/*, Archive& archive*/)
        {
            return {};
        }

        std::error_code bake(const AnyHandle& handle/*, Archive& archive*/)
        {
            return {};
        }

        template<typename AssetT>
        Handle<AssetT> find(AssetId asset_id) noexcept
        {
            auto& st = storage<AssetT>();
            auto* slot = st.findSlot(asset_id);

            Handle<AssetT> handle(slot, core::typeIndex<AssetT>());

            return handle;
        }

        AnyHandle find(AssetId asset_id, std::uint32_t type_id) noexcept
        {
            auto* st = storage(type_id);
            if(!st) return {};

            auto* control = st->slotControl(asset_id);
            if(!control) return {};

            AnyHandle handle(control, type_id);

            return handle;
        }

        template<typename AssetT>
        requires(std::is_copy_constructible_v<AssetT>)
        Handle<AssetT> copy(const Handle<AssetT>& source) noexcept
        {
            auto& storage = this->storage<AssetT>();

            const auto new_id = catalog_.acquireId();
            // get new slot for asset
            auto* slot = storage.acquireSlot(new_id);
            // creating new asset using copy constructor with allocator
            // or without allocator if T does not have such constructor
            auto new_asset = std::make_obj_using_allocator<AssetT>(get_allocator(), *source);
            slot->insert(std::move(new_asset));

            return Handle<AssetT>(slot);
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

        AssetServer& operator=(const AssetServer&) = delete;
        AssetServer& operator=(AssetServer&&) noexcept = default;

    private:
        mutable core::TypeMap storages_map_;
        AssetCatalog catalog_;
        size_t page_size_ {};
    };
}