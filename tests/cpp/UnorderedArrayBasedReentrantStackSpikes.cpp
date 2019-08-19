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
#define BACK_AND_FORTH (18264)

#define N_ELEMENTS     (65535)   // should be >= N_THREADS * BATCH


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
    unsigned        next;
    _OriginalStruct original;
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
void dumpStack(_StackType stack, unsigned expected, string listName, _debug debug) {
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
    std::cout << "Result of checking list '"<<listName<<"': " << (expected == count ? "PASSED":"FAILED") << ": expected="<<expected<<"; counted="<<count<<"\n" << std::flush;
}

void backAndForth(unsigned taskId) {
    MyStackElement* stackEntry;
    MyStruct* myData;

    for (int i=0; i<BATCH; i++) {
        unsigned poppedId;
        while ( (poppedId = freeStack.pop(&stackEntry)) == -1 ) {
            // 'freeStack' prematurely ran out of elements
            std::cout << "'freeStack' prematurely ran out of elements. ABORTING...\n" << std::flush;
            exit(1);
        }
        if (stackEntry->next == poppedId) {
            std::cout << "'freeStack' entry #"<<poppedId<<", after a pop operation, stated he was the ->next of himself (meaning the 'stackHead' is still pointing to him). ABORTING due to REENTRANCY BUG...\n" << std::flush;
            exit(1);
        }

        myData = &(stackEntry->original);
        myData->nSqrt = taskId+poppedId;
        myData->n     = (double)myData->nSqrt*(double)myData->nSqrt;

        usedStack.push(poppedId);

        if (stackEntry->next == poppedId) {
            std::cout << "'usedStack' entry #"<<poppedId<<" stated he is the ->next of himself . ABORTING due to REENTRANCY BUG...\n" << std::flush;
            exit(1);
        }
    }

    for (int i=0; i<BATCH; i++) {
        unsigned poppedId;
        while ( (poppedId = usedStack.pop(&stackEntry)) == -1 ) {
            std::cout << "'usedStack' prematurely ran out of elements. ABORTING...\n" << std::flush;
            exit(1);
        }
        if (stackEntry->next == poppedId) {
            std::cout << "'usedStack' entry #"<<poppedId<<", after a pop operation, stated he was the ->next of himself (meaning the 'stackHead' is still pointing to him). ABORTING due to REENTRANCY BUG...\n" << std::flush;
            exit(1);
        }

        myData = &(stackEntry->original);
        unsigned nSqrt = myData->nSqrt;
        double   n     = myData->n;
        if (n != (double)nSqrt*(double)nSqrt) {
            std::cout << "popped element #" << poppedId << " assert failed:" << n << " != " << (double)nSqrt*(double)nSqrt << ". ABORTING...\n";
            exit(1);
        }

        freeStack.push(poppedId);

        if (stackEntry->next == poppedId) {
            std::cout << "'freeStack' entry #"<<poppedId<<" stated he is the ->next of himself . ABORTING due to REENTRANCY BUG...\n" << std::flush;
            exit(1);
        }
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
        std::cout << "Poped array element #" << poppedId << "...";
        MyStruct& myData = stackEntry->original;
        std::cout << ": nSqrt=" << myData.nSqrt << "\n";
    }

    std::cout << "\n\nStaring the multithreaded tests:\n";
    unsigned long long start = getMonotonicRealTimeNS();
    populateFreeStack();
    std::thread threads[N_THREADS];
	for (unsigned _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
		threads[_threadNumber] = std::thread([](int threadNumber) {
            for (unsigned taskId=0; taskId<BACK_AND_FORTH; taskId++) {
                backAndForth(BACK_AND_FORTH*threadNumber+taskId);
            }
        }, _threadNumber);
	}
    for (int _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
    	threads[_threadNumber].join();
    }
    unsigned long long finish = getMonotonicRealTimeNS();

    dumpStack(freeStack, N_ELEMENTS, "freeStack", false);
    dumpStack(usedStack, 0, "usedStack", false);

    std::cout << "--> Executed in " << ((finish-start) / (unsigned long long)1000'000) << "ms.\n";

    return 0;
}
