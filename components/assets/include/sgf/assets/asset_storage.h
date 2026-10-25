//
// Created by stuka on 25.10.2026.
//

#pragma once

#include "asset_loader.h"
#include "asset_slot.h"
#include "sgf/core/memory.h"

namespace sgf::assets
{
    struct IAssetStorage
    {
        virtual ~IAssetStorage() = default;
    };

    template<typename AssetT>
    struct AssetStorage final : IAssetStorage
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        AssetStorage(size_t page_size) :
            pages_({ Page { .assets = page_size, .free_slots_count = page_size } }),
            page_size_(page_size)
        { }
        AssetStorage(size_t page_size, const allocator_type& alloc) :
            pages_({ Page { .assets = core::Vector<AssetSlot<AssetT>>(page_size, alloc), .free_slots_count = page_size } }, alloc),
            page_size_(page_size)
        { }
        AssetStorage(const AssetStorage&) = delete;
        AssetStorage(AssetStorage&&) = default;
        AssetStorage(AssetStorage&& other, const allocator_type& alloc) noexcept :
            pages_(std::move(other.pages_), alloc),
            loader_(std::move(other.loader_)),
            page_size_(other.page_size_)
        { }

        void setLoader(core::UniquePtr<IAssetLoader<AssetT>> loader) noexcept
        {
            loader_ = std::move(loader);
        }

        IAssetLoader<AssetT>* loader() noexcept
        {
            return loader_.get();
        }

        const IAssetLoader<AssetT>* loader() const noexcept
        {
            return loader_.get();
        }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return pages_.get_allocator();
        }

        AssetStorage& operator=(const AssetStorage&) = delete;
        AssetStorage& operator=(AssetStorage&&) noexcept = default;

    private:
        struct Page final
        {
            core::Vector<AssetSlot<AssetT>> assets;
            size_t free_slots_count {};
        };

        /// Every page contains slots_count slots.
        /// At the beginning we have one page and we create more pages if there is no more free space in the first page.
        core::List<Page> pages_;
        core::UniquePtr<IAssetLoader<AssetT>> loader_;
        size_t page_size_ {};

        Page& freePage() noexcept
        {
            for(auto& page : pages_)
            {
                if(page.free_slots_count != 0) return page;
            }

            Page page;
            page.assets = core::Vector<AssetSlot<AssetT>>(page_size_, get_allocator());
            page.free_slots_count = page_size_;
            pages_.push_back(std::move(page));

            return *pages_.rbegin();
        }
    };
}