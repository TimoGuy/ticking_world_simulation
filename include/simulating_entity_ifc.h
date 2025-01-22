#pragma once

#include <cinttypes>


using pool_elem_key_t = uint64_t;

class Simulating_entity_ifc
{
public:
    virtual void on_sim_entity_start(pool_elem_key_t key) = 0;
    virtual void tick_sim_entity() = 0;
    virtual void on_sim_entity_destroy() = 0;
};
