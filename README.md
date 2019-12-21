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
     - **RingBufferQueue** -- *draft: will a ring buffer queue be always non-zero-copy? we may do it like we have today on event framework, which might not be efficient... it will, nonetheles, be dequeued in the reservation order* / *ring buffer queues might have multiple consumers... each one with their own head?* / *if we drop the zero-copy requisite, multiple producers may insert (using a pointer to the data), although it may lead to false-sharing performance degradation -- also, the RingBufferQueue may provide a single or several tails -- each tail may be consumed by several threads, but multiple tails may be used so each tail will never lose an event.
  - Efficient and reentrant allocators optimized for known object types, using atomic operations (to be used by queues, stacks, ...);
  - **MCSTL** -- *Mutua's Client/Server Template Library* -- Flexible and fast; binary or text, client/server facility, featuring zero-copy and the ability to serve, in a single thread, a huge number of connections with very little overhead (+1M connections were achieved on the little Raspberry Pi 1, 512MiB of RAM). A simple, but fast and flexible HTTP/HTTPS server is provided as well, for creating embedded servers with embedded content, with authentication and RESTful operations;
  - **METL** -- *Mutua's Event Template Library* -- A very flexible and hard to beat in performance template based event system using these structures & allocators;
  - A zero-cost (\*) instrumentation system (logs and statistics) -- \* provided that you are already using the mentioned event system;
  - A set of useful header functions and classes to bring some of Java/C#/Lua/Python/Perl/D abstractions to C++ (BetterExceptions, ...).

# Data Structures
# Stacks
# Queues

# METL -- *Mutua's Event Template Library*

This is our response to the event based software development paradigm -- simple, flexible and fast. The main analogy we use is that, for event based software, we have:
  1. **Producers** of events;
  2. **Listeners** or even **Consumers** for those events;
  3. **'EventLinker's**, whose responsability is to deliver the messages -- and here lies the all the abstraction you software may benefit from;
  4. **EventProcessors**, which are very flexible and configurable ways in which you will propagate these events, designed for optimal execution.

Our **'EventLinker's** allow your software to abstract wether it will be:
  * Single or Multi-threaded
  * Local or distributed
  * Backed by persistent media or just RAM memory
    - If backed by persistent media, will it be logged or recycled?

The **'EventLinker's** are:
  * **DirectEventLinker** -- simply do the calls, wether to listeners or consumers, as soon as the event is ready (as soon as it is delivered). This 'EventLinker' is, therefore, single-threaded and may be zero-cost (no function calls cost) if the source for the listeners and consumers are available at compile time and may be inlined by the compiler. When designing new software, debugging old ones or simply measuring the multi-thread overhead, this 'EventLinker' is very useful. 
  * **PointerQueueEventLinker** -- this is the main 'EventLinker' when performance metters: backed by the `PointerQueue32` data structure defined above, it allows multi-threaded with any number of producer threads / any number of listener & consumer threads;
  * **RingBufferQueueEventLinker** -- for particular cases where there will be just one producer thread and just one consumer / listener thread, this 'EventLinker' offers slighly better performance and better memory utilization than the above. It is useful, for instance, when transforming streams of raw data.
  * **LogQueueEventLinker** & **LogPointerQueueEventLinker** -- these 'EventLinker's are backed by a the corresponding log queues, which are designed store their slots in persistent media, using the `mmap` facility, not recycling any positions -- useful to keep permanent track of the submitted events. Regarding the reentrancy & thread safety, `LogQueue` is similar to `RingBufferQueue`: only 1 thread for producers and only 1 thread for listeners / consumers. `LogPointerQueue`, on the other hand, allows many threads for producers & many threads for listeners / consumers.
  * **DistributedEventLinker** -- uses **MCSTL** -- *Mutua's Client/Server Template Library* to allow producers & listeners/consumers to be on different machines;

After the communications between the producers and listeners/consumers are set, it is time to deal with the threads that will process the 'consumer' and 'listener' methods: **EventProcessor's** are the ones for this task -- they are the creators, destructors and responsible for the main loop of all processing threads. Note that event 'consumers' and 'listeners' differ from one another in the sense that an issued event may be listened by many parties, but will only be consumed by, at most, 1.

With **EventProcessor's** you may control:

  * Which *threads* will process which events -- even telling how many threads for consumers and how many threads for listeners for each event;
  * What is the CPU priority for each thread and other OS properties like affinity, IO priority;
  * How many listeners will be processed by each thread -- consumers of the same event are never more than 1 per thread;
  * For listeners running on the same thread, the LISTENER_PRIORITY is taken into account;
  * For events that share their consumer/listeners thread with other event's consumer/listeners, the EVENT_PRIORITY is taken into account.
  * When the threads are waiting for work to do, they will use the given **SpinLock** instance, which, depending on the chosen algorithm, may busy wait for a certain time (to reduce latency in case a new work arrives) and then, if there is still nothing to do, resort to a system call to lock the thread (either mutex or futex), until an unlock is issued.
  
For the sake of the event processing algorithm explanation, please bear with me that the "traditional" event processing loop be single threaded, because it is easier to implement, and may be described as the following steps:

  1. Dequeue the next issued event for a given event type
  2. Call the consumer method, if any
  3. Call all listener methods
  4. Free the resources occupied by the issued event

Since we allow consumers and listeners to be on different threads and also listeners to be split among more than one thread, we need a way to synchronize the freeing of resources occupied by each issued event -- with the processing happening in parallel. One possible algorithm for this sincronization, to run on each thread, would be:

  1. Dequeue an issued event from the event queue -- each thread having their own head
  2. Process the event, like the algorithm above -- each thread may have their own set of listeners
  3. Decrease the 'remaining processing threads counter' -- set to the number of threads when the event is added to the queue
  4. The thread that is able to CAS the counter to zero should free the issued event resources

When this is the case (when an event is processed by more than one thread), some queue structures may be more appropriate / more efficient than others:

  - A **LogQueue**, with one 'head' for each processing thread, don't require (4) -- freeing any resources -- nor (3) -- keeping track of the 'remaining processing threads counter'. Disk writes are a side-effect, with possible performance issues;
  - A **RingBufferQueue**, without zero-copy, also allows one 'head' for each processing thread, but both (3) and (4) steps are still necessary;
  - **PointerQueue**s, which is always zero-copy, cannot be used for it is not possible to have more than one 'head'
  - Hibrid 

The most efficient structure 

For events that are configured to have more than one processor thread, their dispatching will be controlled by another queue, accessed 

If an event have more than one thread for the listeners, the 'EventProcessor' wiil keep an structure for that event to tell how many threads are still dispatching that event to the listeners. Since the thread count will be set at the 'EventProcessor' instantiation, every issued event will start with that constant number (no less than 2) The thread that can do a CAS operation 

# Testing Helpers

