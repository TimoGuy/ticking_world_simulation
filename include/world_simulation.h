#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include "multithreaded_job_system_public.h"
#include "simulating_entity_ifc.h"


class World_simulation : public Job_source
{
public:
    World_simulation();

    void add_sim_entity_to_world(std::unique_ptr<Simulating_entity_ifc>&& entity);
    void remove_entity_from_world(pool_elem_key_t entity_key);

private:
    // Job cycle:
    //     1. Calculate start of next tick.
    //     2. Execute simulation ticks (multiple jobs per available object).
    //     3. Remove pending delete objects.
    //     4. Add pending addition objects.
    //     5. Idle until start of next tick.
    //     6. Jump to step 1.
    class J1_calc_start_next_tick_job : public Job_ifc
    {
    public:
        J1_calc_start_next_tick_job(Job_source& source, World_simulation& world_sim)
            : Job_ifc("World Simulation calc next tick start time job", source)
            , m_world_sim(world_sim)
        {
        }

        int32_t execute() override;

    private:
        World_simulation& m_world_sim;
    };

    class J2_execute_simulation_tick_job : public Job_ifc
    {
    public:
        J2_execute_simulation_tick_job(Job_source& source, World_simulation& world_sim, pool_elem_key_t key)
            : Job_ifc("World Simulation execute sim tick job", source)
            , m_world_sim(world_sim)
            , m_elem_key(key)
        {
        }

        int32_t execute() override;

    private:
        World_simulation& m_world_sim;
        pool_elem_key_t m_elem_key;
    };

    class J3_remove_pending_objs_job : public Job_ifc
    {
    public:
        J3_remove_pending_objs_job(Job_source& source, std::vector<pool_elem_key_t> keys)
            : Job_ifc("World Simulation remove pending objs job", source)
            , m_elem_keys(keys)
        {
        }

        int32_t execute() override;

    private:
        std::vector<pool_elem_key_t> m_elem_keys;
    };

    class J4_add_pending_objs_job : public Job_ifc
    {
    public:
        J4_add_pending_objs_job(Job_source& source, std::vector<pool_elem_key_t> keys)
            : Job_ifc("World Simulation add pending objs job", source)
            , m_elem_keys(keys)
        {
        }

        int32_t execute() override;

    private:
        std::vector<pool_elem_key_t> m_elem_keys;
    };

    std::vector<Job_ifc*> fetch_next_jobs_callback() override;

    // Next tick.
    Job_timekeeper m_timekeeper;

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

        auto& dwv{ m_data_pool[idx] };
        if (dwv.version_num != version_num)
            return nullptr;

        return dwv.data.get();
    }
    std::array<Data_with_version, 4096> m_data_pool;
};
