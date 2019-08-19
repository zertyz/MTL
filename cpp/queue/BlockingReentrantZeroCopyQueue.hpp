#ifndef MUTUA_EVENTS_BLOCKINGREENTRANTZEROCOPYQUEUE_HPP_
#define MUTUA_EVENTS_BLOCKINGREENTRANTZEROCOPYQUEUE_HPP_

#include <atomic>
#include <iostream>
#include <mutex>
using namespace std;

#include <BetterExceptions.h>
//using namespace mutua::cpputils;


// linux kernel macros for optimizing branch instructions
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

namespace mutua::events::queues {
    /**
     * BlockingReentrantZeroCopyQueue.hpp
     * ==================================
     * created by luiz, Nov 9, 2018
     *
     * Provides a queue with the following attributes:
     *   - Fully reentrant (multiple producers and multiple consumers)
     *   - Blocks the caller (instead of returning an error) when queue is full or empty (both producers and consumers wait when attempting to enqueue/dequeue from full/empty queues)
     *   - Zero Copy (the element data is not copyed uppon enqueueing/dequeueing)
     *
    */
    template <typename _ElementType, uint_fast8_t _Log2_QueueSlots>
    class BlockingReentrantZeroCopyQueue {

    public:

    	constexpr static int numberOfQueueSlots = (int) 1 << (int) _Log2_QueueSlots;
    	constexpr static int queueSlotsModulus  = numberOfQueueSlots-1;

        struct QueueElement {
            int previous;
            int next;
            _ElementType element;
        };

        alignas(64) QueueElement  slots[numberOfQueueSlots];	// here are the elements of the queue
        // note: std::hardware_destructive_interference_size seems to not be supported in gcc -- 64 is x86_64 default (possibly the same for armv7)

        // lists -- representing the queue slots on their states (free, preEnqueued, enqueued, preDequeued)
        alignas(64) atomic<int> freeSlotsStack;       // uses only ->next member
        alignas(64) atomic<int> preEnqueuedSlotsHead;
        alignas(64) atomic<int> preEnqueuedSlotsTail;
        alignas(64) atomic<int> enqueuedSlotsHead;
        alignas(64) atomic<int> enqueuedSlotsTail;
        alignas(64) atomic<int> preDequeuedHead;
        alignas(64) atomic<int> preDequeuedTail;

        // mutexes
        mutex  dequeueGuard;
        mutex  queueGuard;


        BlockingReentrantZeroCopyQueue()
            , preEnqueuedSlotsHead (-1)
            , preEnqueuedSlotsTail (-1)
            , enqueuedSlotsHead    (-1)
            , enqueuedSlotsTail    (-1)
            , preDequeuedHead      (-1)
            , preDequeuedTail      (-1) {

            // prepares the 'freeSlots' list
            for (int i=0; i<numberOfQueueSlots; i++) {
                slots[i].previous = -1;
                slots[i].next     = i+1;
            }
            slots[numberOfQueueSlots-1].next = -1;
            freeSlotsStack = 0;

            // queue starts empty & locked for dequeueing
			dequeueGuard.lock();
		}

        /** Queue length, differently than the queue size, is the number of elements currently waiting to be dequeued */
        int getQueueLength() {
        	scoped_lock<mutex> lock(queueGuard);
        	if (queueTail == queueHead) {
        		if (isFull) {
        			return numberOfQueueSlots;
        		} else {
        			return 0;
        		}
        	}
        	if (queueTail > queueHead) {
        		return queueTail - queueHead;
        	} else {
        		return (numberOfQueueSlots + queueTail) - queueHead;
        	}
        }

        /** Returns the number of slots that are currently holding a dequeuable element + the ones that are currently compromised into holding one of those */
        int getQueueReservedLength() {
        	scoped_lock<mutex> lock(queueGuard);
        	if (queueReservedTail == queueReservedHead) {
        		if (isFull) {
        			return numberOfQueueSlots;
        		} else {
        			return 0;
        		}
        	}
        	if (queueReservedTail > queueReservedHead) {
        		return queueReservedTail - queueReservedHead;
        	} else {
        		return (numberOfQueueSlots + queueReservedTail) - queueReservedHead;
        	}
        }

        /** Non-blocking code to reserve a slot for further enqueueing -- In case the queue is full,
         *  this method does not block: it simply returns -1.
         *  A mutex is still used to allow reentrancy. */
        inline int zeroCopyNonBlockingReentrantReserveSlot(_ElementType*& elementPointer) {

			// take one of the elements from the 'freeSlotsStack'
            int slotId;
            QueueElement *freeSlot;
            do {
                slotId = freeSlotsStack.load(memory_order_relaxed);

                // is queue full?
    			if (unlikely( slotId == -1 )) {
    				return -1;
    			}

                freeSlot = &(slots[slotId]);
            } while (!freeSlotsStack.compare_exchange_weak(slotId, freeSlot->next,
			                                               std::memory_order_release,
			                                               std::memory_order_relaxed));

            elementPointer = freeSlot->element;

            return slotId;
        }

