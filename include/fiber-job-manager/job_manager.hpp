#pragma once

#include "fiber-job-manager/queued_job.hpp"

struct JobManagerInitOptions {
    // The amount of worker threads to make
    uint32_t workerThreadCount = 4;
    // Amount of fibers to make for each thread, will make
    //  workerThreads*fibersPerThreads amount of threads
    uint32_t fibersPerThread = 16;
    // Amount of bytes allocated for a fiber's stack
    uint64_t fiberStackSize = 2u * 1024 * 1024 * 1024; // 2MiB
};

class JobManager {
public:
    JobManager() = delete;
    JobManager(const JobManager&) = delete;
    JobManager(JobManager&&) = delete;

    /**
     * Initialize the job manager, call at program startup or whenever
     *  just as long as it's before calling other functions
     */
    static void init(const JobManagerInitOptions& options);

    /**
     * Shutdown the job manager, call when you're done using this
     */
    static void shutdown();

    /**
     * Queue a new job to be ran
     * @param Job to be ran
     */
    static void queue(QueuedJob job);

    /**
     * Get the address of the fiber calling this function
     */
    static void* getCurrentFiber();

    /**
     * Convert the executing thread to a fiber
     * @param data The data to pass into the fiber
     */
    static void* convertCurrentThreadToFiber(void* data);

    /**
     * Switch the currently executing fiber to another
     * @param fiberAddress Address of the fiber to switch to
     * @param updateThreadAddress If this fiber was possibly executing on a
     *  different thread, set this to true
     */
    static void switchToFiber(void* fiberAddress, bool updateThreadAddress = false);

    /**
     * Put the current fiber to "sleep". Stops its execution so it can resume
     *  at a later point in time
     */
    static void sleepCurrentFiber();
};


