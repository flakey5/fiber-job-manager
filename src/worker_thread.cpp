#include "worker_thread.hpp"

#include <cassert>

#ifdef PLATFORM_WINDOWS
#   include <Windows.h>
#endif

WorkerThread::~WorkerThread() {
    if (this->thread != nullptr) {
        delete this->thread;
    }
}

void WorkerThread::init(WorkerThreadEntrypoint* entrypoint) {
    this->thread = new std::thread(entrypoint, this);
    this->thread->detach();
}

uint64_t WorkerThread::getThreadId() {
    assert("thread is null" && this->thread != nullptr);
    return GetThreadId(this->thread->native_handle());
}

bool WorkerThread::setAffinity(uint64_t mask) {
    DWORD_PTR result = SetThreadAffinityMask(GetCurrentThread(), mask);
    return result != 0;
}
