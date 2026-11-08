//
// Created by stuka on 17.10.2026.
//

#include "sgf/core/thread_pool.h"

#include <boost/asio/post.hpp>
#include <boost/asio/bind_allocator.hpp>

sgf::core::ThreadPool::ThreadPool(std::size_t thread_count) :
    pool_(thread_count)
{
}

sgf::core::ThreadPool::ThreadPool(std::size_t thread_count, const allocator_type& alloc) :
    allocator_(alloc),
    pool_(std::allocator_arg, alloc, thread_count)
{
}

sgf::core::ThreadPool::ThreadPool(const allocator_type& alloc) noexcept :
    allocator_(alloc),
    pool_(std::allocator_arg, alloc)
{

}

void sgf::core::ThreadPool::post(std::move_only_function<void()> task)
{
    boost::asio::post(pool_, boost::asio::bind_allocator(allocator_, std::move(task)));
}

void sgf::core::ThreadPool::wait()
{
    pool_.wait();
}

sgf::core::ThreadPool::allocator_type sgf::core::ThreadPool::get_allocator() const noexcept
{
    return allocator_;
}
