//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <cstddef>

#include "asset_slot.h"

namespace sgf::assets
{
    template<typename>
    struct WeakHandle;

    template<typename AssetT>
    struct Handle final
    {
        template<typename>
        friend struct WeakHandle;

        Handle() noexcept = default;
        ~Handle() noexcept
        {
            if(slot_)
            {
                slot_->decrementRefCounter();
            }
        }
        Handle(std::nullptr_t) noexcept :
            slot_(nullptr)
        {

        }
        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT>)
        Handle(const Handle<OtherAssetT>& other) noexcept
        {
            doCopy(other);
        }
        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT>)
        Handle(Handle<OtherAssetT>&& other) noexcept
        {
            doMove(other);
        }

        [[nodiscard]] const AssetT* get() const noexcept
        {
            return valid() ? &(*slot_->value) : nullptr;
        }

        [[nodiscard]] AssetT* get() noexcept
        {
            return valid() ? &(*slot_->value) : nullptr;
        }

        [[nodiscard]] WeakHandle<AssetT> weak() const noexcept;

        operator bool() const noexcept
        {
            return valid();
        }

        AssetT* operator->() const noexcept
        {
            return valid() ? &(*slot_->value) : nullptr;
        }

        AssetT* operator->() noexcept
        {
            return valid() ? &(*slot_->value) : nullptr;
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT>)
        Handle& operator=(const Handle<OtherAssetT>& other) noexcept
        {
            if(std::addressof(other) == this) return *this;

            doCopy(other);

            return *this;
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT>)
        Handle& operator=(Handle<OtherAssetT>&& other) noexcept
        {
            if(std::addressof(other) == this) return *this;

            doMove(std::move(other));

            return *this;
        }

        Handle& operator=(std::nullptr_t) noexcept
        {
            if(slot_)
            {
                slot_->decrementRefCounter();
            }

            slot_ = {};

            return *this;
        }

    private:
        AssetSlot<AssetT>* slot_ {};
        AssetId asset_id_ {};

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT>)
        void doCopy(const Handle<OtherAssetT>& other) noexcept
        {
            if(slot_ == other.slot_) return;

            if(slot_)
            {
                slot_->decrementRefCounter();
            }

            slot_ = other.slot_;

            if(slot_)
            {
                slot_->incrementRefCounter();
            }
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT>)
        void doMove(Handle<OtherAssetT>&& other) noexcept
        {
            if(slot_ == other.slot_) return;

            if(slot_)
            {
                slot_->decrementRefCounter();
            }

            slot_ = other.slot_;
            other.slot_ = {};
        }

        bool valid() const noexcept
        {
            return slot_ && slot_->ready() && slot_->value;
        }
    };

    template<typename AssetT>
    struct WeakHandle final
    {
        template<typename>
        friend struct Handle;

        Handle<AssetT> lock() const noexcept
        {
            if(!slot_)
            {
                return {};
            }

            Handle<AssetT> handle;
            handle.slot_ = slot_;

            return handle;
        }

        bool expired() const noexcept
        {
            if(!slot_)
            {
                return true;
            }

            return slot_->strong_refs_ == 0;
        }

    private:
        AssetSlot<AssetT>* slot_ {};
        std::uint32_t generation_ {};
        AssetId asset_id_ {};
    };

    template<typename AssetT>
    WeakHandle<AssetT> Handle<AssetT>::weak() const noexcept
    {
        WeakHandle<AssetT> handle;
        handle.slot_ = slot_;
        handle.generation_ = slot_ ? slot_->generation_ : 0;
        handle.asset_id_ = asset_id_;

        return handle;
    }
}