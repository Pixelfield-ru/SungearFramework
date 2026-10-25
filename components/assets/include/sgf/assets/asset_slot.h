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

    /// Has stable address in memory. Always valid if created.
    template<typename AssetT>
    struct AssetSlot final
    {
        std::optional<AssetT> value {};

        AssetState state() const noexcept
        {
            return state_;
        }

        bool ready() const noexcept
        {
            return state_ == AssetState::kLoaded;
        }

    private:
        std::atomic<std::uint32_t> strong_refs_ {};
        /// Generation of asset (number of assets held by this slot earlier).
        std::atomic<std::uint32_t> generation_ = 1;
        /// Version of asset (number of reloads of the same asset).
        std::atomic<std::uint32_t> version_ = 1;
        std::atomic<AssetState> state_ = AssetState::kUnloaded;
        AssetId id_ {};

        void incrementRefCounter() noexcept
        {
            ++strong_refs_;
        }

        void decrementRefCounter() noexcept
        {
            --strong_refs_;
        }
    };
}
