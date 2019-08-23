#include <iostream>
#include <iomanip>
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
#define N_THREADS      (1*/*std::thread::hardware_concurrency()*/4)
#define BATCH          4096
#define BACK_AND_FORTH (12345)

#define N_ELEMENTS     (N_THREADS*BATCH)   // should be >= N_THREADS * BATCH


// statistics
#define OPERATIONS_COUNT (freeStack.pushCount+freeStack.popCount+usedStack.pushCount+usedStack.popCount)
#define COLLISIONS_COUNT (freeStack.pushCollisions+freeStack.popCollisions+usedStack.pushCollisions+usedStack.popCollisions)


// test section
///////////////

static struct timespec timespec_now;
static inline unsigned long long getMonotonicRealTimeNS() {
    clock_gettime(CLOCK_MONOTONIC, &timespec_now);
    return (timespec_now.tv_sec*1000000000ll) + timespec_now.tv_nsec;
}


// spike vars
/////////////

// the types
struct UserSlot {
    unsigned  taskId;
    unsigned  nSqrt;
    double    n;
    char c[63-2-20];
};
typedef mutua::MTL::stack::UnorderedArrayBasedReentrantStackSlot<UserSlot> StackSlot;

// the backing array
StackSlot backingArray[N_ELEMENTS];

// the stacks
mutua::MTL::stack::UnorderedArrayBasedReentrantStack<StackSlot, N_ELEMENTS, true, true, true> stack(backingArray);
mutua::MTL::stack::UnorderedArrayBasedReentrantStack<StackSlot, N_ELEMENTS, true, true, true> usedStack(backingArray);
mutua::MTL::stack::UnorderedArrayBasedReentrantStack<StackSlot, N_ELEMENTS, true, true, true> freeStack(backingArray);

void populateFreeStack() {
    for (unsigned i=0; i<N_ELEMENTS; i++) {
        freeStack.push(i);
    }
}


// info section
///////////////
struct DoubleIntStruct {
    alignas(sizeof(unsigned)*2) unsigned a;
                                unsigned b;
};
void printHardwareInfo() {
    std::atomic<unsigned>        a_int;
    std::atomic<DoubleIntStruct> a_DoubleIntStruct;
    std::cout << "For this hardware:\n"
              << std::boolalpha <<
                 "\tis std::atomic<int> lock free?                    : "  << std::atomic_is_lock_free(&a_int) << "\n"
                 "\tis std::atomic<struct{int,int}> lock free?        : "  << std::atomic_is_lock_free(&a_DoubleIntStruct) << "\n"
                 "\tstd::atomic<int>::is_always_lock_free             : "  << std::atomic<unsigned>::is_always_lock_free << "\n"
                 "\tstd::atomic<struct{int,int}>::is_always_lock_free : "  << std::atomic<DoubleIntStruct>::is_always_lock_free << "\n\n"
    ;
}

template<typename _StackType, typename _debug>
void dumpStack(_StackType& stack, unsigned expected, string listName, _debug debug) {
    if (debug) std::cout << "Checking list '"<<listName<<"'..." << std::flush;
    unsigned count=0;
    StackSlot* stackEntry;
    while (true && (count <= (N_ELEMENTS*2))) {
        unsigned poppedId = stack.pop(&stackEntry);
        if (stackEntry == nullptr) {
            break;
        }
        if (poppedId == -1) {
            std::cerr << "API error: how come poppedId is -1 and stackEntry != nullptr?\n" << std::flush;
            break;
        }
        count++;
        if (debug) std::cout << poppedId << ",";
    }
    if (debug) std::cout << "\n";
    std::cout << "Result of checking list '"<<listName<<"': " << (expected == count ? "PASSED":"FAILED") << ": expected="<<expected<<"; counted="<<count<<"; collisions="<<COLLISIONS_COUNT<<"\n" << std::flush;
}


// spike methods
////////////////

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

std::atomic<unsigned> scaleUpsCount(0);
std::atomic<unsigned> errorsCount(0);
std::atomic<unsigned> runningThreads(0);
void backAndForth(unsigned threadNumber, unsigned taskId) {

    StackSlot* stackEntry;
    UserSlot* myData;
    unsigned poppedId;

    for (unsigned i=0; i<BATCH; i++) {

        // push to 'usedStack'
        //////////////////////

        while (unlikely ((poppedId = freeStack.pop(&stackEntry)) == -1) ) {
            errorsCount.fetch_add(1);
            std::cout << "'freeStack' prematurely ran out of elements in iteraction "<<i<<" of "<<BATCH<<", thread #"<<threadNumber<<". ABORTING...\n" << std::flush;
            niceExit(1);
        }

        //myData = &(stackEntry->userSlot);
        myData = stackEntry;

        // this section will probably need to be surrounded by a lock
        // once we manage to make the stack lock-free

        myData->taskId       = taskId;
        myData->nSqrt        = taskId+poppedId;
        myData->n            = (double)(myData->nSqrt)*(double)(myData->nSqrt);

        usedStack.push(poppedId);
    }


    for (unsigned i=0; i<BATCH; i++) {

        // pop from 'usedStack'
        ///////////////////////

        while (unlikely ((poppedId = usedStack.pop(&stackEntry)) == -1) ) {
            errorsCount.fetch_add(1);
            std::cout << "'usedStack' prematurely ran out of elements in iteraction "<<i<<" of "<<BATCH<<", thread #"<<threadNumber<<". ABORTING...\n" << std::flush;
            niceExit(1);
        }

        //myData = &(stackEntry->userSlot);
        myData = stackEntry;

        // this section will probably need to be surrounded by a lock
        // once we manage to make the stack lock-free

        double   id    = myData->taskId;
        double   n     = myData->n;
        unsigned nSqrt = myData->nSqrt;

        // check
        if ( (nSqrt != (id+poppedId)) || (n != (double)nSqrt*(double)nSqrt) ) {
            errorsCount.fetch_add(1);
        }

        freeStack.push(poppedId);
    }
}

