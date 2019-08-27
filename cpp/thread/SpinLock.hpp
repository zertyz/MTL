#ifndef MTL_THREAD_SpinLock_HPP_
#define MTL_THREAD_SpinLock_HPP_

#include <atomic>
#include <iostream>
#include <string>
#include <mutex>
//#include <boost/fiber/detail/cpu_relax.hpp>     // provides cpu_relax() macro, which uses the x86's "pause" or arm's "yield" instructions -- this has been commented out because boost fiber is not present on CentOS 7
//#include <xmmintrin.h>                        // provides _mm_pause(). This would be an alternative to boost dependency, but it only works on x86, as of aug, 2019 -- and doesn't provide _mm_yield() as well...
using namespace std;

#include "../TimeMeasurements.hpp"
using namespace MTL::TimeMeasurements;

// the 'cpu_relax()' is a macro calling a specific machine instruction
//////////////////////////////////////////////////////////////////////
#if __x86_64
    #define cpu_relax() asm volatile ("pause" ::: "memory");
#elif  __arm__
    #define cpu_relax() asm volatile ("yield" ::: "memory");
#else
    #error Unknown CPU
#endif
//////////////////////////////////////////////////////////////////////

// linux kernel macros for optimizing branch instructions
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

namespace mutua::MTL {

    /** conditional base class when using a spin lock with metrics DISABLED */
    struct SpinLockStandardMetricsNoAdditionalFields {};
    /** conditional base class when using a spin lock with metrics ENABLED */
    struct SpinLockStandardMetricsAdditionalFields {
        // metrics variables
        unsigned lockCallsCount, unlockCallsCount, tryLocksCount;
        unsigned realLocksCount, realUnlocksCount;
        uint64_t cpuCyclesLocked, cpuCyclesSpinning;
        // auxiliar variables
        uint64_t lockStart;

        inline void reset() {
            lockCallsCount    = 0;
            unlockCallsCount  = 0;
            realLocksCount    = 0;
            realUnlocksCount  = 0;
            tryLocksCount     = 0;
            cpuCyclesLocked   = 0;
            cpuCyclesSpinning = 0;
            lockStart         = -1; // special val denoting we are unlocked
        }

        inline void debugMetrics(stringstream& c) {
            c << "lockCallsCount="     << lockCallsCount         << ", "
                 "unlockCallsCount="   << unlockCallsCount       << ", "
                 "tryLocksCount="      << tryLocksCount          << ", "
                 "cpuCyclesLocked="    << cpuCyclesLocked        << ", "
                 "cpuCyclesSpinning="  << cpuCyclesSpinning;
            if (lockStart != ((int64_t)-1)) {
                c << /*", lastLockStartCycles="   << lockStart <<*/     // this is most probably an unneeded info...
                     ", lastLockElapseCycles="  << (getProcessorCycleCount()-lockStart);
            }
        }
    };

    /** conditional base class when having metrics enabled and mutex fallback DISABLED */
    struct SpinLockMutexMetricsNoAdditionalFields {};
    /** conditional base class when having metrics enabled and mutex fallback also ENABLED */
    struct SpinLockMutexMetricsAdditionalFields {
        unsigned    mutexFallbackCount;
        std::mutex  mutex;

