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
    struct _ReentrantNonBlockingQueueNext {
		atomic<unsigned> next;
    };

	template <typename _UserSlot>
	struct alignas(64) ReentrantNonBlockingQueueSlot
           : public _ReentrantNonBlockingQueueNext<ReentrantNonBlockingQueueSlot<_UserSlot>>
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
     *   - This structure is ready to be "mmap-ped", since the internal pointers are actually indexes using the 'backingArray' pointer
     *     as the reference -- remember it would be impossible to implement this structure with full pointers because atomic
     *     operations on 128 bits (the two 64 bit full pointers) are not supported by x86_64 nor ARM32/64 (and a double pointer
     *     is needed by the 'head' and 'tail' markers in order to prevent the ABA concurrency problem).
    */
    template <typename _UserSlot>
    class ReentrantNonBlockingQueue {

    	typedef ReentrantNonBlockingQueueSlot<_UserSlot> QueueSlot;

        // the following structures were designed to point to the same memory region
        // 'AtomicQueue' has atomic members -- when we only want to refer to one of them at a time
        // 'Queue' is used to refer to both HEAD and TAIL atomically

        struct AtomicQueue {
            atomic<unsigned> head;
            atomic<unsigned> tail;
        };

        struct Queue {
            unsigned head;
            unsigned tail;
        };

    public:

        /** The region of memory where all elements from this queue resides.
            NOTE: this queue was designed to be used together with an allocator,
                  which is another queue or stack sharing the same 'backingArray' */
        QueueSlot*         backingArray;

    	/** 'atomicQueue' provides the queue HEAD and TAIL, able to be fetched, set and cmpexch simultaneously.
    	  * It is the base of the HEAD-TAIL atomic strategy as described in https://.....
    	  *   - TAIL always points to the last enqueued element, guaranteed to have a 'next' pointer either valid or 'null'
          *   - when enqueueing, HEAD will only come from null after the TAIL has been set
          *   - when dequeueing, if HEAD is TAIL, HEAD will be set to null -- otherwise it will be HEAD.next
          *   - if HEAD.next is 'null', we must spin -- for if HEAD is not null, this element is still being enqueued
          *     in another thread. */
        alignas(64) atomic<Queue> atomicQueue;
                    AtomicQueue&        queue = reinterpret_cast<AtomicQueue&>(atomicQueue);

        
        ReentrantNonBlockingQueue(QueueSlot* backingArray)
        	: backingArray (backingArray)
        {
            atomicQueue.store({(unsigned)-1, (unsigned)-1}, memory_order_release);
        }

        /** add to the end of the list */
        inline void enqueue(unsigned elementId) {

            backingArray[elementId].next.store(-1, memory_order_relaxed);
            // advance TAIL
            unsigned currentTail = queue.tail.exchange(elementId, memory_order_release);
            // set 'next' pointer from old tail
            if (currentTail != -1) {
                backingArray[currentTail].next.store(elementId, memory_order_release);
                // if there was a HEAD (old tail was not -1), the work is done.
                return ;
            } else {

                // at this point, HEAD was 'null' -- and the only way HEAD is 'null' is if TAIL is also 'null'.
                // the thread responsible for pointing HEAD to the new list is the one that
                // create the first element of the list -- a new list happens whenever HEAD is 'null'
                // the new HEAD is, therefore, the first element of the new list: 'elementId'

                unsigned newHead = elementId;

                // set HEAD to TAIL if HEAD is null -- so 'dequeue' is able to proceed
                Queue currentQueue = {(unsigned)-1, elementId}; // if nothing is happening in parallel, this is the exact value of atomicQueue.load(memory_order_relaxed);
                Queue newQueue;
                do {
/*
                    if (currentQueue.head != -1) {
                        cerr << "*** Unexpected HEAD IS NO LONGER NULL ERROR!!" << flush;
                        break;
                    }*/
                    newQueue.head = elementId;
                    newQueue.tail = currentQueue.tail;  // tail might have changed if another enqueueing is happening in another thread
                } while (unlikely (!atomicQueue.compare_exchange_weak(currentQueue, newQueue,
                                                                      memory_order_release,
                                                                      memory_order_relaxed)) );
            }
        }

        /** remove from the beginning of the list
          * returns the index to one of the elements of the 'backingArray' while
          * pointing `slot` to that location -- or, -1 (and `nullptr` in `slot`)
          * if the queue is empty */
        inline unsigned dequeue(QueueSlot** slot) {
    
            Queue currentQueue = atomicQueue.load(memory_order_relaxed);
            Queue newQueue;

            do {

                if (currentQueue.head == -1) {
                    // 'EMPTY QUEUE' case:
                    (*slot) = nullptr;
                    return -1;
                } else if (currentQueue.head == currentQueue.tail) {
                    // 'SINGLE ELEMENT' case -- attempt to null the queue so the single element can be returned
                    newQueue.head = -1;
                    newQueue.tail = -1;
                    if (likely (atomicQueue.compare_exchange_strong(currentQueue, newQueue,
                                                                    memory_order_release,
                                                                    memory_order_relaxed)) ) {
                        (*slot) = &(backingArray[currentQueue.head]);
                        return currentQueue.head;
                    } else {
                        // some other thread changed HEAD or TAIL. try again
                        continue;
                    }
                } else {
                    // 'MULTIPLE ELEMENT' case
                    if ( unlikely ( (newQueue.head = backingArray[currentQueue.head].next.load(memory_order_relaxed)) == -1 ) ) {
                        // if 'next' is -1, it is still being enqueued by another thread. reload and try again...
                        currentQueue = atomicQueue.load(memory_order_relaxed);
                        continue;
                    } else
                    // attempt to get the authorization to dequeue the element without attention to 'TAIL' changes
                    if (likely (queue.head.compare_exchange_strong(currentQueue.head, newQueue.head,
                                                                   memory_order_release,
                                                                   memory_order_relaxed)) ) {
                        // head advanced, meaning 'currentQueue.head' is the element to dequeue
                        (*slot) = &(backingArray[currentQueue.head]);
                        return currentQueue.head;
                    } else {
                        // some other thread already dequeued our candidate. lets repeat it all over
                        continue;
                    }
                }
            } while (true);
        }

        inline unsigned dequeue() {
            QueueSlot* slot;
            return dequeue(&slot);
        }

        inline void dump(string queueName) {
            Queue currentQueue = atomicQueue.load(memory_order_release);
            unsigned head = currentQueue.head;
            unsigned tail = currentQueue.tail;
            cerr << "\nDumping queue '"<<queueName<<"': "
                    "queueHead="<<head<<"; "
                    "queueTail="<<tail<<"\n" << flush;
            unsigned count=0;
            unsigned index = (head == -1) ? tail : head;
            unsigned maxIndex = index;
            while (index != -1) {
                cerr << '['<<index<<"]={next="<<backingArray[index].next.load(memory_order_relaxed)<<",...}; " << flush;
                count++;
				index = (index != tail) ? backingArray[index].next.load(memory_order_relaxed) : -1;
				if (index > maxIndex) {
					maxIndex = index;
				} else if (index == maxIndex) {
					cerr << "*** DUMP FAILED: circular reference detected to element " << index << ". Aborting...\n" << flush;
					return ;
				}
            }
            cerr << "\n--> queue '"<<queueName<<"' has "<<count<<" elements\n\n";
        }

        /** returns the number of elements this queue is currently holding.
          * NOTE 1: this method is inefficient for it traverses all elements
          * NOTE 2: this method is not atomic -- there must
          *         be no enqueue/dequeue operations while it executes. */
        inline unsigned getLength() {
            Queue currentQueue = atomicQueue.load(memory_order_release);
            unsigned head = currentQueue.head;
            unsigned tail = currentQueue.tail;
            unsigned count = 0;
            unsigned index = (head == -1) ? tail : head;
            unsigned maxIndex = index;
            while (index != -1) {
            	count++;
				index = (index != tail) ? backingArray[index].next.load(memory_order_relaxed) : -1;
				if (index > maxIndex) {
					maxIndex = index;
				} else if (index == maxIndex) {
					cerr << "*** getLength FAILED: circular reference detected to element " << index << " at count="<<count<<". Aborting...\n" << flush;
					return -1;
				}
            }
            return count;
        }

    };
}
#endif /* MTL_QUEUE_ReentrantNonBlockingPointerQueue_HPP_ */