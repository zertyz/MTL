#ifndef MTL_TimeMeasurements_hpp
#define MTL_TimeMeasurements_hpp

#include <time.h>
#include <sys/time.h>

#include "../compiletime/HostInfo.h"

/**
 * TimeMeasurements.hpp
 * ====================
 * created by luiz, Sep 19, 2018
 *
 * Elapsed Real Time measurement functions -- milli, micro, nano and even less seconds.
 *
 * With the exception of 'getProcessorCycleCount', no other function here is reentrant.
 *
 * NOTE: arm code gathered from
 * https://stackoverflow.com/questions/3247373/how-to-measure-program-execution-time-in-arm-cortex-a8-processor?answertab=active#tab-top
 * https://matthewarcus.wordpress.com/2018/01/27/using-the-cycle-counter-registers-on-the-raspberry-pi-3/
 * */
namespace MTL::time::TimeMeasurements {

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
      *     - In theory, sub nano second measurements are possible;
      *     - There is a limit for the elapsed time (ARM uses a 32bit int, for instance, but may tick once every 64 cycles)
      *     - On x86, if your CPU has the 'constant_tsc' feature, the measurement is reliable
      *       among cores as well as it is reliable even when the CPU scales up or down
      *       (maybe the actual CPU_MAX_FREQUENCY is greater than advertised by the CPU
      *       due to turbo); */
    static inline uint64_t getProcessorCycleCount();

    /** initialization for 'getProcessorCycleCount' needed by ARM */
    static inline unsigned armClockInit();
    static unsigned _armClockInit = armClockInit();

}


// scoped functions definitions
using namespace MTL::time;

static inline unsigned TimeMeasurements::armClockInit() {

	#if MTL_CPU_INSTR_ARMv7 || MTL_CPU_INSTR_ARMv8

    // this code is needed to initialize both Raspberry Pi 2 & Raspberry Pi 3 before any measurements
    // can be made. It's execution is guranteed by the initialization of the static variable '_armClockInit'.
    // Apart from that, both rPi2 & rPi3 need to load a kernel module to execute a code that enable user space
    // processes to read the measurement registers -- see kernel/arm/enable_ccr.c

		// in general enable all counters (including cycle counter)
		int32_t value = 1;

		// perform reset:
		if (/*do_reset || */true) {
			value |= 2;     // reset all counters to zero.
			value |= 4;     // reset cycle counter to zero.
		}

		if (/*enable_divider || */true) {
			value |= 8;     // enable "by 64" divider for CCNT, making the 32bit overflow 64 times less likely.
		}

		value |= 16;

		// program the performance-counter control-register:
		asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(value));

		// enable all counters:
		asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));

		// clear overflows:
		asm volatile ("MCR p15, 0, %0, c9, c12, 3\t\n" :: "r"(0x8000000f));

		return 42;	// can be any number...

	#else
    // x86 & Raspberry Pi 1 does not need any per-process initialization code
    // Raspberry Pi 1 still needs to execute code in kernel space once per boot
		return 0;
	#endif
}

static inline uint64_t TimeMeasurements::getProcessorCycleCount() {
    
    // intel
    #ifdef __x86_64
        unsigned lo, hi;
        __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
        return ((uint64_t)hi << 32) | lo;


    // Raspberry Pi 2 & 3
    #elif MTL_CPU_INSTR_ARMv7 || MTL_CPU_INSTR_ARMv8
        unsigned int value;
        // Read CCNT Register
        asm volatile ("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(value));  
        return value;

    // Raspberry Pi 1
    #elif MTL_CPU_INSTR_ARMv6
        unsigned cc;
        asm volatile ("mrc p15, 0, %0, c15, c12, 1" : "=r" (cc));
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
