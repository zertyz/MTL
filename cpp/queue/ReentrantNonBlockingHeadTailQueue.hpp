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
     *   - This structure is ready to be "mmap-ped", since the internal pointers are actually indexes using the 'baseAddr' as the
     *     reference -- remember it would be impossible to implement this structure with full pointers because atomic
     *     operations on 128 bits (the two 64 bit full pointers) are not supported by x86_64 nor ARM32/64 (and a double pointer
     *     is needed by the 'head' and 'tail' markers in order to prevent the ABA concurrency problem).
    */
    template <typename _UserSlot>
    class ReentrantNonBlockingQueue {

    	typedef ReentrantNonBlockingQueueSlot<_UserSlot> QueueSlot;

    	struct AtomicBoundaries {
    		unsigned  head;
    		unsigned  tail;
		};

    public:

        alignas(64) QueueSlot*         backingArray;

    	/** this structure provides the queue HEAD and TAIL, able to be fetched, set and cmpexch simultaneously.
    	  * It is the base of the HEAD-TAIL atomic strategy as described in https://.....
    	  *   - TAIL always point to the last enqueued element, but it's 'next' cannot be trusted -- the stop condition is not 'is next null?' but 'is next equals TAIL?'
    	  *   - TAIL will only be updated after the previous TAIL.next became trustable */
        alignas(64) atomic<AtomicBoundaries> queueBoundaries;


        ReentrantNonBlockingQueue(QueueSlot* backingArray)
        	: backingArray (backingArray)
        {
            queueBoundaries = {(unsigned)-1, (unsigned)-1};
        }

        /** add to the end of the list */
        inline void enqueue(unsigned elementId) {

            AtomicBoundaries currentBoundaries = queueBoundaries.load(memory_order_relaxed);
            AtomicBoundaries newBoundaries;

            backingArray[elementId].next.store(-1, memory_order_release);

            do {

            	// build 'newBoundaries'
            	if (currentBoundaries.tail == -1) {
                	// 'EMPTY QUEUE' case:
            		newBoundaries.head = elementId;
            		newBoundaries.tail = elementId;
                	// apply 'newBoundaries'
                	if (likely (queueBoundaries.compare_exchange_strong(currentBoundaries, newBoundaries,
                	                                                    memory_order_release,
                	                                                    memory_order_relaxed)) ) {
                		// job done.
                		break;
                	} else {
                        // wait a little before a retry -- preserving CPU resources
                        cpu_relax();
                	}

            	} else {
					// 'non-EMPTY QUEUE' case:
					newBoundaries.head = currentBoundaries.head;
            		newBoundaries.tail = elementId;
                	// apply 'newBoundaries', setting 'tail.next' if it succeeds
                	if (likely (queueBoundaries.compare_exchange_strong(currentBoundaries, newBoundaries,
                	                                                    memory_order_relaxed,
                	                                                    memory_order_relaxed)) ) {
                		// set the 'next' pointer. Keep in mind another thread might be dequeueing this element before this code executes.
                		// in such case, the other thread will still see it as -1
            			backingArray[currentBoundaries.tail].next.store(elementId, memory_order_release);
                		// job done
                		break;
                	} else {
                        // wait a little before a retry -- preserving CPU resources
                        cpu_relax();
                	}
            	}
            } while (true);

        }

        /** remove from the beginning of the list
          * returns the index to one of the elements of the 'backingArray' while
          * pointing `slot` to that location -- or, -1 (and `nullptr` in `slot`)
          * if the queue is empty */
        inline unsigned dequeue(QueueSlot** slot) {

        	AtomicBoundaries currentBoundaries = queueBoundaries.load(memory_order_relaxed);
        	AtomicBoundaries newBoundaries;
        	unsigned         dequeueCandidate;

        	do {

        		// build 'newBondaries'
        		if (currentBoundaries.head == -1) {
					// 'EMPTY QUEUE' case:
					slot = nullptr;
					return -1;
        		} else if (currentBoundaries.head == currentBoundaries.tail) {
        			// 'SINGLE ELEMENT' (with synchronized 'next') case
        			dequeueCandidate = currentBoundaries.head;
            		*slot = &(backingArray[dequeueCandidate]);
        			newBoundaries.head = -1;
        			newBoundaries.tail = -1;
        		} else {
        			// 'MULTIPLE ELEMENT' case
            		dequeueCandidate = currentBoundaries.head;
            		*slot = &(backingArray[dequeueCandidate]);
            		unsigned observedNext = (*slot)->next.load(memory_order_relaxed);
        			if (observedNext == -1) {
        				// pointer is still being set in another 'enqueue' operation. spin until it is ready for dequeueing.
        				do {
        					cpu_relax();
        				} while ( (observedNext = (*slot)->next.load(memory_order_relaxed)) == -1 );
        			}
        			newBoundaries.head = observedNext;
        			newBoundaries.tail = currentBoundaries.tail;
        		}

            	// apply 'newBoundaries'
            	if (likely (queueBoundaries.compare_exchange_strong(currentBoundaries, newBoundaries,
            	                                                    memory_order_release,
            	                                                    memory_order_relaxed)) ) {
            		// job done.
            		return dequeueCandidate;
            	} else {
                    // wait a little before a retry -- preserving CPU resources
                    cpu_relax();
            	}

        	} while (true);
        }

        inline unsigned dequeue() {
            QueueSlot* slot;
            return dequeue(&slot);
        }

        inline void dump(string queueName) {
            AtomicBoundaries currentBoundaries = queueBoundaries.load(memory_order_relaxed);
            unsigned head = currentBoundaries.head;
            unsigned tail = currentBoundaries.tail;
            cerr << "\nDumping queue '"<<queueName<<"': "
                    "queueHead="<<head<<"; "
                    "queueTail="<<tail<<"\n" << flush;
            unsigned count=0;
            unsigned index = head == -1 ? tail : head;
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
            AtomicBoundaries currentBoundaries = queueBoundaries.load(memory_order_relaxed);
            unsigned head = currentBoundaries.head;
            unsigned tail = currentBoundaries.tail;
            unsigned count = 0;
            unsigned index = head == -1 ? tail : head;
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
