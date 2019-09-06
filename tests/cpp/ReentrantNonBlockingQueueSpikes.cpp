#include <iostream>
#include <iomanip>
#include <atomic>
#include <thread>
#include <cstring>
#include <mutex>

#include "../../cpp/queue/ReentrantNonBlockingQueue.hpp"

#include "../../cpp/time/TimeMeasurements.hpp"
using namespace MTL::time::TimeMeasurements;


// compile with (clan)g++ -std=c++17 -O3 -march=native -mtune=native -pthread -latomic ReentrantNonBlockingQueueSpikes.cpp -o ReentrantNonBlockingQueueSpikes && ./ReentrantNonBlockingQueueSpikes

#define DOCS "spikes on 'ReentrantNonBlockingQueue'\n" \
             "=====================================\n" \
             "\n" \
             "This is the helper program used to develop the very first\n"  \
             "version of 'ReentrantNonBlockingQueue.cpp', before\n" \
             "its API got mature enough to be included on this module's\n"  \
             "unit tests.\n";


// increase both of these if the reentrancy problem is not exposed
// (disabling compilation optimizations might also help)
#define N_THREADS      (1*/*std::thread::hardware_concurrency()*/4)
#define BATCH          4096
#define BACK_AND_FORTH (12345)

//#define N_ELEMENTS     (N_THREADS*BATCH)   // should be >= N_THREADS * BATCH
#define N_ELEMENTS     (26'777'216)   // should be >= N_THREADS * BATCH


// spike vars
/////////////

// the types
struct DataSlot {
    unsigned  taskId;
    unsigned  nSqrt;
    double    n;
};
typedef MTL::queue::ReentrantNonBlockingQueueSlot<DataSlot> QueueSlot;

// the backing array
QueueSlot backingArray[N_ELEMENTS];

// the queues
MTL::queue::ReentrantNonBlockingQueue<DataSlot> freeElements (backingArray);
MTL::queue::ReentrantNonBlockingQueue<DataSlot> queue        (backingArray);

void populateAllocator() {
    for (unsigned i=0; i<N_ELEMENTS; i++) {
//        backingArray[i].prev = -1;
//        backingArray[i].next = -1;
        freeElements.enqueue(i);
    }
}


// info section
///////////////

void printHardwareInfo() {
    struct DoubleInt {
        alignas(sizeof(unsigned)*2) unsigned a;
                                    unsigned b;
    };
    std::atomic<unsigned>  a_int;
    std::atomic<DoubleInt> a_DoubleInt;
    std::cout << "For this hardware:\n"
                 "\tsizeof(DoubleInt)                                 : "  << sizeof(DoubleInt) << '\n'
              << std::boolalpha <<
                 "\tis std::atomic<int> lock free?                    : "  << std::atomic_is_lock_free(&a_int) << "\n"
                 "\tis std::atomic<struct{int,int}> lock free?        : "  << std::atomic_is_lock_free(&a_DoubleInt) << "\n"
                 "\tstd::atomic<int>::is_always_lock_free             : "  << std::atomic<unsigned>::is_always_lock_free << "\n"
                 "\tstd::atomic<struct{int,int}>::is_always_lock_free : "  << std::atomic<DoubleInt>::is_always_lock_free << "\n\n"
    ;
}

template<typename _BackingArraySlot>
void dumpQueue(_BackingArraySlot& queue, string queueName) {
    std::cout << "Dumping queue '"<<queueName<<"': " << std::flush;
    unsigned count=0;
    _BackingArraySlot* queueEntry;
    while (count < N_ELEMENTS) {
        unsigned elementId = count++;
        std::cout << '['<<elementId<<"]={prev="<<queue[elementId].prev<<",next="<<queue[elementId].next<<",taskId="<<queue[elementId].taskId
                  << ",nSqrt="<<queue[elementId].nSqrt<<",n="<<queue[elementId].n<<"}; ";
    }
}


// spike methods
////////////////

// linux kernel macros for optimizing branch instructions
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

template <bool _debug, bool _check>
void simpleTest() {
    unsigned id, count;

//    if (_check && (!freeElements.check(count) || count!=N_ELEMENTS)) std::cerr << "'freeElements' check failed after enqueueing. count="<<count<<'\n';

    if constexpr (_debug) queue.dump("empty queue");
    queue.enqueue(freeElements.dequeue());
    queue.enqueue(freeElements.dequeue());
    queue.enqueue(freeElements.dequeue());
    queue.enqueue(freeElements.dequeue());

    if (_check && ((count=queue.getLength()) != 4)) std::cerr << "'queue' check failed after enqueueing. count="<<count<<'\n';


    if constexpr (_debug) queue.dump("with 4 elements...");


    freeElements.enqueue(id = queue.dequeue());
    if constexpr (_debug) {
        std::cerr << "dequeued element " << id << std::flush;
        queue.dump("removed element 0...");
    }
    if (_check && ((count=queue.getLength()) != 3))                   std::cerr <<  "3 elements 'queue' check failed. count="<<count<<'\n';
    if (_check && ((count=freeElements.getLength()) != N_ELEMENTS-3)) std::cerr << "-3 elements 'freeElements' check failed. count="<<count<<'\n';


    freeElements.enqueue(id = queue.dequeue());
    if constexpr (_debug) {
        std::cerr << "dequeued element " << id << std::flush;
        queue.dump("removed elements 0 & 1...");
    }
    if (_check && ((count=queue.getLength()) != 2))                   std::cerr <<  "2 elements 'queue' check failed. count="<<count<<'\n';
    if (_check && ((count=freeElements.getLength()) != N_ELEMENTS-2)) std::cerr << "-2 elements 'freeElements' check failed. count="<<count<<'\n';


    freeElements.enqueue(id = queue.dequeue());
    if constexpr (_debug) {
        std::cerr << "dequeued element " << id << std::flush;
        queue.dump("removed elements 0, 1 & 2...");
    }
    if (_check && ((count=queue.getLength()) != 1))                   std::cerr <<  "1 elements 'queue' check failed. count="<<count<<'\n';
    if (_check && ((count=freeElements.getLength()) != N_ELEMENTS-1)) std::cerr << "-1 elements 'freeElements' check failed. count="<<count<<'\n';


    freeElements.enqueue(id = queue.dequeue());
    if constexpr (_debug) {
        std::cerr << "dequeued element " << id <<  std::flush;
        queue.dump("removed elements 0, 1, 2 & 3...");
    }
    if (_check && ((count=queue.getLength()) != 0))                 std::cerr <<  "0 elements 'queue' check failed. count="<<count<<'\n';
    if (_check && ((count=freeElements.getLength()) != N_ELEMENTS)) std::cerr << "-0 elements 'freeElements' check failed. count="<<count<<'\n';
}


