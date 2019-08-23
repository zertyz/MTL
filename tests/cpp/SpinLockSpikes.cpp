#include <iostream>
#include <iomanip>
#include <atomic>
#include <thread>
#include <cstring>
#include <mutex>

#include "../../cpp/thread/SpinLock.hpp"

// compile & run with clear; echo -en "\n\n###############\n\n"; echo (clan)g++ -std=c++17 -O3 -mcpu=native -march=native -mtune=native -pthread SpinLockSpikes.cpp -o SpinLockSpikes && sudo sync && sleep 2 && pkill -stop -f visual-studio-code && pkill -stop -f chrome && time nice -n 20 ./SpinLockSpikes; pkill -cont -f visual-studio-code; pkill -cont -f chrome

#define DOCS "spikes on 'SpinLock'\n"                                        \
             "====================\n"                                        \
             "\n"                                                            \
             "This is the helper program used to develop 'SpinLock.hpp',\n"  \
             "aiming at measuring and stressing the hardware in the\n"       \
             "following comparisons:\n"                                      \
             "  - producer/consumer latency using a regular mutex;\n"        \
             "  - producer/consumer latency using our spin-lock;\n"          \
             "  - linear processing -- the same thread 'produces' and\n"     \
             "    'consumes', without using mutexes or spin-locks;\n"        \
             "  --> we have two versions of the 'linear processing': one\n"  \
             "      using atomics (so we can measure the thread overhead)\n" \
             "      and another not using atomics, so we have a baseline\n"  \
             "      for how fast can our hardware do.\n"


#define NUMBER_OF_EVENTS_PER_MEASURE 2143657
#define N_THREADS                    (std::thread::hardware_concurrency()-1)    // remember we have 1 thread for the consumer

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define PAD(_number, _width)         std::setfill(' ') << std::setw(_width) << std::fixed << (_number)


#define DEBUG_PRODUCER     std::this_thread::sleep_for(std::chrono::milliseconds(1000)); \
                           std::cerr << "produced="<<producerCount<<"; consumed="<<consumerCount<<std::endl;


// test section
///////////////

static struct timespec timespec_now;
static inline unsigned long long getMonotonicRealTimeNS() {
    clock_gettime(CLOCK_MONOTONIC, &timespec_now);
    return (timespec_now.tv_sec*1000000000ll) + timespec_now.tv_nsec;
}


// spike vars
/////////////

std::atomic<unsigned> producerCount;
std::atomic<unsigned> consumerCount;

std::atomic_bool stopConsumers           = false;
std::atomic_int  numberOfActiveConsumers = 0;


std::mutex producingMutex;
std::mutex consumingMutex;

mutua::MTL::SpinLock<true> producingRelaxSpinLock;
mutua::MTL::SpinLock<true> consumingRelaxSpinLock;

mutua::MTL::SpinLock<false> producingBusySpinLock;
mutua::MTL::SpinLock<false> consumingBusySpinLock;

// info section
///////////////

void check(unsigned long long& elapsed) {
    if ( (producerCount == NUMBER_OF_EVENTS_PER_MEASURE) && (consumerCount  == NUMBER_OF_EVENTS_PER_MEASURE) ) {
        std::cout << "--> Checking PASSED for " << NUMBER_OF_EVENTS_PER_MEASURE << " events! --> " << PAD(elapsed / NUMBER_OF_EVENTS_PER_MEASURE, 4) << "ns per event\n\n";
    } else {
        std::cout << "--> Checking FAILED: only " << producerCount << " events were produced, " << consumerCount << " consumed for a total of " << NUMBER_OF_EVENTS_PER_MEASURE << " expected...\n\n";
    }
}


// spike methods
////////////////


void reset() {
    producerCount.store(0, std::memory_order_release);
    consumerCount.store(0, std::memory_order_release);
    stopConsumers.store(false, std::memory_order_release);
    numberOfActiveConsumers.store(0, std::memory_order_release);

    // consumers have nothing to consume until a new event arrives...
    consumingMutex.try_lock();
    consumingRelaxSpinLock.try_lock();
    consumingBusySpinLock.try_lock();
}


