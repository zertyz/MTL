#include <iostream>
#include <atomic>
#include <thread>
#include <cstring>
#include <mutex>

#include "../../cpp/stack/UnorderedArrayBasedReentrantStack.hpp"

// compile with (clan)g++ -std=c++17 -O3 -march=native -mtune=native -pthread UnorderedArrayBasedReentrantStackSpikes.cpp -o UnorderedArrayBasedReentrantStackSpikes

#define DOCS "spikes on 'UnorderedArrayBasedReentrantStack'\n" \
             "=============================================\n" \
             "\n" \
             "This is the helper program used to develop the very first\n"  \
             "version of 'UnorderedArrayBasedReentrantStack.cpp', before\n" \
             "its API got mature enough to be included on this module's\n"  \
             "unit tests.\n";


// increase both of these if the reentrancy problem is not exposed
// (disabling compilation optimizations might also help)
#define N_THREADS      12
#define BATCH          4096
#define BACK_AND_FORTH (654321)

#define N_ELEMENTS     (655350)   // should be >= N_THREADS * BATCH


// test section
///////////////

static struct timespec timespec_now;
static inline unsigned long long getMonotonicRealTimeNS() {
    clock_gettime(CLOCK_MONOTONIC, &timespec_now);
    return (timespec_now.tv_sec*1000000000ll) + timespec_now.tv_nsec;
}


// spike vars
/////////////

struct MyStruct {
    double    n;
    unsigned  nSqrt;
};
template <typename _OriginalStruct>
struct StackElement {
    alignas(64) unsigned next;
    _OriginalStruct      original;
    std::atomic_flag                  atomic_flag = ATOMIC_FLAG_INIT;
    //std::mutex           mutex;
};
typedef StackElement<MyStruct> MyStackElement;

MyStackElement         backingArray[N_ELEMENTS];
alignas(64) std::atomic<unsigned>  stackHead;

mutua::MTL::stack::UnorderedArrayBasedReentrantStack<MyStackElement, N_ELEMENTS> stack(backingArray, stackHead);

// spike methods
////////////////

alignas(64) std::atomic<unsigned>  usedStackHead;
mutua::MTL::stack::UnorderedArrayBasedReentrantStack<MyStackElement, N_ELEMENTS> usedStack(backingArray, usedStackHead);
alignas(64) std::atomic<unsigned>  freeStackHead;
mutua::MTL::stack::UnorderedArrayBasedReentrantStack<MyStackElement, N_ELEMENTS> freeStack(backingArray, freeStackHead);

void populateFreeStack() {
    for (unsigned i=0; i<N_ELEMENTS; i++) {
        freeStack.push(i);
    }
}

template<typename _StackType, typename _debug>
void dumpStack(_StackType& stack, unsigned expected, string listName, _debug debug) {
    if (debug) std::cout << "Checking list '"<<listName<<"'..." << std::flush;
    unsigned count=0;
    MyStackElement* stackEntry;
    while (true && (count <= (N_ELEMENTS*2))) {
        unsigned poppedId = stack.pop(&stackEntry);
        if (stackEntry == nullptr) {
            break;
        }
        count++;
        if (debug) std::cout << poppedId << ",";
    }
    if (debug) std::cout << "\n";
    std::cout << "Result of checking list '"<<listName<<"': " << (expected == count ? "PASSED":"FAILED") << ": expected="<<expected<<"; counted="<<count<<"; collisions="<<stack.collisions<<"\n" << std::flush;
}

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#include <chrono>
std::atomic<unsigned> errorCounter(0);
void niceExit(int n) {
    if (unlikely (errorCounter.fetch_add(1) > 30) ) {
        std::cout << "--> too many errors. Quitting...\n" << std::flush;
        exit(n);
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(100+1000*errorCounter));
    }
}

