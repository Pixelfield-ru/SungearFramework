//
// Created by stuka on 11.11.2026.
//

#include "sgf/assets/asset_slot.h"

#include "sgf/assets/asset_storage.h"

bool sgf::assets::AssetSlotControl::tryAddStrongRef(std::uint32_t expected_generation) noexcept
{
    if(generation.load(std::memory_order_acquire) != expected_generation) return false;

    auto refs = strong_refs.load(std::memory_order_acquire);
    while(refs != 0)
    {
        // exchange only if strong_refs equals to last refs value
        // writing refs + 1 into strong_refs if strong_refs equals to last refs
        // if strong_refs does not equal to last refs, then strong_refs assigned to refs
        if(strong_refs.compare_exchange_weak(
            refs, refs + 1,
            std::memory_order_acq_rel,
            std::memory_order_acquire))
        {
            // if generation was changed (for example, in other thread) then decrement strong_refs.
            if(generation.load(std::memory_order_acquire) != expected_generation)
            {
                decrementRefCounter();
                return false;
            }

            return true;
        }
    }

    return false;
}

void sgf::assets::AssetSlotControl::decrementRefCounter() noexcept
{
    if(storage && strong_refs.fetch_sub(1) == 1)
    {
        storage->releaseSlot(this);
    }
}
