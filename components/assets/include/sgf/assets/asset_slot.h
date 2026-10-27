//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <atomic>
#include <optional>

#include "asset_state.h"

namespace sgf::assets
{
    using AssetId = std::uint32_t;

    struct AssetSlotControl
    {
        virtual ~AssetSlotControl() noexcept = default;

        AssetSlotControl() = default;
        AssetSlotControl(const AssetSlotControl& other) noexcept :
            strong_refs(other.strong_refs.load()),
            generation(other.generation.load()),
            version(other.version.load()),
            state(other.state.load()),
            id(other.id.load())
        { }
        AssetSlotControl(AssetSlotControl&& other) noexcept :
            strong_refs(other.strong_refs.load()),
            generation(other.generation.load()),
            version(other.version.load()),
            state(other.state.load()),
            id(other.id.load())
        { }

        std::atomic<std::uint32_t> strong_refs {};
        /// Generation of asset (number of assets held by this slot earlier).
        std::atomic<std::uint32_t> generation = 1;
        /// Version of asset (number of reloads of the same asset).
        std::atomic<std::uint32_t> version = 1;
        std::atomic<AssetState> state = AssetState::kUnloaded;
        std::atomic<AssetId> id {};

        void incrementRefCounter() noexcept
        {
            ++strong_refs;
        }

        void decrementRefCounter() noexcept
        {
            --strong_refs;
        }

        bool loaded() const noexcept
        {
            return state == AssetState::kLoaded;
        }

        AssetSlotControl& operator=(const AssetSlotControl& other) noexcept
        {
            if(std::addressof(other) == this) return *this;

            strong_refs = other.strong_refs.load();
            generation = other.generation.load();
            version = other.version.load();
            state = other.state.load();
            id = other.id.load();

            return *this;
        }

        AssetSlotControl& operator=(AssetSlotControl&& other) noexcept
        {
            if(std::addressof(other) == this) return *this;

            strong_refs = other.strong_refs.load();
            generation = other.generation.load();
            version = other.version.load();
            state = other.state.load();
            id = other.id.load();

            return *this;
        }
    };

    /// Has stable address in memory. Always valid if created.
    template<typename AssetT>
    struct AssetSlot final : AssetSlotControl
    {
        template<typename>
        friend struct Handle;

        template<typename>
        friend struct WeakHandle;

        friend struct AnyHandle;
        friend struct AnyWeakHandle;

        std::optional<AssetT> value {};

        void insert(AssetT asset)
        {
            value = std::move(asset);
            state = AssetState::kLoaded;
            ++generation;
        }
    };
}
