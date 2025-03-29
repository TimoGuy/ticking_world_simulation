#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include "jolt_physics_headers.h"
#include "multithreaded_job_system_public.h"
#include "simulating_ifc.h"


constexpr uint32_t k_world_sim_hz{ 50 };
constexpr float_t k_world_sim_delta_time{ 1.0f / k_world_sim_hz };

class World_simulation : public Job_source, public simulating::Edit_behavior_groups_ifc
{
public:
    World_simulation(uint32_t num_threads);

    void add_sim_entity_to_world(std::unique_ptr<simulating::Entity_ifc>&& entity);
    void remove_entity_from_world(size_t entity_idx);

    void add_behavior_group(std::vector<simulating::Behavior_ifc*>&& group) override;
    void remove_behavior_group(std::vector<simulating::Behavior_ifc*>&& group) override;

private:
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
            , m_entity_idx(0)
        {
        }

        void set_entity_idx(size_t new_entity_idx)
        {
            m_entity_idx = new_entity_idx;
        }

        int32_t execute() override;

    private:
        World_simulation& m_world_sim;
        size_t m_entity_idx;
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

    std::vector<std::vector<simulating::Behavior_ifc*>> m_behavior_pool;
    std::mutex m_behavior_pool_mutex;

#if 0
    // Data pool.
    void pool_elem_key_extract_data(pool_elem_key_t key,
                                    uint32_t& out_idx,
                                    uint32_t& out_version_num)
    {
        out_idx         = static_cast<uint32_t>(key & 0x00000000ffffffff);
        out_version_num = static_cast<uint32_t>((key & 0xffffffff00000000) >> 32);
    }

    pool_elem_key_t create_pool_elem_key(uint32_t idx, uint32_t version_num)
    {
        return static_cast<pool_elem_key_t>(
            static_cast<uint64_t>(idx) |
            static_cast<uint64_t>(version_num) << 32);
    }

    struct Data_with_version
    {
        std::unique_ptr<Simulating_entity_ifc> data{ nullptr };
        std::atomic_uint32_t version_num{ (uint32_t)-1 };
    };

    Simulating_entity_ifc* get_sim_entity(pool_elem_key_t key)
    {
        uint32_t idx, version_num;
        pool_elem_key_extract_data(key, idx, version_num);
        
        if (idx >= m_data_pool.size())
            return nullptr;

        auto& pool_elem{ m_data_pool[idx] };
        uint32_t pool_elem_version_num{ pool_elem.version_num.load() };
        if (pool_elem_version_num == (uint32_t)-1 ||
            pool_elem_version_num != version_num)
            return nullptr;

        return pool_elem.data.get();
    }

    pool_elem_key_t get_sim_entity_key_from_index(uint32_t idx)
    {
        return create_pool_elem_key(idx, m_data_pool[idx].version_num.load());
    }

    bool delete_sim_entity(pool_elem_key_t key)
    {
        uint32_t idx, version_num;
        pool_elem_key_extract_data(key, idx, version_num);
        
        if (idx >= m_data_pool.size())
            return false;

        // Remove elem from data pool and active indices list.
        auto& pool_elem{ m_data_pool[idx] };
        pool_elem.data.reset();
        pool_elem.data = nullptr;  // @CHECK: is this necessary?????
        m_active_data_pool_indices.erase(
            std::remove(
                m_active_data_pool_indices.begin(),
                m_active_data_pool_indices.end(),
                idx
            ),
            m_active_data_pool_indices.end()
        );
        return true;
    }

    bool add_sim_entity(std::unique_ptr<Simulating_entity_ifc>&& sim_entity_to_insert, uint32_t& pool_idx_ref)
    {
        bool found{ false };
        for (; pool_idx_ref < m_data_pool.size() && !found; pool_idx_ref++)
        {
            // Scan data pool for empty position.
            auto& pool_elem{ m_data_pool[pool_idx_ref] };
            if (pool_elem.version_num == (uint32_t)-1 ||
                pool_elem.data == nullptr)
            {
                // Add elem into data pool and active indices list.
                pool_elem.data = std::move(sim_entity_to_insert);
                pool_elem.version_num++;
                m_active_data_pool_indices.emplace_back(pool_idx_ref);
                found = true;
            }
        }
        return found;
    }

    std::array<Data_with_version, 4096> m_data_pool;
    std::vector<uint32_t> m_active_data_pool_indices;
    std::mutex m_data_pool_mutex;
#endif  // 0


    // Physics system.
    std::unique_ptr<JPH::PhysicsSystem> m_physics_system;
};
