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


#define NUMBER_OF_EVENTS_PER_MEASURE 1234567
#define N_THREADS                    (std::thread::hardware_concurrency()-1)    // remember we have 1 thread for the consumer

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define PAD(_number, _width)         std::setfill(' ') << std::setw(_width) << std::fixed << (_number)


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

mutua::MTL::SpinLock producingSpinLock;
mutua::MTL::SpinLock consumingSpinLock;

// info section
///////////////

void check(unsigned long long& elapsed) {
    if ( (producerCount == consumerCount) && (producerCount  == NUMBER_OF_EVENTS_PER_MEASURE) ) {
        std::cout << "--> Checking PASSED for " << NUMBER_OF_EVENTS_PER_MEASURE << " events! --> " << PAD(elapsed / NUMBER_OF_EVENTS_PER_MEASURE, 4) << "ns per event\n\n";
    } else {
        std::cout << "--> Checking FAILED: only " << producerCount << " events were produced, " << consumerCount << " consumed for a total of " << NUMBER_OF_EVENTS_PER_MEASURE << " expected...\n\n";
    }
}


// spike methods
////////////////


void reset() {
    producerCount = 0;
    consumerCount = 0;
    stopConsumers = false;
    numberOfActiveConsumers = 0;
}


void mutexProducer() {
    for (unsigned i=0; i<NUMBER_OF_EVENTS_PER_MEASURE; i++) {
        producingMutex.lock();
        producerCount++;
        consumingMutex.unlock();
    }
    stopConsumers = true;

    // wait for the last event to be consumed
    while (producerCount > consumerCount) ;

    unsigned _producerCount = producerCount;
    unsigned _consumerCount = consumerCount;

    // generate extra events to stop the consumers
    while (numberOfActiveConsumers > 0) {
        producingMutex.lock();
        producerCount++;
        consumingMutex.unlock();
    }
    // restore the measured events
    producerCount = _producerCount;
    consumerCount = _consumerCount;
}

void mutexConsumer() {
    numberOfActiveConsumers++;
    while (!stopConsumers) {
        consumingMutex.lock();
        consumerCount++;
        producingMutex.unlock();
    }
    numberOfActiveConsumers--;
}


void spinLockProducer() {
    for (unsigned i=0; i<NUMBER_OF_EVENTS_PER_MEASURE; i++) {
        producingSpinLock.lock();
        producerCount++;
        consumingSpinLock.unlock();
    }
    stopConsumers = true;

    // wait for the last event to be consumed
    while (producerCount > consumerCount) ;

    unsigned _producerCount = producerCount;
    unsigned _consumerCount = consumerCount;
    
    // generate extra events to stop the consumers
    while (numberOfActiveConsumers > 0) {
        producingSpinLock.lock();
        producerCount++;
        consumingSpinLock.unlock();
    }
    // restore the measured events
    producerCount = _producerCount;
    consumerCount = _consumerCount;
}

void spinLockConsumer() {
    numberOfActiveConsumers++;
    while (!stopConsumers) {
        consumingSpinLock.lock();
        consumerCount++;
        producingSpinLock.unlock();
    }
    numberOfActiveConsumers--;
}


void busyWaitProducer() {
    for (unsigned i=0; i<NUMBER_OF_EVENTS_PER_MEASURE; i++) {
        while (producerCount > consumerCount) ;     // the busy wait loop
        producerCount++;
//        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//        std::cerr << "produced="<<producerCount<<"; consumed="<<consumerCount<<std::endl;

    }
    stopConsumers = true;

    // wait for the last event to be consumed
    while (producerCount > consumerCount) ;

    unsigned _producerCount = producerCount;
    unsigned _consumerCount = consumerCount;
    
    // generate extra events to stop the consumers
    while (numberOfActiveConsumers > 0) {
        while (producerCount > consumerCount) ;
        producerCount++;
    }
    // restore the measured events
    producerCount = _producerCount;
    consumerCount = _consumerCount;
}

void busyWaitConsumer() {
    numberOfActiveConsumers++;
    while (!stopConsumers) {
        while (producerCount == consumerCount) ;     // the busy wait loop -- remembar all consumers may leave this loop at once
        // all consumers may leave the loop at once, but only one should increase the counter
        unsigned targetConsumerCount = producerCount-1;
        consumerCount.compare_exchange_strong(targetConsumerCount, consumerCount+1);
    }
    numberOfActiveConsumers--;
}

void linearProducerConsumer() {
    for (unsigned i=0; i<NUMBER_OF_EVENTS_PER_MEASURE; i++) {
        // simulates the work the others producers / consumers do: simply increase their counters
        producerCount++;
        consumerCount++;
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


    std::cout << "\n\nStaring the 'spinLockProducer' / 'spinLockConsumer' measurements: " << std::flush;
    reset();
	for (unsigned _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
		threads[_threadNumber] = std::thread(spinLockConsumer);
	}
    start = getMonotonicRealTimeNS();
    spinLockProducer();
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
