#pragma once

#include <cstdint>
#include <functional>
#include <variant>
#include "fiber-job-manager/job_manager.hpp"

enum PromiseStatus : int8_t {
    // Promise was rejected with an error
    PROMISE_REJECTED = -1,
    // Promise hasn't finished 
    PROMISE_INCOMPLETE = 0,
    // Promise successfully resolved
    PROMISE_RESOLVED = 1,
};

// Called when a promise successfully resolves
template<typename T>
using PromiseResolve = std::function<void(T&&)>;

// Called to reject a promise
template<typename E>
using PromiseReject = std::function<void(E)>;

template<typename T, typename E>
using PromiseBody = std::function<void(PromiseResolve<T>&, PromiseReject<E>&)>;

template<typename T, typename E>
class Promise {
public:
    using Result = std::variant<T, E>;

private:
    PromiseStatus status = PROMISE_INCOMPLETE;
    
    Result result;
    void* waitingFiber = nullptr;

public:
    Promise(PromiseBody<T, E> body) {
        QueuedJob job = [this, body]() {
            PromiseResolve<T> resolve = [this](T&& value) {
                this->status = PROMISE_RESOLVED;
                this->result = std::forward<T>(value);

                if (this->waitingFiber != nullptr) {
                    JobManager::switchToFiber(this->waitingFiber, true);
                }
            };

            PromiseReject<E> reject = [this](E error) {
                this->status = PROMISE_REJECTED;
                this->result = error;

                if (this->waitingFiber != nullptr) {
                    JobManager::switchToFiber(this->waitingFiber, true);
                }
            };

            body(resolve, reject);
        };

        JobManager::queue(job);
    }

    // No copy
    Promise(const Promise&) = delete;

    inline PromiseStatus getStatus() const {
        return this->status;
    }

    /**
     * Has the promise been resolved or rejected?
     */
    inline bool hasCompleted() const {
        return this->status != PROMISE_INCOMPLETE;
    }

    inline bool isResolved() const {
        return this->status == PROMISE_RESOLVED;
    }

    inline bool isRejected() const {
        return this->status == PROMISE_REJECTED;
    }

    Result& await() {
        if (!this->hasCompleted()) {
            this->waitingFiber = JobManager::getCurrentFiber();
            JobManager::sleepCurrentFiber();
        }

        return this->result;
    }
};
