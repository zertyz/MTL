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
    struct SpinMutex {
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
              bool    _Debug   = false>      // enable to output to stderr debug information & activelly check for reentrancy errors
    class UnorderedArrayBasedReentrantStack {

    public:

        _BackingArrayElementType* backingArray;
        atomic<unsigned>&         stackHead;

        // operation metrics
        atomic<unsigned>          pushCount;
        atomic<unsigned>          popCount;

        // collision metrics
        atomic<unsigned>          pushCollisions;
        atomic<unsigned>          popCollisions;

        // debug
        string                    stackName;
//mutex                     m;
mutua::MTL::SpinMutex     m;


        /** initiates a stack manipulation object, receiving as argument pointers to the 'backingArray'
         *  and the atomic 'stackHead' pointer.
         *  Please note that 'stackHead' must be declared using 'alignas(64)' to prevent performance
         *  degradation via the false-sharing phenomenon */
        UnorderedArrayBasedReentrantStack(_BackingArrayElementType* backingArray,
                                          atomic<unsigned>&         stackHead,
                                          const string              stackName = "noname_debug_stack")
            : backingArray (backingArray)
            , stackHead    (stackHead)
            , stackName    (stackName) {

            // start with an empty stack
            stackHead.store(-1, memory_order_release);

            if constexpr (_OpMetrics) {
                pushCount.store(0, memory_order_release);
                popCount.store(0, memory_order_release);
            }

            if constexpr (_ColMetrics) {
                pushCollisions.store(0, memory_order_release);
                popCollisions.store(0, memory_order_release);
            }
        }

        /** pushes into the stack one of the elements of the 'backingArray',
         *  returning a pointer to that element */
        inline _BackingArrayElementType* push(unsigned elementId) {

            _BackingArrayElementType* elementSlot = &(backingArray[elementId]);

            unsigned int next = stackHead.load(memory_order_relaxed);
            elementSlot->next.store(next, memory_order_release);

            // debug
            if constexpr (_Debug) if (unlikely (next == elementId) ) {
                cerr << "mutua::MTL::stack::UnorderedArrayBasedReentrantStack('"<<stackName<<"') -- Usage Error: pushing element #"<<elementId<<" twice in a row\n" << flush;
            }

            // the "spin lock" to to set the new 'stackHead' and 'elementSlot->next'
            while (unlikely (!stackHead.compare_exchange_strong(next, elementId,
                                                                      memory_order_release,
                                                                      memory_order_relaxed)) ) {
                elementSlot->next.store(next, memory_order_release);

                if constexpr (_ColMetrics) {
                    pushCollisions.fetch_add(1, memory_order_relaxed);
                }

            }

            // debug
            if constexpr (_Debug) if (unlikely (next == elementId) ) {
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

            unsigned headId;
            unsigned next;

            // this is the pop loop. This could not be done using just CAS (compare and exchange)
            // because of the ABA problem -- https://en.wikipedia.org/wiki/ABA_problem
            // to solve it atomically (and avoid the spin locks used here), we needed to test
            // both the HEAD and its NEXT in a single operation. That might be done if we manage to
            // put them both in a struct -- the ListHead structure.

            do {
                // is stack empty? we check this out of the spin lock because it is so cheap we may do it twice
                if (unlikely (headId == -1) ) {
                    *headSlot = nullptr;
                    return -1;
                }

                m.lock();
                headId = stackHead.load(memory_order_relaxed);
                // check again if the stack became empty
                if (unlikely (headId == -1) ) {
                    *headSlot = nullptr;
                    return -1;
                }
                *headSlot = &(backingArray[headId]);
                next = (*headSlot)->next.load(memory_order_relaxed);

                if (likely (stackHead.compare_exchange_strong(headId, next,
                                                              memory_order_release,
                                                              memory_order_relaxed))) {
                    m.unlock();                
                    break;
                } else if constexpr (_ColMetrics) {
                    popCollisions.fetch_add(1, memory_order_relaxed);
                }
                m.unlock();                
            } while (true);

            if constexpr (_OpMetrics) {
                popCount.fetch_add(1, memory_order_relaxed);
            }

            return headId;
        }


        /** overload method to be used to pop the head from the stack when one doesn't want to know
         *  what index of the 'backingArray' it is stored at */
        inline _BackingArrayElementType* pop() {
            _BackingArrayElementType* headSlot;
            return pop(&headSlot);
        }

    };
}

#undef likely
#undef unlikely

#endif /* MTL_STACK_UnorderedArrayBasedReentrantStack_HPP_ */
