//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <cstddef>

#include "sgf/core/utility/type_traits.h"

#include "asset_slot.h"

namespace sgf::assets
{
    template<typename>
    struct WeakHandle;

    template<typename AssetT>
    struct Handle final
    {
        friend struct AssetServer;

        template<typename>
        friend struct WeakHandle;

        friend struct AnyHandle;

        Handle() noexcept = default;
        ~Handle() noexcept
        {
            if(slot_) slot_->decrementRefCounter();
        }
        Handle(std::nullptr_t) noexcept { }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        Handle(const Handle<OtherAssetT>& other) noexcept :
            slot_(other.slot_),
            asset_id_(other.asset_id_)
        {
            if(slot_) slot_->incrementRefCounter();
        }
        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        Handle(Handle<OtherAssetT>&& other) noexcept :
            slot_(other.slot_),
            asset_id_(other.asset_id_)
        { }

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
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        Handle& operator=(const Handle<OtherAssetT>& other) noexcept
        {
            if(std::addressof(other) == this) return *this;
            if(slot_ == other.slot_) return *this;

            if(slot_) slot_->decrementRefCounter();

            slot_ = other.slot_;
            asset_id_ = other.asset_id_;

            if(slot_) slot_->incrementRefCounter();

            return *this;
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        Handle& operator=(Handle<OtherAssetT>&& other) noexcept
        {
            if(std::addressof(other) == this) return *this;
            if(slot_ == other.slot_) return *this;

            if(slot_) slot_->decrementRefCounter();

            slot_ = std::exchange(other.slot_, {});
            asset_id_ = std::exchange(other.asset_id_, {});

            return *this;
        }

        Handle& operator=(std::nullptr_t) noexcept
        {
            if(slot_) slot_->decrementRefCounter();

            slot_ = {};
            asset_id_ = {};

            return *this;
        }

    private:
        AssetSlot<AssetT>* slot_ {};
        AssetId asset_id_ {};

        Handle(AssetSlot<AssetT>* slot) noexcept :
            slot_(slot),
            asset_id_(slot_ ? slot->id.load() : 0)
        {
            if(slot_) slot_->incrementRefCounter();
        }

        [[nodiscard]] bool valid() const noexcept
        {
            return slot_ && slot_->loaded();
        }
    };

    template<typename AssetT>
    struct WeakHandle final
    {
        template<typename>
        friend struct Handle;

        friend struct AnyWeakHandle;

