//
// Created by stuka on 25.10.2026.
//

#pragma once

#include "asset_handle.h"
#include "sgf/core/memory.h"
#include "sgf/core/structures/type_map.h"

#include "asset_storage.h"
#include "errors.h"

namespace sgf::assets
{
    // todo: use log callback
    struct AssetsServer final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        AssetsServer(size_t page_size) :
            page_size_(page_size)
        { }
        AssetsServer(size_t page_size, const allocator_type& alloc) :
            storages_map_(alloc),
            page_size_(page_size)
        { }
        AssetsServer(const AssetsServer&) = delete;
        AssetsServer(AssetsServer&&) = default;
        AssetsServer(AssetsServer&& other, const allocator_type& alloc) noexcept :
            storages_map_(std::move(other.storages_map_), alloc),
            page_size_(other.page_size_)
        { }

        template<typename AssetT>
        std::expected<Handle<AssetT>, std::error_code> load(const std::filesystem::path& path) noexcept
        {
            auto& st = storage<AssetT>();
            auto* loader = st.loader();

            if(!loader)
            {
                return std::unexpected(Error::kLoaderNotSpecified);
            }

            // todo: check if slot exists and return it or load asset.
            return nullptr;
            /*auto asset = loader->loadSource(path);
            if(!asset)
            {
                return std::unexpected(asset.error());
            }*/
        }

        template<typename AssetT>
        Handle<AssetT> copy(const Handle<AssetT>& source) noexcept
        {
            // todo: make copy
            return nullptr;
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

        template<typename AssetT>
        const AssetStorage<AssetT>& storage() const noexcept
        {
            return storage<AssetT>();
        }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return storages_map_.get_allocator();
        }

        AssetsServer& operator=(const AssetsServer&) = delete;
        AssetsServer& operator=(AssetsServer&&) noexcept = default;

    private:
        mutable core::TypeMap storages_map_;
        size_t page_size_ {};
    };
}