std::atomic<unsigned> r(0);
std::atomic<unsigned> operationsCount(0);
std::atomic<unsigned> errorsCount(0);
void backAndForth(unsigned threadNumber, unsigned taskId) {
    MyStackElement* stackEntry;
    MyStruct* myData;
    unsigned poppedId;

    for (unsigned i=0; i<BATCH; i++) {
//        std::atomic_thread_fence(std::memory_order_seq_cst);
        while (unlikely ((poppedId = freeStack.pop(&stackEntry)) == -1) ) {
            operationsCount.fetch_add(1);
//            std::atomic_thread_fence(std::memory_order_seq_cst);
            std::cout << "'freeStack' prematurely ran out of elements in iteraction "<<i<<" of "<<BATCH<<", thread #"<<threadNumber<<", "<<freeStack.collisions<<" collisions so far. ABORTING...\n" << std::flush;
            niceExit(1);
        }
        operationsCount.fetch_add(1);
//        std::atomic_thread_fence(std::memory_order_seq_cst);

//         if (unlikely (stackEntry->next.load(std::memory_order_relaxed) == poppedId) ) {
// //            std::atomic_thread_fence(std::memory_order_acquire);
//             std::cout << "'freeStack' popped entry #"<<poppedId<<" stated he was the ->next of himself (meaning the 'stackHead' is still pointing to him), thread #"<<threadNumber<<", "<<freeStack.collisions<<" collisions so far. ABORTING due to REENTRANCY BUG...\n" << std::flush;
//             niceExit(1);
//         }
// //        std::atomic_thread_fence(std::memory_order_acquire);

        myData = &(stackEntry->original);
        myData->n = (double)(taskId+poppedId)*(double)(taskId+poppedId);
        myData->nSqrt = taskId+poppedId;

//        std::atomic_thread_fence(std::memory_order_seq_cst);
        operationsCount.fetch_add(1);
        usedStack.push(poppedId);
//        std::atomic_thread_fence(std::memory_order_seq_cst);

//         if (unlikely (stackEntry->next.load(std::memory_order_relaxed) == poppedId) ) {
// //            std::atomic_thread_fence(std::memory_order_acquire);
//             std::cout << "'usedStack' pushed entry #"<<poppedId<<" stated he is the ->next of himself, thread #"<<threadNumber<<", "<<usedStack.collisions<<" collisions so far. ABORTING due to REENTRANCY BUG...\n" << std::flush;
//             niceExit(1);
//         }
// //        std::atomic_thread_fence(std::memory_order_acquire);
    }

    for (unsigned i=0; i<BATCH; i++) {
//        std::atomic_thread_fence(std::memory_order_seq_cst);
        while (unlikely ((poppedId = usedStack.pop(&stackEntry)) == -1) ) {
            operationsCount.fetch_add(1);
//            std::atomic_thread_fence(std::memory_order_seq_cst);
            std::cout << "'usedStack' prematurely ran out of elements in iteraction "<<i<<" of "<<BATCH<<", thread #"<<threadNumber<<", "<<usedStack.collisions<<" collisions so far. ABORTING...\n" << std::flush;
            niceExit(1);
        }
        operationsCount.fetch_add(1);
//        std::atomic_thread_fence(std::memory_order_seq_cst);

//         if (unlikely (stackEntry->next.load(std::memory_order_relaxed) == poppedId) ) {
// //            std::atomic_thread_fence(std::memory_order_acquire);
//             std::cout << "'usedStack' popped entry #"<<poppedId<<" stated he was the ->next of himself (meaning the 'stackHead' is still pointing to him), thread #"<<threadNumber<<", "<<usedStack.collisions<<" collisions so far. ABORTING due to REENTRANCY BUG...\n" << std::flush;
//             niceExit(1);
//         }
// //        std::atomic_thread_fence(std::memory_order_acquire);

//        std::atomic_thread_fence(std::memory_order_seq_cst);
        myData = &(stackEntry->original);
        double   n     = myData->n;
        unsigned nSqrt = myData->nSqrt;
        if (n != (double)nSqrt*(double)nSqrt) {
            errorsCount.fetch_add(1);
            //std::cout << "popped element #" << poppedId << " assert failed: " << n << " != " << (double)nSqrt*(double)nSqrt << ", thread #"<<threadNumber<<", collisions(free,used): "<<freeStack.collisions<<","<<usedStack.collisions<<" so far. continueing anyway...\n" << std::flush;
            //niceExit(1);
        }
        r.fetch_add(n+nSqrt);
////        std::atomic_thread_fence(std::memory_order_seq_cst);

//        std::atomic_thread_fence(std::memory_order_seq_cst);
        operationsCount.fetch_add(1);
        freeStack.push(poppedId);
//        std::atomic_thread_fence(std::memory_order_seq_cst);

//         if (unlikely (stackEntry->next.load(std::memory_order_relaxed) == poppedId) ) {
// //            std::atomic_thread_fence(std::memory_order_acquire);
//             std::cout << "'freeStack' pushed entry #"<<poppedId<<" stated he is the ->next of himself, thread #"<<threadNumber<<", "<<freeStack.collisions<<" collisions so far. ABORTING due to REENTRANCY BUG...\n" << std::flush;
//             niceExit(1);
//         }
// //        std::atomic_thread_fence(std::memory_order_acquire);
    }
}

