/*! \file FutexAdapter.hpp
    \brief Easy bits to deal with futexes (fast user mutexes), Linux or Windows
    \author luiz
    \date aug, 31, 2019
    
    The 'lock' method here is about the same speed as 'mutex.lock', but
    our 'try_lock' is about twice as fast, making this class a good mutex
    substitute for spinning algorithms.
*/

#ifndef MTL_THREAD_FutexAdapter_hpp_
#define MTL_THREAD_FutexAdapter_hpp_

#include <atomic>
#include <linux/futex.h>
#include <syscall.h>
#include <unistd.h>

namespace MTL::thread::FutexAdapter {

	inline int wait(std::atomic<int32_t>& id) {
		return ::syscall(SYS_futex, static_cast<void*>(&id), FUTEX_WAIT_PRIVATE, 0, nullptr, nullptr, 0);
	}

	inline int wake(std::atomic<int32_t>& id) {
		return ::syscall(SYS_futex, static_cast<void*>(&id), FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
	}

/*	inline int sys_futex(void* addr, std::int32_t op, std::int32_t x) {
	    return syscall(SYS_futex, addr, op, x, nullptr, nullptr, 0);
	}

	inline int futex_wake(std::atomic<std::int32_t>* addr) {
	    return 0 <= sys_futex(static_cast<void*>(addr), FUTEX_WAKE_PRIVATE, 1) ? 0 : -1;
	}

	inline int futex_wait( std::atomic< std::int32_t > * addr, std::int32_t x) {
	    return 0 <= sys_futex(static_cast<void*>(addr), FUTEX_WAIT_PRIVATE, x) ? 0 : -1;
	}
*/
}


namespace MTL::thread {

	struct Futex {

		alignas(64) std::atomic<int32_t> futexWord   = 0;

		inline void lock() {

			int32_t value = 0;
			while (!futexWord.compare_exchange_strong(value, 1, std::memory_order_release, std::memory_order_relaxed)) {
				if (value == 2 || futexWord.exchange(2, std::memory_order_relaxed)) {
					do {
						::syscall(SYS_futex, static_cast<void*>(&futexWord), FUTEX_WAIT_PRIVATE, 2, NULL, NULL, 0);
					} while (futexWord.exchange(2, std::memory_order_relaxed));
				}
				break;
				//value = 0;
			}

		}

		inline bool try_lock() {
			int32_t value = 0;
			return (futexWord.load(std::memory_order_relaxed) == 0) && futexWord.compare_exchange_strong(value, 1, std::memory_order_release, std::memory_order_relaxed);
		}

		inline void unlock() {
			if (futexWord.fetch_sub(1, std::memory_order_release) != 1) {
				futexWord.store(0, std::memory_order_release);
				::syscall(SYS_futex, static_cast<void*>(&futexWord), FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
			}
		}

	};

}

#endif	// MTL_THREAD_FutexAdapter_hpp_