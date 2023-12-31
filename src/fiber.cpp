#include "fiber.hpp"

#include <cassert>

#ifdef PLATFORM_WINDOWS
#   include <Windows.h>
#endif

Fiber::~Fiber() {
    if (this->address != nullptr) {
#ifdef PLATFORM_WINDOWS
        DeleteFiber(this->address);
#else
#   error Unsupported platform
#endif
    }
}

void Fiber::init(size_t stackSize, FiberEntrypoint* entrypoint) {
    this->address = CreateFiber(stackSize, entrypoint, this);
    assert("Fiber::init create fiber failed" && this->address != nullptr);
}
