#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include "jolt_physics_headers.h"
#include "multithreaded_job_system_public.h"
#include "simulating_ifc.h"


class World_simulation : public Job_source, public simulating::Edit_behavior_groups_ifc
{
public:
    using Behavior_group = std::vector<std::unique_ptr<simulating::Behavior_ifc>>;

    World_simulation(std::atomic_size_t& num_job_sources_setup_incomplete,
                     uint32_t num_threads);

    void add_sim_entity_to_world(std::unique_ptr<simulating::Entity_ifc>&& entity);
    void remove_entity_from_world(size_t entity_idx);

    behavior_group_key_t add_behavior_group(Behavior_group&& group) override;
    void remove_behavior_group(behavior_group_key_t group_key) override;

private:

    std::atomic_size_t& m_num_job_sources_setup_incomplete;

    // Job cycle:
    // - Setup.
    //   - Create Jolt Physics World.
    //
    // - Main cycle.
    //   - Use timekeeper to find when next tick starts.
    //   - Execute simulation ticks (multiple jobs per available object).
    //   - Remove pending delete objects.
    //   - Add pending addition objects.
    //   - Check if should shut down.
    //   - Repeat.

    class S1_create_jolt_physics_world : public Job_ifc
    {
    public:
        S1_create_jolt_physics_world(World_simulation& world_sim, uint32_t num_threads)
            : Job_ifc("World Simulation setup jolt physics job", world_sim)
            , m_world_sim(world_sim)
            , m_num_threads(num_threads)
        {
        }

        int32_t execute() override;

    private:
        World_simulation& m_world_sim;
        uint32_t m_num_threads;
    };
    std::unique_ptr<S1_create_jolt_physics_world> m_s1_create_jolt_physics_world;

    class J2_execute_simulation_tick_job : public Job_ifc
    {
    public:
        J2_execute_simulation_tick_job(World_simulation& world_sim)
            : Job_ifc("World Simulation execute sim tick job", world_sim)
            , m_world_sim(world_sim)
            , m_group_ptr(nullptr)
        {
        }

        void set_behavior_group(Behavior_group* group_ptr)
        {
            m_group_ptr = group_ptr;
        }

        int32_t execute() override;

    private:
        World_simulation& m_world_sim;
        Behavior_group* m_group_ptr;
    };
    std::vector<std::unique_ptr<J2_execute_simulation_tick_job>> m_j2_execute_simulation_tick_jobs;

    class J3_remove_pending_objs_job : public Job_ifc
    {
    public:
        J3_remove_pending_objs_job(World_simulation& world_sim)
            : Job_ifc("World Simulation remove pending objs job", world_sim)
            , m_world_sim(world_sim)
        {
        }

        int32_t execute() override;

        World_simulation& m_world_sim;
    };
    std::unique_ptr<J3_remove_pending_objs_job> m_j3_remove_pending_objs_job;

    class J4_add_pending_objs_job : public Job_ifc
    {
    public:
        J4_add_pending_objs_job(World_simulation& world_sim)
            : Job_ifc("World Simulation add pending objs job", world_sim)
            , m_world_sim(world_sim)
        {
        }

        int32_t execute() override;

        World_simulation& m_world_sim;
    };
    std::unique_ptr<J4_add_pending_objs_job> m_j4_add_pending_objs_job;

    // States.
    enum class Job_source_state : uint32_t
    {
        // Setup.
        SETUP_PHYSICS_WORLD = 0,
        WAIT_FOR_GLOBAL_SETUP_COMPLETION,

        // Main cycle.
        WAIT_UNTIL_TIMEOUT,

        EXECUTE_LOGIC_UPDATE,  // Read input, logic step, calc skeletal anim bone matrices, write physics inputs, etc.
        STEP_PHYSICS_WORLD,    // Run physics world update procedure.

        REMOVE_PENDING_SIM_OBJS,
        ADD_PENDING_SIM_OBJS,
        CHECK_FOR_SHUTDOWN_REQUEST,

        NUM_STATES
    };
    std::atomic<Job_source_state> m_current_state;
    Job_timekeeper m_timekeeper;
    Job_next_jobs_return_data fetch_next_jobs_callback() override;

    // Insertion and deletion queues.
    std::vector<std::unique_ptr<simulating::Entity_ifc>> m_insertion_queue;
    std::mutex m_insertion_queue_mutex;
    std::vector<size_t> m_deletion_indices_queue;
    std::mutex m_deletion_indices_queue_mutex;

    static constexpr uint32_t k_num_max_entities{ 1024 };
    std::vector<std::unique_ptr<simulating::Entity_ifc>> m_entity_pool;
    std::mutex m_entity_pool_mutex;

    std::atomic_size_t m_behavior_pool_key_generator{ 0 };
    std::unordered_map<behavior_group_key_t, Behavior_group> m_behavior_pool;
    std::mutex m_behavior_pool_mutex;

    // Physics system.
    std::unique_ptr<JPH::PhysicsSystem> m_physics_system;

    bool update_physics_system();
};
