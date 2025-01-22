#include "world_simulation.h"

#include <chrono>
#include "simulating_entity_ifc.h"


World_simulation::World_simulation()
    : m_timekeeper(50, true)
{
}

void World_simulation::add_sim_entity_to_world(std::unique_ptr<Simulating_entity_ifc>&& entity)
{
    std::lock_guard<std::mutex> lock{ m_insertion_queue_mutex };
    m_insertion_queue.emplace_back(entity);
}

void World_simulation::remove_entity_from_world(pool_elem_key_t entity_key)
{
    std::lock_guard<std::mutex> lock{ m_deletion_queue_mutex };
    m_deletion_queue.emplace_back(entity_key);
}

// Jobs.
int32_t World_simulation::J1_calc_start_next_tick_job::execute()
{
    m_world_sim
    return 0;
}

int32_t World_simulation::J2_execute_simulation_tick_job::execute()
{
    m_world_sim.get_sim_entity(m_elem_key)->tick_sim_entity();
    return 0;
}

int32_t World_simulation::J3_remove_pending_objs_job::execute()
{
    return 0;
}

int32_t World_simulation::J4_add_pending_objs_job::execute()
{
    return 0;
}


// Job source callback.
std::vector<Job_ifc*> World_simulation::fetch_next_jobs_callback()
{
    if (m_timekeeper.check_timeout_and_reset())
    {
        // Start the next cycle right here!!!!!!
        // @TODO: START HERE!!!!!
    }
    return {};
}
