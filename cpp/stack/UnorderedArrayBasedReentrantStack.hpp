#ifndef MTL_STACK_UnorderedArrayBasedReentrantStack_HPP_
#define MTL_STACK_UnorderedArrayBasedReentrantStack_HPP_

#include <atomic>
#include <iostream>
#include <string>
#include <mutex>
using namespace std;

// linux kernel macros for optimizing branch instructions
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

//#include <xmmintrin.h>
namespace mutua::MTL {
    struct SpinLock {
        std::atomic_flag flag = ATOMIC_FLAG_INIT;
        std::mutex m;
        inline void lock() {
            // acquire lock
            while (flag.test_and_set(std::memory_order_acquire))  { 
//                _mm_pause();    // yields a PAUSE instruction -- release the processor without context switching
            }
        }
        inline bool try_lock() {
            return !flag.test_and_set(std::memory_order_acquire);
        }
        inline void unlock() {
            flag.clear(std::memory_order_release);
        }

        template <typename _primitive>
        inline bool _compare_exchange(atomic<_primitive>& val, _primitive& old_val, _primitive new_val) {
            //lock();
            m.lock();
            _primitive _val = val.load(memory_order_relaxed);
            if (_val == old_val) {
                val.store(new_val, memory_order_release);
                m.unlock();
                return true;
            } else {
                old_val = _val;
                m.unlock();
                return false;
            }
        }
    };
}

namespace mutua::MTL::stack {
    /**
     * UnorderedArrayBasedReentrantStack.hpp
     * =====================================
     * created by luiz, Aug 18, 2019
     *
     * Provides a stack with the following attributes:
     *   - Mutex-free (we use std::atomic pointers) but fully reentrant -- multiple threads may push and pop simultaneously
     *   - All slots are members of a provided array
     *   - Stack entries are "unordered" -- they may be added in any order, independent from the array's natural element order
     *     (this implies that the stack entries must have an "int next" field, serving as a pointer to the next stack member;
     *      this pointer should be declared with 'alignas(64)' to prevent false-sharing performance degradation)
     *
    */
    template <typename _BackingArrayElementType, unsigned _BackingArrayLength,
              bool    _OpMetrics  = false,   // set to true if you want to keep track of the number of operations performed
              bool    _ColMetrics = false,   // when set, keeps track of the number of "spin lock loops" performed due to concurrent operation
              bool    _Debug      = false>   // enable to output to stderr debug information & activelly check for reentrancy errors
    class UnorderedArrayBasedReentrantStack {

    public:

        // NOTE: the following pointers and counters are declared with 'alignas(64)'
        // to prevent performance degradation via the false-sharing phenomenon.

        // this structure is known to be atomic in: x86_64, armv7l (Raspberry Pi 2),
        // armv6h (Raspberry Pi 1, if compiling with gcc -- clang version 8.0.1 (tags/RELEASE_801/final) fails at this)
        // (alignas(sizeof(int)*2) would never be needed since we are already caring for false-sharing)
        struct AtomicPointer {
            unsigned ptr;
            unsigned next;
        };

        _BackingArrayElementType* backingArray;
        //atomic<unsigned>&         stackHead;
        alignas(64) atomic<AtomicPointer>   stackHead;

        // operation metrics
        alignas(64) atomic<unsigned>        pushCount;
        alignas(64) atomic<unsigned>        popCount;

        // collision metrics
        alignas(64) atomic<unsigned>        pushCollisions;
        alignas(64) atomic<unsigned>        popCollisions;

        // debug
        string                              stackName;
//mutex                     m;
mutua::MTL::SpinLock     m;


        /** initiates a stack manipulation object, receiving as argument a pointer to the pre-allocated
         *  pool of slots named 'backingArray' */
        UnorderedArrayBasedReentrantStack(_BackingArrayElementType* backingArray,
                                          const string              stackName = "noname_debug_stack")
            : backingArray (backingArray)
            , stackName    (stackName) {

            // start with an empty stack
            stackHead.store({(unsigned)-1, (unsigned)-1}, memory_order_release);

            if constexpr (_OpMetrics) {
                pushCount = 0;
                popCount  = 0;
            } else {
                pushCount = -1;
                popCount  = -1;
            }

            if constexpr (_ColMetrics) {
                pushCollisions = 0;
                popCollisions  = 0;
            } else {
                pushCollisions = -1;
                popCollisions  = -1;
            }
        }

