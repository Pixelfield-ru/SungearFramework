//
// Created by stuka on 25.10.2026.
//

#pragma once

#include <atomic>
#include <optional>
#include <system_error>

#include "state.h"

namespace sgf::assets
{
    using AssetId = std::uint32_t;

    struct IAssetStorage;

    struct AssetSlotControl
    {
        virtual ~AssetSlotControl() noexcept = default;

        AssetSlotControl() = default;
        AssetSlotControl(const AssetSlotControl& other) noexcept :
            strong_refs(other.strong_refs.load()),
            generation(other.generation.load()),
            version(other.version.load()),
            state(other.state.load()),
            id(other.id.load()),
            storage(other.storage)
        { }
        AssetSlotControl(AssetSlotControl&& other) noexcept :
            strong_refs(other.strong_refs.load()),
            generation(other.generation.load()),
            version(other.version.load()),
            state(other.state.load()),
            id(other.id.load()),
            storage(other.storage)
        { }

        std::atomic<std::uint32_t> strong_refs {};
        /// Generation of asset (number of assets held by this slot earlier).
        std::atomic<std::uint32_t> generation = 0;
        /// Version of asset (number of reloads of the same asset).
        std::atomic<std::uint32_t> version = 1;
        std::atomic<AssetState> state = AssetState::kUnloaded;
        std::atomic<AssetId> id {};

        IAssetStorage* storage {};

        /// If an error occurred while loading the asset.
        std::error_code error {};

        void incrementRefCounter() noexcept
        {
            ++strong_refs;
        }

        [[nodiscard]] bool tryAddStrongRef(std::uint32_t expected_generation) noexcept;

        void decrementRefCounter() noexcept;

        bool loaded() const noexcept
        {
            return state == AssetState::kLoaded;
        }

        virtual std::byte* erased() noexcept = 0;
        virtual std::byte const* erased() const noexcept = 0;
        virtual void clear() noexcept = 0;

        AssetSlotControl& operator=(const AssetSlotControl& other) noexcept
        {
            if(std::addressof(other) == this) return *this;

            strong_refs = other.strong_refs.load();
            generation = other.generation.load();
            version = other.version.load();
            state = other.state.load();
            id = other.id.load();
            storage = other.storage;

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
            storage = other.storage;

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
            ++generation;
        }

        std::byte* erased() noexcept override
        {
            return value ? reinterpret_cast<std::byte*>(&*value) : nullptr;
        }

        std::byte const* erased() const noexcept override
        {
            return value ? reinterpret_cast<std::byte const*>(&*value) : nullptr;
        }

        void clear() noexcept override
        {
            state = AssetState::kUnloaded;
            error = {};
            id = 0;
            version = 1;
            storage = {};
            value = std::nullopt;
        }
    };
}
