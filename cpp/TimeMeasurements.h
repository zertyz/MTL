#ifndef HASHTABLESPIKES_TIMEMEASUREMENTS_H
#define HASHTABLESPIKES_TIMEMEASUREMENTS_H

#include <time.h>
#include <sys/time.h>

/** <pre>
 * TimeMeasurements.cpp
 * ====================
 * created by luiz, Sep 19, 2015
 *
 * Real Time measurement functions -- milliseconds and microseconds.
 *
 * None of these functions are reentrant.
 *
*/
namespace mutua::utils::TimeMeasurements {

    static inline unsigned long long getRealTimeMS();
    static inline unsigned long long getRealTimeUS();
    static inline unsigned long long getMonotonicRealTimeMS();
    static inline unsigned long long getMonotonicRealTimeUS();
    static inline unsigned long long getMonotonicRealTimeNS();

}


// inlined functions
using namespace mutua::utils;

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

#endif //HASHTABLESPIKES_TIMEMEASUREMENTS_H
