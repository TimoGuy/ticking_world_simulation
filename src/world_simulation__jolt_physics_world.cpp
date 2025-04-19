#include "world_simulation.h"

#define HAWSOO_USE_JOLT_MULTITHREADED_JOB_SYSTEM 0

#include <atomic>
#include <cassert>
#include <memory>  // std::unique_ptr
#include "jolt_physics_headers.h"
#include "Jolt/Core/JobSystemSingleThreaded.h"

#include "jolt_phys_impl__error_callbacks.h"
#if HAWSOO_USE_JOLT_MULTITHREADED_JOB_SYSTEM
#include "jolt_phys_impl__job_system_integration.h"
#endif  // HAWSOO_USE_JOLT_MULTITHREADED_JOB_SYSTEM
#include "jolt_phys_impl__layers.h"
#include "jolt_phys_impl__obj_vs_broad_phase_filter.h"
#include "jolt_phys_impl__custom_listeners.h"
#include "physics_objects.h"
#include "world_simulation_settings.h"


namespace
{

static std::atomic_bool s_is_initialized{ false };

static std::unique_ptr<JPH::Factory> s_jolt_factory;
static std::unique_ptr<JPH::TempAllocatorImpl> s_jolt_temp_allocator;
#if HAWSOO_USE_JOLT_MULTITHREADED_JOB_SYSTEM
static std::unique_ptr<Job_system_integration> s_job_system_integration;
#endif  // HAWSOO_USE_JOLT_MULTITHREADED_JOB_SYSTEM
static std::unique_ptr<JPH::JobSystemSingleThreaded> s_job_system_single_threaded;
static JPH::JobSystem* s_using_job_system_ptr{ nullptr };

static BP_layer_interface_impl s_broad_phase_layer_ifc_impl;
static Object_vs_broad_phase_layer_filter_impl s_obj_vs_broad_phase_layer_filter;
static Object_layer_pair_filter_impl s_obj_layer_pair_filter;

static My_body_activation_listener s_body_activation_listener;
static My_contact_listener s_contact_listener;

}  // namespace


// Jobs.
int32_t World_simulation::S1_create_jolt_physics_world::execute()
{
    bool expected{ false };
    if (!s_is_initialized.compare_exchange_strong(expected, true))
    {
        // Multiple initialization of physics system!!!! Should not happen.
        assert(false);
        return 1;
    }

    // Setup jolt physics.
    JPH::RegisterDefaultAllocator();

    JPH::Trace = trace_impl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = assert_failed_impl);

    s_jolt_factory = std::make_unique<JPH::Factory>();
    JPH::Factory::sInstance = s_jolt_factory.get();
    JPH::RegisterTypes();

    s_jolt_temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    constexpr uint32_t k_max_physics_jobs{ 2048 };
    constexpr uint32_t k_max_physics_barriers{ 8 };
    constexpr int32_t k_max_concurrency{ 16 };  // @NOTE: The point at which Jolt physics' multithreaded performance starts to degrade (w/ current version).  -Thea 2025/03/13

#if HAWSOO_USE_JOLT_MULTITHREADED_JOB_SYSTEM
    int32_t concurrency{
        std::min(
            static_cast<int32_t>(m_num_threads - 1),  // Subtract 1 due to `PhysicsSystem::Update()` thread blocking.
            k_max_concurrency) };
    if (concurrency > 1)
    {
        // @TODO: IMPLEMENT THE MULTITHREADED JOB SYSTEM INTO THE ENGINE JOB SYSTEM!!!!
        s_job_system_integration =
            std::make_unique<Job_system_integration>(k_max_physics_jobs,
                                                     k_max_physics_barriers,
                                                     concurrency);
        s_using_job_system_ptr = s_job_system_integration.get();
    }
    else
#else
    {
        s_job_system_single_threaded =
            std::make_unique<JPH::JobSystemSingleThreaded>(k_max_physics_jobs);
        s_using_job_system_ptr = s_job_system_single_threaded.get();
    }
#endif  // HAWSOO_USE_JOLT_MULTITHREADED_JOB_SYSTEM

    assert(s_using_job_system_ptr != nullptr);

    // Setup physics world.
    auto& phys_sys{ m_world_sim.m_physics_system };
    phys_sys = std::make_unique<JPH::PhysicsSystem>();

    constexpr uint32_t k_max_bodies{ 65536 };
    constexpr uint32_t k_num_body_mutexes{ 0 };  // Default settings is no mutexes to protect bodies from concurrent access.
    constexpr uint32_t k_max_body_pairs{ 65536 };
    constexpr uint32_t k_max_contact_constraints{ 10240 };
    phys_sys->Init(k_max_bodies,
                   k_num_body_mutexes,
                   k_max_body_pairs,
                   k_max_contact_constraints,
                   s_broad_phase_layer_ifc_impl,
                   s_obj_vs_broad_phase_layer_filter,
                   s_obj_layer_pair_filter);

    phys_sys->SetBodyActivationListener(&s_body_activation_listener);
    phys_sys->SetContactListener(&s_contact_listener);

    phys_sys->SetGravity(JPH::Vec3{ 0.0f, -37.5f, 0.0f });  // From previous project  -Thea 2025/03/13 (2023/09/29)

    phys_sys->OptimizeBroadPhase();

    phys_obj::set_references(phys_sys.get(), &phys_sys->GetBodyInterface());

    return 0;
}

// Physics system.
bool World_simulation::update_physics_system()
{
    // @THOUGHTS: Perhaps the reason why there's an error here is bc the move constructor is making copies of the physics objects???
    JPH::EPhysicsUpdateError error =
        m_physics_system->Update(k_world_sim_delta_time,
                                 1,
                                 s_jolt_temp_allocator.get(),
                                 s_using_job_system_ptr);

    if (error != JPH::EPhysicsUpdateError::None)
    {
        // Error occurred during physics update.
        assert(false);
        return false;
    }

    // Physics update completed successfully.
    return true;
}