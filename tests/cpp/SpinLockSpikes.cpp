#include <iostream>
#include <iomanip>
#include <atomic>
#include <thread>
#include <cstring>
#include <mutex>

#include "../../cpp/TimeMeasurements.hpp"
using namespace MTL::TimeMeasurements;

#include "../../cpp/thread/SpinLock.hpp"    // also provides cpu_relax() macro, which uses the x86's "pause" or arm's "yield" instructions

// compile & run with clear; echo -en "\n\n###############\n\n"; toStop="chrome vscode visual-studio-code"; for p in $toStop; do pkill -stop -f "$p"; done; sudo sync; g++ -std=c++17 -O3 -mcpu=native -march=native -mtune=native -pthread -I../../external/EABase/include/Common/ SpinLockSpikes.cpp -o SpinLockSpikes && sudo sync && sleep 2 && sudo time nice -n -20 ./SpinLockSpikes; for p in $toStop; do pkill -cont -f "$p"; done

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


// mutex
std::mutex producingMutex;
std::mutex consumingMutex;

// relax spin
mutua::MTL::SpinLock<true> producingRelaxSpinLock;
mutua::MTL::SpinLock<true> consumingRelaxSpinLock;

// relax spin with metrics
const char producingLockName[] = "producing lock";
const char consumingLockName[] = "consuming lock";
mutua::MTL::SpinLock<true, true, 0, producingLockName> producingRelaxMetricsSpinLock;
mutua::MTL::SpinLock<true, true, 0, consumingLockName> consumingRelaxMetricsSpinLock;

// relax spin falling back to system mutex
mutua::MTL::SpinLock<true, true, 5'000'000'000, producingLockName, 6'500'000'000> producingRelaxMutexFallbackSpinLock;
mutua::MTL::SpinLock<true, true, 5'000'000'000, consumingLockName, 6'500'000'000> consumingRelaxMutexFallbackSpinLock;

// busy spin
mutua::MTL::SpinLock<false> producingBusySpinLock;
mutua::MTL::SpinLock<false> consumingBusySpinLock;


// info section
///////////////

#include <EABase/config/eacompiler.h>
#include <EABase/config/eaplatform.h>
void printInfo() {

    std::cout << "Compile-time Info:\n"
                 "\tCompiler & Version :  " EA_COMPILER_STRING "\n"
                 "\tOS                 :  " EA_PLATFORM_NAME "\n"
                 "\tPlatform           :  " EA_PLATFORM_DESCRIPTION "\n"
                 "\tEA_CACHE_LINE_SIZE :  " << EA_CACHE_LINE_SIZE << "\n" << std::flush
    ;

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
    consumingRelaxMetricsSpinLock.try_lock();
    consumingRelaxMutexFallbackSpinLock.try_lock();
    consumingBusySpinLock.try_lock();
}

/** works for all lock-like APIs, including mutexes and spin locks */
template <auto& _producingLock, auto& _consumingLock>
void lockProducer(unsigned nEvents) {
    for (unsigned i=0; i<nEvents; i++) {
        _producingLock.lock();
        producerCount.fetch_add(1, std::memory_order_relaxed);
        _consumingLock.unlock();
    }
}

template <auto& _producingLock, auto& _consumingLock>
void lockStop() {
    // ask consumers to stop & wait for them
    _producingLock.lock();
    stopConsumers.store(true, std::memory_order_release);
    while (numberOfActiveConsumers.load(std::memory_order_acquire) > 0) _consumingLock.unlock();
    _producingLock.unlock();
}

template <auto& _producingLock, auto& _consumingLock, bool _isInstanceOfSpinLock = true>
void debugDeadLock() {
    unsigned lastProducerCount = producerCount.load(std::memory_order_relaxed);
    unsigned lastConsumerCount = consumerCount.load(std::memory_order_relaxed);
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        unsigned currentProducerCount = producerCount.load(std::memory_order_relaxed);
        unsigned currentConsumerCount = consumerCount.load(std::memory_order_relaxed);
        if (stopConsumers.load(std::memory_order_acquire)) {
            break;
        }
        if ((currentProducerCount == lastProducerCount) || (currentConsumerCount == lastConsumerCount)) {
            std::cerr << "\n\n--> 'debugDeadLock()' detected a dead lock! Dumping semaphores...\n" << std::flush;
            if constexpr ( _isInstanceOfSpinLock /* how to detect it automatically?? */ ) {
                _producingLock.issueDebugMessage("producing lock metrics");
                _consumingLock.issueDebugMessage("consuming lock metrics");
            } else {
                std::cerr << "    ** NO INFO ** -- Are we debugging a mutex lock?\n" << std::flush;
            }
        }
        std::cerr << "\033[s** debug ** --> producerCount=" << PAD(currentProducerCount, 6) << "; consumerCount=" << PAD(currentConsumerCount, 6) << "   ...   \033[u" << std::flush;
    }
}

