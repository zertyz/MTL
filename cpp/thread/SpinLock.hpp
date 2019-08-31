/*! \file SpinLock.hpp
    \brief A drop-in for mutexes, with steroid options for statistics, debug and, specially, performance.
    
    Maybe one wants to write more...
*/

#ifndef MTL_THREAD_SpinLock_hpp_
#define MTL_THREAD_SpinLock_hpp_

#include <atomic>
#include <iostream>
#include <string>
#include <mutex>
//#include <boost/fiber/detail/cpu_relax.hpp>     // provides cpu_relax() macro, which uses the x86's "pause" or arm's "yield" instructions -- this has been commented out because boost fiber is not present on CentOS 7
//#include <xmmintrin.h>                        // provides _mm_pause(). This would be an alternative to boost dependency, but it only works on x86, as of aug, 2019 -- and doesn't provide _mm_yield() as well...
using namespace std;

#include "../time/TimeMeasurements.hpp"
using namespace MTL::time::TimeMeasurements;

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

namespace MTL::thread {

    /** conditional base class when using a spin lock with metrics DISABLED */
    struct SpinLockStandardMetricsNoAdditionalFields {};
    /** conditional base class when using a spin lock with metrics ENABLED */
    struct SpinLockStandardMetricsAdditionalFields {
        // metrics variables
        unsigned lockCallsCount, unlockCallsCount, tryLocksCount;
        uint64_t cpuCyclesLocked, cpuCyclesWaitingToLock;
        // auxiliar variables
        uint64_t lockStart;

        inline void reset() {
            lockCallsCount         = 0;
            unlockCallsCount       = 0;
            tryLocksCount          = 0;
            cpuCyclesLocked        = 0;
            cpuCyclesWaitingToLock = 0;
            lockStart              = !(uint64_t)0; // special value (to debug messages) denoting we are unlocked
        }

        inline void debugMetrics(stringstream& c) {
            c << "lockCallsCount="          << lockCallsCount         << ", "
                 "unlockCallsCount="        << unlockCallsCount       << ", "
                 "tryLocksCount="           << tryLocksCount          << ", "
                 "cpuCyclesLocked="         << cpuCyclesLocked        << ", "
                 "cpuCyclesWaitingToLock="  << cpuCyclesWaitingToLock;
            if (lockStart != !(uint64_t)0) {
                c << /*", lastLockStartCycles="   << lockStart <<*/     // this is most probably an unneeded info...
                     ", lastLockElapseCycles="  << (getProcessorCycleCount()-lockStart);
            }
        }
    };

    /** conditional base class when having metrics enabled and hard_lock fallback DISABLED */
    struct SpinLockHardLockMetricsNoAdditionalFields {};
    /** conditional base class when having metrics enabled and hard_lock fallback also ENABLED */
    struct SpinLockHardLockMetricsAdditionalFields {
        unsigned hardLocksCount;
        inline void reset() {
        	hardLocksCount = 0;
        }

        inline void debugHardLockMetrics(stringstream& c) {
            c << ", hardLocksCount="          << hardLocksCount;
        }
    };

    /** signature type for our callbacks */
    typedef void(*SpinLockCallbackSignature)(size_t);

    /** constexpr to check if the 'SpinLock' templates options allows for additional debugging fields and operations */
    template <unsigned long long _debugAfterCycles>
    inline constexpr bool isSpinLockDebugEnabled() {
        return _debugAfterCycles != !(uint64_t)0;
    }

    /** constexpr to check if we must enable code for hard_lock metrics & statistics */
    template <bool _opMetrics, unsigned long long _hardLockFallbackAfterCycles>
    inline constexpr bool isHardLockMetricsEnabled() {
        if constexpr (_opMetrics && (_hardLockFallbackAfterCycles != !(uint64_t)0)) {
            return true;
        } else {
            return false;
        }
    }

    template <unsigned long long _hardLockFallbackAfterCycles>
    inline constexpr bool isHardLockFallbackEnabled() {
        if constexpr (_hardLockFallbackAfterCycles != !(uint64_t)0) {
            return true;
        } else {
            return false;
        }
    }

