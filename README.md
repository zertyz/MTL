# MTL

## MutuaTech's Template Library -- Efficient Data Structures, Event System, Instrumentation System &amp; Helper Macros for C++

On this package you will find:

  - **TimeMeasurements** -- efficient elapsed time measurements using RDTSC Intel instruction or the CCNT ARM register to get 100x (or more) faster time measurements than using an OS call to do it (the default in C++, which requires a context switch); 
  - **SpinLock** -- A flexible drop-in replacement for Mutex, with ~16x lower latency (when you choose the right spin algorithm for your hardware), with cheap instrumentation and debug options (zero cost if you don't use them);
  - Efficient and reentrant data structures **very hard to beat in performance**, using **atomic operations**:
     - **ReentrantNonBlockingStack32** -- a hard-to-beat (in performance) multi producer / multi consumer atomic stack with the following characteristics:
        - Lock-free (no mutexes or context switches) yet fully reentrant -- multiple threads may push and pop simultaneously, in any order;
        - 32-bit internal slot references, offering up to 2^32 (4,2 billion) slots;
        - mmap-ready, since all addresses are relative to a base pointer and all slots are indexes from there on;
        - Custom provided allocators -- the best performance comes with `ReentrantNonBlockingBaseStack` allocator;
        - Zero-cost callback hooks (constexpr callbacks) may be used to implement locking, allocators and other goodies;
     - **ReentrantNonBlockingQueue32** -- A hard-to-beat (in performance) multi producer/multi consumer atomic queue with the following characteristics:
        - Lock-free (no mutexes or context switches) yet fully reentrant -- multiple threads may enqueue and dequeue simultaneously, in any order;
        - Custom provided allocators -- the best performance comes with `ReentrantNonBlockingBaseStack` allocator;
        - Pointers assure enqueueing/dequeueing may occur in any order -- in opposition to a `RingBufferQueue`, which is faster, but only allows sequential enqueueing/dequeueing (meaning there must be only up to 2 threads: 1 to produce and 1 to consume);
        - 32-bit internal slot references, offering up to 2^32 (4,2 billion) slots;
        - mmap-ready, since all addresses are relative to a base pointer and all slots are indexes from there on;
        - Zero-cost callback hooks (constexpr callbacks) may be used to implement locking and other goodies
  - Efficient and reentrant allocators optimized for known object types, using atomic operations (to be used by queues, stacks, ...);
  - A very flexible and hard to beat in performance template based event system using these structures & allocators;
  - A zero-cost (\*) instrumentation system (logs and statistics) -- \* provided that you are already using the mentioned event system;
  - A set of useful header functions and classes to bring some of Java/C#/Lua/Python/Perl/D abstractions to C++ (BetterExceptions, ...).

# Data Structures
## Stacks
## Queues

# Testing Helpers

