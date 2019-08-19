#include <iostream>
#include <atomic>
#include <thread>
#include <cstring>
#include <mutex>

#include "../../cpp/stack/UnorderedArrayBasedReentrantStack.hpp"

#define DOCS "spikes on 'UnorderedArrayBasedReentrantStack'\n" \
             "=============================================\n" \
             "\n" \
             "This is the helper program used to develop the very first\n"  \
             "version of 'UnorderedArrayBasedReentrantStack.cpp', before\n" \
             "its API got mature enough to be included on this module's\n"  \
             "unit tests.\n";


// increase both of these if the reentrancy problem is not exposed
// (disabling compilation optimizations might also help)
#define N_THREADS   12
#define ITERACTIONS 1<<24

#define N_ELEMENTS  1<<12


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
    int    n;
    double nSqrt;
};
template <typename _OriginalStruct>
struct StackElement {
    unsigned        next;
    _OriginalStruct original;
};
typedef StackElement<MyStruct> MyStackElement;

alignas(64) MyStackElement         backingArray[N_ELEMENTS];
alignas(64) std::atomic<unsigned>  stackHead;

alignas(64) mutua::MTL::stack::UnorderedArrayBasedReentrantStack<MyStackElement, N_ELEMENTS> stack(backingArray, stackHead);

// spike methods
////////////////

alignas(64) std::atomic<unsigned>  usedStackHead;
alignas(64) mutua::MTL::stack::UnorderedArrayBasedReentrantStack<MyStackElement, N_ELEMENTS> usedStack(backingArray, usedStackHead);
alignas(64) std::atomic<unsigned>  freeStackHead;
alignas(64) mutua::MTL::stack::UnorderedArrayBasedReentrantStack<MyStackElement, N_ELEMENTS> freeStack(backingArray, freeStackHead);

void populateFreeStack() {
    for (unsigned i=0; i<N_ELEMENTS; i++) {
        freeStack.push(i);
    }
}

void dumpFreeStack() {
    unsigned count=0;
    while (true) {
        unsigned poppedId;
        MyStackElement* stackEntry = freeStack.pop(poppedId);
        if (stackEntry == nullptr) {
            break;
        }
        count++;
        std::cout << poppedId << ",";
    }
    std::cout << "... for a total of " << count << " elements\n";
}

int anagram[] = {1,2,3,4,4,3,2,1};
constexpr int anagram_length = sizeof(anagram)/sizeof(anagram[0]);
void backAndForth(unsigned taskId) {
    constexpr unsigned nElements = 128;
    unsigned poppedId;
    for (int i=0; i<nElements; i++) {
        MyStackElement* stackEntry = freeStack.pop(poppedId);
        if (stackEntry == nullptr) {
            std::cout << "'freeStack' prematurely ran out of elements while popping at task #" << taskId << "\n";
            break;
        }
        MyStruct& myData = stackEntry->original;
        myData.nSqrt = poppedId;
        myData.n     = (double)poppedId*(double)poppedId;
        usedStack.push(poppedId);
    }
    for (int i=0; i<nElements; i++) {
        StackElement<MyStruct>* stackEntry = usedStack.pop(poppedId);
        if (stackEntry == nullptr) {
            std::cout << "'usedStack' prematurely ran out of elements while popping at task #" << taskId << "\n";
            break;
        }
        freeStack.push(poppedId);
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
        unsigned poppedId;
        MyStackElement* stackEntry = stack.pop(poppedId);
        if (stackEntry == nullptr) {
            std::cout << "... end reached\n";
            break;
        }
        std::cout << "Poped array element #" << poppedId << "...";
        MyStruct& myData = stackEntry->original;
        std::cout << ": nSqrt=" << myData.nSqrt << "\n";
    }

    std::cout << "\n\nStaring the multithreaded tests:\n";
    populateFreeStack();
    std::thread threads[N_THREADS];
	for (unsigned _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
		threads[_threadNumber] = std::thread([](int threadNumber) {
            for (unsigned taskId=0; taskId<654321; taskId++) {
                backAndForth(654321*threadNumber+taskId);
            }
        }, _threadNumber+1);
	}
    for (int _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
    	threads[_threadNumber].join();
    }
    dumpFreeStack();


    return 0;
}