    /** These are the classes inheriting from 'LockSpecialization' which implement their particular ways of guarding critical sessions of code */
    enum ELockSpecializations {
        /** Uses a regular system mutex to do the locking. Depending on the template options given to the MTL's `SpinLock` instance,
         *  spins are possible (using 'try_lock()'). If no options that require control over the wait process are used, calls to
         *  [SpinLock::lock()](@ref SpinLock::lock()), [SpinLock.unlock()](@ref SpinLock.unlock()) and [SpinLock.try_lock()](@ref SpinLock.try_lock())
         *  are optimized to direct 'mutex' calls, with no extra code. This feature makes MTL's `SpinLock` template class as drop-in &
         *  weight-less replacement for `mutex`es. */
        Mutex,
        /** Uses a naive spin strategy based on `atomic_flag`. The naive implementation assumes you will have total control over
         *  the number of stakeholders -- if you use the minimum possible numbers, this specialization will, most likely, give you
         *  100 times less latency than a `mutex` -- the exception goes for some CPU's which have very costly RMW (read, modify, write)
         *  operations, like Raspberry Pi 2 (ARMv7, 4 cores) and Intel SandyBridge (48 cores). On these machines,
         *  @ref RMWLightLockSpecialization may be a better choice. */
        AtomicFlag,
        /** Also uses atomic types to control the critical sessions, but try to reduce the number of RMW (read, modify, write) operations
          * by using an `atomic_bool`, which allows fetching the state of the flag without modifying it. This is known to heavily improve
          * performance, in relation to @ref AtomicFlag,  on Raspberry Pi 2. */
        RMWLight,
    };

    // pseudo base-class named 'LockSpecialization' used to build
    // both 'MutexLockSpecialization' and 'AtomicFlagLockSpecialization'
    // (no implementation of this virtual class is made because
    // we are using templates with conditional compilation)
    // template <bool _useRelaxInstruction>
    // struct LockSpecialization {
    //    inline volatile void _hard_lock();
    //    inline volatile bool _try_lock() noexcept;
    //    inline volatile void _unlock();
    // }

    /** 'std::mutex' based lock specialization. See more in [coco](@ref ELockSpecializations::mutex) */
    template <bool _useRelaxInstruction>
    struct MutexLockSpecialization {
        std::mutex  mutex;
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
    struct AtomicFlagLockSpecialization {
        std::atomic_flag flag = ATOMIC_FLAG_INIT;
        inline void _hard_lock() {
            while (!_try_lock()) {      // the double negation '!!' will be optimized out by the compiler
//            	cerr << "!" << flush;
                if constexpr (_useRelaxInstruction) cpu_relax();
            }
        }
        inline bool _try_lock() noexcept {
            return !flag.test_and_set(std::memory_order_relaxed);   // the '!' will be optimized for '!try_lock()' tests
        }
        inline void _unlock() {
            flag.clear(std::memory_order_release);
        }
    };

    /** 'atomic_flag' based lock specialization  */
    template <bool _useRelaxInstruction>
    struct RMWLightLockSpecialization {
        atomic_bool flag = ATOMIC_VAR_INIT(false);
        inline void _hard_lock() {
            while (!_try_lock()) {      // the double negation '!!' will be optimized out by the compiler
                if constexpr (_useRelaxInstruction) cpu_relax();
            }
        }
        inline bool _try_lock() noexcept {
            // this conditional will only execute the RMW `exchange` instruction when the flag will be turned from `false` to `true`.
            // when already locked, it will only execute the cheaper `atomic_load` instruction.
            return !(flag.load(std::memory_order_relaxed) || flag.exchange(true, std::memory_order_release));
        }
        inline void _unlock() {
            flag.store(false, std::memory_order_release);
        }
    };

