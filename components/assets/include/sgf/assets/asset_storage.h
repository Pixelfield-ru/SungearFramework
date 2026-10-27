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

        virtual AssetSlotControl* slotControl(AssetId asset_id) = 0;
    };

    template<typename AssetT>
    struct AssetStorage final : IAssetStorage
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        AssetStorage(std::uint32_t page_size) :
            pages_({ Page(page_size) }),
            page_size_(page_size)
        { }
        AssetStorage(std::uint32_t page_size, const allocator_type& alloc) :
            pages_({ Page(page_size, alloc) }, alloc),
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

        [[nodiscard]] AssetSlot<AssetT>* findSlot(AssetId asset_id) noexcept
        {
            const auto it = slots_mapping_.find(asset_id);
            if(it == slots_mapping_.end()) return {};

            return &pages_[it->second.page_id].slots[it->second.slot_id];
        }

        [[nodiscard]] const AssetSlot<AssetT>* findSlot(AssetId asset_id) const noexcept
        {
            return const_cast<AssetStorage*>(this)->findSlot(asset_id);
        }

        AssetSlot<AssetT>* acquireSlot(AssetId asset_id) noexcept
        {
            auto& free_page = freePage();

            const auto index = free_page.free_slots.back();
            free_page.free_slots_.pop_back();

            PageSlotId slot_id;
            slot_id.page_id = free_page.id;
            slot_id.slot_id = index;
            slots_mapping_[asset_id] = slot_id;

            auto& slot = free_page.slots[index];
            slot.id_ = asset_id;

            return &slot;
        }

        void releaseSlot(const AssetSlot<AssetT>* slot) noexcept
        {
            const auto it = slots_mapping_.find(slot->id_);
            if(it == slots_mapping_.end()) return;

            const auto& slot_id = it->second;

            slots_mapping_.erase(it->second);
            pages_[slot_id.page_id].free_slots[slot_id.slot_id].push_back(it->second);
        }

        AssetSlotControl* slotControl(AssetId asset_id) noexcept override
        {
            return findSlot(asset_id);
        }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return pages_.get_allocator();
        }

        AssetStorage& operator=(const AssetStorage&) = delete;
        AssetStorage& operator=(AssetStorage&&) noexcept = default;

    private:
        struct PageSlotId final
        {
            std::uint32_t page_id {};
            std::uint32_t slot_id {};
        };

        struct Page final
        {
            using allocator_type = std::pmr::polymorphic_allocator<>;

            core::Vector<AssetSlot<AssetT>> slots;
            core::Vector<std::uint32_t> free_slots;
            std::uint32_t id {};

            Page() = default;
            Page(std::uint32_t size) :
                slots(size),
                free_slots(size)
            {
                for(std::uint32_t i = 0; i < size; ++i)
                {
                    free_slots[i] = i;
                }
            }
            Page(std::uint32_t size, const allocator_type& alloc) :
                slots(size, alloc),
                free_slots(size, alloc)
            {
                for(std::uint32_t i = 0; i < size; ++i)
                {
                    free_slots[i] = i;
                }
            }
            Page(const Page&) = default;
            Page(const Page& other, const allocator_type& alloc) :
                slots(other.slots, alloc),
                free_slots(other.free_slots, alloc)
            { }
            Page(Page&&) = default;
            Page(Page&& other, const allocator_type& alloc) :
                slots(std::move(other.slots), alloc),
                free_slots(std::move(other.free_slots), alloc)
            { }

            [[nodiscard]] allocator_type get_allocator() const noexcept
            {
                return slots.get_allocator();
            }

            Page& operator=(const Page&) = default;
            Page& operator=(Page&&) = default;
        };

        /// Every page contains page_size_ slots.
        /// At the beginning we have one page and we create more pages if there is no more free space in the first page.
        /// Vector because slots are valid even after reallocation.
        core::Vector<Page> pages_;
        core::UnorderedMap<AssetId, PageSlotId> slots_mapping_;
        core::UniquePtr<IAssetLoader<AssetT>> loader_;
        std::uint32_t page_size_ {};

        Page& freePage() noexcept
        {
            for(auto& page : pages_)
            {
                if(!page.free_slots_.empty()) return page;
            }

            Page page(page_size_, get_allocator());
            page.id = pages_.size();
            pages_.push_back(std::move(page));

            return *pages_.rbegin();
        }
    };
}