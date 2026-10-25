//
// Created by stuka on 25.10.2026.
//

#pragma once

#include "asset_slot.h"

namespace sgf::assets
{
    template<typename AssetT>
    struct Handle final
    {
        Handle() noexcept = default;
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
            return &slot_->value;
        }

        [[nodiscard]] AssetT* get() noexcept
        {
            return &slot_->value;
        }

        AssetT& operator*() noexcept
        {
            return slot_->value;
        }

        const AssetT& operator*() const noexcept
        {
            return slot_->value;
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT>)
        Handle& operator=(const Handle<OtherAssetT>& other) const noexcept
        {
            if(std::addressof(other) == this) return *this;

            doCopy(other);

            return *this;
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT>)
        Handle& operator=(Handle<OtherAssetT>&& other) const noexcept
        {
            if(std::addressof(other) == this) return *this;

            doMove(std::move(other));

            return *this;
        }

    private:
        AssetSlot<AssetT>* slot_ {};

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT>)
        void doCopy(const Handle<OtherAssetT>& other) noexcept
        {
            if(slot_ == other.slot_) return;

            if(slot_)
            {
                --slot_->strong_refs_;
            }

            slot_ = other.slot_;

            if(slot_)
            {
                ++slot_->strong_refs_;
            }
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT>)
        void doMove(Handle<OtherAssetT>&& other) noexcept
        {
            if(slot_ == other.slot_) return;

            if(slot_)
            {
                --slot_->strong_refs_;
            }

            slot_ = other.slot_;
            other.slot_ = {};
        }
    };
}