int main(void) {

	std::cout << DOCS;


    std::cout << "\n\nStaring the simple tests:\n";
    int anagram[] = {1,2,3,4,4,3,2,1};
    constexpr int anagram_length = sizeof(anagram)/sizeof(anagram[0]);

    for (int i=1010; i<1010+anagram_length; i++) {
        std::cout << "Populating and pushing array element #" << i << " (stackHead is now " << stackHead << ")...";
        MyStackElement* stackEntry = stack.push(i);
        MyStruct& myData = stackEntry->original;
        myData.nSqrt = anagram[i-1010];
        myData.n     = anagram[i-1010]*anagram[i-1010];
        std::cout << " OK\n";
    }

    while (true) {
        MyStackElement* stackEntry;
        unsigned poppedId = stack.pop(&stackEntry);
        if (stackEntry == nullptr) {
            std::cout << "... end reached\n";
            break;
        }
        std::cout << "Popped array element #" << poppedId << "...";
        MyStruct& myData = stackEntry->original;
        std::cout << ": nSqrt=" << myData.nSqrt << "\n";
    }

    std::cout << "\n\nStaring the multithreaded tests:\n";
    unsigned long long start = getMonotonicRealTimeNS();
    populateFreeStack();
    std::thread threads[N_THREADS];
	for (unsigned _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
		threads[_threadNumber] = std::thread([](int threadNumber) {
            for (unsigned taskId=threadNumber; taskId<BACK_AND_FORTH; taskId++) {
                backAndForth(threadNumber, BACK_AND_FORTH*threadNumber+taskId);
                if ( (taskId % ((BACK_AND_FORTH/(threadNumber+1))-1)) == 0 ) {
                    std::cout << "." << std::flush;
                }
            }
        }, _threadNumber);
        std::this_thread::sleep_for(std::chrono::milliseconds(10*_threadNumber));
	}

    // statistics
    unsigned lastOperationsCount = 0;
    unsigned deltaOps;
    unsigned lastErrorsCount = 0;
    unsigned deltaErrs;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        deltaOps = operationsCount-lastOperationsCount;
        lastOperationsCount += deltaOps;
        deltaErrs = errorsCount-lastErrorsCount;
        lastErrorsCount += deltaErrs;
        std::cout << "Performance: " << deltaOps << " \tops/s; \t" << deltaErrs << "   \terr/s   \t(" << lastErrorsCount << " \ttotal)    \r" << std::flush;
    } while ((deltaOps > 0) || (deltaErrs > 0));

    for (int _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
    	threads[_threadNumber].join();
        std::cout << "#" << std::flush;
    }
    unsigned long long finish = getMonotonicRealTimeNS();

    std::cout << std::endl << std::flush;
    dumpStack(freeStack, N_ELEMENTS, "freeStack", false);
    dumpStack(usedStack, 0, "usedStack", false);

    std::cout << "--> Executed in " << ((finish-start) / (unsigned long long)1000'000) << "ms (r=="<<r<<").\n";

    return 0;
}
