//
// Created by stuka on 01.11.2026.
//

#pragma once

#if defined _MSC_VER
#   define SGF_GENERATOR_PRETTY_FUNCTION __FUNCSIG__
#elif defined __clang__ || (defined __GNUC__)
#   define SGF_GENERATOR_PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif

#define SGF_CONCAT_IMPL(a, b) a ## b
#define SGF_CONCAT(a, b) SGF_CONCAT_IMPL(a, b)