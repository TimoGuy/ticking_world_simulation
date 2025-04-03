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

// Transform_holder.
simulating::Transform_holder::Transform_holder(bool interpolate, Transform_decomposed&& initial_transform)
    : m_interpolate_transform(interpolate)
{
    for (size_t i = 0; i < k_num_buffers; i++)
    {
        m_transform_triple_buffer[i].position[0] = initial_transform.position[0];
        m_transform_triple_buffer[i].position[1] = initial_transform.position[1];
        m_transform_triple_buffer[i].position[2] = initial_transform.position[2];
        glm_quat_copy(initial_transform.rotation, m_transform_triple_buffer[i].rotation);
        glm_vec3_copy(initial_transform.scale, m_transform_triple_buffer[i].scale);
    }
}

void simulating::Transform_holder::deposit_physics_transform(Transform_decomposed&& transform)
{
    auto& current_transform{
        m_transform_triple_buffer[(m_buffer_offset + k_write_offset) % k_num_buffers] };
    current_transform.position[0] = transform.position[0];
    current_transform.position[1] = transform.position[1];
    current_transform.position[2] = transform.position[2];
    glm_quat_copy(transform.rotation, current_transform.rotation);
    glm_vec3_copy(transform.scale, current_transform.scale);
}

void simulating::Transform_holder::calculate_current_transform(mat4& out_transform)
{
    // @TODO: In the future with world chunks and camera tricks for rendering large worlds,
    //   convert the true transforms into the camera based transform here!
    //   For now tho, it all just gets truncated.

    vec3   pos;
    versor rot;
    vec3   sca;

    size_t buffer_offset_copy{ m_buffer_offset };  // To only do one atomic load.

    if (m_interpolate_transform)
    {
        float_t t{ 0.5f };

        auto& transform_a{
            m_transform_triple_buffer[(buffer_offset_copy + k_read_a_offset) % k_num_buffers] };
        auto& transform_b{
            m_transform_triple_buffer[(buffer_offset_copy + k_read_b_offset) % k_num_buffers] };

        // @INCOMPLETE: Truncated data (if using double real JPH::RVec3).
        pos[0] = transform_a.position[0] + t * (transform_b.position[0] - transform_a.position[0]);
        pos[1] = transform_a.position[1] + t * (transform_b.position[1] - transform_a.position[1]);
        pos[2] = transform_a.position[2] + t * (transform_b.position[2] - transform_a.position[2]);

        glm_quat_nlerp(transform_a.rotation, transform_b.rotation, t, rot);

        glm_vec3_lerp(transform_a.scale, transform_b.scale, t, sca);
    }
    else
    {
        auto& transform_b{
            m_transform_triple_buffer[(buffer_offset_copy + k_read_b_offset) % k_num_buffers] };

        // @INCOMPLETE: Truncated data (if using double real JPH::RVec3).
        pos[0] = transform_b.position[0];
        pos[1] = transform_b.position[1];
        pos[2] = transform_b.position[2];

        glm_quat_copy(transform_b.rotation, rot);

        glm_vec3_copy(transform_b.scale, sca);
    }

    // Write transform.
    glm_translate_make(out_transform, pos);
    glm_quat_rotate(out_transform, rot, out_transform);
    glm_scale(out_transform, sca);
}