        WeakHandle() = default;
        WeakHandle(std::nullptr_t) noexcept { }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        WeakHandle(const WeakHandle<OtherAssetT>& other) noexcept
        {
            slot_ = other.slot_;
            generation_ = other.generation_;
            asset_id_ = other.asset_id_;
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        WeakHandle(const Handle<OtherAssetT>& other) noexcept
        {
            slot_ = other.slot_;
            generation_ = other.slot_ ? other.slot_->generation.load() : 0;
            asset_id_ = other.asset_id_;
        }

        WeakHandle(const WeakHandle&) = default;
        WeakHandle(WeakHandle&) noexcept = default;

        Handle<AssetT> lock() const noexcept
        {
            if(!slot_) return {};

            Handle<AssetT> handle;
            handle.slot_ = slot_;
            handle.asset_id_ = asset_id_;

            return handle;
        }

        [[nodiscard]] bool expired() const noexcept
        {
            if(!slot_) return true;

            return slot_->strong_refs_ == 0;
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        WeakHandle& operator=(const WeakHandle<OtherAssetT>& other) noexcept
        {
            if(std::addressof(other) == this) return *this;

            slot_ = other.slot_;
            generation_ = other.generation_;
            asset_id_ = other.asset_id_;

            return *this;
        }

        WeakHandle& operator=(std::nullptr_t) noexcept
        {
            slot_ = {};
            generation_ = {};
            asset_id_ = {};

            return *this;
        }

        WeakHandle& operator=(const WeakHandle&) = default;
        WeakHandle& operator=(WeakHandle&&) noexcept = default;

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
        handle.generation_ = slot_ ? slot_->generation.load() : 0;
        handle.asset_id_ = asset_id_;

        return handle;
    }

    struct AnyHandle final
    {
        friend struct AssetServer;
        friend struct AnyWeakHandle;

        AnyHandle() = default;
        AnyHandle(std::nullptr_t) noexcept { }

        template<typename AssetT>
        AnyHandle(const Handle<AssetT>& handle) noexcept :
            slot_(handle.slot_),
            type_index_(core::typeIndex<AssetT>()),
            asset_id_(handle.asset_id_)
        {
            if(slot_) slot_->incrementRefCounter();
        }

        AnyHandle(const AnyHandle& other) :
            slot_(other.slot_),
            type_index_(other.type_index_),
            asset_id_(other.asset_id_)
        {
            if(slot_) slot_->incrementRefCounter();
        }

        AnyHandle(AnyHandle&& other) noexcept :
            slot_(other.slot_),
            type_index_(other.type_index_),
            asset_id_(other.asset_id_)
        { }

        ~AnyHandle() noexcept
        {
            if(slot_) slot_->decrementRefCounter();
        }

        template<typename AssetT>
        Handle<AssetT> as() const noexcept
        {
            if(!slot_ || core::typeIndex<AssetT>() != type_index_) return {};

            Handle<AssetT> handle;
            handle.slot_ = static_cast<AssetSlot<AssetT>*>(slot_);
            handle.asset_id_ = asset_id_;
            handle.slot_->incrementRefCounter();

            return handle;
        }

        operator bool() const noexcept
        {
            return valid();
        }

        AnyHandle& operator=(const AnyHandle& other)
        {
            if(std::addressof(other) == this) return *this;
            if(other.slot_ == slot_) return *this;

            if(slot_) slot_->decrementRefCounter();

            slot_ = other.slot_;
            type_index_ = other.type_index_;
            asset_id_ = other.asset_id_;

            if(slot_) slot_->incrementRefCounter();

            return *this;
        }

        AnyHandle& operator=(AnyHandle&& other) noexcept
        {
            if(std::addressof(other) == this) return *this;
            if(other.slot_ == slot_) return *this;

            if(slot_) slot_->decrementRefCounter();

            slot_ = std::exchange(other.slot_, {});
            type_index_ = std::exchange(other.type_index_, {});
            asset_id_ = std::exchange(other.asset_id_, {});

            return *this;
        }

        template<typename AssetT>
        AnyHandle& operator=(const Handle<AssetT>& other)
        {
            if(other.slot_ == slot_) return *this;

            if(slot_) slot_->decrementRefCounter();

            slot_ = other.slot;
            type_index_ = core::typeIndex<AssetT>();
            asset_id_ = other.asset_id_;

            if(slot_) slot_->incrementRefCounter();

            return *this;
        }

        template<typename AssetT>
        AnyHandle& operator=(Handle<AssetT>&& other)
        {
            if(other.slot_ == slot_) return *this;

            if(slot_) slot_->decrementRefCounter();

            slot_ = std::exchange(other.slot_, {});
            type_index_ = core::typeIndex<AssetT>();
            asset_id_ = std::exchange(other.asset_id_, {});

            return *this;
        }

        AnyHandle& operator=(std::nullptr_t) noexcept
        {
            if(slot_) slot_->decrementRefCounter();

            slot_ = nullptr;
            type_index_ = {};
            asset_id_ = {};

            return *this;
        }

    private:
        AnyHandle(AssetSlotControl* slot, std::uint32_t type_index) :
            slot_(slot),
            type_index_(type_index),
            asset_id_(slot ? slot->id.load() : 0)
        {
            if(slot_) slot_->incrementRefCounter();
        }

        AssetSlotControl* slot_ {};
        std::uint32_t type_index_ {};
        AssetId asset_id_ {};

        [[nodiscard]] bool valid() const noexcept
        {
            return slot_ && slot_->loaded();
        }
    };

    struct AnyWeakHandle final
    {
        friend struct AnyHandle;

        AnyWeakHandle() = default;
        AnyWeakHandle(std::nullptr_t) { }
        AnyWeakHandle(const AnyHandle& handle) :
            slot_(handle.slot_),
            generation_(handle.slot_ ? handle.slot_->generation.load() : 0),
            type_index_(handle.type_index_),
            asset_id_(handle.asset_id_)
        { }
        template<typename AssetT>
        AnyWeakHandle(const Handle<AssetT>& handle) noexcept :
            slot_(handle.slot_),
            generation_(handle.slot_ ? handle.slot_->generation.load() : 0),
            type_index_(core::typeIndex<AssetT>()),
            asset_id_(handle.asset_id_)
        { }
        template<typename AssetT>
        AnyWeakHandle(const WeakHandle<AssetT>& handle) noexcept :
            slot_(handle.slot_),
            generation_(handle.generation_),
            type_index_(core::typeIndex<AssetT>()),
            asset_id_(handle.asset_id_)
        { }
        AnyWeakHandle(const AnyWeakHandle&) = default;
        AnyWeakHandle(AnyWeakHandle&&) = default;

        AnyHandle lock() const noexcept
        {
            if(!slot_) return {};

            AnyHandle handle;
            handle.slot_ = slot_;
            handle.asset_id_ = asset_id_;
            handle.type_index_ = type_index_;

            return handle;
        }

        bool expired() const noexcept
        {
            if(!slot_) return true;

            return slot_->strong_refs == 0;
        }

        AnyWeakHandle& operator=(const AnyWeakHandle&) = default;
        AnyWeakHandle& operator=(AnyWeakHandle&&) = default;
        AnyWeakHandle& operator=(std::nullptr_t) noexcept
        {
            slot_ = {};
            generation_ = {};
            type_index_ = {};
            asset_id_ = {};

            return *this;
        }

        AnyWeakHandle& operator=(const AnyHandle& handle) noexcept
        {
            slot_ = handle.slot_;
            generation_ = handle.slot_ ? handle.slot_->generation.load() : 0;
            type_index_ = handle.type_index_;
            asset_id_ = handle.asset_id_;

            return *this;
        }

        template<typename AssetT>
        AnyWeakHandle& operator=(const Handle<AssetT>& handle) noexcept
        {
            slot_ = handle.slot_;
            generation_ = handle.slot_ ? handle.slot_->generation.load() : 0;
            type_index_ = core::typeIndex<AssetT>();
            asset_id_ = handle.asset_id_;

            return *this;
        }
        template<typename AssetT>
        AnyWeakHandle& operator=(const WeakHandle<AssetT>& handle) noexcept
        {
            slot_ = handle.slot_;
            generation_ = handle.generation_;
            type_index_ = core::typeIndex<AssetT>();
            asset_id_ = handle.asset_id_;

            return *this;
        }

    private:
        AssetSlotControl* slot_ {};
        std::uint32_t generation_ {};
        std::uint32_t type_index_ {};
        AssetId asset_id_ {};
    };
}