#pragma once

#include <array>
#include <atomic>
#include <cinttypes>
#include "cglm/types.h"
#include "jolt_physics_headers.h"
#include "pool_elem_key.h"


namespace simulating
{

using rvec3 = JPH::Real[3];

// Abstract class for entities.
class Entity_ifc
{
public:
    // World simulation events.
    virtual void on_create(pool::elem_key_t key) = 0;
    virtual void on_teardown() = 0;
};

enum Behavior_order : uint32_t
{
    SIM_BEHAVIOR_INPUT = 0,
    SIM_BEHAVIOR_LOGIC,
    SIM_BEHAVIOR_ANIMATOR,
    SIM_BEHAVIOR_PRE_PHYSICS_UPDATE,

    NUM_SIM_BEHAVIOR_TYPES
};

constexpr uint32_t k_num_max_behavior_data_blocks{ 1024 };

// Behavior interface: components of entities.
class Behavior_ifc
{
public:
    Behavior_ifc(Behavior_order order, Behavior_ifc& output_behavior);
    virtual ~Behavior_ifc();

    virtual void on_update(const void* input, void* output) = 0;

private:
    Behavior_order m_order;
    pool::elem_key_t m_input_data_key;   // Set automatically.
    pool::elem_key_t m_output_data_key;  // From input of output behavior.
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

    static pool::elem_key_t allocate_one();
    static bool destroy_one(pool::elem_key_t key);
    static Behavior_data_w_version* get_one_from_key(pool::elem_key_t key);

    template<class T>
    const T& read_data();

    template<class T>
    void write_data(T&& data);

private:
    Behavior_data_w_version();
    ~Behavior_data_w_version() = default;
    
    static constexpr size_t k_behavior_data_block_size{ sizeof(float_t) * 3 };  // A vec3 (example benchmark data amount).
    using Data_block = uint8_t[k_behavior_data_block_size];

    Data_block m_data;
    uint32_t m_version{ (uint32_t)-1 };

    static constexpr uint8_t k_unreserved{ 0 };
    static constexpr uint8_t k_setup_reservation{ 1 };
    static constexpr uint8_t k_reserved{ 2 };
    std::atomic_uint8_t m_reserved{ k_unreserved };
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
