//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <cstddef>

#include "sgf/core/utility/type_traits.h"

#include "asset_slot.h"
#include "sgf/serialization/macroses.h"

namespace sgf::assets
{
    template<typename>
    struct WeakHandle;

    struct HandleBase
    {
        SGF_SERIALIZER_AS_FRIEND();

        HandleBase() = default;
        HandleBase(AssetSlotControl* slot, AssetId id, std::uint32_t type) :
            slot_(slot),
            asset_id_(id),
            type_id_(type)
        { }
        HandleBase(std::nullptr_t) noexcept
        { }
        HandleBase(const HandleBase& other) = default;
        HandleBase(HandleBase&& other) noexcept :
            slot_(std::exchange(other.slot_, {})),
            asset_id_(std::exchange(other.asset_id_, {})),
            type_id_(std::exchange(other.type_id_, {}))
        { }

        [[nodiscard]] AssetId assetId() const noexcept
        {
            return asset_id_;
        }

        [[nodiscard]] std::uint32_t typeId() const noexcept
        {
            return type_id_;
        }

        [[nodiscard]] AssetState state() const noexcept
        {
            if(!slot_) return AssetState::kUnloaded;
            return slot_->state;
        }

        [[nodiscard]] std::error_code error() const noexcept
        {
            if(!slot_) return {};
            return slot_->error;
        }

        HandleBase& operator=(const HandleBase&) = default;
        HandleBase& operator=(HandleBase&& other) noexcept
        {
            if(this == std::addressof(other)) return *this;

            slot_ = std::exchange(other.slot_, {});
            asset_id_ = std::exchange(other.asset_id_, {});
            type_id_ = std::exchange(other.type_id_, {});

            return *this;
        }

    protected:
        AssetSlotControl* slot_ {};
        AssetId asset_id_ {};
        std::uint32_t type_id_ {};

        template<typename AssetT>
        AssetSlot<AssetT>* slotAs() noexcept
        {
            return static_cast<AssetSlot<AssetT>*>(slot_);
        }
    };

    template<typename AssetT>
    struct Handle final : HandleBase
    {
        friend struct AssetServer;

        template<typename>
        friend struct WeakHandle;

        friend struct AnyHandle;

        Handle() noexcept = default;
        ~Handle() noexcept
        {
            auto* slot = slotAs<AssetT>();
            if(slot) slot->decrementRefCounter();
        }
        Handle(std::nullptr_t) noexcept :
            HandleBase(nullptr)
        { }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        Handle(const Handle<OtherAssetT>& other) noexcept :
            HandleBase(other)
        {
            auto* slot = slotAs<AssetT>();
            if(slot) slot->incrementRefCounter();
        }
        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        Handle(Handle<OtherAssetT>&& other) noexcept :
            HandleBase(std::move(other))
        { }

        [[nodiscard]] const AssetT* get() const noexcept
        {
            auto* slot = slotAs<AssetT>();
            return valid() ? &(*slot->value) : nullptr;
        }

        [[nodiscard]] AssetT* get() noexcept
        {
            auto* slot = slotAs<AssetT>();
            return valid() ? &(*slot->value) : nullptr;
        }

        [[nodiscard]] WeakHandle<AssetT> weak() const noexcept;

        operator bool() const noexcept
        {
            return valid();
        }

        AssetT* operator->() const noexcept
        {
            auto* slot = slotAs<AssetT>();
            return valid() ? &(*slot->value) : nullptr;
        }

        AssetT* operator->() noexcept
        {
            auto* slot = slotAs<AssetT>();
            return valid() ? &(*slot->value) : nullptr;
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        Handle& operator=(const Handle<OtherAssetT>& other) noexcept
        {
            if(std::addressof(other) == this) return *this;
            if(slot_ == other.slot_) return *this;

            if(slot_) slot_->decrementRefCounter();

            HandleBase::operator=(other);

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

            HandleBase::operator=(std::move(other));

            return *this;
        }

        Handle& operator=(std::nullptr_t) noexcept
        {
            if(slot_) slot_->decrementRefCounter();

            HandleBase::operator=(std::nullptr_t{});

            return *this;
        }

    private:
        Handle(AssetSlot<AssetT>* slot, std::uint32_t type_id) noexcept :
            HandleBase(slot, slot ? slot->id.load() : 0, type_id)
        {
            if(slot_) slot_->incrementRefCounter();
        }

        [[nodiscard]] bool valid() const noexcept
        {
            return slot_ && slotAs<AssetT>()->value && slot_->loaded();
        }
    };

    template<typename AssetT>
    struct WeakHandle final : HandleBase
    {
        template<typename>
        friend struct Handle;

        friend struct AnyWeakHandle;

        WeakHandle() = default;
        WeakHandle(std::nullptr_t) noexcept :
            HandleBase(nullptr)
        { }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        WeakHandle(const WeakHandle<OtherAssetT>& other) noexcept :
            HandleBase(other)
        {
            generation_ = other.generation_;
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        WeakHandle(const Handle<OtherAssetT>& other) noexcept :
            HandleBase(other)
        {
            generation_ = other.slot_ ? other.slot_->generation.load() : 0;
        }

        WeakHandle(const WeakHandle&) = default;
        WeakHandle(WeakHandle&) noexcept = default;

        Handle<AssetT> lock() const noexcept
        {
            if(!slot_) return {};

            Handle<AssetT> handle;
            handle.slot_ = slot_;
            handle.asset_id_ = asset_id_;
            handle.type_id_ = type_id_;

            return handle;
        }

        [[nodiscard]] bool expired() const noexcept
        {
            if(!slot_) return true;

            return slot_->strong_refs == 0;
        }

        template<typename OtherAssetT>
        requires(std::is_base_of_v<AssetT, OtherAssetT> || std::is_same_v<AssetT, OtherAssetT>)
        WeakHandle& operator=(const WeakHandle<OtherAssetT>& other) noexcept
        {
            if(std::addressof(other) == this) return *this;

            HandleBase::operator=(other);
            generation_ = other.generation_;

            return *this;
        }

        WeakHandle& operator=(std::nullptr_t) noexcept
        {
            HandleBase::operator=(nullptr);
            generation_ = {};

            return *this;
        }

        WeakHandle& operator=(const WeakHandle&) = default;
        WeakHandle& operator=(WeakHandle&&) noexcept = default;

    private:
        std::uint32_t generation_ {};
    };