        inline void reset() {
            mutexFallbackCount = 0;
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

    /** constexpr to check if we must enable code for mutex metrics & statistics */
    template <bool _opMetrics, unsigned long long _mutexFallbackAfterCycles>
    inline constexpr bool isMutexMetricsEnabled() {
        if constexpr (_opMetrics && (_mutexFallbackAfterCycles > 0)) {
            return true;
        } else {
            return false;
        }
    }

    template <unsigned long long _mutexFallbackAfterCycles>
    inline constexpr bool isMutexFallbackEnabled() {
        if constexpr (_mutexFallbackAfterCycles > 0) {
            return true;
        } else {
            return false;
        }
    }

    // pseudo base-class named 'LockSpecialization' used to build
    // both 'MutexLockSpecialization' and 'SpinLockSpecialization'
    // (no implementation of this virtual class is made because
    // we are using templates with conditional compilation)
    // template <bool _useRelaxInstruction>
    // struct LockSpecialization {
    //    inline volatile void _hard_lock();
    //    inline volatile bool _try_lock() noexcept;
    //    inline volatile void _unlock();
    // }

    /** 'std::mutex' based lock specialization */
    template <bool _useRelaxInstruction>
    struct MutexLockSpecialization {
        std::mutex  mutex;
        atomic_uint c=0;
        inline void _hard_lock() {
            // once we get here, the mutex is already locked, so it will stop the thread until an unlock takes place.
            mutex.lock();

        }
        inline bool _try_lock() noexcept {
            return mutex.try_lock();
        }
        inline void _unlock() {
            mutex.unlock();
        }
    };

    /** 'atomic_flag' based lock specialization  */
    template <bool _useRelaxInstruction>
    struct SpinLockSpecialization {
        std::atomic_flag flag = ATOMIC_FLAG_INIT;
        inline void _hard_lock() {
            while (!_try_lock()) {
                if constexpr (_useRelaxInstruction) cpu_relax();
            }
            // note that, as of aug, 27, 2019, this method will never be called
            // due to how SpinLock::lock() implements the 'spinning for too long'
            // debug warning & 'mutex finally locked'. Once this is reviewd and
            // the code paths fixed (probably by removing the double implementation)
            // of the mentioned event, then this method will be called.
        }
        inline bool _try_lock() noexcept {
            return !flag.test_and_set(std::memory_order_relaxed);   // the '!' will be optimized for '!try_lock()' tests
        }
        inline void _unlock() {
            flag.clear(std::memory_order_release);
        }
    };


    /**
     * SpinLock.hpp
     * ============
     * created by luiz, Aug 20, 2019
     *
     * Provides a spin-lock (a lock designed to be more efficient than a mutex in "instant wait" situations,
     * for it does not cause a contex-switch), but with the ability to revert to a system mutex if it is
     * locked for more than a specified number of CPU Cycles.
     *
     * When used as a pure spin lock, this class has ~300 times less latency than a mutex;
     * When used in "System Mutex Fallback" mode, it has 
     *
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
               *   - cpuCyclesLocked / cpuCyclesSpinning
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
              /*** CONDITIONAL BASE CLASSES DECLARATION -- in order to allow conditional fields ***/
              // which lock to use? spin (atomic_flag) or system mutex?
              : std::conditional<isMutexFallbackEnabled<_mutexFallbackAfterCycles>(),             MutexLockSpecialization<_useRelaxInstruction>,   SpinLockSpecialization<_useRelaxInstruction>>::type
              // additional class fields based on template parameters
              , std::conditional<_opMetrics,                                                      SpinLockStandardMetricsAdditionalFields,         SpinLockStandardMetricsNoAdditionalFields>::type
              , std::conditional<isMutexMetricsEnabled<_opMetrics, _mutexFallbackAfterCycles>(),  SpinLockMutexMetricsAdditionalFields,            SpinLockMutexMetricsNoAdditionalFields>::type
    {

        // conditional code functionalities from the template parameters -- see the docs on the associated template parameters
        static constexpr bool doUseRelaxInstruction      = _useRelaxInstruction;
        static constexpr bool doCollectStandardMetrics   = _opMetrics;
        static constexpr bool doCollectMutextMetrics     = isMutexMetricsEnabled<_opMetrics, _mutexFallbackAfterCycles>();
        static constexpr bool doDebugSpinTimeouts        = isSpinLockDebugEnabled<_debugAfterCycles>();
        static constexpr bool doWaitForMutexFallback     = isMutexFallbackEnabled<_mutexFallbackAfterCycles>();
        static constexpr bool doInstrumentLocks          = _instrumentLockCallback      != nullptr;
        static constexpr bool doInstrumentUnlocks        = _instrumentUnlockCallback    != nullptr;
        static constexpr bool doInstrumentBusyWaits      = _instrumentBusyWaitCallback  != nullptr;
        static constexpr bool doInstrumentMutexFallbacks = _instrumentMutexLockCallback != nullptr;


        // TODO: the following '_hard_lock', '_try_lock' and '_unlock' implementations shouldn't be needed, but on C++17,
        //       both clang and gcc requires them (gcc allows if we use the -fpermissive compilation flag).
        //       Will them still be needed in C++20?
        inline void _hard_lock() {
            // calls either MutexLockSpecialization::_hard_lock() or SpinLockSpecialization::_hard_lock()
            std::conditional<isMutexFallbackEnabled<_mutexFallbackAfterCycles>(),
                             MutexLockSpecialization<_useRelaxInstruction>,
                             SpinLockSpecialization<_useRelaxInstruction>>::type::_hard_lock();
        }
        inline bool _try_lock() noexcept {
            // calls either MutexLockSpecialization::_try_lock() or SpinLockSpecialization::_try_lock()
            return std::conditional<isMutexFallbackEnabled<_mutexFallbackAfterCycles>(),
                                   MutexLockSpecialization<_useRelaxInstruction>,
                                   SpinLockSpecialization<_useRelaxInstruction>>::type::_try_lock();
        }
        inline void _unlock() {
            // calls either MutexLockSpecialization::_unlock() or SpinLockSpecialization::_unlock()
            std::conditional<isMutexFallbackEnabled<_mutexFallbackAfterCycles>(),
                             MutexLockSpecialization<_useRelaxInstruction>,
                             SpinLockSpecialization<_useRelaxInstruction>>::type::_unlock();
        }


    public:

        SpinLock() {
            // asserts on template parameters
            static_assert((_mutexFallbackAfterCycles == 0) || (_debugAfterCycles == 0) || (_debugAfterCycles < _mutexFallbackAfterCycles),
                          "Template parameter '_debugAfterCycles' cannot be greater than '_mutexFallbackAfterCycles' -- "
                          "otherwise the requested debug messages would never be shown. You must either correct the "
                          "relation or set one of them to zero.");
            // special cases
            if constexpr (doCollectStandardMetrics) {
                SpinLockStandardMetricsAdditionalFields::reset();
            }
            if constexpr (doCollectMutextMetrics) {
                SpinLockMutexMetricsAdditionalFields::reset();
            }
/*            if constexpr (doDebugSpinTimeouts) {
                issueDebugMessage("Just created the mutex");
            }*/
        }

        inline void issueDebugMessage(string message) {
            stringstream c;
            if constexpr (_debugName != nullptr) {
                c << "MTL::SpinLock('" << _debugName << "'): " << message << ": {";
            } else {
                c << "MTL::SpinLock: " << message << ": {";
            }
            if constexpr (doCollectStandardMetrics) {
                SpinLockStandardMetricsAdditionalFields::debugMetrics(c);
            }
            if constexpr (doCollectMutextMetrics) {
                SpinLockMutexMetricsAdditionalFields::debugMutex(c);
            }
            c << "}\n";
            std::cerr << c.str() << std::flush;
        }

        inline void lock() {

            // these variables will be optimized out if the conditional
            // codes in the 'if constexpr's bellow are not used
            uint64_t waitingToLockStart;                // measure the cycles spent spinning
            bool     spinningTooMuchDebugged = false;   // will be true if a "spinning for too long" debug message was issued

            // conditional for 'lockCallsCount' and, possibly, 'cpuCyclesSpinning' metrics
            if constexpr (doCollectStandardMetrics) {
                SpinLockStandardMetricsAdditionalFields::lockCallsCount++;
                // flag is already locked?
                if (_try_lock()) {
                    SpinLockStandardMetricsAdditionalFields::lockStart = getProcessorCycleCount();  // will be used to increment 'cpuCyclesLocked'
                    // if it wasn't lock (remember that now it is), bail out since the work is done
                    // and we don't want to touch 'cpuCyclesSpinning' because we didn't spin at all
                    return ;
                }
            }

            if constexpr (doDebugSpinTimeouts || doWaitForMutexFallback) {
                    // 'waitingToLockStart' is used to detect when to issue a debug warning on
                    // 'spinning for too long' and 'fallback to system mutex' events.
                    // if 'doCollectStandardMetrics' is enabled, it will also be used to
                    // increment 'cpuCyclesSpinning' when the lock is acquired (or when
                    // if falls back to a system mutex)
                    waitingToLockStart = getProcessorCycleCount();
            }

            // do the following until we may acquire the lock...
            while (!_try_lock())  {

                // calls PAUSE or YIELD instruction, releasing the core to another CPU without context switching
                // (else just do a normal busy wait)
                if constexpr (doUseRelaxInstruction) cpu_relax();

                // spin timeouts & mutex fall back optional codes
                if constexpr (doDebugSpinTimeouts || doWaitForMutexFallback) {

                    uint64_t elapsedCycles = getProcessorCycleCount()-waitingToLockStart;

                    // conditional for debugging "spinning for too long" conditions
                    if constexpr (doDebugSpinTimeouts) {
                        if ((elapsedCycles > _debugAfterCycles) && (!spinningTooMuchDebugged)) {
                            issueDebugMessage("Waiting too long to acquire a lock");
                            spinningTooMuchDebugged = true;
                        }
                    }

                    // conditional for falling backing to a system mutex -- note that the system
                    // mutex code has already been inherited via 'MutexLockSpecialization'
                    // and that what we really have to do now is to "hard lock" it --
                    // the call to "_hard_lock()" will only return when 'unlock()' is called in another thread
                    if constexpr (doWaitForMutexFallback) {
                        if (elapsedCycles > _mutexFallbackAfterCycles) {
                            // when we call '_hard_lock()' bellow, we must assume "the lock has finally been acquired"
                            // event already happened -- which, when using a spin lock, only happens after
                            // this loop ends. Therefore, we must execute that event's code before the
                            // "hard lock" takes place.

                            /*** PLEASE, SEARCH THIS TAG AND KEEP THIS CODE THE SAME -- OR PUT THEM INTO A DEFINE ***/
                            // conditional for giving a satisfaction on any eventually issue "spinning for too long" message
                            if constexpr (doDebugSpinTimeouts) if (spinningTooMuchDebugged) {
                                issueDebugMessage("Falling back to system mutex' hard lock");
                            }

                            // conditional for 'realLocksCount' and 'cpuCyclesSpinning' metrics
                            if constexpr (doCollectStandardMetrics) {
                                SpinLockStandardMetricsAdditionalFields::realLocksCount++;
                                SpinLockStandardMetricsAdditionalFields::lockStart = getProcessorCycleCount();   // will be used to increment 'cpuCyclesLocked' when this gets unlocked
                                SpinLockStandardMetricsAdditionalFields::cpuCyclesSpinning += SpinLockStandardMetricsAdditionalFields::lockStart-waitingToLockStart;
                            }
                            /*** END OF THE COPY & KEEP TAG ***/

                            if constexpr (doCollectMutextMetrics) {
                                SpinLockMutexMetricsAdditionalFields::mutexFallbackCount++;
                            }

                            _hard_lock();    // really blocks the execution of this thread for an undefined ammount of time

                            // now we must ignore the rest of this function, for the mutex
                            // will have already been unlocked at this point and the
                            // "lock has finally been acquired event" have already been accounted for
                            return;
                        }
                    }

                }

            }

            // the 'lock has finally been acquired' event:

            /*** PLEASE, SEARCH THIS TAG AND KEEP THIS CODE THE SAME -- OR PUT THEM INTO A DEFINE ***/
            // conditional for giving a satisfaction on any eventually issue "spinning for too long" message
            if constexpr (doDebugSpinTimeouts) if (spinningTooMuchDebugged) {
                issueDebugMessage("Lock finally acquired");
            }

            // conditional for 'realLocksCount' and 'cpuCyclesSpinning' metrics
            if constexpr (doCollectStandardMetrics) {
                SpinLockStandardMetricsAdditionalFields::realLocksCount++;
                SpinLockStandardMetricsAdditionalFields::lockStart = getProcessorCycleCount();   // will be used to increment 'cpuCyclesLocked' when this gets unlocked
                SpinLockStandardMetricsAdditionalFields::cpuCyclesSpinning += SpinLockStandardMetricsAdditionalFields::lockStart-waitingToLockStart;
            }
            /*** END OF THE COPY & KEEP TAG ***/

        }


        inline bool try_lock() {
            
            if constexpr (doCollectStandardMetrics) {
                SpinLockStandardMetricsAdditionalFields::tryLocksCount++;
                // are we already locked?
                if (unlikely (!_try_lock()) ) {
                    return false;
                }
                SpinLockStandardMetricsAdditionalFields::lockStart = getProcessorCycleCount();   // will be used to increment 'cpuCyclesLocked' when this gets unlocked
                return true;
            }

            return _try_lock();
        }


        inline void unlock() {

            // conditional for 'unlockCallsCount', 'realUnlocksCount' and 'cpuCyclesLocked' metrics
            if constexpr (doCollectStandardMetrics) {
                SpinLockStandardMetricsAdditionalFields::unlockCallsCount++;
                // were we really locked?
                if (likely (SpinLockStandardMetricsAdditionalFields::lockStart != ((int64_t)-1)) ) {
                    SpinLockStandardMetricsAdditionalFields::realUnlocksCount++;
                    SpinLockStandardMetricsAdditionalFields::cpuCyclesLocked += getProcessorCycleCount() - SpinLockStandardMetricsAdditionalFields::lockStart;
                    SpinLockStandardMetricsAdditionalFields::lockStart = ((int64_t)-1);     // get back to the special val denoting we are unlocked
                }
            }

            _unlock();
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