template <auto& _producingLock, auto& _consumingLock>
void lockConsumer() {
    numberOfActiveConsumers.fetch_add(1, std::memory_order_relaxed);
    while (true) {
        _consumingLock.lock();

        if (unlikely (stopConsumers.load(std::memory_order_acquire)) ) {
            _consumingLock.unlock();
            break;
        }

        consumerCount.fetch_add(1, std::memory_order_relaxed);
        _producingLock.unlock();
    }
    numberOfActiveConsumers.fetch_sub(1, std::memory_order_relaxed);
}

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
#define PERFORM_MEASUREMENT(_strategyNumber, _producerFunction, _consumerFunction, _stopFunction, _debugFunction) \
        performMeasurement(_strategyNumber,                                \
                           _producerFunction, STRINGFY(_producerFunction), \
                           _consumerFunction, STRINGFY(_consumerFunction), \
                           _stopFunction,     STRINGFY(_stopFunction),     \
                           _debugFunction,     STRINGFY(_debugFunction) )
template <typename _ProducerFunction, typename _ConsumerFunction, typename _StopFunction, typename _DebugFunction>
void performMeasurement(unsigned strategyNumber,
                        _ProducerFunction producerFunction, string producerFunctionName,
                        _ConsumerFunction consumerFunction, string consumerFunctionName,
                        _StopFunction         stopFunction, string stopFunctionName,
                        _DebugFunction       debugFunction, string debugFunctionName) {

    // each measurement will be stored for analysis
    struct {
        unsigned            nConsumers;
        unsigned            nProducers;
        unsigned long long  averageEventDurationNS;
    } measurements[N_TESTS_PER_STRATEGY];

    constexpr bool DEBUG_DEADLOCKS = !std::is_same_v<_DebugFunction, std::nullptr_t>;      // debug enabled when possible
    //constexpr bool DEBUG_DEADLOCKS = false;                                                // debug disabled even if possible

    std::cout << "\n\nStaring the STRATEGY #" << strategyNumber << " measurements: '" << producerFunctionName << "' / '" << consumerFunctionName << "':\n";
    std::cout << "\tnTest; nProducers; nConsumers;    nEvents   -->   (  tEvent;         tTotal  ):\n" << std::flush;
    for (unsigned nTest=0; nTest<N_TESTS_PER_STRATEGY; nTest++) {
        unsigned nProducers = N_PRODUCERS_CONSUMERS[nTest][0];
        unsigned nConsumers = N_PRODUCERS_CONSUMERS[nTest][1];
        unsigned nEvents    = BASE_NUMBER_OF_EVENTS * N_EVENTS_FACTOR[nTest] * STRATEGY_FACTOR[strategyNumber];
        std::thread consumerThreads[nConsumers];
        std::thread producerThreads[nProducers];
        std::thread debuggerThread;
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
            std::this_thread::yield();  // assure the consumers will start before the producers
        }
        // start the producers
        start = getMonotonicRealTimeNS();
        for (unsigned threadNumber=0; threadNumber<nProducers; threadNumber++) {
            producerThreads[threadNumber] = std::thread(producerFunction, nEvents/nProducers);
        }
        // start the deadlock debugger?
        if constexpr (DEBUG_DEADLOCKS) {
            debuggerThread = std::thread(debugFunction);
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
                     PAD(elapsed, 14) << "ns)\033[K" <<     // here, "\033[K" erases 'til the end of the line (clean debug info)
                     (nTest < N_TESTS_PER_STRATEGY-1 ? ",\n":".\n") << std::flush;

        // record measurement
        measurements[nTest].nConsumers             = nConsumers;
        measurements[nTest].nProducers             = nProducers;
        measurements[nTest].averageEventDurationNS = tEvent;

        // wait for the debugger thread, if it has been started
        if (DEBUG_DEADLOCKS) {
            debuggerThread.join();
        }

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
            if (  // "filter by one of them if the other is -1" condition
                  ( ( (nProducers == -1) || (measurements[i].nProducers == nProducers) ) &&
                    ( (nConsumers == -1) || (measurements[i].nConsumers == nConsumers) ) ) ||
                  // "includes when they are both the same"
                  ( (nProducers == -1) && (nConsumers == -1) &&
                    (measurements[i].nProducers == measurements[i].nConsumers) )
               )  {
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
    unsigned long long averageEventDurations[4];
    getTimesForProducersAndConsumers(-1, -1, averageEventDurations);
    double bias;
    double exponentiality = getExponentiality(averageEventDurations, 4, bias);
    std::cout << "\t\tExponentiality among total threads (2, 4, 8, 32): "
                << exponentiality << "; bias:" << bias << "\n";

}

static const char fslName[] = "mySpin";
int main(void) {

	std::cout << DOCS;

    printInfo();

    // formatted output
    std::cout.imbue(std::locale(""));

    /* exponentiality test
    unsigned long long data[] = {49,71,120,436};
    std::cout << "Exponentiality for {";
    for (unsigned long long n : data) std::cout << n << ",";
    double bias;
    double exponentiality = getExponentiality(data, 4, bias);
    std::cout << "}: " << exponentiality << " (bias: " << bias << ")\n\n";
//    exit(1);*/

    // RDTSC
    unsigned long long m_start  = getMonotonicRealTimeNS();
    uint64_t cc_start  = getProcessorCycleCount();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    unsigned long long m_finish = getMonotonicRealTimeNS();
    uint64_t cc_finish = getProcessorCycleCount();
    uint64_t cc_split = getProcessorCycleCount();

    std::cout << "m_start   : " << m_start   << "\n"
                 "m_finish  : " << m_finish  << "\n"
                 "cc_start  : " << cc_start  << "\n"
                 "cc_finish : " << cc_finish << "\n"
                 "min measurement: " << (cc_split-cc_finish) << "\n";
    

/*    // spin lock tests
    mutua::MTL::SpinLock spl;
    std::cout << "Standard spinlock size: " << sizeof(spl) << "\n";
    mutua::MTL::SpinLock<true, true> msl;
    std::cout << "Metrics-enabled spinlock size: " << sizeof(msl) << "\n";
    mutua::MTL::SpinLock<true, true, 2'000'000'000> dsl;    // this is 1 seconds in a 2GHz CPU
    std::cout << "Timeout-enabled spinlock size: " << sizeof(dsl) << "\n";
    dsl.lock();
    {thread t([&dsl]{dsl.lock();dsl.unlock();});
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    dsl.unlock();
    t.join();}
    std::cout << "--> Did you see any timeout messages above?\n";
    mutua::MTL::SpinLock<true, true, 2'000'000'000, fslName, 4'000'000'000> fsl;    // this is 1 seconds in a 2GHz CPU
    std::cout << "Named, Fallback-enabled spinlock size: " << sizeof(fsl) << "\n";
    fsl.lock();cerr << "<<1>>" << flush;
    {thread t([&fsl]{fsl.lock();cerr << "<<2>>" << flush;fsl.unlock();cerr << "<<3>>" << flush;});
    std::this_thread::sleep_for(std::chrono::milliseconds(8000));
    fsl.unlock();
    cerr << "<<4>>" << flush;
    t.join();cerr << "<<5>>" << flush;}
    std::cout << "--> Now lets see the real mutexes count:" << std::flush;
    fsl.issueDebugMessage("After fully unlocked...");
    exit(0);*/

    // template function instances
    //////////////////////////////

    // mutex
    void (&mutexProducer) (unsigned) = lockProducer  <producingMutex, consumingMutex>;
    void (&mutexConsumer) ()         = lockConsumer  <producingMutex, consumingMutex>;
    void (&mutexStop)     ()         = lockStop      <producingMutex, consumingMutex>;
    void (&mutexDebug)    ()         = debugDeadLock <producingMutex, consumingMutex, false>;

    // relax spin
    void (&relaxSpinLockProducer) (unsigned) = lockProducer  <producingRelaxSpinLock, consumingRelaxSpinLock>;
    void (&relaxSpinLockConsumer) ()         = lockConsumer  <producingRelaxSpinLock, consumingRelaxSpinLock>;
    void (&relaxSpinLockStop)     ()         = lockStop      <producingRelaxSpinLock, consumingRelaxSpinLock>;
    void (&relaxSpinLockDebug)    ()         = debugDeadLock <producingRelaxSpinLock, consumingRelaxSpinLock>;

    // busy spin
    void (&busySpinLockProducer) (unsigned) = lockProducer  <producingBusySpinLock, consumingBusySpinLock>;
    void (&busySpinLockConsumer) ()         = lockConsumer  <producingBusySpinLock, consumingBusySpinLock>;
    void (&busySpinLockStop)     ()         = lockStop      <producingBusySpinLock, consumingBusySpinLock>;
    void (&busySpinLockDebug)    ()         = debugDeadLock <producingBusySpinLock, consumingBusySpinLock>;

    // spin with relax and metrics
    void (&relaxMetricsSpinLockProducer) (unsigned) = lockProducer  <producingRelaxMetricsSpinLock, consumingRelaxMetricsSpinLock>;
    void (&relaxMetricsSpinLockConsumer) ()         = lockConsumer  <producingRelaxMetricsSpinLock, consumingRelaxMetricsSpinLock>;
    void (&relaxMetricsSpinLockStop)     ()         = lockStop      <producingRelaxMetricsSpinLock, consumingRelaxMetricsSpinLock>;
    void (&relaxMetricsSpinLockDebug)    ()         = debugDeadLock <producingRelaxMetricsSpinLock, consumingRelaxMetricsSpinLock>;

    // spin with relax and falling back to system mutex
    void (&relaxMutexFallbackSpinLockProducer) (unsigned) = lockProducer  <producingRelaxMutexFallbackSpinLock, consumingRelaxMutexFallbackSpinLock>;
    void (&relaxMutexFallbackSpinLockConsumer) ()         = lockConsumer  <producingRelaxMutexFallbackSpinLock, consumingRelaxMutexFallbackSpinLock>;
    void (&relaxMutexFallbackSpinLockStop)     ()         = lockStop      <producingRelaxMutexFallbackSpinLock, consumingRelaxMutexFallbackSpinLock>;
    void (&relaxMutexFallbackSpinLockDebug)    ()         = debugDeadLock <producingRelaxMutexFallbackSpinLock, consumingRelaxMutexFallbackSpinLock>;


    PERFORM_MEASUREMENT(0,                mutexProducer,                mutexConsumer,                 mutexStop, mutexDebug);

    PERFORM_MEASUREMENT(1,        relaxSpinLockProducer,        relaxSpinLockConsumer,         relaxSpinLockStop, relaxSpinLockDebug);

    PERFORM_MEASUREMENT(1, relaxMetricsSpinLockProducer, relaxMetricsSpinLockConsumer,  relaxMetricsSpinLockStop, relaxMetricsSpinLockDebug);
    producingRelaxMetricsSpinLock.issueDebugMessage("final statistics");
    consumingRelaxMetricsSpinLock.issueDebugMessage("final statistics");

    PERFORM_MEASUREMENT(1, relaxMutexFallbackSpinLockProducer, relaxMutexFallbackSpinLockConsumer,  relaxMutexFallbackSpinLockStop, relaxMutexFallbackSpinLockDebug);
    producingRelaxMutexFallbackSpinLock.issueDebugMessage("final statistics");
    consumingRelaxMutexFallbackSpinLock.issueDebugMessage("final statistics");

    PERFORM_MEASUREMENT(2,         busySpinLockProducer,         busySpinLockConsumer,          busySpinLockStop, busySpinLockDebug);

    PERFORM_MEASUREMENT(3,          relaxAtomicProducer,          relaxAtomicConsumer,           relaxAtomicStop, nullptr);

    PERFORM_MEASUREMENT(4,           busyAtomicProducer,           busyAtomicConsumer,            busyAtomicStop, nullptr);


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
