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


/** This number will get multiplyed by a function of consumer and producer threads
    and also the lock algorithm to give the total number of events to be processed */
#define BASE_NUMBER_OF_EVENTS 4320      // must be divisible by each number in 'N_PRODUCERS_CONSUMERS'

/** N_PRODUCERS_CONSUMERS := {{nProducers1, nConsumers1}, {nProducers2, nConsumers2}, ...}
    where 1, 2, ... correspond to 'nTest' -- the test number to perform */
constexpr unsigned N_PRODUCERS_CONSUMERS[][2]   = {{1,1}, {1,2}, {1,4}, {1,16}, {2,1}, {2,2}, {2,4}, {2,16}, {4,1}, {4,2}, {4,4}, {4,16}, {16,1}, {16,2}, {16,4}, {16,16}};

/** 'N_EVENTS_FACTOR[nTest] * BASE_NUMBER_OF_EVENTS' specifyes the number of events to run for each 'N_PRODUCERS_CONSUMERS[nTest]' pair */
constexpr unsigned N_EVENTS_FACTOR[]            = {   80,    40,    20,      5,    40,    20,    10,      3,    20,    10,     5,      2,      5,      4,      4,       1};
constexpr unsigned N_TESTS_PER_STRATEGY         = sizeof(N_PRODUCERS_CONSUMERS)/sizeof(N_PRODUCERS_CONSUMERS[0]);
static_assert(N_TESTS_PER_STRATEGY == sizeof(N_EVENTS_FACTOR)/sizeof(N_EVENTS_FACTOR[0]),
              "'N_PRODUCERS_CONSUMERS' and 'N_EVENTS_FACTOR' must be of the same length");

/** Multiplication factor for each algorithm:     {mutex, relaxed spin-lock, busy spin-lock, relaxed atomic, busy atomic, linear} */
constexpr unsigned STRATEGY_FACTOR[]            = {    1,               100,            100,            100,         100,   10000};

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define PAD(_number, _width)         std::setfill(' ') << std::setw(_width) << std::fixed << (_number)


#define DEBUG_PRODUCER     {std::this_thread::sleep_for(std::chrono::milliseconds(1000)); \
                            std::cerr << "produced="<<producerCount<<"; consumed="<<consumerCount<<std::endl;}


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

//#include <cpuid.h>
void printInfo() {

    string compilerVersion;
    if (strstr(__VERSION__, "Clang") == nullptr) {
        compilerVersion = "gcc " __VERSION__;
    } else {
        compilerVersion = __VERSION__;
    }

    std::cout << "Info:\n"
                 "\tCompiler & Version :  " << compilerVersion << "\n"
                 "\tOS                 :  " <<

                #ifdef _WIN32
                "Windows 32-bit"
                #elif _WIN64
                "Windows 64-bit"
                #elif __linux__
                "Linux"
                #elif __FreeBSD__
                "FreeBSD"
                #elif __APPLE__ || __MACH__
                "Mac OSX"
                #elif __unix || __unix__
                "Unix"
                #else
                "Unknown"
                #endif

              << "\n" << std::flush;

/*
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0,0,0,0};

    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);

        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }

    std::cout << "\tCPU Type           :  " << CPUBrandString << std::endl << std::endl;
*/
}


// math section
///////////////
#include <cmath>

/** Returns the average exponential factor between the given numbers and the
  * 'bias' -- how well they fit -- 1 being perfect fit */
