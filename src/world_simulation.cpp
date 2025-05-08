#include "world_simulation.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include "simulating_ifc.h"
#include "world_simulation_settings.h"


World_simulation::World_simulation(std::atomic_size_t& num_job_sources_setup_incomplete,
                                   uint32_t num_threads)
    : m_num_job_sources_setup_incomplete(num_job_sources_setup_incomplete)
    , m_s1_create_jolt_physics_world(
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

simulating::Edit_behavior_groups_ifc::behavior_group_key_t
    World_simulation::add_behavior_group(Behavior_group&& group)
{
    std::lock_guard<std::mutex> lock{ m_behavior_pool_mutex };
    behavior_group_key_t key{ m_behavior_pool_key_generator++ };
    m_behavior_pool.emplace(key, std::move(group));
    return key;
}

void World_simulation::remove_behavior_group(behavior_group_key_t group_key)
{
    std::lock_guard<std::mutex> lock{ m_behavior_pool_mutex };

    if (m_behavior_pool.find(group_key) != m_behavior_pool.end())
    {
        m_behavior_pool.erase(group_key);
    }
    else
    {
        // Key not found.
        assert(false);
    }
}

// Jobs.
int32_t World_simulation::J2_execute_simulation_tick_job::execute()
{
    // Execute all behavior groups.
    for (auto& behavior : *m_group_ptr)
    {
        behavior->on_update();
    }

    // Tick physics system.
    // @TODO: @NOCHECKIN: For some reason with this lock, the update function works? Is this an issue with the thread system's work??? Idk.
    //static std::mutex s_force_visibility_mutex;
    //std::lock_guard<std::mutex> lock{ s_force_visibility_mutex };

    // @NOTE: @FAILED: Tried doing a simple wait to see if that would be enough
    //   time for visibility to propagate, however, it needs to be the lock or
    //   else it does not work.  -Thea 2025/04/19
#if 0
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif  // 0

    m_world_sim.update_physics_system();

    // Propagate new simulated positions to transform holders.
    assert(false);  // @TODO.

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
            m_world_sim.m_entity_pool[i]->on_create(m_world_sim, i);
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
            m_current_state = Job_source_state::WAIT_FOR_GLOBAL_SETUP_COMPLETION;
            break;

        case Job_source_state::WAIT_FOR_GLOBAL_SETUP_COMPLETION:
        {
            // Mark setup as complete here by decrementing global counter.
            // When this counter reaches 0, then that means that all the job sources are
            // finished with their individual setups.
            // @INCOMPLETE: Create a better system.
            static std::atomic_bool s_mark_executed{ false };
            bool executed_expect{ false };
            if (s_mark_executed.compare_exchange_strong(executed_expect, true))
            {
                m_num_job_sources_setup_incomplete--;
            }

            if (m_num_job_sources_setup_incomplete == 0)
            {
                m_current_state = Job_source_state::WAIT_UNTIL_TIMEOUT;
            }
            break;
        }

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
            size_t i{ 0 };
            for (auto& behavior_group : m_behavior_pool)
            {
                m_j2_execute_simulation_tick_jobs[i]
                    ->set_behavior_group(&behavior_group.second);
                return_data.jobs.emplace_back(m_j2_execute_simulation_tick_jobs[i].get());
                i++;
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
