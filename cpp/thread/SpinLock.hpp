#ifndef MTL_THREAD_SpinLock_HPP_
#define MTL_THREAD_SpinLock_HPP_

#include <atomic>
#include <iostream>
#include <string>
#include <mutex>
#include <boost/fiber/detail/cpu_relax.hpp>     // provides cpu_relax() macro, which uses the x86's "pause" or arm's "yield" instructions
//#include <xmmintrin.h>                        // provides _mm_pause(). This would be an alternative to boost dependency, but it only works on x86, as of aug, 2019 -- and doesn't provide _mm_yield() as well...
using namespace std;

#include "../TimeMeasurements.hpp"
using namespace MTL::TimeMeasurements;


// linux kernel macros for optimizing branch instructions
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

namespace mutua::MTL {

    /** conditional base class when using a spin lock with metrics DISABLED */
    struct SpinLockMetricsNoAdditionalFields {};
    /** conditional base class when using a spin lock with metrics ENABLED */
    struct SpinLockMetricsAdditionalFields {
        // metrics variables
        unsigned lockCallsCount, unlockCallsCount, tryLocksCount;
        unsigned realLocksCount, realUnlocksCount;
        uint64_t cpuCyclesLocked, cpuCyclesWaitingToLock;
        // auxiliar variables
        uint64_t lockStart;

        inline void resetSpinLockMetricsAdditionalFields() {
            lockCallsCount         = 0;
            unlockCallsCount       = 0;
            realLocksCount         = 0;
            realUnlocksCount       = 0;
            tryLocksCount          = 0;
            cpuCyclesLocked        = 0;
            cpuCyclesWaitingToLock = 0;
            lockStart              = -1; // special val denoting we are unlocked
        }

        inline void debugMetrics(stringstream& c) {
            c << "lockCallsCount="         << lockCallsCount         << ", "
                 "unlockCallsCount="       << unlockCallsCount       << ", "
                 "tryLocksCount="          << tryLocksCount          << ", "
                 "cpuCyclesLocked="        << cpuCyclesLocked        << ", "
                 "cpuCyclesWaitingToLock=" << cpuCyclesWaitingToLock << ", "
                 "lastLockStartCycles="    << lockStart;
            if (lockStart != ((int64_t)-1)) {
                c << ", lastLockStartCycles="   << lockStart <<
                     ", lastLockElapseCycles="  << (getProcessorCycleCount()-lockStart);
            } else {
                c << ", lastLockElapseCycles=-1";
            }
        }
    };

    /** conditional base class when using a spin lock with mutex fallback DISABLED */
    struct SpinLockMutexFallbackNoAdditionalFields {};
    /** conditional base class when using a spin lock with mutex fallback ENABLED */
    struct SpinLockMutexFallbackAdditionalFields {
        unsigned    mutexFallbackCount;
        std::mutex  mutex;

        inline void resetSpinLockMutexFallbackAdditionalFields() {
            mutexFallbackCount = 0;
            mutex.unlock();
            mutex.lock();   // the fallback mutex is left on a pre-lock state
        }

        inline void debugMutex(stringstream& c) {
            c << ", mutexFallbackCount=" << mutexFallbackCount;
        }
    };

    /** signature type for our callbacks */
    typedef void(*SpinLockCallbackSignature)(size_t);

    /** constexpr to check if the 'SpinLock' templates options allows for additional debugging fields and operations */
    template <unsigned long long _debugAfterCycles>
    inline constexpr bool isSpinLockDebugEnabled() {
        return _debugAfterCycles > 0;
    }

    template <bool _opMetrics, unsigned long long _mutexFallbackAfterCycles>
    inline constexpr bool isSpinLockMutexFallbackEnabled() {
        if constexpr (_mutexFallbackAfterCycles > 0) {
            static_assert(_opMetrics, "MTL::SpinLock: to enable system mutex fallback you must also enable '_opMetrics'");
            return true;
        } else {
            return false;
        }
    }

    /** fields required on all sub-specializations of a 'SpinLock' */
    struct SpinLockCommonFields {
        std::atomic_flag flag = ATOMIC_FLAG_INIT;
    };

