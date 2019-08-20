#ifndef MTL_STACK_UnorderedArrayBasedReentrantStack_HPP_
#define MTL_STACK_UnorderedArrayBasedReentrantStack_HPP_

#include <atomic>
#include <iostream>
#include <mutex>
using namespace std;

// linux kernel macros for optimizing branch instructions
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

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
        }

        /** pushes into the stack one of the elements of the 'backingArray',
         *  returning a pointer to that element */
        inline _BackingArrayElementType* push(unsigned elementId) {

            _BackingArrayElementType* elementSlot = &(backingArray[elementId]);

            unsigned int next = stackHead.load(memory_order_relaxed);
            std::atomic_store_explicit(&elementSlot->next, next, std::memory_order_release);
            while(!stackHead.compare_exchange_weak(next, elementId,
                                                     memory_order_release,
                                                     memory_order_relaxed)) {
                std::atomic_store_explicit(&elementSlot->next, next, std::memory_order_release);
            }
            return elementSlot;

        }

        /** pops the head of the stack -- returning a pointer to one of the elements of the 'backingArray'.
         *  Returns 'nullptr' if the stack is empty */
        inline unsigned pop(_BackingArrayElementType** headSlot) {

            unsigned next;
            unsigned headId = stackHead.load(memory_order_relaxed);
            do {
                // is stack empty?
    			if (headId == -1) {
                    *headSlot = nullptr;
    				return -1;
    			}
                *headSlot = &(backingArray[headId]);
                next = std::atomic_load_explicit(&((*headSlot)->next), memory_order_relaxed);
                atomic_thread_fence(memory_order_acquire);
            } while (!stackHead.compare_exchange_weak(headId, next,
			                                          memory_order_release,
			                                          memory_order_relaxed));
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