void mutexProducer() {
    for (unsigned i=0; i<NUMBER_OF_EVENTS_PER_MEASURE; i++) {
        producingMutex.lock();
        producerCount.fetch_add(1, std::memory_order_acq_rel);
        consumingMutex.unlock();
    }
    // ask consumers to stop & wait for them
    producingMutex.lock();
    stopConsumers.store(true, std::memory_order_release);
    while (numberOfActiveConsumers.load(std::memory_order_acquire) > 0) consumingMutex.unlock();
    producingMutex.unlock();
}

void mutexConsumer() {
    numberOfActiveConsumers.fetch_add(1, std::memory_order_acq_rel);
    while (true) {
        consumingMutex.lock();

        if (unlikely (stopConsumers.load(std::memory_order_acquire)) ) {
            consumingMutex.unlock();
            break;
        }

        consumerCount.fetch_add(1, std::memory_order_acq_rel);
        producingMutex.unlock();
    }
    numberOfActiveConsumers.fetch_sub(1, std::memory_order_acq_rel);
}


void relaxSpinLockProducer() {
    for (unsigned i=0; i<NUMBER_OF_EVENTS_PER_MEASURE; i++) {
        producingRelaxSpinLock.lock();
        producerCount.fetch_add(1, std::memory_order_acq_rel);
        consumingRelaxSpinLock.unlock();
    }
    // ask consumers to stop & wait for them
    producingRelaxSpinLock.lock();
    stopConsumers.store(true, std::memory_order_release);
    while (numberOfActiveConsumers.load(std::memory_order_acquire) > 0) consumingRelaxSpinLock.unlock();
    producingRelaxSpinLock.unlock();
}

void relaxSpinLockConsumer() {
    numberOfActiveConsumers.fetch_add(1, std::memory_order_acq_rel);
    while (true) {
        consumingRelaxSpinLock.lock();

        if (unlikely (stopConsumers.load(std::memory_order_acquire)) ) {
            consumingRelaxSpinLock.unlock();
            break;
        }

        consumerCount.fetch_add(1, std::memory_order_acq_rel);
        producingRelaxSpinLock.unlock();
    }
    numberOfActiveConsumers.fetch_sub(1, std::memory_order_acq_rel);
}


void busySpinLockProducer() {
    for (unsigned i=0; i<NUMBER_OF_EVENTS_PER_MEASURE; i++) {
        producingBusySpinLock.lock();
//        DEBUG_PRODUCER
        producerCount.fetch_add(1, std::memory_order_acq_rel);
        consumingBusySpinLock.unlock();
    }
    // ask consumers to stop & wait for them
    producingBusySpinLock.lock();
    stopConsumers.store(true, std::memory_order_release);
    while (numberOfActiveConsumers.load(std::memory_order_acquire) > 0) consumingBusySpinLock.unlock();
    producingBusySpinLock.unlock();
}

void busySpinLockConsumer() {
    numberOfActiveConsumers.fetch_add(1, std::memory_order_acq_rel);
    while (true) {
        consumingBusySpinLock.lock();

        if (unlikely (stopConsumers.load(std::memory_order_acquire)) ) {
            consumingBusySpinLock.unlock();
            break;
        }

        consumerCount.fetch_add(1, std::memory_order_acq_rel);
        producingBusySpinLock.unlock();
    }
    numberOfActiveConsumers.fetch_sub(1, std::memory_order_acq_rel);
}


void busyWaitProducer() {
    for (unsigned i=0; i<NUMBER_OF_EVENTS_PER_MEASURE; i++) {
        while (producerCount.load(std::memory_order_acquire) > consumerCount.load(std::memory_order_acquire)) ;     // the busy wait loop
        producerCount.fetch_add(1, std::memory_order_acq_rel);
//        DEBUG_PRODUCER

    }
    // ask consumers to stop & wait for them
    stopConsumers.store(true, std::memory_order_release);
    while (numberOfActiveConsumers.load(std::memory_order_acquire) > 0) ;
}

