#pragma once

#include <atomic>
#include <cinttypes>
#include "jolt_physics_headers.h"
#include "Jolt/Core/JobSystemWithBarrier.h"
#include "Jolt/Core/FixedSizeFreeList.h"
#include "Jolt/Core/Semaphore.h"


/// Implementation of a JobSystem using a thread pool
/// 
/// Note that this is considered an example implementation. It is expected that when you integrate
/// the physics engine into your own project that you'll provide your own implementation of the
/// JobSystem built on top of whatever job system your project uses.
class Job_system_integration final : public JPH::JobSystemWithBarrier
{
public:
    JPH_OVERRIDE_NEW_DELETE

    /// Creates a thread pool.
    /// @see Job_system_integration::Init
    Job_system_integration(uint32_t in_max_jobs, uint32_t in_max_barriers, int32_t in_num_threads);
    Job_system_integration() = default;
    virtual ~Job_system_integration() override;

    /// Initialize the thread pool
    /// @param inMaxJobs Max number of jobs that can be allocated at any time
    /// @param inMaxBarriers Max number of barriers that can be allocated at any time
    /// @param inNumThreads Number of threads to start (the number of concurrent jobs is 1 more because the main thread will also run jobs while waiting for a barrier to complete). Use -1 to autodetect the amount of CPU's.
    void init(uint32_t in_max_jobs, uint32_t in_max_barriers, int32_t in_num_threads);

    // See JobSystem
    virtual int GetMaxConcurrency() const override
    {
        return m_num_threads;
    }
    virtual JobHandle CreateJob(const char *in_name, JPH::ColorArg in_color, const JobFunction &in_job_function, uint32_t in_num_dependencies = 0) override;

    /// Change the max concurrency after initialization
    void set_num_threads_usage(int in_num_threads);
    
protected:
    // See JobSystem
    virtual void QueueJob(Job *inJob) override;
    virtual void QueueJobs(Job **inJobs, uint32_t inNumJobs) override;
    virtual void FreeJob(Job *inJob) override;

private:
    // Storage for concurrency.
    int32_t m_num_threads;

    /// Start/stop the worker threads
    void start_threads(int in_num_threads);
    void stop_threads();
    
    // /// Entry point for a thread
    // void ThreadMain(int inThreadIndex);

    /// Get the head of the thread that has processed the least amount of jobs
    inline uint32_t get_head() const;

    /// Internal helper function to queue a job
    inline void queue_job_internal(Job *inJob);

    /// Array of jobs (fixed size)
    using Available_jobs = JPH::FixedSizeFreeList<Job>;
    Available_jobs m_jobs;

    // The job queue
    static constexpr uint32_t k_queue_length{ 1024 };
    static_assert(JPH::IsPowerOf2(k_queue_length)); // We do bit operations and require queue length to be a power of 2
    std::atomic<Job*> m_queue[k_queue_length];

    // Head and tail of the queue, do this value modulo k_queue_length - 1 to get the element in the mQueue array
    std::atomic<uint32_t>* m_heads{ nullptr }; ///< Per executing thread the head of the current queue
    alignas(JPH_CACHE_LINE_SIZE) std::atomic<uint32_t> m_tail{ 0 }; ///< Tail (write end) of the queue

    // Semaphore used to signal worker threads that there is new work
    JPH::Semaphore m_semaphore;

    /// Boolean to indicate that we want to stop the job system
    std::atomic<bool> m_quit{ false };
};
