#ifndef MTL_STACK_UnorderedArrayBasedReentrantStack_HPP_
#define MTL_STACK_UnorderedArrayBasedReentrantStack_HPP_

#include <atomic>
#include <iostream>
#include <string>
#include <mutex>
using namespace std;

#include "../thread/cpu_relax.h"			// provides 'cpu_relax()'

// linux kernel macros for optimizing branch instructions
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


namespace mutua::MTL::stack {

    struct UnorderedArrayBasedReentrantStackNext {
        atomic<unsigned> next;          // the 64-byte alignment requirement is guaranteed by 'UnorderedArrayBasedReentrantStackSlot'
    };

    /** Struct used to define backing arrays for 'UnorderedArrayBasedReentrantStack'. Example:
     *      struct UserSlot { ... };
     *      typedef mutua::MTL::stack::UnorderedArrayBasedReentrantStackSlot<UserSlot> StackSlot;
     *      StackSlot backingArray[N_ELEMENTS]; */ 
    template <typename _UserSlot>
    struct alignas(64) UnorderedArrayBasedReentrantStackSlot: public UnorderedArrayBasedReentrantStackNext, _UserSlot {};
    // the struct above, empty but inheriting from 'UnorderedArrayBasedReentrantStackNext' and '_UserSlot',
    // is used to guarantee the order of the fields. The 'next' pointer will be the first and the whole
    // structure is aligned at 64 bytes, to prevent false-sharing performance degradation.



    /**
     * UnorderedArrayBasedReentrantStack.hpp
     * =====================================
     * created by luiz, Aug 18, 2019
     *
     * Provides a stack with the following attributes:
     *   - Lock-free (no mutexes or context-switches) yet fully reentrant -- multiple threads may push and pop simultaneously
     *   - All slots are members of a provided array -- the whole structure may be "mmap-ped"
     *   - Stack entries are "unordered" -- they may be added in any order, independent from the array's natural element order.
     *     This implies that the stack entries must have an "unsigned next" field, serving as a pointer to the next stack member:
     *         alignas(64) atomic<unsigned> next;       // 'alignas(64)' prevents false-sharing performance degradation.
     * 
     * Usage example:
     * 
     *      struct UserSlot { ... };
     *      typedef mutua::MTL::stack::UnorderedArrayBasedReentrantStackSlot<UserSlot> StackSlot;
     *      StackSlot backingArray[N_ELEMENTS];
     * 
     *      mutua::MTL::stack::UnorderedArrayBasedReentrantStack<StackSlot, N_ELEMENTS, true, true, true> stack(backingArray);
     * 
     *      
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
        inline void push(unsigned elementId) {

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

                // wait a little before a retry -- preserving CPU resources
                cpu_relax();

            }

            // debug
            if constexpr (_Debug) if (unlikely (currentHead.ptr == elementId) ) {
                cerr << "mutua::MTL::stack::UnorderedArrayBasedReentrantStack('"<<stackName<<"') -- Reentrancy Error: just pushed element #"<<elementId<<" and it ended having it's ->next pointed to itself\n" << flush;
            }

            if constexpr (_OpMetrics) {
                pushCount.fetch_add(1, memory_order_relaxed);
            }

        }

        /** pops the head of the stack -- returning the index to one of the elements of the 'backingArray'
          * & pointing `headSlot` to that slot.
         *  Returns '-1' if the stack is empty, in which case `headSlot` is also set to `nullptr` */
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
                } else {
                	if constexpr (_ColMetrics) {
						popCollisions.fetch_add(1, memory_order_relaxed);
					}
                    // wait a little before a retry -- preserving CPU resources
                    cpu_relax();
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