void busyWaitConsumer() {
    numberOfActiveConsumers.fetch_add(1, std::memory_order_acq_rel);
    while ( (!stopConsumers.load(std::memory_order_acquire)) ||
            (producerCount.load(std::memory_order_acquire) > consumerCount.load(std::memory_order_acquire)) ) {
        unsigned targetConsumerCount = producerCount.load(std::memory_order_acquire)-1;
        consumerCount.compare_exchange_strong(targetConsumerCount, consumerCount.load(std::memory_order_acquire)+1);
    }
    numberOfActiveConsumers.fetch_sub(1, std::memory_order_acq_rel);
}

void linearProducerConsumer() {
    // simulates the work the others producers / consumers do: simply increase their counters
    for (unsigned i=0; i<NUMBER_OF_EVENTS_PER_MEASURE; i++) {
        producerCount.fetch_add(1, std::memory_order_acq_rel);
        consumerCount.fetch_add(1, std::memory_order_acq_rel);
    }
}

int main(void) {

	std::cout << DOCS;

    unsigned long long start;
    unsigned long long elapsed;

    std::thread threads[N_THREADS];

    // formatted output
    std::cout.imbue(std::locale(""));


    std::cout << "\n\nStaring the 'mutexProducer' / 'mutexConsumer' measurements: " << std::flush;
    reset();
	for (unsigned _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
		threads[_threadNumber] = std::thread(mutexConsumer);
	}
    start = getMonotonicRealTimeNS();
    mutexProducer();
    elapsed = getMonotonicRealTimeNS() - start;
    for (int _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
        threads[_threadNumber].join();
    }
    std::cout << PAD(elapsed, 6) << "ns\n";
    check(elapsed);


    std::cout << "\n\nStaring the 'relaxSpinLockProducer' / 'relaxSpinLockConsumer' measurements: " << std::flush;
    reset();
	for (unsigned _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
		threads[_threadNumber] = std::thread(relaxSpinLockConsumer);
	}
    start = getMonotonicRealTimeNS();
    relaxSpinLockProducer();
    elapsed = getMonotonicRealTimeNS() - start;
    for (int _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
        threads[_threadNumber].join();
    }
    std::cout << PAD(elapsed, 6) << "ns\n";
    check(elapsed);


    std::cout << "\n\nStaring the 'busySpinLockProducer' / 'busySpinLockConsumer' measurements: " << std::flush;
    reset();
	for (unsigned _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
		threads[_threadNumber] = std::thread(busySpinLockConsumer);
	}
    start = getMonotonicRealTimeNS();
    busySpinLockProducer();
    elapsed = getMonotonicRealTimeNS() - start;
    for (int _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
        threads[_threadNumber].join();
    }
    std::cout << PAD(elapsed, 6) << "ns\n";
    check(elapsed);


    std::cout << "\n\nStaring the 'busyWaitProducer' / 'busyWaitConsumer' measurements: " << std::flush;
    reset();
	for (unsigned _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
		threads[_threadNumber] = std::thread(busyWaitConsumer);
	}
    start = getMonotonicRealTimeNS();
    busyWaitProducer();
    elapsed = getMonotonicRealTimeNS() - start;
    for (int _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
        threads[_threadNumber].join();
    }
    std::cout << PAD(elapsed, 6) << "ns\n";
    check(elapsed);


    std::cout << "\n\nStaring the 'linearProducerConsumer' (no threads) measurements: " << std::flush;
    reset();
    start = getMonotonicRealTimeNS();
    linearProducerConsumer();
    elapsed = getMonotonicRealTimeNS() - start;
    std::cout << PAD(elapsed, 6) << "ns\n";
    check(elapsed);


    start = getMonotonicRealTimeNS();
    elapsed = getMonotonicRealTimeNS() - start;
    std::cout << "DONE!  Note, however, simply measuring the elapsed time takes about " << elapsed << "ns on your system...\n\n";

    return 0;
}
