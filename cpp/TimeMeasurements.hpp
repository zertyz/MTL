#ifndef MTL_TimeMeasurements_hpp
#define MTL_TimeMeasurements_hpp

#include <time.h>
#include <sys/time.h>

/**
 * TimeMeasurements.hpp
 * ====================
 * created by luiz, Sep 19, 2019
 *
 * Elapsed Real Time measurement functions -- milli, micro, nano and even less seconds.
 *
 * With the exception of 'getProcessorCycleCount', no other function here is reentrant. */
namespace MTL::TimeMeasurements {

    static inline unsigned long long getRealTimeMS();
    static inline unsigned long long getRealTimeUS();
    static inline unsigned long long getMonotonicRealTimeMS();
    static inline unsigned long long getMonotonicRealTimeUS();
    static inline unsigned long long getMonotonicRealTimeNS();

    /** This is the fastest 'time' measurement function among all, but
      * it does not return the time directly -- it returns the cycle
      * count as in the following formula:
      *     CYCLE_COUNT = CPU_MAX_FREQUENCY (in hertz) / ELAPSED_TIME (in seconds, since last reset)
      * so, in order to get the elapsed nano seconds, use:
      *     ELAPSED_TIME_SECONDS = (CYCLE_COUNT * CPU_MAX_FREQUENCY) / (1/10^9)
      * NOTES:
      *     - In theory, sub nano second measurement is possible;
      *     - On x86, if your CPU has the 'constant_tsc' feature, the measurement is reliable
      *       among cores as well as it is reliable even when the CPU scales up or down
      *       (maybe the actual CPU_MAX_FREQUENCY is greater than advertised by the CPU
      *       due to turbo); */
    static inline uint64_t getProcessorCycleCount();

}


// scoped functions definitions
using namespace MTL;


static inline uint64_t TimeMeasurements::getProcessorCycleCount() {
    
    // intel
    #ifdef __x86_64
        unsigned lo, hi;
        __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
        return ((uint64_t)hi << 32) | lo;


    // arm
    #elif __arm__
        // from -- https://www.raspberrypi.org/forums/viewtopic.php?t=30821
        volatile unsigned cc;
        static int init = 0;
        if(!init) {
            __asm__ __volatile__ ("mcr p15, 0, %0, c9, c12, 2" :: "r"(1<<31)); /* stop the cc */
            __asm__ __volatile__ ("mcr p15, 0, %0, c9, c12, 0" :: "r"(5));     /* initialize */
            __asm__ __volatile__ ("mcr p15, 0, %0, c9, c12, 1" :: "r"(1<<31)); /* start the cc */
            init = 1;
        }
        __asm__ __volatile__ ("mrc p15, 0, %0, c9, c13, 0" : "=r"(cc));

        return cc;

    #else

        #error Unknown Processor

    #endif
}


static struct timeval timeval_now;
static struct timespec timespec_now;

static inline unsigned long long TimeMeasurements::getRealTimeMS() {
    gettimeofday(&timeval_now, NULL);
    return (timeval_now.tv_sec*1000l) + (timeval_now.tv_usec/1000l);
}

static inline unsigned long long TimeMeasurements::getRealTimeUS() {
    gettimeofday(&timeval_now, NULL);
    return (timeval_now.tv_sec*1000000ll) + timeval_now.tv_usec;
}

static inline unsigned long long TimeMeasurements::getMonotonicRealTimeMS() {
    clock_gettime(CLOCK_MONOTONIC, &timespec_now);
    return (timespec_now.tv_sec*1000ll) + (timespec_now.tv_nsec/1000000ll);
}

static inline unsigned long long TimeMeasurements::getMonotonicRealTimeUS() {
    clock_gettime(CLOCK_MONOTONIC, &timespec_now);
    return (timespec_now.tv_sec*1000000ll) + (timespec_now.tv_nsec/1000ll);
}

static inline unsigned long long TimeMeasurements::getMonotonicRealTimeNS() {
    clock_gettime(CLOCK_MONOTONIC, &timespec_now);
    return (timespec_now.tv_sec*1000000000ll) + timespec_now.tv_nsec;
}

#endif //MTL_TimeMeasurements_hpp
