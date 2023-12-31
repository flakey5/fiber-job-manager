#pragma once

#include <atomic>
#include "fiber-job-manager/queued_job.hpp"

using FiberEntrypoint = void(void*);

class Fiber {
private:
    // Address of this fiber
    void* address;
    // Address of the current executing fiber
    void* threadFiberAddress = nullptr;
    std::atomic<bool> jobFinished = false;
    QueuedJob job;

public:
    Fiber() = default;
    ~Fiber();

    void init(size_t stackSize, FiberEntrypoint* entrypoint);

    inline void* getAddress() const {
        return this->address;
    }

    inline void* getThreadFiberAddress() const {
        return this->threadFiberAddress;
    }

    inline void setThreadFiberAddress(void* address) {
        this->threadFiberAddress = address;
    }

    inline bool isJobFinished() const {
        return this->jobFinished;
    }

    inline void setJobFinished(bool jobFinished) {
        this->jobFinished = jobFinished;
    }

    inline QueuedJob& getJob() {
        return this->job;
    }

    inline void setJob(QueuedJob job) {
        this->job = job;
    }
};
