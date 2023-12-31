#pragma once

#include <thread>
#include "fiber.hpp"

class WorkerThread;
using WorkerThreadEntrypoint = void(WorkerThread*);

/**
 * Light wrapper around std::thread to couple it with the fiber address
 *  and to provide some utility methods
 */
class WorkerThread {
private:
    // The thread instance
    std::thread* thread = nullptr;
    // The fiber address for returning to the thread's entrypoint loop
    void* fiberAddress = nullptr;

public:
    ~WorkerThread();

    /**
     * Creates and detaches the thread
     */
    void init(WorkerThreadEntrypoint* entrypoint);

    inline void* getFiberAddress() {
        return this->fiberAddress;
    }

    inline void setFiberAddress(void* fiberAddress) {
        this->fiberAddress = fiberAddress;
    }

    uint64_t getThreadId();

    /**
     * Set the thread affinity
     * @param mask Mask of which threads to lock to
     *
     * @returns True if successful
     */
    bool setAffinity(uint64_t mask);
};
