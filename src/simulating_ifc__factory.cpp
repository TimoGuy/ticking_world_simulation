#include "simulating_ifc.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <vector>
#include "cglm/cglm.h"
#include "pool_elem_key.h"


// Behavior interface.
simulating::Behavior_ifc::Behavior_ifc()
    : m_input_data_key(Behavior_data_w_version::allocate_one())
{
}

simulating::Behavior_ifc::~Behavior_ifc()
{
    Behavior_data_w_version::destroy_one(m_input_data_key);
}

namespace simulating
{

static char s_behavior_data_block_collection__internal_data_chunk
    [sizeof(Behavior_data_w_version) * k_num_max_behavior_data_blocks];
static Behavior_data_w_version* s_behavior_data_block_collection{
    reinterpret_cast<Behavior_data_w_version*>(s_behavior_data_block_collection__internal_data_chunk) };

}  // namespace simulating

void simulating::Behavior_data_w_version::initialize_data_pool()
{
    static std::atomic_bool s_first_init{ false };
    bool expected_first{ false };
    if (!s_first_init.compare_exchange_strong(expected_first, true))
    {
        // Not first initialization.
        assert(false);
    }

    for (uint32_t idx = 0; idx < k_num_max_behavior_data_blocks; idx++)
    {
        s_behavior_data_block_collection[idx].reset(true);
    }
}

pool::elem_key_t simulating::Behavior_data_w_version::allocate_one()
{
    pool::elem_key_t ret{ pool::invalid_key() };

    for (uint32_t idx = 0; idx < k_num_max_behavior_data_blocks; idx++)
    {
        auto& block{ s_behavior_data_block_collection[idx] };
        uint8_t reserve_expect{ k_unreserved };
        if (block.m_reserved.compare_exchange_strong(reserve_expect, k_setup_reservation))
        {
            // Setup reservation for data.
            block.reset(false);
            uint32_t version_num{ ++block.m_version };

            // Complete reservation.
            block.m_reserved.store(k_reserved);

            ret = pool::create_elem_key(idx, version_num);
            break;
        }
    }

    return ret;
}

bool simulating::Behavior_data_w_version::destroy_one(pool::elem_key_t key)
{
    uint32_t idx, version_num;
    pool::elem_key_extract_data(key, idx, version_num);

    if (idx >= k_num_max_behavior_data_blocks)
    {
        assert(false);
        return false;
    }

    if (s_behavior_data_block_collection[idx].m_reserved.load() != k_reserved)
    {
        assert(false);
        return false;
    }

    if (version_num != s_behavior_data_block_collection[idx].m_version)
    {
        assert(false);
        return false;
    }

    s_behavior_data_block_collection[idx].m_reserved.store(k_unreserved);
    return true;
}

simulating::Behavior_data_w_version* simulating::Behavior_data_w_version::get_one_from_key(pool::elem_key_t key)
{
    uint32_t idx, version_num;
    pool::elem_key_extract_data(key, idx, version_num);

    if (idx >= k_num_max_behavior_data_blocks)
    {
        assert(false);
        return nullptr;
    }

    if (s_behavior_data_block_collection[idx].m_reserved.load() != k_reserved)
    {
        assert(false);
        return nullptr;
    }

    if (version_num != s_behavior_data_block_collection[idx].m_version)
    {
        assert(false);
        return nullptr;
    }

    return &s_behavior_data_block_collection[idx];
}

simulating::Behavior_data_w_version::Behavior_data_w_version()
{
    reset(true);
}

void simulating::Behavior_data_w_version::reset(bool reset_all)
{
    // Reset data.
    std::fill(m_data,
              m_data + k_behavior_data_block_size,
              0);
    
    if (reset_all)
    {
        // Reset metadata as well.
        m_version  = (uint32_t)-1;
        m_reserved = k_unreserved;
    }
}
