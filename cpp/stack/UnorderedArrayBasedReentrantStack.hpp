#ifndef MTL_STACK_UnorderedArrayBasedReentrantStack_HPP_
#define MTL_STACK_UnorderedArrayBasedReentrantStack_HPP_

#include <atomic>
#include <iostream>
#include <mutex>
using namespace std;

// linux kernel macros for optimizing branch instructions
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#include <xmmintrin.h>
namespace mutua::MTL {
    struct SpinMutex {
        std::atomic_flag flag = ATOMIC_FLAG_INIT;
        std::mutex m;
        inline void lock() {
            // acquire lock
            while (flag.test_and_set(std::memory_order_acquire))  { 
                _mm_pause();    // yields a PAUSE instruction -- release the processor without context switching
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
    template <typename _BackingArrayElementType, unsigned _BackingArrayLength>
    class UnorderedArrayBasedReentrantStack {

    public:

        _BackingArrayElementType* backingArray;
        atomic<unsigned>&         stackHead;
        atomic<unsigned>          collisions;
        mutua::MTL::SpinMutex     headGuard;


        /** initiates a stack manipulation object, receiving as argument pointers to the 'backingArray'
         *  and the atomic 'stackHead' pointer.
         *  Please note that 'stackHead' must be declared using 'alignas(64)' to prevent performance
         *  degradation via the false-sharing phenomenon */
        UnorderedArrayBasedReentrantStack(_BackingArrayElementType* backingArray,
                                          atomic<unsigned>&         stackHead)
            : backingArray (backingArray)
            , stackHead    (stackHead) {

            // start with an empty stack
            stackHead.store(-1, memory_order_release);
            collisions.store(0, memory_order_release);
        }

        /** pushes into the stack one of the elements of the 'backingArray',
         *  returning a pointer to that element */
        inline _BackingArrayElementType* push(unsigned elementId) {

            _BackingArrayElementType* elementSlot = &(backingArray[elementId]);

            unsigned int next = stackHead.load(memory_order_relaxed);

            // if (unlikely (next == elementId) ) {
            //     cout << "Error: pushing element #"<<elementId<<" twice ("<<collisions<<" colliisions so far)\n" << flush;
            // }

            elementSlot->next.store(next, memory_order_release);

            // while (unlikely (!headGuard.compare_exchange(stackHead, next, elementId)) ) {
            //     elementSlot->next.store(next, memory_order_release);
            //     //std::this_thread::yield();
            //     collisions += 1;
            // }

            // headGuard.lock();
            while(unlikely (!stackHead.compare_exchange_weak(next, elementId,
                                                             memory_order_release,
                                                             memory_order_relaxed)) ) {
                // atomic_thread_fence(std::memory_order_seq_cst);
                elementSlot->next.store(next, memory_order_release);
                ////std::this_thread::yield();
                // collisions += 1;
            }
            // headGuard.unlock();

            // if (unlikely (next == elementId) ) {
            //     cout << "Exception: pushing element #"<<elementId<<" with ->next pointed to itself ("<<collisions<<" collisions so far)\n" << flush;
            // }

            return elementSlot;

        }

        /** pops the head of the stack -- returning a pointer to one of the elements of the 'backingArray'.
         *  Returns 'nullptr' if the stack is empty */
        inline unsigned pop(_BackingArrayElementType** headSlot) {

            unsigned next;
            unsigned headId = stackHead.load(memory_order_relaxed);

            //headGuard.lock();
            do {
                // is stack empty?
    			if (unlikely (headId == -1) ) {
                    *headSlot = nullptr;
    				return -1;
    			}
                *headSlot = &(backingArray[headId]);
                next = (*headSlot)->next.load(memory_order_relaxed);

                if (likely (stackHead.compare_exchange_weak(headId, next,
                                                            memory_order_release,
                                                            memory_order_relaxed)) ) {
                // if (likely (headGuard.compare_exchange(stackHead, headId, next)) ) {
                    break;
                // } else {
                //     atomic_thread_fence(std::memory_order_seq_cst);
                //     collisions += 1;
                //     //std::this_thread::yield();
                }

            } while (true);
            //headGuard.unlock();


            // if (unlikely (next == headId) ) {
            //     cout << "Exception: popping an element #"<<headId<<" with ->next pointed to itself ("<<collisions<<" collisions so far)\n" << flush;
            // }

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