        /** Reserves an 'eventId' (and returns it) for further enqueueing.
         *  Points 'elementPointer' to a location able to be filled with the event information.
         *  This method takes constant time but blocks if the queue is full.
         *  NOTE: the heading of this code should be the same as in the overloaded method. */
        inline int zeroCopyBlockingReentrantReserveSlot(_ElementType*& elementPointer) {

        FULL_QUEUE_RETRY:

			queueGuard.lock();
			int slotId = zeroCopyNonBlockingReentrantReserveSlot(elementPointer);

			if (unlikely(slotId == -1)) {
				// Queue was already full before the call to this method. Wait on the mutex
				queueGuard.unlock();
				reservationGuard.lock();	// 2nd lock. Any caller will wait here. Unlocked only by 'releaseEvent(...)'
				reservationGuard.unlock();
				goto FULL_QUEUE_RETRY;
			} else if (unlikely(freeSlotsHead == -1)) {
    			// cool! we could reserve a queue slot!
                // ... but... reserving a slot for this element made the queue full?
				reservationGuard.lock();		// 1st lock. Won't wait yet.
            }

			queueGuard.unlock();
			return slotId;

        }

        /** Signals that the slot at 'reservedSlotId' is available for consumption / notification.
         *  This method doesn't block and takes constant time. */
        inline void zeroCopyNonBlockingReentrantEnqueueReservedSlot(int reservedSlotId) {
            QueueElement *reservedSlot = &(slots[reservedSlotId]);      // reserved slot == to be enqueued slot
            reservedSlot->next = -1;
        	enqueueGuard.lock();
            QueueElement *queueTailSlot = &(slots[enqueuedSlotsTail]);
            reservedSlot->previous = enqueuedSlotsTail;
            queueTailSlot->next = reservedSlotId;
            enqueuedSlotsTail = reservedSlotId;
            // if head is -1, it should be set to tail
			enqueueGuard.unlock();
        }

        /** Starts the zero-copy dequeueing process.
         *  Points 'dequeuedElementPointer' to the queue location containing the next element to be dequeued, returning it's 'slotId'.
         *  After processing, the zero copy deque operation must be finished by calling 'zeroCopyNonBlockingReentrantDequeueRelease(...)'
         *  This method takes constant time but blocks if the queue is empty. */
        inline int zeroCopyNonBlockingReentrantDequeuePeek(QueueElement*& dequeuedElementPointer) {

         EMPTY_QUEUE_RETRY:

			queueGuard.lock();

			// is queue empty?
            if (unlikely( enqueuedSlotsHead == -1 )) {
            	// Queue was already empty before the call to this method. Wait on the mutex
            	queueGuard.unlock();
            	dequeueGuard.lock();	// 2nd lock. Any caller will wait here. Unlocked only by 'reportReservedEvent(...)'
            	dequeueGuard.unlock();
            	goto EMPTY_QUEUE_RETRY;
            }

            int queueHeadSlotId = enqueuedSlotsHead;
            QueueElement *queueHeadSlot = &(slots[queueHeadSlotId]);
            enqueuedSlotsHead = queueHeadSlot->next;

            // will dequeueing this element make the queue empty? Set the waiting mutex
            if (enqueuedSlotsHead == -1) {
            	dequeueGuard.lock();		// 1st lock. Don't block yet...
            }

            queueGuard.unlock();

            return queueHeadSlotId;
        }

        /** Allows slot reuse (making the slot pointed to by 'slotId' available for reserving & enqueueing a new element) */
        inline void zeroCopyNonBlockingReentrantDequeueRelease(int slotId) {
        	queueGuard.lock();
            // bota na free list
        	events[eventId].reserved = false;
            if (likely(slotId == queueReservedHead)) {
            	do {
            		queueReservedHead = (queueReservedHead+1) & queueSlotsModulus;
            	} while (unlikely( (queueReservedHead != queueHead) && (!events[queueReservedHead].reserved) ));
                if (unlikely(isFull)) {
                	isFull = false;
                	reservationGuard.unlock();
                }
            }
			queueGuard.unlock();
        }
    };
}

#undef likely
#undef unlikely

#endif /* MUTUA_EVENTS_BLOCKINGREENTRANTZEROCOPYQUEUE_HPP_ */
