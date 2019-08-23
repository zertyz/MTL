#ifndef MTL_THREAD_SpinLock_HPP_
#define MTL_THREAD_SpinLock_HPP_

#include <atomic>
#include <iostream>
#include <string>
#include <mutex>
#include <boost/fiber/detail/cpu_relax.hpp>     // provides cpu_relax() macro, which uses the x86's "pause" or arm's "yield" instructions
//#include <xmmintrin.h>                        // provides _mm_pause(). This would be an alternative to boost dependency, but it only works on x86, as of aug, 2019 -- and doesn't provide _mm_yield() as well...
using namespace std;

// linux kernel macros for optimizing branch instructions
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

namespace mutua::MTL {

    /**
     * SpinLock.hpp
     * ============
     * created by luiz, Aug 20, 2019
     *
     * Provides a spin-lock -- a lock designed to be more efficient than a mutex in "instant wait" situations,
     * since it does not cause a contex-switch.
     *
    */
    struct SpinLock {
        std::atomic_flag flag = ATOMIC_FLAG_INIT;
        std::mutex m;
        inline void lock() {
            // acquire lock
            while (flag.test_and_set(std::memory_order_relaxed))  { 
                cpu_relax();    // calls PAUSE or YIELD instruction, releasing the core to another CPU without context switching
            }
        }
        inline bool try_lock() {
            return !flag.test_and_set(std::memory_order_relaxed);
        }
        inline void unlock() {
            flag.clear(std::memory_order_release);
        }

        template <typename _type>
        inline bool non_atomic_compare_exchange(_type& val, _type& old_val, _type new_val) {
            m.lock();
            if (val == old_val) {
                val = new_val;
                m.unlock();
                return true;
            } else {
                old_val = val;
                m.unlock();
                return false;
            }
        }
    };
}

#undef likely
#undef unlikely

#endif /* MTL_THREAD_SpinLock_HPP_ */
