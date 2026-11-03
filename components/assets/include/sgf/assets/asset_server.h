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
         * Mount catalog in directory.
         * @param path Path to mount catalog.
         */
        std::error_code mount(std::filesystem::path path) noexcept;

        template<typename AssetT, typename... ArgsT>
        std::expected<Handle<AssetT>, std::error_code> load(const std::filesystem::path& path, ArgsT&&... args) noexcept
        {
            const auto key = core::normalizedRelativePath(catalog_.path / path, catalog_.path);
            if(!key) return std::unexpected(key.error());

            const auto actual_path = catalog_.path / *key;

            const auto type = core::typeIndex<AssetT>();
            auto& storage = this->storage<AssetT>();
            // trying to find existing asset
            auto* file = catalog_.find(*key);

            // if file was not found then creating new file in catalog
            if(!file)
            {
                // acquire new id for new asset
                file = &catalog_.acquireFile(*key);
                file->type_id = type;

                // saving catalog to file
                if(const auto err = catalog_.save()) return std::unexpected(err);

                // writing meta of the file near the file
                if(const auto err = storage.loader()->writeMeta(actual_path, file->id)) return std::unexpected(err);
            }

            // if slot is already exists then leave
            if(auto* existing = storage.findSlot(file->id)) return Handle<AssetT>(existing);

            // get new slot for asset
            auto* slot = storage.acquireSlot(file->id);

            auto asset = std::make_obj_using_allocator<AssetT>(get_allocator(), std::forward<ArgsT>(args)...);
            // todo: load async
            if(const auto err = storage.loader()->loadSource(actual_path, asset))
            {
                slot->state = AssetState::kFailed;
                return std::unexpected(asset.error());
            }

            slot->insert(std::move(asset));
            return Handle<AssetT>(slot);
        }

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

            Handle<AssetT> handle(slot);

            return handle;
        }

        AnyHandle find(AssetId asset_id, std::uint32_t type_index) noexcept
        {
            auto* st = storage(type_index);
            if(!st) return {};

            auto* control = st->slotControl(asset_id);
            if(!control) return {};

            AnyHandle handle(control, type_index);

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

        IAssetStorage* storage(std::uint32_t type_index) noexcept
        {
            if(!storages_map_.hasType(type_index))
            {
                return {};
            }

            return static_cast<IAssetStorage*>(storages_map_.erased(type_index));
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