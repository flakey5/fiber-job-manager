#include <cstdio>
#include <chrono>
#include <thread>
#include "fiber-job-manager/job_manager.hpp"
#include "fiber-job-manager/promise.hpp"

static Promise<char, int8_t> doSomethingAsync() {
    return Promise<char, int8_t>([](auto resolve, auto reject) {
        printf("doSomethingAsync start\n");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        printf("doSomethingAsync stop\n");

        resolve('a');
    });
}

int main() {
    JobManager::init({
        .workerThreadCount = 4,
        .fibersPerThread = 4,
    });

    // TODO: this is needed because there's a little bit of a race condition
    //  where the threads aren't setup yet and aren't waiting for jobs
    std::this_thread::sleep_for(std::chrono::seconds(5));

    JobManager::queue([]() {
        printf("before doSomethingAsync\n");
        auto result = doSomethingAsync().await();
        printf("after doSomethingAsync\n");
    });

    while (true);
}