int main(void) {

	std::cout << DOCS;

    printHardwareInfo();
 
    std::cout << "sizeof(QueueSlot) : " << sizeof(QueueSlot) << "\n" << flush;
    std::cout << "sizeof(queue)     : " << sizeof(queue)     << "\n" << flush;

    populateAllocator();

    unsigned long long start = getMonotonicRealTimeNS();

    auto _threadFunction = []() {
        for (unsigned i=0; i<100'000'000/1/*00'000*/; i++) {
            simpleTest<false, false>();
        }
    };

    atomic<unsigned> qE = 0;
    atomic<unsigned> qD = 0;
    atomic<unsigned> feE = 0;
    atomic<unsigned> feD = 0;

    auto threadFunction = [&](unsigned modulus, unsigned val) {
        for (unsigned i=0; i<N_ELEMENTS; i++) {
            if ((i%modulus) == val) {
            	unsigned e;
            	while ((e = freeElements.dequeue()) == -1) {cerr << 'a'<<i<<'\n' << flush; std::cerr <<  "'queue': "<<queue.getLength()<<'\n'; std::cerr <<  "'freeElements': "<<freeElements.getLength()<<'\n';std::cerr <<  "qE="<<qE<<",qD="<<qD<<",feE="<<feE<<",feD="<<feD<<'\n';exit(1);}
            	feD++;
                queue.enqueue(e);
                qE++;
                while ((e = queue.dequeue()) == -1) cerr << 'b'<<i << flush;
                qD++;
                freeElements.enqueue(e);
                feE++;
                while ((e = freeElements.dequeue()) == -1) cerr << 'c'<<i << flush;
                feD++;
                queue.enqueue(e);
                qE++;
            }
        }
    };

    auto oneElementQueue = [&](unsigned threads) {
        for (unsigned i=0; i<N_ELEMENTS/threads; i++) {
        	unsigned e;
        	while ((e = freeElements.dequeue()) == -1) {cerr << "feD:i="<<i<</*"; 'queue': "<<queue.getLength()<<"; 'freeElements': "<<freeElements.getLength()<<*/"; qE="<<qE<<",qD="<<qD<<",feE="<<feE<<",feD="<<feD<<'\n'<<flush;/*exit(1);*/}
        	queue.enqueue(e);
        	feD++; qE++;
        	while ((e = queue.dequeue()) == -1) {cerr << "feD:i="<<i<</*"; 'queue': "<<queue.getLength()<<"; 'freeElements': "<<freeElements.getLength()<<*/"; qE="<<qE<<",qD="<<qD<<",feE="<<feE<<",feD="<<feD<<'\n'<<flush;/*exit(1);*/}
        	freeElements.enqueue(e);
        	qD++; feE++;
        }
    };

    //thread threads[] = {thread(_threadFunction),thread(_threadFunction),thread(_threadFunction),thread(_threadFunction),};
    //thread threads[] = {thread(threadFunction, 4,0),thread(threadFunction, 4,1),thread(threadFunction, 4,2),thread(threadFunction, 4,3),};
    thread threads[] = {thread(oneElementQueue, 4),thread(oneElementQueue, 4),thread(oneElementQueue, 4),thread(oneElementQueue, 4),};
    for (thread& t: threads) {
    	cerr << "\n-->joining " << t.get_id() << '\n' << flush;
        t.join();
    }
    cerr << "\n-->all joined. Checking..." << '\n' << flush;

//    for (unsigned i=0; i<100'000; i++) {
//        simpleTest<false, false>();
//    }
//    simpleTest<true, true>();

    std::cerr <<  "qE="<<qE<<",qD="<<qD<<",feE="<<feE<<",feD="<<feD<<'\n'<<flush;
    //queue.dump("queue");
    std::cerr <<  "'queue': "<<flush<<queue.getLength()<<'\n'<<flush;
    std::cerr <<  "'freeElements': "<<flush<<freeElements.getLength()<<'\n'<<flush;


    unsigned count;
    if ((count=       queue.getLength()) != N_ELEMENTS) std::cerr <<  "full 'queue' check failed. count="<<count<<'\n';
    cerr << "\n-->Checking the second..." << '\n' << flush;
    if ((count=freeElements.getLength()) != 0)          std::cerr <<  "empty 'queue' check failed. count="<<count<<'\n';
    cerr << "\n-->all done, I guess..." << '\n' << flush;




    unsigned long long finish = getMonotonicRealTimeNS();

    std::cout << "--> Executed in " << ((finish-start) / (unsigned long long)1000'000) << "ms.\n";

    return 0;
}
