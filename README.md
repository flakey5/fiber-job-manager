# fiber-jobs-manager
A [Fiber-based](https://en.wikipedia.org/wiki/Fiber_(computer_science)) job manager written in C++.
Plus a [Promise-like](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise) api because why not.

Inspired by Naughty Dog's GDC 2015 talk available here https://www.gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine.

## Basic usage
For basic usage of both the JobManager and Promise api, look at [src/tester.cpp](./src/tester.cpp).

## How it works
 * Creates (`n` worker threads * `n` fibers per thread) fibers with default stack size of 2MiB
 * All fibers that are ready to execute a job are stored in a queue (`gFreeFibers`)
 * Creates `n` worker threads and locks them to the thread they're currently on
 * Worker threads sleep until a condition variable is fired (`gJobSignal`)
 * Worker thread pops job from the queue and pops a fiber from the `gFreeFibers` queue
 * Worker thread passes the job to the fiber to execute and then switches to it
 * Fiber returns back to the worker thread after the job is complete
 * Worker thread adds the fiber back to the `gFreeFibers` queue and moves onto next job

It is possible for a job to "sleep" when waiting for another job to complete with the Promise api.
When this happens, the fiber switches back to the worker thread but the worker thread does not add
the fiber back into the `gFreeFibers` queue and instead moves onto the next job. When a Promise
resolves or rejects and a fiber is waiting on it, the it will immediately switch back to it.

# Building
For now only Windows is supported but that might change later on, we'll see

 * Download the latest version of [Premake 5](https://premake.github.io/download)
 * Run `/path/to/premake5.exe vs2022`
 * Open solution with Visual Studio 2022 `.pmk/fiber-jobs-manager.sln`