    /**
     * SpinLock.hpp
     * ============
     * created by luiz, Aug 20, 2019
     *
     * Provides a spin-lock (a lock designed to be more efficient than a mutex in "instant wait" situations,
     * for it does not cause a contex-switch), but with the ability to revert to a system mutex if it is
     * locked for more than a specified number of CPU Cycles.

     * Another use case for this class is when one wants to collect metrics or debug reentrancy problems.
     * See the template parameters for more info.
     *
    */
    template<
             /** when true, use the PAUSE (x86) or YIELD (arm) instruction, in attempt to
               * instruct the CORE to give priority to another CPU doing useful work or
               * save some power */
             bool      _useRelaxInstruction = true,
             /** when true, provides some operational metrics to keep track of how many
               * times and how many CPU Cycles where spent:
               *   - lockCallsCount / unlockCallsCount / tryLocksCount
               *   - realLocksCount, realUnlocksCount
               *   - cpuCyclesLocked / cpuCyclesWaitingToLock
               * at the extra cost of setting two unatomic integers per operation */
             bool      _opMetrics  = false,
             /** when set to non 0, and requiring that '_opMetrics' is set, issues a
               * debug message to 'stderr' (with the metrics collected so far)
               * whenever that number of cpu cycles is spent on the lock state */
             unsigned long long  _debugAfterCycles = 0,
             /** when set to non nullptr, and requiring that '_debugAfterCycles' is
               * non 0, specifies this object's name to be placed in any debug messages */
             const char* _debugName = nullptr,
             /** when set to non 0, and requiring that '_opMetrics' is set, reverts the lock
               * to a system mutex whenever a spin lock wastes more than the given number of
               * CPU cycles, at the extra cost of an unconditional "mutex unlock" operation
               * per unlock, what shouldn't provoke a contex-switch */
             unsigned long long  _mutexFallbackAfterCycles = 0,
             /** if enabled, calls the several "(void) (size_t lockId)" functions defined
               * in the '_instrumentXXXXX' variables  */
             bool      _instrument                  = false,
             /** if provided, specifies a "(void) (size_t lockId)" function to be called
               * whenever a lock is issued  */
             SpinLockCallbackSignature _instrumentLockCallback      = nullptr,
             SpinLockCallbackSignature _instrumentUnlockCallback    = nullptr,
             SpinLockCallbackSignature _instrumentBusyWaitCallback  = nullptr,
             SpinLockCallbackSignature _instrumentMutexLockCallback = nullptr>   // enable to output to stderr debug information & activelly check for reentrancy errors
    class SpinLock
              // common fields among all sub-specializations
              : SpinLockCommonFields
              // additional fields based on template parameters
              , std::conditional<_opMetrics,                                                               SpinLockMetricsAdditionalFields,        SpinLockMetricsNoAdditionalFields>::type
              , std::conditional<isSpinLockMutexFallbackEnabled<_opMetrics, _mutexFallbackAfterCycles>(),  SpinLockMutexFallbackAdditionalFields,  SpinLockMutexFallbackNoAdditionalFields>::type
    {

    public:

        SpinLock() {
            // special cases
            if constexpr (_opMetrics) {
                SpinLockMetricsAdditionalFields::resetSpinLockMetricsAdditionalFields();
            }
            if constexpr (isSpinLockMutexFallbackEnabled<_opMetrics, _mutexFallbackAfterCycles>()) {
                SpinLockMutexFallbackAdditionalFields::resetSpinLockMutexFallbackAdditionalFields();
            }
        }

        inline void issueDebugMessage(string message) {
            stringstream c;
            if constexpr (_debugName != nullptr) {
                c << "MTL::SpinLock('" << _debugName << "'): " << message << ": {";
            } else {
                c << "MTL::SpinLock: " << message << ": {";
            }
            if constexpr (_opMetrics) {
                SpinLockMetricsAdditionalFields::debugMetrics(c);
            }
            if constexpr (isSpinLockMutexFallbackEnabled<_opMetrics, _mutexFallbackAfterCycles>()) {
                SpinLockMutexFallbackAdditionalFields::debugMutex(c);
            }
            c << "}\n";
            std::cerr << c.str() << std::flush;
        }

        inline void lock() {

            // these variables will be optimized out if the template vars
            // unsed in the 'if constexpr's bellow are not set
            uint64_t waitingToLockStart;                // measure the cycles spent spinning
            bool     spinningTooMuchDebugged = false;   // will be true if a "spinning for too long" debug message was issued

            // conditional for 'lockCallsCount' and, possibly, 'cpuCyclesWaitingToLock' metrics
            if constexpr (_opMetrics) {
                SpinLockMetricsAdditionalFields::lockCallsCount++;
                // flag is already locked?
                if (flag.test_and_set(std::memory_order_relaxed)) {
                    waitingToLockStart = getProcessorCycleCount();     // will, eventually, be used to increment 'cpuCyclesWaitingToLock'
                } else {
                    SpinLockMetricsAdditionalFields::lockStart = getProcessorCycleCount();  // will be used to increment 'cpuCyclesLocked'
                    // if it wasn't lock (remember that now it is), bail out since the work is done
                    // and we don't want to touch 'cpuCyclesWaitingToLock' because we didn't wait at all
                    return ;
                }
            }

            // until we may acquire the lock...
            while (flag.test_and_set(std::memory_order_relaxed))  {

                // calls PAUSE or YIELD instruction, releasing the core to another CPU without context switching
                // (else just do a normal busy wait)
                if constexpr (_useRelaxInstruction) cpu_relax();

                // debug & mutex fall back optional codes
                if constexpr (isSpinLockDebugEnabled<_debugAfterCycles>() ||
                              isSpinLockMutexFallbackEnabled<_opMetrics, _mutexFallbackAfterCycles>()) {

                    uint64_t elapsedCycles = getProcessorCycleCount()-waitingToLockStart;

                    // conditional for debugging "spinning for too long" conditions
                    if constexpr (isSpinLockDebugEnabled<_debugAfterCycles>()) {
                        if ((elapsedCycles > _debugAfterCycles) && (!spinningTooMuchDebugged)) {
                            issueDebugMessage("Waiting too long to acquire a lock");
                            spinningTooMuchDebugged = true;
                        }
                    }

                    // conditional for fallbacking to a system mutex
                    if constexpr (isSpinLockMutexFallbackEnabled<_opMetrics, _mutexFallbackAfterCycles>()) {
                        if (elapsedCycles > _mutexFallbackAfterCycles) {
                            SpinLockMutexFallbackAdditionalFields::mutexFallbackCount++;
std::cerr << "mlock:\n" << std::flush;
                            SpinLockMutexFallbackAdditionalFields::mutex.lock();
std::cerr << ":mlocked\n" << std::flush;
                        }
                    }

                }

            }

            // conditional for giving a satisfaction on any eventually issue "spinning for too long" message
            if constexpr (isSpinLockDebugEnabled<_debugAfterCycles>()) if (spinningTooMuchDebugged) {
                issueDebugMessage("Lock finally acquired");
            }

            // conditional for 'realLocksCount' and 'cpuCyclesWaitingToLock' metrics
            if constexpr (_opMetrics) {
                SpinLockMetricsAdditionalFields::realLocksCount++;
                SpinLockMetricsAdditionalFields::lockStart = getProcessorCycleCount();   // will be used to increment 'cpuCyclesLocked' when this gets unlocked
                SpinLockMetricsAdditionalFields::cpuCyclesWaitingToLock += SpinLockMetricsAdditionalFields::lockStart-waitingToLockStart;
            }

        }


        inline bool try_lock() {
            
            if constexpr (_opMetrics) {
                SpinLockMetricsAdditionalFields::tryLocksCount++;
                // are we already locked?
                if (unlikely (flag.test_and_set(std::memory_order_relaxed)) ) {
                    return false;
                }
                SpinLockMetricsAdditionalFields::lockStart = getProcessorCycleCount();   // will be used to increment 'cpuCyclesLocked' when this gets unlocked
                return true;
            }

            return !flag.test_and_set(std::memory_order_relaxed);
        }


        inline void unlock() {

            // conditional for 'unlockCallsCount', 'realUnlocksCount' and 'cpuCyclesLocked' metrics
            if constexpr (_opMetrics) {
                SpinLockMetricsAdditionalFields::unlockCallsCount++;
                // were we really locked?
                if (likely (SpinLockMetricsAdditionalFields::lockStart != ((int64_t)-1)) ) {
                    SpinLockMetricsAdditionalFields::realUnlocksCount++;
                    SpinLockMetricsAdditionalFields::cpuCyclesLocked += getProcessorCycleCount() - SpinLockMetricsAdditionalFields::lockStart;
                    SpinLockMetricsAdditionalFields::lockStart = ((int64_t)-1);     // get back to the special val denoting we are unlocked
                }
            }

            // conditional for clearing any fallback to a system mutex
            if constexpr (isSpinLockMutexFallbackEnabled<_opMetrics, _mutexFallbackAfterCycles>()) {
                SpinLockMutexFallbackAdditionalFields::mutex.unlock();
                SpinLockMutexFallbackAdditionalFields::mutex.try_lock();    // put it back in the pre-lock state
std::cerr << ":munlocked\n" << std::flush;
            }

            flag.clear(std::memory_order_release);
        }

        template <typename _type>
        inline bool non_atomic_compare_exchange(_type& val, _type& old_val, _type new_val) {
            lock();
            if (val == old_val) {
                val = new_val;
                unlock();
                return true;
            } else {
                old_val = val;
                unlock();
                return false;
            }
        }
    };
}

#undef likely
#undef unlikely

#endif /* MTL_THREAD_SpinLock_HPP_ */