    template<typename AssetT>
    WeakHandle<AssetT> Handle<AssetT>::weak() const noexcept
    {
        WeakHandle<AssetT> handle;
        handle.slot_ = slot_;
        handle.generation_ = slot_ ? slot_->generation.load() : 0;
        handle.asset_id_ = asset_id_;
        handle.type_id_ = type_id_;

        return handle;
    }

    struct AnyHandle final : HandleBase
    {
        friend struct AssetServer;
        friend struct AnyWeakHandle;

        AnyHandle() = default;
        AnyHandle(std::nullptr_t) noexcept { }

        template<typename AssetT>
        AnyHandle(const Handle<AssetT>& handle) noexcept :
            HandleBase(handle)
        {
            if(slot_) slot_->incrementRefCounter();
        }

        AnyHandle(const AnyHandle& other) :
            HandleBase(other)
        {
            if(slot_) slot_->incrementRefCounter();
        }

        AnyHandle(AnyHandle&& other) noexcept :
            HandleBase(std::move(other))
        { }

        ~AnyHandle() noexcept
        {
            if(slot_) slot_->decrementRefCounter();
        }

        template<typename AssetT>
        Handle<AssetT> as() const noexcept
        {
            if(!slot_ || core::typeIndex<AssetT>() != type_id_) return {};

            Handle<AssetT> handle;
            handle.slot_ = static_cast<AssetSlot<AssetT>*>(slot_);
            handle.asset_id_ = asset_id_;
            handle.type_id_ = type_id_;
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

            HandleBase::operator=(other);

            if(slot_) slot_->incrementRefCounter();

            return *this;
        }

        AnyHandle& operator=(AnyHandle&& other) noexcept
        {
            if(std::addressof(other) == this) return *this;
            if(other.slot_ == slot_) return *this;

            if(slot_) slot_->decrementRefCounter();

            HandleBase::operator=(std::move(other));

            return *this;
        }

        template<typename AssetT>
        AnyHandle& operator=(const Handle<AssetT>& other)
        {
            if(other.slot_ == slot_) return *this;

            if(slot_) slot_->decrementRefCounter();

            HandleBase::operator=(other);

            if(slot_) slot_->incrementRefCounter();

            return *this;
        }

        template<typename AssetT>
        AnyHandle& operator=(Handle<AssetT>&& other)
        {
            if(other.slot_ == slot_) return *this;

            if(slot_) slot_->decrementRefCounter();

            HandleBase::operator=(std::move(other));

            return *this;
        }

        AnyHandle& operator=(std::nullptr_t) noexcept
        {
            if(slot_) slot_->decrementRefCounter();

            HandleBase::operator=(nullptr);

            return *this;
        }

    private:
        AnyHandle(AssetSlotControl* slot, std::uint32_t type_id) :
            HandleBase(slot, slot ? slot->id.load() : 0, type_id)
        {
            if(slot_) slot_->incrementRefCounter();
        }

        [[nodiscard]] bool valid() const noexcept
        {
            return slot_ && slot_->erased() && slot_->loaded();
        }
    };

    struct AnyWeakHandle final : HandleBase
    {
        friend struct AnyHandle;

        AnyWeakHandle() = default;
        AnyWeakHandle(std::nullptr_t) { }
        AnyWeakHandle(const AnyHandle& handle) :
            HandleBase(handle),
            generation_(handle.slot_ ? handle.slot_->generation.load() : 0)
        { }
        template<typename AssetT>
        AnyWeakHandle(const Handle<AssetT>& handle) noexcept :
            HandleBase(handle),
            generation_(handle.slot_ ? handle.slot_->generation.load() : 0)
        { }
        template<typename AssetT>
        AnyWeakHandle(const WeakHandle<AssetT>& handle) noexcept :
            HandleBase(handle),
            generation_(handle.generation_)
        { }
        AnyWeakHandle(const AnyWeakHandle&) = default;
        AnyWeakHandle(AnyWeakHandle&&) = default;

        [[nodiscard]] AnyHandle lock() const noexcept
        {
            if(!slot_) return {};

            AnyHandle handle;
            handle.slot_ = slot_;
            handle.asset_id_ = asset_id_;
            handle.type_id_ = type_id_;

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
            HandleBase::operator=(nullptr);
            generation_ = {};

            return *this;
        }

        AnyWeakHandle& operator=(const AnyHandle& handle) noexcept
        {
            HandleBase::operator=(handle);
            generation_ = handle.slot_ ? handle.slot_->generation.load() : 0;

            return *this;
        }

        template<typename AssetT>
        AnyWeakHandle& operator=(const Handle<AssetT>& handle) noexcept
        {
            HandleBase::operator=(handle);
            generation_ = handle.slot_ ? handle.slot_->generation.load() : 0;

            return *this;
        }
        template<typename AssetT>
        AnyWeakHandle& operator=(const WeakHandle<AssetT>& handle) noexcept
        {
            HandleBase::operator=(handle);
            generation_ = handle.generation_;

            return *this;
        }

    private:
        std::uint32_t generation_ {};
    };
}
