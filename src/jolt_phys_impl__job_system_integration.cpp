#if 0

#include "jolt_phys_impl__job_system_integration.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cinttypes>
#include <thread>
#include "Jolt/Jolt.h"
#include "Jolt/Core/JobSystemWithBarrier.h"
#include "Jolt/Core/Profiler.h"
#include "Jolt/Core/FPException.h"


Job_system_integration::Job_system_integration(uint32_t in_max_jobs, uint32_t in_max_barriers, int32_t in_num_threads)
    : m_num_threads(in_num_threads)
{
    // @ASSERT: If this is a single-threaded application, use the single threaded job system!
    assert(m_num_threads > 1);

    init(in_max_jobs, in_max_barriers, m_num_threads);
}

Job_system_integration::~Job_system_integration()
{}

void Job_system_integration::init(uint32_t in_max_jobs, uint32_t in_max_barriers, int32_t in_num_threads)
{}

// See JobSystem
JPH::JobSystem::JobHandle Job_system_integration::CreateJob(const char *in_name,
                                                            JPH::ColorArg in_color,
                                                            const JobFunction &in_job_function,
                                                            uint32_t in_num_dependencies /*= 0*/)
{}

/// Change the max concurrency after initialization
void Job_system_integration::set_num_threads_usage(int in_num_threads) {
    // StopThreads(); StartThreads(in_num_threads);
    assert(false);
}

// See JobSystem
void Job_system_integration::QueueJob(Job *inJob)
{}

void Job_system_integration::QueueJobs(Job **inJobs, uint32_t inNumJobs)
{}

void Job_system_integration::FreeJob(Job *inJob)
{}

// private:
// /// Start/stop the worker threads
// void start_threads(int in_num_threads);
// void stop_threads();

inline uint32_t Job_system_integration::get_head() const
{

}

inline void Job_system_integration::queue_job_internal(Job *inJob)
{

}

#endif  // 0
