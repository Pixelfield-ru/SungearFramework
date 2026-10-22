//
// Created by stuka on 17.10.2026.
//

#include "sgf/runtime/execution/thread_pool.h"

#include <boost/asio/post.hpp>
#include <boost/asio/bind_allocator.hpp>

sgf::runtime::ThreadPool::ThreadPool(std::size_t thread_count) :
    pool_(thread_count)
{
}

sgf::runtime::ThreadPool::ThreadPool(std::size_t thread_count, const allocator_type& alloc) :
    allocator_(alloc),
    pool_(std::allocator_arg, alloc, thread_count)
{
}

sgf::runtime::ThreadPool::ThreadPool(const allocator_type& alloc) noexcept :
    allocator_(alloc),
    pool_(std::allocator_arg, alloc)
{

}

void sgf::runtime::ThreadPool::post(std::move_only_function<void()> task)
{
    boost::asio::post(pool_, boost::asio::bind_allocator(allocator_, std::move(task)));
}

void sgf::runtime::ThreadPool::wait()
{
    pool_.wait();
}

sgf::runtime::ThreadPool::allocator_type sgf::runtime::ThreadPool::get_allocator() const noexcept
{
    return allocator_;
}