template <typename _Numeric>
double getExponentiality(const _Numeric* dataPoints, unsigned nDataPoints, double& bias) {

    // helper lambdas
    auto computeLogs = [] (const auto* dataPoints, unsigned nDataPoints, double* logs) {
        for (unsigned i=0; i<nDataPoints; i++) {
            logs[i] = log(dataPoints[i]);
        }
    };
    auto computeFactors = [](const double* dataPoints, unsigned nDataPoints, double* factors) {
        for (unsigned i=0; i<(nDataPoints-1); i++) {
            factors[i] = ((double)dataPoints[i+1])/((double)dataPoints[i]);
        }
    };
    auto computeAverage = [](const double* values, unsigned nValues) -> double {
        double sum = 0;
        for (unsigned i=0; i<nValues; i++) {
            sum += values[i];
        }
        return sum / ((double)nValues);
    };

    // logs:                  local _a,_b,_c,_d=math.log(a),math.log(b),math.log(c),math.log(d)
    // factor of logs:        print(_d/_c,_c/_b,_b/_a)
    // logOfFactors:          local __a,__b,__c=math.log(_d/_c),math.log(_c/_b),math.log(_b/_a)
    // factorsOfLogOfFactors: print(__c/__b,__b/__a) end

    // if we don't have at least 2 points, there is nothing to do
    if (nDataPoints < 2) {
        bias = -1;
        return -1;
    }

    #define DEBUG(_VAR, _LEN) { \
        std::cout << "\t--> " #_VAR ": {"; \
        for (unsigned i=0; i<_LEN; i++) { \
            auto n =_VAR[i]; \
            std::cout << n << ","; \
        } \
        std::cout << "}\n" << std::flush; }

    double logOfPoints[nDataPoints];
//DEBUG(dataPoints, nDataPoints)
    computeLogs(dataPoints, nDataPoints, logOfPoints);
//DEBUG(logOfPoints, nDataPoints)
    double factorOfLogs[nDataPoints-1];
    computeFactors(logOfPoints, nDataPoints, factorOfLogs);
//DEBUG(factorOfLogs, nDataPoints-1)
    double averageExponentiality = computeAverage(factorOfLogs, nDataPoints-1);
//std::cout << "\t\t*** averageExponentiality is " << averageExponentiality << "\n";

    // compute the expoents of the expoents if we have more than 2 points
    if (nDataPoints > 2) {
        double logOfFactors[nDataPoints-1];
        computeLogs(factorOfLogs, nDataPoints-1, logOfFactors);
//DEBUG(logOfFactors, nDataPoints-1)
        double factorOfLogOfFactors[nDataPoints-2];
        computeFactors(logOfFactors, nDataPoints-1, factorOfLogOfFactors);
//DEBUG(factorOfLogOfFactors, nDataPoints-2)
        bias = computeAverage(factorOfLogOfFactors, nDataPoints-2);
    } else {
        bias = 1;
    }

    return averageExponentiality;
}


// test section
///////////////

static struct timespec timespec_now;
static inline unsigned long long getMonotonicRealTimeNS() {
    clock_gettime(CLOCK_MONOTONIC, &timespec_now);
    return (timespec_now.tv_sec*1000000000ll) + timespec_now.tv_nsec;
}

