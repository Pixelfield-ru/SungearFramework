//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <shared_mutex>

#include "loader.h"
#include "slot.h"
#include "sgf/core/memory.h"

namespace sgf::assets
{
    struct IAssetStorage
    {
        virtual ~IAssetStorage() = default;

        [[nodiscard]] virtual AssetSlotControl* slotControl(AssetId asset_id) = 0;
        /// Finds a slot and adds one strong reference. Empty when the slot is missing or already released.
        [[nodiscard]] virtual AssetSlotControl* retainErasedSlot(AssetId asset_id) = 0;
        [[nodiscard]] virtual AssetSlotControl* acquireErasedSlot(AssetId asset_id) = 0;
        virtual void releaseSlot(AssetSlotControl* slot) = 0;
        [[nodiscard]] virtual IErasedAssetLoader* erasedLoader() = 0;
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
        AssetStorage(AssetStorage&& other) noexcept :
            pages_(std::move(other.pages_)),
            slots_mapping_(std::move(other.slots_mapping_)),
            loader_(std::move(other.loader_)),
            page_size_(other.page_size_)
        {
            rebindStorage();
        }
        AssetStorage(AssetStorage&& other, const allocator_type& alloc) noexcept :
            pages_(std::move(other.pages_), alloc),
            slots_mapping_(std::move(other.slots_mapping_), alloc),
            loader_(std::move(other.loader_)),
            page_size_(other.page_size_)
        {
            rebindStorage();
        }

        void setLoader(core::UniquePtr<IAssetLoader<AssetT>> loader) noexcept
        {
            loader_ = std::move(loader);
        }

        [[nodiscard]] IAssetLoader<AssetT>* loader() noexcept
        {
            return loader_.get();
        }

        [[nodiscard]] IErasedAssetLoader* erasedLoader() noexcept override
        {
            return loader_.get();
        }

        [[nodiscard]] const IAssetLoader<AssetT>* loader() const noexcept
        {
            return loader_.get();
        }

        [[nodiscard]] AssetSlot<AssetT>* findSlot(AssetId asset_id) noexcept
        {
            std::shared_lock lock(mutex_);
            const auto it = slots_mapping_.find(asset_id);
            if(it == slots_mapping_.end()) return {};

            return &pages_[it->second.page_id].slots[it->second.slot_id];
        }

        [[nodiscard]] const AssetSlot<AssetT>* findSlot(AssetId asset_id) const noexcept
        {
            std::shared_lock lock(mutex_);
            const auto it = slots_mapping_.find(asset_id);
            if(it == slots_mapping_.end()) return {};

            return &pages_[it->second.page_id].slots[it->second.slot_id];
        }

        [[nodiscard]] AssetSlot<AssetT>* acquireSlot(AssetId asset_id) noexcept
        {
            std::unique_lock lock(mutex_);
            auto& free_page = freePage();

            const auto index = free_page.free_slots.back();
            free_page.free_slots.pop_back();

            PageSlotId slot_id;
            slot_id.page_id = free_page.id;
            slot_id.slot_id = index;
            slots_mapping_[asset_id] = slot_id;

            auto& slot = free_page.slots[index];
            slot.id = asset_id;
            slot.storage = this;
            slot.insert(std::make_obj_using_allocator<AssetT>(get_allocator()));

            return &slot;
        }

        /// Finds a slot and adds one strong reference. Empty when the slot is missing or already released.
        [[nodiscard]] AssetSlot<AssetT>* retainSlot(AssetId asset_id) noexcept
        {
            AssetSlot<AssetT>* slot = nullptr;
            std::uint32_t generation = 0;
            {
                std::shared_lock lock(mutex_);
                const auto it = slots_mapping_.find(asset_id);
                if(it == slots_mapping_.end()) return {};

                slot = &pages_[it->second.page_id].slots[it->second.slot_id];
                generation = slot->generation.load(std::memory_order_acquire);
            }

            if(!slot->tryAddStrongRef(generation)) return {};
            return slot;
        }

        [[nodiscard]] AssetSlotControl* retainErasedSlot(AssetId asset_id) noexcept override
        {
            return retainSlot(asset_id);
        }

        [[nodiscard]] AssetSlotControl* acquireErasedSlot(AssetId asset_id) noexcept override
        {
            return acquireSlot(asset_id);
        }

        void releaseSlot(AssetSlotControl* slot) noexcept override
        {
            std::uint32_t page_id = 0;
            std::uint32_t slot_index = 0;
            {
                std::unique_lock lock(mutex_);
                const auto it = slots_mapping_.find(slot->id.load(std::memory_order_acquire));
                if(it == slots_mapping_.end()) return;

                const auto slot_id = it->second;
                if(slot_id.page_id >= pages_.size()) return;

                auto& page = pages_[slot_id.page_id];
                if(slot_id.slot_id >= page.slots.size()) return;
                if(static_cast<AssetSlotControl*>(&page.slots[slot_id.slot_id]) != slot) return;

                page_id = slot_id.page_id;
                slot_index = slot_id.slot_id;
                slots_mapping_.erase(it);
            }

            // clear without the lock: the asset destructor can release or acquire another slot of this storage
            slot->clear();

            std::unique_lock lock(mutex_);
            pages_[page_id].free_slots.push_back(slot_index);
        }

        [[nodiscard]] AssetSlotControl* slotControl(AssetId asset_id) noexcept override
        {
            return findSlot(asset_id);
        }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return pages_.get_allocator();
        }

        AssetStorage& operator=(const AssetStorage&) = delete;
        AssetStorage& operator=(AssetStorage&& other) noexcept
        {
            if(this == std::addressof(other)) return *this;

            pages_ = std::move(other.pages_);
            slots_mapping_ = std::move(other.slots_mapping_);
            loader_ = std::move(other.loader_);
            page_size_ = other.page_size_;
            rebindStorage();

            return *this;
        }

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
        /// Shared for find. Exclusive for acquire and release.
        mutable std::shared_mutex mutex_;

        void rebindStorage() noexcept
        {
            for(auto& page : pages_)
            {
                for(auto& slot : page.slots)
                {
                    if(slot.storage) slot.storage = this;
                }
            }
        }

        /// Caller holds the exclusive lock.
        Page& freePage() noexcept
        {
            for(auto& page : pages_)
            {
                if(!page.free_slots.empty()) return page;
            }

            Page page(page_size_, get_allocator());
            page.id = pages_.size();
            pages_.push_back(std::move(page));

            return *pages_.rbegin();
        }
    };
}