        /** pushes into the stack one of the elements of the 'backingArray',
         *  returning a pointer to that element */
        inline _BackingArrayElementType* push(unsigned elementId) {

            _BackingArrayElementType* elementSlot = &(backingArray[elementId]);

            AtomicPointer currentHead = stackHead.load(memory_order_relaxed);

            AtomicPointer pushedHead    = {elementId, currentHead.ptr};
            elementSlot->next.store(currentHead.ptr, memory_order_release);

            // debug
            if constexpr (_Debug) if (unlikely (currentHead.ptr == elementId) ) {
                cerr << "mutua::MTL::stack::UnorderedArrayBasedReentrantStack('"<<stackName<<"') -- Usage Error: pushing element #"<<elementId<<" twice in a row\n" << flush;
            }

            // the "spin lock" to to set the new 'stackHead' and 'elementSlot->next'
            while (unlikely (!stackHead.compare_exchange_strong(currentHead, pushedHead,
                                                                memory_order_release,
                                                                memory_order_relaxed)) ) {
                elementSlot->next.store(currentHead.ptr, memory_order_release);
                pushedHead.next = currentHead.ptr;

                if constexpr (_ColMetrics) {
                    pushCollisions.fetch_add(1, memory_order_relaxed);
                }

            }

            // debug
            if constexpr (_Debug) if (unlikely (currentHead.ptr == elementId) ) {
                cerr << "mutua::MTL::stack::UnorderedArrayBasedReentrantStack('"<<stackName<<"') -- Reentrancy Error: just pushed element #"<<elementId<<" and it ended having it's ->next pointed to itself\n" << flush;
            }

            if constexpr (_OpMetrics) {
                pushCount.fetch_add(1, memory_order_relaxed);
            }

            return elementSlot;

        }

        /** pops the head of the stack -- returning a pointer to one of the elements of the 'backingArray'.
         *  Returns 'nullptr' if the stack is empty */
        inline unsigned pop(_BackingArrayElementType** headSlot) {

            alignas(sizeof(unsigned)*2) AtomicPointer currentHead;
            alignas(sizeof(unsigned)*2) AtomicPointer nextHead;

            // this is the pop loop. This could not be done using just CAS (compare and exchange)
            // because of the ABA problem -- https://en.wikipedia.org/wiki/ABA_problem
            // to solve it atomically (and avoid the spin locks used here), we needed to test
            // both the HEAD and its NEXT in a single operation. That might be done if we manage to
            // put them both in a struct -- the ListHead structure.

            currentHead = stackHead.load(memory_order_relaxed);

            do {
                // is stack empty?
                if (unlikely (currentHead.ptr == -1) ) {
                    *headSlot = nullptr;
                    return -1;
                }

                // the potential slot to be popped -- when the CAS operation succeeds
                *headSlot = &(backingArray[currentHead.ptr]);

                // if CAS is to succeed, 'nextHead.ptr' should be the same as 'currentHead.next'. Let's check:
                nextHead.ptr = (*headSlot)->next.load(memory_order_relaxed);
                if (nextHead.ptr != currentHead.next) {
//std::cout << "--> exiting because nextHead.ptr != currentHead.next: " << nextHead.ptr << " != " << currentHead.next << "\n";
//exit(1);
                    // head->next has changed. No need for a CAS operation, since it will fail. Let's loop again and try one more time...
                    if constexpr (_ColMetrics) {    // account for the collision metrics
                        popCollisions.fetch_add(1, memory_order_relaxed);
                    }
                    currentHead = stackHead.load(memory_order_relaxed);
                    continue;
                }

                // keep on building the 'nextHead' structure -- to replace 'stackHead' when the CAS succeeds
                if (nextHead.ptr == -1) {
                    nextHead.next = -1;
                } else {
                    _BackingArrayElementType* nextSlot = &(backingArray[nextHead.ptr]);
                    nextHead.next = nextSlot->next.load(memory_order_relaxed);
                }

                if (likely (stackHead.compare_exchange_strong(currentHead, nextHead,
                                                              memory_order_release,
                                                              memory_order_relaxed))) {
                    break;
                } else if constexpr (_ColMetrics) {
                    popCollisions.fetch_add(1, memory_order_relaxed);
                }
            } while (true);

            if constexpr (_OpMetrics) {
                popCount.fetch_add(1, memory_order_relaxed);
            }

            return currentHead.ptr;
        }


        /** overload method to be used to pop the head from the stack when one doesn't want to know
         *  what index of the 'backingArray' it is stored at */
        inline _BackingArrayElementType* pop() {
            _BackingArrayElementType* headSlot;
            return pop(&headSlot);
        }

        inline unsigned getStackHead() {
            AtomicPointer head = stackHead.load(memory_order_relaxed);
            return head.ptr;
        }

        inline unsigned getStackHeadNext() {
            AtomicPointer head = stackHead.load(memory_order_relaxed);
            return head.next;
        }

    };
}

#undef likely
#undef unlikely

#endif /* MTL_STACK_UnorderedArrayBasedReentrantStack_HPP_ */
