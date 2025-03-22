#pragma once

#include <cinttypes>


using pool_elem_key_t = uint64_t;

class Simulating_entity_ifc
{
public:
    // World simulation events.
    virtual void on_create(pool_elem_key_t key) = 0;
    virtual void on_teardown() = 0;
    // Renderer query event.
    virtual void on_renderer_transform_update() = 0;
};

enum Simulating_behavior_order : uint32_t
{
    SIM_BEHAVIOR_INPUT = 0,
    SIM_BEHAVIOR_LOGIC,
    SIM_BEHAVIOR_ANIMATOR,
    SIM_BEHAVIOR_PRE_PHYSICS_UPDATE,

    NUM_SIM_BEHAVIOR_TYPES
};

class Simulating_behavior_ifc
{
public:
    Simulating_behavior_ifc(Simulating_behavior_order order)
        : m_order(order)
    {
    }

    virtual void on_update() = 0;

protected:
    Simulating_behavior_order m_order;
};
