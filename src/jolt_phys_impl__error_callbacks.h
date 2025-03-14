#pragma once

#include <cassert>
#include <cstdarg>
#include <iostream>
#include "jolt_physics_headers.h"

// Callback for traces, connect this to your own trace function if you have one.
static void trace_impl(const char* in_fmt, ...)
{
    // Format message.
    va_list list;
    va_start(list, in_fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), in_fmt, list);
    va_end(list);

    std::cout << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS
// Callback for asserts, connect this to your own assert handler if you have one
static bool assert_failed_impl(const char* in_expression,
                               const char* in_message,
                               const char* in_file,
                               uint32_t in_line)
{
    std::cout
        << in_file << ":" << in_line
        << ": (" << in_expression << ") "
        << (in_message != nullptr ? in_message : "")
        << std::endl;

    // Breakpoint.
    assert(false);
    return true;
};
#endif  // JPH_ENABLE_ASSERTS
