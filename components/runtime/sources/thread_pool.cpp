//
// Created by stuka on 17.10.2026.
//

#include "sgf/runtime/execution/thread_pool.h"

#include <boost/asio/post.hpp>

sgf::runtime::ThreadPool::ThreadPool(std::size_t thread_count) :
    pool_(thread_count)
{
}

void sgf::runtime::ThreadPool::post(std::move_only_function<void()> task)
{
    boost::asio::post(pool_, std::move(task));
}

void sgf::runtime::ThreadPool::wait()
{
    pool_.wait();
}
