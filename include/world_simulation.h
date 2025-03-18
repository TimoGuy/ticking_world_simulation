#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "multithreaded_job_system_public.h"
#include "simulating_entity_ifc.h"


constexpr uint32_t k_world_sim_hz{ 40 };
constexpr float_t k_world_sim_delta_time{ 1.0f / k_world_sim_hz };

class World_simulation : public Job_source
{
public:
    World_simulation(uint32_t num_threads);

    void add_sim_entity_to_world(std::unique_ptr<Simulating_entity_ifc>&& entity);
    void remove_entity_from_world(pool_elem_key_t entity_key);

private:
    // Job cycle:
    // - Setup.
    //   - Create Jolt Physics World.
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
            , m_elem_key(0)
        {
        }

        void set_elem_key(pool_elem_key_t new_key)
        {
            m_elem_key = new_key;
        }

        int32_t execute() override;

    private:
        World_simulation& m_world_sim;
        pool_elem_key_t m_elem_key;
    };
    std::vector<std::unique_ptr<J2_execute_simulation_tick_job>> m_j2_execute_simulation_tick_jobs;

    class J3_remove_pending_objs_job : public Job_ifc
    {
    public:
        J3_remove_pending_objs_job(
            World_simulation& world_sim,
            std::vector<pool_elem_key_t>& elem_keys_ref)
            : Job_ifc("World Simulation remove pending objs job", world_sim)
            , m_world_sim(world_sim)
            , m_elem_keys(elem_keys_ref)
        {
        }

        int32_t execute() override;

        World_simulation& m_world_sim;
        std::vector<pool_elem_key_t>& m_elem_keys;  // @TODO: remove or refactor.
    };
    std::unique_ptr<J3_remove_pending_objs_job> m_j3_remove_pending_objs_job;

    class J4_add_pending_objs_job : public Job_ifc
    {
    public:
        J4_add_pending_objs_job(
            World_simulation& world_sim,
            std::vector<std::unique_ptr<Simulating_entity_ifc>>& elems_ref)
            : Job_ifc("World Simulation add pending objs job", world_sim)
            , m_world_sim(world_sim)
            , m_elems(elems_ref)
        {
        }

        int32_t execute() override;

        World_simulation& m_world_sim;
        std::vector<std::unique_ptr<Simulating_entity_ifc>>& m_elems;  // @TODO: remove or refactor.
    };
    std::unique_ptr<J4_add_pending_objs_job> m_j4_add_pending_objs_job;

    // States.
    enum class Job_source_state : uint32_t
    {
        // Setup.
        SETUP_PHYSICS_WORLD = 0,

        // Main cycle.
        WAIT_UNTIL_TIMEOUT,
        EXECUTE_SIMULATION_TICKS,
        REMOVE_PENDING_OBJS,
        ADD_PENDING_OBJS,
        CHECK_FOR_SHUTDOWN_REQUEST,

        NUM_STATES
    };
    std::atomic<Job_source_state> m_current_state;
    Job_timekeeper m_timekeeper;
    std::atomic_bool m_rebuild_entity_list;
    Job_next_jobs_return_data fetch_next_jobs_callback() override;

    // Insertion and deletion queues.
    std::vector<std::unique_ptr<Simulating_entity_ifc>> m_insertion_queue;
    std::mutex m_insertion_queue_mutex;
    std::vector<pool_elem_key_t> m_deletion_queue;
    std::mutex m_deletion_queue_mutex;

    // Data pool.
    struct Data_with_version
    {
        std::unique_ptr<Simulating_entity_ifc> data{ nullptr };
        std::atomic_uint32_t version_num{ (uint32_t)-1 };
    };

    Simulating_entity_ifc* get_sim_entity(pool_elem_key_t key)
    {
        uint32_t idx{ key & 0x00000000ffffffff };
        uint32_t version_num{ (key & 0xffffffff00000000) >> 32 };
        
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
        pool_elem_key_t key{
            static_cast<uint64_t>(idx) |
            static_cast<uint64_t>(m_data_pool[idx].version_num.load()) << 32
        };
        return key;
    }

    bool delete_sim_entity(pool_elem_key_t key)
    {
        uint32_t idx{ key & 0x00000000ffffffff };
        uint32_t version_num{ (key & 0xffffffff00000000) >> 32 };
        
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

    // Physics system.
    std::unique_ptr<JPH::PhysicsSystem> m_physics_system;
};
