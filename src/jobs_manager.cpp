#include "fiber-job-manager/job_manager.hpp"

#include <cassert>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include "fiber-job-manager/queued_job.hpp"
#include "fiber.hpp"
#include "worker_thread.hpp"

#ifdef PLATFORM_WINDOWS
#   define NOMINMAX
#   include <Windows.h>
#endif

static bool gIsInitialized = false;

// Lock used by worker threads in their entrypoint when waiting for new jobs
static std::mutex gWorkerThreadLock;

// All of the fibers
static Fiber* gFibers = nullptr;
// Fibers ready to execute a new job
static std::list<Fiber*> gFreeFibers;
// Lock used by worker threads in their entrypoint when waiting for free fibers
static std::mutex gFreeFibersLock;
// Fires when a fiber is freed and ready to execute a new job
static std::condition_variable gFiberFreedSignal;
// Maps a fiber's address to our representation of a fiber
static std::unordered_map<void*, Fiber*> gAddressToFiberMap;

// All of the worker threads
static WorkerThread* gWorkerThreads = nullptr;

// Jobs waiting to be executed
static std::list<QueuedJob> gJobQueue;
// Fires when a new job is queued or when a fiber is to be resumed
static std::condition_variable gJobSignal;

static void fiberEntrypoint(void* fiberPtr) {
    Fiber* fiber = reinterpret_cast<Fiber*>(fiberPtr);
    while (true) {
        if (fiber->isJobFinished()) {
            continue;
        }

        fiber->getJob()();
        fiber->setJobFinished(true);

        JobManager::switchToFiber(fiber->getThreadFiberAddress());
    }
}

static void workerThreadEntrypoint(WorkerThread* thread) {
    void* threadFiberAddress = JobManager::convertCurrentThreadToFiber(
        thread
    );
    assert(
        "workerThreadEntrypoint convert thread to fiber" &&
        threadFiberAddress
    );

    thread->setFiberAddress(threadFiberAddress);

    while (true) {
        // Wait for a new job if none are queued
        std::unique_lock lock(gWorkerThreadLock);
        gJobSignal.wait(lock);

        Fiber* fiber = nullptr;
        if (gJobQueue.size() != 0) {
            // There's a new job to execute
            QueuedJob job = gJobQueue.front();
            gJobQueue.pop_front();

            // No free fibers at this time, wait for one
            if (gFreeFibers.size() == 0) {
                std::unique_lock fiberLock(gFreeFibersLock);
                gFiberFreedSignal.wait(fiberLock);
            }

            // Grab the first free fiber
            fiber = gFreeFibers.front();
            gFreeFibers.pop_front();

            // Give it this job
            fiber->setJob(job);
        } else {
            assert("gJobSignal should not have fired" && false);
        }
        
        assert(fiber != nullptr);

        // Switch to the fiber so it can execute
        fiber->setJobFinished(false);
        fiber->setThreadFiberAddress(threadFiberAddress);
        JobManager::switchToFiber(fiber->getAddress());
        fiber->setThreadFiberAddress(nullptr);

        if (fiber->isJobFinished()) {
            // Job finished executing, return fiber back to the free queue
            gFreeFibers.push_back(fiber);
            gFiberFreedSignal.notify_one();
        }
    }
}

void JobManager::init(const JobManagerInitOptions& options) {
    assert("JobManager::init already called" && !gIsInitialized);

    uint32_t fiberCount = std::max(
        options.workerThreadCount * options.fibersPerThread,
        1u
    );
    gFibers = new Fiber[fiberCount];
    for (uint32_t i = 0; i < fiberCount; i++) {
        Fiber& fiber = gFibers[i];
        fiber.init(options.fiberStackSize, fiberEntrypoint);
        gFreeFibers.push_back(&fiber);
        gAddressToFiberMap[fiber.getAddress()] = &fiber;
    }

    gWorkerThreads = new WorkerThread[options.workerThreadCount];
    for (uint32_t i = 0; i < options.workerThreadCount; i++) {
        WorkerThread& workerThread = gWorkerThreads[i];
        workerThread.init(workerThreadEntrypoint);
        workerThread.setAffinity(static_cast<uint64_t>(1) << i);
    }

    gIsInitialized = true;
}

void JobManager::shutdown() {
    // TODO: should probably wait until no more jobs are running but it's fine
    delete[] gWorkerThreads;

    gFreeFibers.clear();
    delete[] gFibers;

    gIsInitialized = false;
}

void JobManager::queue(QueuedJob job) {
    assert(gIsInitialized == true);
    gJobQueue.push_back(job);
    gJobSignal.notify_one();
}

void* JobManager::getCurrentFiber() {
#ifdef PLATFORM_WINDOWS
    return GetCurrentFiber();
#else
#   error Unsupported platform
    return nullptr;
#endif
}

void* JobManager::convertCurrentThreadToFiber(void* data) {
#ifdef PLATFORM_WINDOWS
    LPVOID result = ConvertThreadToFiber(data);
    return result;
#else
#   error Unsupported platform
    return nullptr;
#endif
}

void JobManager::switchToFiber(
    void* fiberAddress,
    bool updateThreadAddress /* = false */
) {
    assert("JobManager::switchToFiber fiberAddress not null" && fiberAddress);

#ifdef PLATFORM_WINDOWS
    if (updateThreadAddress) {
        // We need to give the fiber we're switching to the executing thread's
        //  fiber address so we know where to switch to once the job finishes.
        //  This normally isn't a problem when executing jobs on the main
        //  thread, but when awaiting Promises we sleep the initial job and
        //  that means when the job eventually resumes it's on a different
        //  thread
        auto fiber = reinterpret_cast<Fiber*>(GetFiberData());
        assert("JobManager::switchToFiber current fiber data null" && fiber);

        void* threadFiberAddress = fiber->getThreadFiberAddress();

        gAddressToFiberMap[fiberAddress]->setThreadFiberAddress(
            threadFiberAddress
        );
    }

    SwitchToFiber(fiberAddress);
#else
#   error Unsupported platform
#endif
}

void JobManager::sleepCurrentFiber() {
    auto fiber = reinterpret_cast<Fiber*>(GetFiberData());
    assert("JobManager::sleepCurrentFiber fiber data null" && fiber);
    
    JobManager::switchToFiber(fiber->getThreadFiberAddress());
}
