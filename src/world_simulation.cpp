#include "world_simulation.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include "simulating_ifc.h"


World_simulation::World_simulation(uint32_t num_threads)
    : m_s1_create_jolt_physics_world(
        std::make_unique<S1_create_jolt_physics_world>(*this, num_threads))
    , m_j3_remove_pending_objs_job(
        std::make_unique<J3_remove_pending_objs_job>(*this))
    , m_j4_add_pending_objs_job(
        std::make_unique<J4_add_pending_objs_job>(*this))
    , m_current_state(Job_source_state::SETUP_PHYSICS_WORLD)
    , m_timekeeper(k_world_sim_hz, true)
{
    // Init behavior data pool.
    simulating::Behavior_data_w_version::initialize_data_pool();

    // Init entity pool.
    std::lock_guard<std::mutex> lock{ m_entity_pool_mutex };
    m_entity_pool.reserve(k_num_max_entities);
    for (size_t i = 0; i < k_num_max_entities; i++)
    {
        m_entity_pool.emplace_back(nullptr);
    }
}

void World_simulation::add_sim_entity_to_world(std::unique_ptr<simulating::Entity_ifc>&& entity)
{
    std::lock_guard<std::mutex> lock{ m_insertion_queue_mutex };
    m_insertion_queue.emplace_back(std::move(entity));
}

void World_simulation::remove_entity_from_world(size_t entity_idx)
{
    std::lock_guard<std::mutex> lock{ m_deletion_indices_queue_mutex };
    m_deletion_indices_queue.emplace_back(entity_idx);
}

void World_simulation::add_behavior_group(std::vector<simulating::Behavior_ifc*>&& group)
{
    std::lock_guard<std::mutex> lock{ m_behavior_pool_mutex };
    m_behavior_pool.emplace_back(std::move(group));
}

void World_simulation::remove_behavior_group(std::vector<simulating::Behavior_ifc*>&& group)
{
    std::lock_guard<std::mutex> lock{ m_behavior_pool_mutex };

    bool erased{ false };
    for (size_t pool_idx = 0; pool_idx < m_behavior_pool.size(); pool_idx++)
    {
        auto& behavior_execution_list{ m_behavior_pool[pool_idx] };
        if (group.size() == behavior_execution_list.size())
        {
            bool matches{ true };
            for (size_t behavior_idx = 0;
                behavior_idx < group.size();
                behavior_idx++)
            {
                if (group[behavior_idx] != behavior_execution_list[behavior_idx])
                {
                    matches = false;
                    break;
                }
            }

            if (matches)
            {
                // Found correct behavior list to delete.
                m_behavior_pool.erase(
                    m_behavior_pool.begin() + pool_idx);
                erased = true;
                break;
            }
        }
    }

    assert(erased);
}

// Jobs.
int32_t World_simulation::J2_execute_simulation_tick_job::execute()
{
    // Execute all behaviors in order of the group.
    m_world_sim.m_behavior_pool_mutex.lock();
    auto& behavior_exec_grp{ m_world_sim.m_behavior_pool[m_entity_idx] };
    m_world_sim.m_behavior_pool_mutex.unlock();

    for (auto behavior : behavior_exec_grp)
    {
        behavior->on_update();
    }

    return 0;
}

int32_t World_simulation::J3_remove_pending_objs_job::execute()
{
    std::lock_guard<std::mutex> lock1{ m_world_sim.m_deletion_indices_queue_mutex };
    std::lock_guard<std::mutex> lock2{ m_world_sim.m_entity_pool_mutex };
    for (auto idx : m_world_sim.m_deletion_indices_queue)
    {
        auto& entity{ m_world_sim.m_entity_pool[idx] };
        entity->on_teardown(m_world_sim);
        entity = nullptr;
    }
    // if (!m_world_sim.m_deletion_indices_queue.empty())
    //     m_world_sim.m_rebuild_entity_list = true;

    m_world_sim.m_deletion_indices_queue.clear();
    return 0;
}

int32_t World_simulation::J4_add_pending_objs_job::execute()
{
    std::lock_guard<std::mutex> lock1{ m_world_sim.m_insertion_queue_mutex };
    std::lock_guard<std::mutex> lock2{ m_world_sim.m_entity_pool_mutex };

    uint32_t pool_idx_iter_pos{ 0 };
    for (auto& sim_entity_uptr : m_world_sim.m_insertion_queue)
    {
        bool inserted{ false };
        for (size_t i = 0; i < k_num_max_entities; i++)
        if (m_world_sim.m_entity_pool[i] == nullptr)
        {
            // Insert.
            m_world_sim.m_entity_pool[i] = std::move(sim_entity_uptr);
            m_world_sim.m_entity_pool[i]->on_create(i, m_world_sim);
            inserted = true;
            break;
        }

        if (!inserted)
        {
            std::cerr << "ERROR: Inserting sim entity failed." << std::endl;
            assert(false);
            break;
        }
    }
    // if (!m_world_sim.m_insertion_queue.empty())
    //     m_world_sim.m_rebuild_entity_list = true;

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
            m_current_state = Job_source_state::WAIT_UNTIL_TIMEOUT;
            break;

        case Job_source_state::WAIT_UNTIL_TIMEOUT:
            //if ()  @TODO: add stop doing stuff condition here.
            // else
            if (m_timekeeper.check_timeout_and_reset())
            {
                m_current_state = Job_source_state::EXECUTE_LOGIC_UPDATE;
            }
            break;

        case Job_source_state::EXECUTE_LOGIC_UPDATE:
        {
            std::lock_guard<std::mutex> lock{ m_behavior_pool_mutex };

            size_t num_behavior_grps{ m_behavior_pool.size() };
            size_t old_execute_grps{ m_j2_execute_simulation_tick_jobs.size() };
            if (num_behavior_grps > m_j2_execute_simulation_tick_jobs.size())
            {
                m_j2_execute_simulation_tick_jobs.reserve(num_behavior_grps);
                while (m_j2_execute_simulation_tick_jobs.size() < num_behavior_grps)
                {
                    m_j2_execute_simulation_tick_jobs.emplace_back(
                        std::make_unique<J2_execute_simulation_tick_job>(*this));
                }
            }

            return_data.jobs.reserve(num_behavior_grps);
            for (size_t i = 0; i < num_behavior_grps; i++)
            {
                m_j2_execute_simulation_tick_jobs[i]->set_entity_idx(i);
                return_data.jobs.emplace_back(m_j2_execute_simulation_tick_jobs[i].get());
            }

            m_current_state = Job_source_state::STEP_PHYSICS_WORLD;
        }
        break;

        case Job_source_state::STEP_PHYSICS_WORLD:

            m_current_state = Job_source_state::REMOVE_PENDING_SIM_OBJS;
            break;

        case Job_source_state::REMOVE_PENDING_SIM_OBJS:
            return_data.jobs.emplace_back(m_j3_remove_pending_objs_job.get());
            m_current_state = Job_source_state::ADD_PENDING_SIM_OBJS;
            break;

        case Job_source_state::ADD_PENDING_SIM_OBJS:
            return_data.jobs.emplace_back(m_j4_add_pending_objs_job.get());
            m_current_state = Job_source_state::WAIT_UNTIL_TIMEOUT;
            break;
    }

    return return_data;
}
