#pragma once

#include <cinttypes>


namespace pool
{

using elem_key_t = uint64_t;

inline void elem_key_extract_data(elem_key_t key,
                                  uint32_t& out_idx,
                                  uint32_t& out_version_num)
{
    out_idx         = static_cast<uint32_t>(key & 0x00000000ffffffff);
    out_version_num = static_cast<uint32_t>((key & 0xffffffff00000000) >> 32);
}

inline elem_key_t create_elem_key(uint32_t idx, uint32_t version_num)
{
    return static_cast<elem_key_t>(
        static_cast<uint64_t>(idx) |
        (static_cast<uint64_t>(version_num) << 32));
}

inline elem_key_t invalid_key()
{
    return (elem_key_t)-1;
}

inline bool is_invalid_key(elem_key_t key)
{
    return (key == invalid_key());
}

}  // namespace pool