    /** Type traits for returning one of the `LockSpecialization` derived classes based on provided `ELockSpecializations` enum member.
     *  (used by [SpinLock](@ref SpinLock) when determining which class should be one of it's bases) */
    template <bool _useRelaxInstruction, ELockSpecializations _sp> struct TLockSpecialization {static_assert("false", "Unknown 'ELockSpecializations' used when attempting to get an instance of 'TLockSpecialization'. Please fix the template's type traits selection");};
    template <bool _useRelaxInstruction> struct TLockSpecialization<_useRelaxInstruction, ELockSpecializations::Mutex>      {typedef MutexLockSpecialization<_useRelaxInstruction> type;};
    template <bool _useRelaxInstruction> struct TLockSpecialization<_useRelaxInstruction, ELockSpecializations::AtomicFlag> {typedef AtomicFlagLockSpecialization<_useRelaxInstruction> type;};
    template <bool _useRelaxInstruction> struct TLockSpecialization<_useRelaxInstruction, ELockSpecializations::RMWLight>   {typedef RMWLightLockSpecialization<_useRelaxInstruction> type;};


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
             bool  _useRelaxInstruction = true,
             /** Specifies which code's critical session guarding strategy to use */
             ELockSpecializations _lockStrategy = ELockSpecializations::Mutex,
             /** when true, provides some operational metrics to keep track of how many
               * times and how many CPU Cycles where spent:
               *   - lockCallsCount / unlockCallsCount / tryLocksCount
               *   - realLocksCount, realUnlocksCount
               *   - cpuCyclesLocked / cpuCyclesWaitingToLock
               * at the extra cost of setting two non-atomic integers per operation */
             bool  _opMetrics  = false,
             /** when set to non 0, and requiring that '_opMetrics' is set, issues a
               * debug message to 'stderr' (with the metrics collected so far)
               * whenever that number of cpu cycles is spent on the lock state */
             uint64_t  _debugAfterCycles = !(uint64_t)0,
             /** when set to non nullptr, and requiring that '_debugAfterCycles' is
               * non 0, specifies this object's name to be placed in any debug messages */
             const char* _debugName = nullptr,
             /** when set to non 0, and requiring that '_opMetrics' is set, reverts the lock
               * to a system mutex whenever a spin lock wastes more than the given number of
               * CPU cycles, at the extra cost of an unconditional "mutex unlock" operation
               * per unlock, what shouldn't provoke a contex-switch */
             uint64_t  _hardLockFallbackAfterCycles = !(uint64_t)0,
             /** if enabled, calls the several "(void) (size_t lockId)" functions defined
               * in the '_instrumentXXXXX' variables  */
             bool  _instrument                  = false,
             /** if provided, specifies a "(void) (size_t lockId)" function to be called
               * whenever a lock is issued  */
             SpinLockCallbackSignature _instrumentLockCallback      = nullptr,
             SpinLockCallbackSignature _instrumentUnlockCallback    = nullptr,
             SpinLockCallbackSignature _instrumentBusyWaitCallback  = nullptr,
             SpinLockCallbackSignature _instrumentMutexLockCallback = nullptr>   // enable to output to stderr debug information & activelly check for reentrancy errors
    class SpinLock
              /*** CONDITIONAL BASE CLASSES DECLARATION -- in order to allow conditional fields ***/
              : TLockSpecialization<_useRelaxInstruction, _lockStrategy>::type
//              // which lock to use? spin (atomic_flag) or system mutex?
//              : std::conditional<isMutexFallbackEnabled<_mutexFallbackAfterCycles>(),             MutexLockSpecialization<_useRelaxInstruction>,   AtomicFlagLockSpecialization<_useRelaxInstruction>>::type
              // additional class fields based on template parameters
              , std::conditional<_opMetrics,                                                            SpinLockStandardMetricsAdditionalFields,   SpinLockStandardMetricsNoAdditionalFields>::type
              , std::conditional<isHardLockMetricsEnabled<_opMetrics, _hardLockFallbackAfterCycles>(),  SpinLockHardLockMetricsAdditionalFields,   SpinLockHardLockMetricsNoAdditionalFields>::type
    {

        // conditional code functionalities from the template parameters -- see the docs on the associated template parameters //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /** specifies if a controlled spin is really needed or if we can delegate the lock to one of the 'XXXXSpecialization' classes */
        static constexpr bool doControlledSpin           = ( (_debugAfterCycles != !(uint64_t)0) || (_hardLockFallbackAfterCycles != !(uint64_t)0) ) &&    // at least one of them are activated (non -1)
                                                           ( (_debugAfterCycles > 0)            || (_hardLockFallbackAfterCycles > 0) );                   // one of them implies waiting non-zero time
        /** specifies if pre and/or post code must be included to compute the usage this lock */
        static constexpr bool doCollectStandardMetrics   = _opMetrics;
        /** when doing a controlled spin and not using a System Mutex (_mutexFallbackAfterCycles == -1), this instructs any
         *  spin (both controlled or performing a hard lock) to use the PAUSE x86's assembly instruction or ARM's YIELD */
        static constexpr bool doUseRelaxInstruction      = _useRelaxInstruction;
        /** candidate for removal -- since we may now compute 'hardLocks' on any 'XXXXSpecialization' class */
        static constexpr bool doCollectMutextMetrics     = isHardLockMetricsEnabled<_opMetrics, _hardLockFallbackAfterCycles>();
        /** should we account for "spinning for too long" situations? */
        static constexpr bool doDebugSpinTimeouts        = isSpinLockDebugEnabled<_debugAfterCycles>();
        static constexpr bool doHardLockFallback         = isHardLockFallbackEnabled<_hardLockFallbackAfterCycles>();
        static constexpr bool doInstrumentLocks          = _instrumentLockCallback      != nullptr;
        static constexpr bool doInstrumentUnlocks        = _instrumentUnlockCallback    != nullptr;
        static constexpr bool doInstrumentBusyWaits      = _instrumentBusyWaitCallback  != nullptr;
        static constexpr bool doInstrumentMutexFallbacks = _instrumentMutexLockCallback != nullptr;


        // TODO: the following '_hard_lock', '_try_lock' and '_unlock' implementations shouldn't be needed, but on C++17,
        //       both clang and gcc requires them (gcc allows if we use the -fpermissive compilation flag).
        //       Will them still be needed in C++20?
        //       Anyways, they don't incur in additional code at all
        inline void _hard_lock() {
            TLockSpecialization<_useRelaxInstruction, _lockStrategy>::type::_hard_lock();
        }
        inline bool _try_lock() noexcept {
            return TLockSpecialization<_useRelaxInstruction, _lockStrategy>::type::_try_lock();
        }
        inline void _unlock() {
            TLockSpecialization<_useRelaxInstruction, _lockStrategy>::type::_unlock();
        }


    public:

        SpinLock() {
            // asserts on template parameters
            static_assert( !( (_debugAfterCycles != !(uint64_t)0) && (_hardLockFallbackAfterCycles != !(uint64_t)0) ) ||		// not both of them were activated
                           (_debugAfterCycles <= _hardLockFallbackAfterCycles),													// debug exceeds hard_lock
                          "Template parameter '_debugAfterCycles' cannot be greater than '_hardLockFallbackAfterCycle' -- "
                          "otherwise the requested debug messages would never be shown. You must either correct the "
                          "relation or set one of them to zero.");
            // special cases
            if constexpr (doCollectStandardMetrics) {
                SpinLockStandardMetricsAdditionalFields::reset();
            }
            if constexpr (doCollectMutextMetrics) {
                SpinLockHardLockMetricsAdditionalFields::reset();
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
                SpinLockHardLockMetricsAdditionalFields::debugHardLockMetrics(c);
            }
            c << "}\n";
            std::cerr << c.str() << std::flush;
        }

        inline void lock() {


            // PRE-LOCK CODE
            ////////////////

            // NOTE: all local variables in this procedure will be optimized out if their
            //       code is not enable in the following `if constexpr` expressions

            uint64_t waitingToLockStart;                // measure the cycles spent spinning
        	if (doDebugSpinTimeouts || doHardLockFallback || doCollectStandardMetrics) {
                // 'waitingToLockStart' is used to detect when to issue a debug warning on
                // 'spinning for too long' and 'hard_lock fallback' events.
                // if 'doCollectStandardMetrics' is enabled, it will also be used to
                // increment 'cpuCyclesWaitingToLock' when the lock is acquired
                waitingToLockStart = getProcessorCycleCount();
                // conditional for 'lockCallsCount' and, possibly, 'cpuCyclesWaitingToLock' metrics
                if constexpr (doCollectStandardMetrics) {
                    SpinLockStandardMetricsAdditionalFields::lockCallsCount++;
                }
        	}


            // LOCK CODE
            ////////////

            bool spinningTooMuchDebugged = false;   // will be true if a "spinning for too long" debug message was issued

            // if we don't need to do any measurements while waiting to acquire the lock, lets simply use the 'XXXXXSpecialization._hard_lock()' implementation
            if constexpr (!doControlledSpin && !doCollectStandardMetrics) {
                _hard_lock();
            } else {

                // CONTROLLED SPIN
                //////////////////

                // do the following until we may acquire the lock...
                while (!_try_lock())  {

                    // calls PAUSE or YIELD instruction, releasing the core to another CPU without context switching
                    // (else just do a normal busy wait)
                    if constexpr (doUseRelaxInstruction) cpu_relax();

                    // spin timeouts & hard_lock fall back optional codes
                    if constexpr (doDebugSpinTimeouts || doHardLockFallback) {

                        uint64_t elapsedCycles = getProcessorCycleCount()-waitingToLockStart;

                        // conditional for debugging "spinning for too long" conditions
                        if constexpr (doDebugSpinTimeouts) {
                            if ((elapsedCycles > _debugAfterCycles) && (!spinningTooMuchDebugged)) {
                                issueDebugMessage("Waiting too long to acquire a lock");
                                spinningTooMuchDebugged = true;
                            }
                            // if there is no more reasons to spin, fall back to 'hard_lock'ing, if appropriate
                            if constexpr (!doHardLockFallback) {
                                _hard_lock();
                                break;
                            }
                        }

                        // conditional for falling back to the 'hard_lock' state.
                        // 'hard_locking' implies a kernel call (and, therefore, a context-switch) is issued asking
                        // the operating system to don't execute this thread again until otherwise stated, which will
                        // eventually be done when another thread calls their `unlock()` procedure
                        if constexpr (doHardLockFallback) {
                            if (elapsedCycles > _hardLockFallbackAfterCycles) {
                                /*** PLEASE, SEARCH THIS TAG AND KEEP THIS CODE THE SAME -- OR PUT THEM INTO A DEFINE ***/
                                // conditional for giving a satisfaction on any eventually issue "spinning for too long" message
                                if constexpr (doDebugSpinTimeouts) {
                                	if (spinningTooMuchDebugged) {
                                		issueDebugMessage("Falling back to hard lock");
                                    }
								}

                                _hard_lock();	// really blocks the execution of this thread for an undefined amount of time
                                break;
                            }
                        }

                    }

                }

            }


            // POS-LOCK CODE
            ////////////////

            // conditional for giving a satisfaction on any eventually issue "spinning for too long" message
            if constexpr (doDebugSpinTimeouts) {
                if (spinningTooMuchDebugged) {
                    issueDebugMessage("Lock finally acquired");
                }
            }

            // conditionals for after the lock has been acquired

            if constexpr (doCollectStandardMetrics) {
                SpinLockStandardMetricsAdditionalFields::lockStart = getProcessorCycleCount();   // will be used to increment 'cpuCyclesLocked' when this gets unlocked
                SpinLockStandardMetricsAdditionalFields::cpuCyclesWaitingToLock += SpinLockStandardMetricsAdditionalFields::lockStart-waitingToLockStart;
            }

            if constexpr (doCollectMutextMetrics) {
                SpinLockHardLockMetricsAdditionalFields::hardLocksCount++;
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

            _unlock();

            // conditional for 'unlockCallsCount', 'realUnlocksCount' and 'cpuCyclesLocked' metrics
            if constexpr (doCollectStandardMetrics) {
                SpinLockStandardMetricsAdditionalFields::unlockCallsCount++;
                // were we really locked?
                if (likely (SpinLockStandardMetricsAdditionalFields::lockStart != !(uint64_t)0) ) {
                    SpinLockStandardMetricsAdditionalFields::cpuCyclesLocked += getProcessorCycleCount() - SpinLockStandardMetricsAdditionalFields::lockStart;
                    SpinLockStandardMetricsAdditionalFields::lockStart = !(uint64_t)0;     // get back to the special value denoting we are unlocked
                }
            }
        }

    };
}

#undef likely
#undef unlikely

#endif /* MTL_THREAD_SpinLock_hpp_ */
