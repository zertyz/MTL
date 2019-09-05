#ifndef MTL_QUEUE_ReentrantNonBlockingQueue_HPP_
#define MTL_QUEUE_ReentrantNonBlockingQueue_HPP_

#include <iostream>
#include <atomic>
using namespace std;

#include "../thread/cpu_relax.h"			// provides 'cpu_relax()'

// linux kernel macros for optimizing branch instructions
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


namespace MTL::queue {

	template <typename _QueueSlot>
    struct _ReentrantNonBlockingQueuePrev {
        //atomic<unsigned> prev;
    };

	template <typename _QueueSlot>
    struct _ReentrantNonBlockingQueueNext {
        atomic<unsigned> next;
    };
/*
    template <typename _UserSlotType1, typename _UserSlotType2>
    struct ReentrantNonBlockingQueueSplitSlot
           : public alignas(64) _ReentrantNonBlockingQueuePrev<ReentrantNonBlockingQueueSplitSlot>
           ,        _UserSlotType1
           ,        alignas(64) _ReentrantNonBlockingQueueNext<ReentrantNonBlockingQueueSplitSlot>
           ,        _UserSlotType2 {};
*/
	template <typename _UserSlot>
	struct alignas(64) ReentrantNonBlockingQueueSlot
           : public _ReentrantNonBlockingQueuePrev<ReentrantNonBlockingQueueSlot<_UserSlot>>
           ,        _ReentrantNonBlockingQueueNext<ReentrantNonBlockingQueueSlot<_UserSlot>>
           ,        _UserSlot {};

    /**
     * ReentrantNonBlockingQueue.hpp
     * =============================
     * created by luiz, Sep 4, 2019
     *
     * Provides a queue with the following attributes:
     *   - Lock-free (no mutexes or context switches) yet fully reentrant -- multiple threads may enqueue and dequeue simultaneously
     *   - Since it is lock-free, the 'dequeue' method may fail if there are no elements left. A spin is required to gather an element
     *     when it arrives or the callback hooks may be used to implement locking / unlocking
     *   - This structure is ready to be "mmap-ped", since the internal pointers are actually indexes using the 'baseAddr' as the
     *     reference -- remember it would be impossible to implement this structure with full pointers because atomic
     *     operations on 128 bits (the two 64 bit full pointers) are not supported by x86_64 nor ARM32/64 (and a double pointer
     *     is needed by the 'head' and 'tail' markers in order to prevent the ABA concurrency problem).
    */
    template <typename _UserSlotType>
    class ReentrantNonBlockingQueue {

    	typedef ReentrantNonBlockingQueueSlot<_UserSlotType> QueueSlot;

    	struct AtomicHead {
    		unsigned  ptr;
    		unsigned  next;
		};

		struct AtomicTail {
    		unsigned  ptr;
    		unsigned  prev;
		};

    public:

        alignas(64) QueueSlot*         backingArray;

    	// atomic pointers
        alignas(64) atomic<AtomicHead> queueHead;
        alignas(64) atomic<AtomicTail> queueTail;


        ReentrantNonBlockingQueue(QueueSlot* backingArray)
        	: backingArray (backingArray)
        {
            queueHead = {(unsigned)-1, (unsigned)-1};
            queueTail = {(unsigned)-1, (unsigned)-1};
        }

