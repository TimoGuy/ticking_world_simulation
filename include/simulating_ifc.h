#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cinttypes>
#include <vector>
#include "cglm/types.h"
#include "jolt_physics_headers.h"
#include "pool_elem_key.h"


namespace simulating
{

using rvec3 = JPH::Real[3];
class Behavior_ifc;

// Interface for entities to edit behavior groups.
class Edit_behavior_groups_ifc
{
public:
    using behavior_group_key_t = uint64_t;

    virtual behavior_group_key_t add_behavior_group(std::vector<std::unique_ptr<Behavior_ifc>>&& group) = 0;
    virtual void remove_behavior_group(behavior_group_key_t group_key) = 0;
};

// Abstract class for entities.
class Entity_ifc
{
public:
    // World simulation events.
    virtual void on_create(Edit_behavior_groups_ifc& editor, size_t creation_idx) = 0;
    virtual void on_teardown(Edit_behavior_groups_ifc& editor) = 0;
};

constexpr uint32_t k_num_max_behavior_data_blocks{ 4096 };

class Behavior_data_w_version;

// Behavior interface: components of entities.
class Behavior_ifc
{
public:
    Behavior_ifc();
    virtual ~Behavior_ifc();

    inline pool::elem_key_t get_data_key() { return m_input_data_key; }

    template<class T>
    const T& get_data_from_input()
    {
        if (pool::is_invalid_key(m_input_data_key))
        {
            assert(false);
        }

        return
            *reinterpret_cast<T*>(
                Behavior_data_w_version::get_one_from_key(m_input_data_key)
                    ->read_data());
    }

    template<class T>
    void send_data_to_output(pool::elem_key_t output_key, T&& data)
    {
        if (pool::is_invalid_key(output_key))
        {
            assert(false);
        }

        Behavior_data_w_version::get_one_from_key(output_key)
            ->write_data<T>(std::move(data));
    }

    virtual void on_update() = 0;

private:
    pool::elem_key_t m_input_data_key;  // Set automatically.
};

// @NOTE: I'm sure it's self explanatory, but the order that atomics are used is super
//   important to ensure data visibility with stores and loads of the data.  -Thea 2025/03/23
class Behavior_data_w_version
{
public:
    // Disallow copying/moving.
    Behavior_data_w_version(const Behavior_data_w_version&)            = delete;
    Behavior_data_w_version(Behavior_data_w_version&&)                 = delete;
    Behavior_data_w_version& operator=(const Behavior_data_w_version&) = delete;
    Behavior_data_w_version& operator=(Behavior_data_w_version&&)      = delete;

    static void initialize_data_pool();
    static pool::elem_key_t allocate_one();
    static bool destroy_one(pool::elem_key_t key);
    static Behavior_data_w_version* get_one_from_key(pool::elem_key_t key);

    void* read_data()
    {
        return reinterpret_cast<void*>(m_data);
    }

    template<class T>
    void write_data(T&& data)
    {
        static_assert(sizeof(T) <= k_behavior_data_block_size);
        *reinterpret_cast<T*>(m_data) = data;
    }

private:
    Behavior_data_w_version();
    ~Behavior_data_w_version() = default;

    void reset(bool reset_all);
    
    static constexpr size_t k_behavior_data_block_size{ sizeof(float_t) * 3 };  // A vec3 (example benchmark data amount).
    using Data_block = uint8_t[k_behavior_data_block_size];

    Data_block m_data;
    uint32_t m_version;

    static constexpr uint8_t k_unreserved{ 0 };
    static constexpr uint8_t k_setup_reservation{ 1 };
    static constexpr uint8_t k_reserved{ 2 };
    std::atomic_uint8_t m_reserved;

    // inline static std::array<uint8_t, sizeof(Behavior_data_w_version) * k_num_max_behavior_data_blocks> s_behavior_data_block_collection;
};

// Physics system deposits transforms here and renderer withdraws.
class Transform_holder
{
public:
    struct Transform_decomposed
    {
        rvec3  position;
        versor rotation;
        vec3   scale;
    };

    Transform_holder(bool interpolate, Transform_decomposed&& initial_transform);

    inline void set_interpolate(bool interpolate) { m_interpolate_transform = interpolate; }

    void deposit_physics_transform(Transform_decomposed&& transform);
    void calculate_current_transform(mat4& out_transform);

private:
    std::atomic_bool m_interpolate_transform;

    inline static std::atomic_size_t m_buffer_offset{ 0 };
    static constexpr size_t k_read_a_offset{ 0 };
    static constexpr size_t k_read_b_offset{ 1 };
    static constexpr size_t k_write_offset{ 2 };

    static constexpr size_t k_num_buffers{ 3 };
    std::array<Transform_decomposed, k_num_buffers> m_transform_triple_buffer;
};

}  // namespace simulating
