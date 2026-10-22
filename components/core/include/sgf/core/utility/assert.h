//
// Created by stuka on 14.10.2026.
//

#pragma once

#include <cassert>
#include <source_location>

#ifndef NDEBUG
#define sgf_assert(expr, msg) \
    sgf::core::impl::assert_(#expr, expr, __FILE__, __LINE__, msg)
#else
#define sgf_assert(expr, msg) ((void)0)
#endif

namespace sgf::core::impl
{
    void assert_(const char* expr, bool expr_eval, const char* file, int line, const char* msg);
}