void check(unsigned& nEvents) {
    if ( (producerCount != nEvents) || (consumerCount != nEvents) ) {
        std::cout << "--> Checking FAILED: only " << producerCount << " events were produced, " << consumerCount << " consumed for a total of " << nEvents << " expected. Exiting..\n\n";
        exit(1);
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


void mutexProducer(unsigned nEvents) {
    for (unsigned i=0; i<nEvents; i++) {
        producingMutex.lock();
        producerCount.fetch_add(1, std::memory_order_acq_rel);
        consumingMutex.unlock();
    }
}

void mutexStop() {
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


void relaxSpinLockProducer(unsigned nEvents) {
    for (unsigned i=0; i<nEvents; i++) {
        producingRelaxSpinLock.lock();
        producerCount.fetch_add(1, std::memory_order_acq_rel);
        consumingRelaxSpinLock.unlock();
    }
}

void relaxSpinLockStop() {
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


void busySpinLockProducer(unsigned nEvents) {
    for (unsigned i=0; i<nEvents; i++) {
        producingBusySpinLock.lock();
//        DEBUG_PRODUCER
        producerCount.fetch_add(1, std::memory_order_acq_rel);
        consumingBusySpinLock.unlock();
    }
}

void busySpinLockStop() {
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


#include <boost/fiber/detail/cpu_relax.hpp>     // provides cpu_relax() macro, which uses the x86's "pause" or arm's "yield" instructions

void relaxAtomicProducer(unsigned nEvents) {
    for (unsigned i=0; i<nEvents; i++) {
        unsigned currentProducerCount = producerCount.load(std::memory_order_acquire);
        while (!producerCount.compare_exchange_weak(currentProducerCount, currentProducerCount+1,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed)) {
            cpu_relax();
        }
        // wait until the event is consumed before passing on to the next one -- remember we are measuring the latency, not the throughput
        while (producerCount.load(std::memory_order_acquire) > consumerCount.load(std::memory_order_acquire)) {
            cpu_relax();
        }
    }

}

void relaxAtomicStop() {
    // ask consumers to stop & wait for them
    stopConsumers.store(true, std::memory_order_release);
    while (numberOfActiveConsumers.load(std::memory_order_acquire) > 0) ;
}

void relaxAtomicConsumer() {
    numberOfActiveConsumers.fetch_add(1, std::memory_order_acq_rel);
    while (!stopConsumers.load(std::memory_order_acquire)) {

        unsigned currentConsumerCount = consumerCount.load(std::memory_order_acquire);

        // is there a next event to be consumed already?
        if (currentConsumerCount < producerCount.load(std::memory_order_acquire)) {

            // try to consume it
            if (consumerCount.compare_exchange_weak(currentConsumerCount, currentConsumerCount+1,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed)) {
                // yeah! authorization to consume acquired (the counter was already increased)
            } else {
                cpu_relax();
            }
        }
    }
    numberOfActiveConsumers.fetch_sub(1, std::memory_order_acq_rel);
}


void busyAtomicProducer(unsigned nEvents) {
    for (unsigned i=0; i<nEvents; i++) {
        //if (!(i%1000000)) DEBUG_PRODUCER
        //if ( !(i%500'000) && (nEvents==4312000) ) DEBUG_PRODUCER
        // loop until this thread can produce the next event (until it is the thread who could increase the counter)
        unsigned currentProducerCount = producerCount.load(std::memory_order_acquire);
        while (!producerCount.compare_exchange_weak(currentProducerCount, currentProducerCount+1,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed))
            ;
        // wait until the event is consumed before passing on to the next one -- remember we are measuring the latency, not the throughput
        while (producerCount.load(std::memory_order_acquire) > consumerCount.load(std::memory_order_acquire))
            ;
    }

}

void busyAtomicStop() {
    // ask consumers to stop & wait for them
    stopConsumers.store(true, std::memory_order_release);
    while (numberOfActiveConsumers.load(std::memory_order_acquire) > 0) ;
}

void busyAtomicConsumer() {
    numberOfActiveConsumers.fetch_add(1, std::memory_order_acq_rel);
    while (!stopConsumers.load(std::memory_order_acquire)) {

        unsigned currentConsumerCount = consumerCount.load(std::memory_order_acquire);

        // is there a next event to be consumed already?
        if (currentConsumerCount < producerCount.load(std::memory_order_acquire)) {

            // try to consume it
            if (consumerCount.compare_exchange_weak(currentConsumerCount, currentConsumerCount+1,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed)) {
                // yeah! authorization to consume acquired (the counter was already increased)
            } else {
                ;   // we were not the one to consume the event yet
            }
        }
    }
    numberOfActiveConsumers.fetch_sub(1, std::memory_order_acq_rel);
}


void linearProducerConsumer(unsigned nEvents) {
    // simulates the work the others producers / consumers do: simply increase their counters
    for (unsigned i=0; i<nEvents; i++) {
        producerCount.fetch_add(1, std::memory_order_acq_rel);
        consumerCount.fetch_add(1, std::memory_order_acq_rel);
    }
}


#define STRINGFY(_str)  #_str
#define PERFORM_MEASUREMENT(_strategyNumber, _producerFunction, _consumerFunction, _stopFunction) \
        performMeasurement(_strategyNumber,                                \
                           _producerFunction, STRINGFY(_producerFunction), \
                           _consumerFunction, STRINGFY(_consumerFunction), \
                           _stopFunction,     STRINGFY(_stopFunction) )
template <typename _ProducerFunction, typename _ConsumerFunction, typename _StopFunction>
void performMeasurement(unsigned strategyNumber,
                        _ProducerFunction producerFunction, string producerFunctionName,
                        _ConsumerFunction consumerFunction, string consumerFunctionName,
                        _StopFunction         stopFunction, string stopFunctionName) {

    // each measurement will be stored for analysis
    struct {
        unsigned            nConsumers;
        unsigned            nProducers;
        unsigned long long  averageEventDurationNS;
    } measurements[N_TESTS_PER_STRATEGY];

    std::cout << "\n\nStaring the STRATEGY #" << strategyNumber << " measurements: '" << producerFunctionName << "' / '" << consumerFunctionName << "':\n";
    std::cout << "\tnTest; nProducers; nConsumers;    nEvents   -->   (  tEvent;         tTotal  ):\n" << std::flush;
    for (unsigned nTest=0; nTest<N_TESTS_PER_STRATEGY; nTest++) {
        unsigned nProducers = N_PRODUCERS_CONSUMERS[nTest][0];
        unsigned nConsumers = N_PRODUCERS_CONSUMERS[nTest][1];
        unsigned nEvents    = BASE_NUMBER_OF_EVENTS * N_EVENTS_FACTOR[nTest] * STRATEGY_FACTOR[strategyNumber];
        std::thread consumerThreads[nConsumers];
        std::thread producerThreads[nProducers];
        unsigned long long start;
        unsigned long long elapsed;

        reset();

        std::cout << "\t" <<
                  PAD(nTest,      5)  << "; " <<
                  PAD(nProducers, 10) << "; " <<
                  PAD(nConsumers, 10) << "; " <<
                  PAD(nEvents,    10) << "   -->   " << std::flush;

        // start the consumers
        for (unsigned threadNumber=0; threadNumber<nConsumers; threadNumber++) {
            consumerThreads[threadNumber] = std::thread(consumerFunction);
        }
        // start the producers
        start = getMonotonicRealTimeNS();
        for (unsigned threadNumber=0; threadNumber<nProducers; threadNumber++) {
            producerThreads[threadNumber] = std::thread(producerFunction, nEvents/nProducers);
        }
        // wait for the producers
        for (int threadNumber=0; threadNumber<nProducers; threadNumber++) {
            producerThreads[threadNumber].join();
        }
        elapsed = getMonotonicRealTimeNS() - start;
        stopFunction();
        // wait for the consumers
        for (int threadNumber=0; threadNumber<nConsumers; threadNumber++) {
            consumerThreads[threadNumber].join();
        }
        check(nEvents);
        unsigned tEvent = elapsed / nEvents;    // ns per event
        std::cout << "(" <<
                     PAD(tEvent, 6)   << "ns; " <<
                     PAD(elapsed, 14) << "ns)" <<
                     (nTest < N_TESTS_PER_STRATEGY-1 ? ",\n":".\n") << std::flush;

        // record measurement
        measurements[nTest].nConsumers             = nConsumers;
        measurements[nTest].nProducers             = nProducers;
        measurements[nTest].averageEventDurationNS = tEvent;
    }
    std::cout << "\n";

    std::cout << "\t--> Analisys:\n";

    // helper computations
    //////////////////////

    // filters 'measurements' array and set the values into 'averageEventDurations'
    // -- it's size should be:
    //      1                           -- if you provide both 'nProducers' and 'nConsumers'
    //      sqrt(N_TESTS_PER_STRATEGY)  -- if you just provide one of them (passing the other as -1)
    auto getTimesForProducersAndConsumers = [&measurements](unsigned nProducers, unsigned nConsumers, unsigned long long* averageEventDurations) {
        unsigned index=0;
        for (unsigned i=0; i<N_TESTS_PER_STRATEGY; i++) {
            if ( ( (nProducers == -1) || (measurements[i].nProducers == nProducers) ) &&
                 ( (nConsumers == -1) || (measurements[i].nConsumers == nConsumers) ) )  {
                averageEventDurations[index++] = measurements[i].averageEventDurationNS;
            }
        }
    };

    // analysis
    ///////////

    unsigned valuesForConsumerAndProducers[] = {1,2,4,16};   // these are the sqrt(N_TESTS_PER_STRATEGY) elements from 'N_PRODUCERS_CONSUMERS'
    for(unsigned nConsumers : valuesForConsumerAndProducers) {
        unsigned long long averageEventDurations[4];
        getTimesForProducersAndConsumers(-1, nConsumers, averageEventDurations);
        double bias;
        double exponentiality = getExponentiality(averageEventDurations, 4, bias);
        std::cout << "\t\tExponentiality among 'nProducers' when nConsumers = " << PAD(nConsumers, 2) << ":  "
                  << exponentiality << "; bias:" << bias << "\n";
    }
    for(unsigned nProducers : valuesForConsumerAndProducers) {
        unsigned long long averageEventDurations[4];
        getTimesForProducersAndConsumers(nProducers, -1, averageEventDurations);
        double bias;
        double exponentiality = getExponentiality(averageEventDurations, 4, bias);
        std::cout << "\t\tExponentiality among 'nConsumers' when nProducers = " << PAD(nProducers, 2) << ":  "
                  << exponentiality << "; bias:" << bias << "\n";
    }
}

int main(void) {

	std::cout << DOCS;

    printInfo();

    // formatted output
    std::cout.imbue(std::locale(""));

    unsigned long long data[] = {49,71,120,436};
    std::cout << "Exponentiality for {";
    for (unsigned long long n : data) std::cout << n << ",";
    double bias;
    double exponentiality = getExponentiality(data, 4, bias);
    std::cout << "}: " << exponentiality << " (bias: " << bias << ")\n\n";
//    exit(1);

    PERFORM_MEASUREMENT(0,         mutexProducer,         mutexConsumer,         mutexStop);
    PERFORM_MEASUREMENT(1, relaxSpinLockProducer, relaxSpinLockConsumer, relaxSpinLockStop);
    PERFORM_MEASUREMENT(2,  busySpinLockProducer,  busySpinLockConsumer,  busySpinLockStop);
    PERFORM_MEASUREMENT(3,   relaxAtomicProducer,   relaxAtomicConsumer,   relaxAtomicStop);
    PERFORM_MEASUREMENT(4,    busyAtomicProducer,    busyAtomicConsumer,    busyAtomicStop);

/*
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


    std::cout << "\n\nStaring the 'busyAtomicProducer' / 'busyAtomicConsumer' measurements: " << std::flush;
    reset();
	for (unsigned _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
		threads[_threadNumber] = std::thread(busyAtomicConsumer);
	}
    start = getMonotonicRealTimeNS();
    busyAtomicProducer();
    elapsed = getMonotonicRealTimeNS() - start;
    for (int _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
        threads[_threadNumber].join();
    }
    std::cout << PAD(elapsed, 6) << "ns\n";
    check(elapsed);

*/

    unsigned long long start;
    unsigned long long elapsed;

    std::cout << "\n\nStaring the 'linearProducerConsumer' (no threads) measurements:\n";
    std::cout << "\t(tEvent, nEvents, tTotal): " << std::flush;
    reset();
    unsigned nEvents = BASE_NUMBER_OF_EVENTS * STRATEGY_FACTOR[5];   // 5 is the STRATEGY_NUMBER for linearProducerConsumer
    start = getMonotonicRealTimeNS();
    linearProducerConsumer(nEvents);
    elapsed = getMonotonicRealTimeNS() - start;
    check(nEvents);
    unsigned tEvent = elapsed / nEvents;    // ns per event
    std::cout << "(" << PAD(tEvent, 3) << "ns, " << PAD(nEvents, 6) << ", " << PAD(elapsed, 6) << "ns).\n\n" << std::flush;

    start = getMonotonicRealTimeNS();
    elapsed = getMonotonicRealTimeNS() - start;
    std::cout << "DONE!  Note, however, simply measuring the elapsed time takes about " << elapsed << "ns on your system...\n\n";

    return 0;
}
