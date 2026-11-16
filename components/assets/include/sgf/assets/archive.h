//
// Created by stuka on 16.11.2026.
//

#pragma once

#include "sgf/core/utility/mapped_file.h"

namespace sgf::assets
{
    struct Archive final
    {
        /// Owns the busy flag of one archive. The flag is cleared when the locker is destroyed.
        class Locker final
        {
        public:
            Locker() noexcept = default;
            Locker(const Locker&) = delete;
            Locker(Locker&& other) noexcept :
                archive_(std::exchange(other.archive_, nullptr))
            { }

            ~Locker()
            {
                release();
            }

            Locker& operator=(const Locker&) = delete;
            Locker& operator=(Locker&& other) noexcept
            {
                if(this != &other)
                {
                    release();
                    archive_ = std::exchange(other.archive_, nullptr);
                }

                return *this;
            }

            explicit operator bool() const noexcept
            {
                return archive_ != nullptr;
            }

        private:
            friend struct Archive;

            explicit Locker(Archive& archive) noexcept :
                archive_(&archive)
            { }

            void release() noexcept
            {
                if(!archive_) return;

                archive_->busy_.store(false);
                archive_ = nullptr;
            }

            Archive* archive_ {};
        };

        core::MappedFile file;

        /// Marks the archive busy. An empty locker means another bake already holds it.
        [[nodiscard]] Locker tryLock() noexcept
        {
            bool expected = false;
            if(!busy_.compare_exchange_strong(expected, true)) return {};
            return Locker(*this);
        }

    private:
        /// True while this archive is being baked.
        std::atomic<bool> busy_ {};
    };
}
