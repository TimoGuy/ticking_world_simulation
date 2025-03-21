#include "world_simulation.h"

#include <chrono>
#include <iostream>
#include "simulating_entity_ifc.h"


World_simulation::World_simulation(uint32_t num_threads)
    : m_s1_create_jolt_physics_world(
        std::make_unique<S1_create_jolt_physics_world>(*this, num_threads))
    , m_j3_remove_pending_objs_job(
        std::make_unique<J3_remove_pending_objs_job>(*this, m_deletion_queue))
    , m_j4_add_pending_objs_job(
        std::make_unique<J4_add_pending_objs_job>(*this, m_insertion_queue))
    , m_current_state(Job_source_state::SETUP_PHYSICS_WORLD)
    , m_timekeeper(k_world_sim_hz, true)
{
}

void World_simulation::add_sim_entity_to_world(std::unique_ptr<Simulating_entity_ifc>&& entity)
{
    std::lock_guard<std::mutex> lock{ m_insertion_queue_mutex };
    m_insertion_queue.emplace_back(std::move(entity));
}

void World_simulation::remove_entity_from_world(pool_elem_key_t entity_key)
{
    std::lock_guard<std::mutex> lock{ m_deletion_queue_mutex };
    m_deletion_queue.emplace_back(entity_key);
}

// Jobs.
int32_t World_simulation::J2_execute_simulation_tick_job::execute()
{
    m_world_sim.get_sim_entity(m_elem_key)->tick_sim_entity();
    return 0;
}

int32_t World_simulation::J3_remove_pending_objs_job::execute()
{
    std::lock_guard<std::mutex> lock1{ m_world_sim.m_deletion_queue_mutex };
    std::lock_guard<std::mutex> lock2{ m_world_sim.m_data_pool_mutex };
    for (auto key : m_world_sim.m_deletion_queue)
    {
        bool result{
            m_world_sim.delete_sim_entity(key)
        };
        if (!result)
            std::cerr << "ERROR: Deleting key " << key << " failed." << std::endl;
    }
    if (!m_world_sim.m_deletion_queue.empty())
        m_world_sim.m_rebuild_entity_list = true;

    m_world_sim.m_deletion_queue.clear();
    return 0;
}

int32_t World_simulation::J4_add_pending_objs_job::execute()
{
    std::lock_guard<std::mutex> lock1{ m_world_sim.m_insertion_queue_mutex };
    std::lock_guard<std::mutex> lock2{ m_world_sim.m_data_pool_mutex };

    uint32_t pool_idx_iter_pos{ 0 };
    for (auto& sim_entity_uptr : m_world_sim.m_insertion_queue)
    {
        bool result{
            m_world_sim.add_sim_entity(std::move(sim_entity_uptr), pool_idx_iter_pos)
        };
        if (!result)
            std::cerr << "ERROR: Inserting sim entity failed." << std::endl;
    }
    if (!m_world_sim.m_insertion_queue.empty())
        m_world_sim.m_rebuild_entity_list = true;

    m_world_sim.m_insertion_queue.clear();
    return 0;
}


// Job source callback.
Job_source::Job_next_jobs_return_data World_simulation::fetch_next_jobs_callback()
{
    Job_next_jobs_return_data return_data;

    switch (m_current_state)
    {
        case Job_source_state::SETUP_PHYSICS_WORLD:
            return_data.jobs.emplace_back(m_s1_create_jolt_physics_world.get());
            break;

        case Job_source_state::WAIT_UNTIL_TIMEOUT:
            //if ()  @TODO: add stop doing stuff condition here.
            // else
            if (m_timekeeper.check_timeout_and_reset())
            {
                m_current_state = Job_source_state::EXECUTE_SIMULATION_TICKS;
            }
            break;

        case Job_source_state::EXECUTE_SIMULATION_TICKS:
        {
            std::lock_guard<std::mutex> lock{ m_data_pool_mutex };
            
            if (m_rebuild_entity_list)
            {
                // Expand tick jobs buffer with empty tick jobs, don't contract.
                if (m_active_data_pool_indices.size() > m_j2_execute_simulation_tick_jobs.size())
                {
                    size_t num_new_jobs{
                        m_active_data_pool_indices.size() - m_j2_execute_simulation_tick_jobs.size()
                    };
                    m_j2_execute_simulation_tick_jobs.reserve(m_active_data_pool_indices.size());
                    for (size_t _ = 0; _ < num_new_jobs; _++)
                    {
                        m_j2_execute_simulation_tick_jobs.emplace_back(
                            std::make_unique<J2_execute_simulation_tick_job>(*this)
                        );
                    }
                }

                // Fill in tick job indices and add to jobs.
                size_t num_jobs{ m_active_data_pool_indices.size() };
                return_data.jobs.reserve(num_jobs);
                for (size_t i = 0; i < num_jobs; i++)
                {
                    auto pool_idx{ m_active_data_pool_indices[i] };
                    m_j2_execute_simulation_tick_jobs[i]->set_elem_key(
                        get_sim_entity_key_from_index(pool_idx)
                    );
                    return_data.jobs.emplace_back(m_j2_execute_simulation_tick_jobs[i].get());
                }

                m_rebuild_entity_list = false;
            }
            else
            {
                // Return currently built job list.
                return_data.jobs.reserve(m_j2_execute_simulation_tick_jobs.size());
                for (auto& job_uptr_ref : m_j2_execute_simulation_tick_jobs)
                {
                    return_data.jobs.emplace_back(job_uptr_ref.get());
                }
            }

            m_current_state = Job_source_state::REMOVE_PENDING_OBJS;
        }
        break;

        case Job_source_state::REMOVE_PENDING_OBJS:
            return_data.jobs.emplace_back(m_j3_remove_pending_objs_job.get());
            m_current_state = Job_source_state::ADD_PENDING_OBJS;
            break;

        case Job_source_state::ADD_PENDING_OBJS:
            return_data.jobs.emplace_back(m_j4_add_pending_objs_job.get());
            m_current_state = Job_source_state::WAIT_UNTIL_TIMEOUT;
            break;
    }

    return return_data;
}