int main(void) {

	std::cout << DOCS;

    printHardwareInfo();
 
    std::cout << "\n\nStaring the simple tests:\n";
    int anagram[] = {1,2,3,4,4,3,2,1};
    constexpr int anagram_length = sizeof(anagram)/sizeof(anagram[0]);

    for (int i=1010; i<1010+anagram_length; i++) {
        std::cout << "Populating and pushing array element #" << i << " (stackHead is now " << stack.getStackHead() << ")...";
        backingArray[i].nSqrt = anagram[i-1010];
        backingArray[i].n     = anagram[i-1010]*anagram[i-1010];
        stack.push(i);
        std::cout << " OK\n";
    }

    // dump the array
    for (int i=1010; i<1010+anagram_length; i++) {
        std::cout << "backingArray["<<i<<"](next, nSqrt) == {" << backingArray[i].next << ", " << backingArray[i].nSqrt << "},\n";
    }
    std::cout << "stackHead = " << stack.getStackHead() << "; stackHead->next = " << stack.getStackHeadNext() << "\n";

    while (true) {
        StackSlot* stackEntry;
        unsigned poppedId = stack.pop(&stackEntry);
        if (stackEntry == nullptr) {
            std::cout << "... end reached\n";
            if (poppedId != -1) {
                std::cout <<"\t ... but 'poppedId' isn't -1!!\n";
            }
            break;
        }
        std::cout << "Popped array element #" << poppedId << "...";
        //UserSlot& myData = stackEntry->userSlot;
        UserSlot* myData = stackEntry;
        std::cout << ": nSqrt=" << myData->nSqrt << "\n";
    }

    std::cout << "\n\nStaring the multithreaded tests:\n";
    unsigned long long start = getMonotonicRealTimeNS();
    populateFreeStack();
    std::thread threads[N_THREADS];
	for (unsigned _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
		threads[_threadNumber] = std::thread([](int threadNumber) {
            runningThreads.fetch_add(1);
            for (unsigned taskId=threadNumber; taskId<BACK_AND_FORTH; taskId++) {
                backAndForth(threadNumber, BACK_AND_FORTH*threadNumber+taskId);
            }
            runningThreads.fetch_sub(1);
        }, _threadNumber);
	}

    // join the threads, showing the statistics...
    {
        #define PAD(_number, _width)          std::setfill(' ') << std::setw(_width) << std::fixed << _number
        #define DECLARE_STATS(_name)          unsigned last ## _name = 0; unsigned delta ## _name;
        #define TICK_STATS(_name, _expr)      delta ## _name = _expr-last ## _name; last ## _name += delta ## _name;
        #define GET_STATS(_name, _unit, _pad) << PAD(delta ## _name, _pad) << " " _unit
        
        DECLARE_STATS(OperationsCount);
        DECLARE_STATS(CollisionsCount);
        DECLARE_STATS(ErrorsCount);
        std::cout.imbue(std::locale(""));
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            TICK_STATS(OperationsCount, OPERATIONS_COUNT);
            TICK_STATS(CollisionsCount, COLLISIONS_COUNT);
            TICK_STATS(ErrorsCount,     errorsCount);
            std::cout << "\rPerformance: " GET_STATS(OperationsCount, "op/s", 8) "  |  " GET_STATS(CollisionsCount, "col/s", 6) "  |  " << PAD(runningThreads, 2) << " threads  |  " GET_STATS(ErrorsCount, "err/s", 3) " (" << std::fixed << lastErrorsCount << "  total)          " << std::flush;
        } while ( (runningThreads != 0) && ((deltaOperationsCount > 0) || (deltaErrorsCount > 0)) );

        // now join all threads
        for (int _threadNumber=0; _threadNumber<N_THREADS; _threadNumber++) {
            threads[_threadNumber].join();
        }
        std::cout << std::endl << std::flush;

        #undef DECLARE_STATS
        #undef TICK_STATS
        #undef GET_STATS
        #undef PAD
    }
    unsigned long long finish = getMonotonicRealTimeNS();

    dumpStack(freeStack, N_ELEMENTS, "freeStack", false);
    dumpStack(usedStack, 0, "usedStack", false);

    std::cout << "--> Executed in " << ((finish-start) / (unsigned long long)1000'000) << "ms ("<<OPERATIONS_COUNT<<" operations, "<<scaleUpsCount<<" scale-ups, "<<errorsCount<<").\n";

    return 0;
}
