//
// Created by stuka on 17.10.2026.
//

#pragma once

#include <optional>
#include <boost/asio/thread_pool.hpp>

namespace sgf::runtime
{
    struct ThreadPool final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        explicit ThreadPool(std::size_t thread_count);
        ThreadPool(std::size_t thread_count, const allocator_type& alloc);
        ThreadPool() noexcept = default;
        ThreadPool(const allocator_type& alloc) noexcept;
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;

        void post(std::move_only_function<void()> task);
        void wait();

        [[nodiscard]] allocator_type get_allocator() const noexcept;

    private:
        allocator_type allocator_ {};
        boost::asio::thread_pool pool_;
    };
}