        /** add to the end of the list -- `queueTail` & `prev` pointer */
        inline void enqueue(unsigned elementId, QueueSlot* slot) {

            AtomicTail currentTail = queueTail.load(memory_order_relaxed);
            AtomicTail newTail     = {elementId, currentTail.ptr};
            //slot->prev.store(currentTail.ptr, memory_order_release);
            slot->next.store(-1, memory_order_release);

            // spin while we get the authorization to set the new 'queueTail' and 'slot->prev'
            while (unlikely (!queueTail.compare_exchange_strong(currentTail, newTail,
                                                                memory_order_release,
                                                                memory_order_relaxed)) ) {
                //slot->prev.store(currentTail.ptr, memory_order_release);
                newTail.prev = currentTail.ptr;

                // wait a little before a retry -- preserving CPU resources
                cpu_relax();
            }

            // set the 'next' pointer of any existing previous element
            if (currentTail.ptr != -1) {
                backingArray[currentTail.ptr].next.store(elementId, memory_order_release);
                // no need to set head.ptr, but it may be necessary to set head.next
                AtomicHead currentHead = queueHead.load(memory_order_relaxed);
                AtomicHead newHead;
                if (currentHead.ptr == currentTail.ptr) {
                    // if there was just 1 element on the queue before this enqueueing took place,
                    // head.next must point to the just enqueued second one
                    newHead.ptr  = currentHead.ptr;
                    newHead.next = elementId;
                    if (likely (queueHead.compare_exchange_strong(currentHead, newHead,
                                                                  memory_order_release,
                                                                  memory_order_relaxed)) ) {
                        // job done
                        return;
                }

            }

            // was the queue either empty or with just one element before this insertion?
            // if so, we must set the `queueHead` as well...
            if ( (currentTail.ptr == -1) || (currentTail.prev == -1) ) {
                AtomicHead currentHead = queueHead.load(memory_order_relaxed);
                AtomicHead newHead;

                do {
                    // ... in case nobody have already done so
/*                    if ( unlikely (currentHead.ptr != -1) && (currentHead.next != -1) ) {
                        break;
                    }
*/
                    if ( ( ( (newTail.ptr  != -1) && (currentHead.ptr  != -1) ) &&
                           ( ( (newTail.prev != -1) && (currentHead.next != -1) ) ||
                             ( (newTail.prev == -1) && (currentHead.next == -1) ) ) ) ||
                         (newTail.ptr  == -1) ) {
                        // some other parallel enqueue operation have already set the `queueHead`. Bail out
                        break;
                    }

                    // prepare the new head
                    if (currentHead.ptr == -1) {
                        // the case where the queue was empty before this enqueueing took place
                        newHead.ptr  = newTail.ptr;     // would be `elementId`, but things might have changed...
                        newHead.next = backingArray[newHead.ptr].next;
                    } else {
                        // the case where the queue had just one element before this enqueueing took place
                        newHead.ptr  = currentHead.ptr;
                        newHead.next = backingArray[newHead.ptr].next;
                    }

                    // attempt to set the new head
                    if (likely (queueHead.compare_exchange_strong(currentHead, newHead,
                                                                  memory_order_release,
                                                                  memory_order_relaxed)) ) {
                        // job done
                        break;
                    } else {
                        // wait a little before a retry -- preserving CPU resources
                        cpu_relax();
                        // reload the tail, to account for changes
                        newTail = queueTail.load(memory_order_relaxed);
                    }
                } while (true);
            }
        }

        inline void enqueue(unsigned elementId) {
            QueueSlot* slot = &(backingArray[elementId]);
            enqueue(elementId, slot);
        }

        /** remove from the beginning of the list -- 'queueHead' & `next` pointer --
          * and returns the index to one of the elements of the 'backingArray' while
          * pointing `slot` to that location.
         *  Returns '-1' (and `nullptr` in `slot`) if the queue is empty */
        inline unsigned dequeue(QueueSlot** slot) {
            AtomicHead currentHead;
            AtomicHead newHead;

            currentHead = queueHead.load(memory_order_relaxed);

            do {
                // is queue empty?
                if (unlikely (currentHead.ptr == -1) ) {
                    *slot = nullptr;
                    return -1;
                }

                // the potential slot to be dequeued -- when the CAS operation succeeds
                *slot = &(backingArray[currentHead.ptr]);

                newHead.ptr = (*slot)->next.load(memory_order_relaxed);

                // *** candidate for removal... ***
                // if CAS is to succeed, 'newHead.ptr' should be the same as 'currentHead.next'. Let's check:
                if (newHead.ptr != currentHead.next) {
                    // head->next has changed. No need for a CAS operation, since it will fail. Let's loop again and try one more time...
                    currentHead = queueHead.load(memory_order_relaxed);
                    cerr << 'l' << flush;
                    continue;
                }

                // keep on building the 'newHead' structure -- to replace 'queueHead' when the CAS succeeds
                if (newHead.ptr == -1) {
                    newHead.next = -1;
                } else {
                    QueueSlot* newSlot = &(backingArray[newHead.ptr]);
                    newHead.next = newSlot->next.load(memory_order_release);
                    // set the 'prev' pointer of the `newHead` element to match the fact it will be the new head element
                    //backingArray[newHead.ptr].prev.store(-1, memory_order_release);
                }

                if (likely (queueHead.compare_exchange_strong(currentHead, newHead,
                                                              memory_order_release,
                                                              memory_order_relaxed))) {
                    break;
                } else {
                    // wait a little before a retry -- preserving CPU resources
                    cpu_relax();
                }
            } while (true);

            // 'currentHead' has been removed from the list.
            // is there any need to adjust `queueTail`?
            // we are looking if the queue (after this dequeue):
            //   - became empty (`newHead.ptr` is -1)
            //   - has a single element (`queueTail.ptr` and `newHead.ptr` points to the same element
            //                           or, alternatively, `newHead.next` == -1)
            if ( (newHead.ptr == -1) || (newHead.next == -1) ) {
                AtomicTail currentTail = queueTail.load(memory_order_relaxed);
                AtomicTail newTail;
                do {
                    // ... provided that the elements didn't change to dicard the need of updating the tail
                    if ((newHead.ptr == -1) && (currentTail.ptr != -1)) {                           // queue is now empty, but the tail do not reflect that yet
                        newTail.ptr  = -1;
                        newTail.prev = -1;
                    } else if ( (currentTail.ptr == newHead.ptr) && (currentTail.prev != -1) ) {    // queue has just 1 element, but the tail does not reflect that yet
                        newTail.ptr  = currentTail.ptr;
                        newTail.prev = -1;
                    } else {
                        // work done. `queueTail` was updated, in the mean time, by another parallel dequeue operation
                        break;
                    }
                    // publish the `newTail`
                    if (likely (queueTail.compare_exchange_strong(currentTail, newTail,
                                                                  memory_order_release,
                                                                  memory_order_relaxed)) ) {
                        // work done. `queueTail` has just been updated
                        break;
                    } else {
                        // wait a little before a retrying -- preserving CPU resources
                        cpu_relax();
                        // reload the head, to account for changes
                        newHead = queueHead.load(memory_order_relaxed);
                    }
                } while (true);
            }

            return currentHead.ptr;
        }

        inline unsigned dequeue() {
            QueueSlot* slot;
            return dequeue(&slot);
        }


        inline void dump(string queueName) {
            AtomicHead head = queueHead.load(memory_order_relaxed);
            AtomicTail tail = queueTail.load(memory_order_relaxed);
            cerr << "\nDumping queue '"<<queueName<<"': "
                    "queueHead={ptr="<<head.ptr<<",next="<<head.next<<"}; "
                    "queueTail={ptr="<<tail.ptr<<",prev="<<tail.prev<<"}\n" << flush;
            unsigned count=0;
            unsigned index = head.ptr;
            while (index != -1) {
//                cerr << '['<<index<<"]={prev="<<backingArray[index].prev.load(memory_order_release)<<",next="<<backingArray[index].next.load(memory_order_release)<<",...}; " << flush;
                cerr << '['<<index<<"]={next="<<backingArray[index].next.load(memory_order_release)<<",...}; " << flush;
                index = backingArray[index].next.load(memory_order_release);
                count++;
            }
            cerr << "\n--> queue '"<<queueName<<"' has "<<count<<" elements\n\n";
        }

        /** checks if the queue structure is consistent,
          * returning 'true' if so. Additionally, the
          * number of elements is set on `count`.
          * NOTE: this method is not atomic -- there must
          *       be no enqueue/dequeue operations while
          *       it executes. */
        inline bool check(unsigned& count) {
            AtomicHead head = queueHead.load(memory_order_relaxed);
            AtomicTail tail = queueTail.load(memory_order_relaxed);
            count = 0;
            unsigned index = head.ptr;
            while (index != -1) {
                unsigned next = backingArray[index].next.load(memory_order_release);
                if (next == -1) {
                    // check if the tail is right
                    if (!(tail.ptr == index)) return false;
//                    if (!(tail.prev == backingArray[index].prev.load(memory_order_release))) return false;
                } else {
                    // check if next & prev pointers are in agreement
//                    if (!(backingArray[next].prev.load(memory_order_release) == index)) return false;
                }

                if (count == 1) {
                    // check if head.next is right
                    if (!(head.next == index)) return false;
                }

                index = next;
                count++;
            }
            return true;
        }

    };



}


#endif /* MTL_QUEUE_ReentrantNonBlockingPointerQueue_HPP_ */
