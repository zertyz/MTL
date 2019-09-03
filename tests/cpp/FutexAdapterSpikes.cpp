#include <iostream>
#include <iomanip>
#include <atomic>
#include <thread>
#include <cstring>
#include <mutex>

#include "../../cpp/time/TimeMeasurements.hpp"
using namespace MTL::time::TimeMeasurements;

#include "../../cpp/thread/FutexAdapter.hpp"
using namespace MTL::thread;

// compile & run with clear; echo -en "\n\n###############\n\n"; toStop="chrome vscode visual-studio-code subl3 java"; for p in $toStop; do pkill -stop -f "$p"; done; sudo sync; g++ -std=c++17 -O3 -mcpu=native -march=native -mtune=native -pthread -I../../external/EABase/include/Common/ FutexAdapterSpikes.cpp -o FutexAdapterSpikes && sudo sync && sleep 2 && sudo time nice -n -20 ./FutexAdapterSpikes; for p in $toStop; do pkill -cont -f "$p"; done

#define DOCS "spikes on 'FutexAdapter'\n"                                        \
             "=======================\n"                                         \
             "\n"                                                                \
             "This is the helper program used to develop 'FutexAdapter.hpp',\n"  \
             "aiming at controling the execution of threads with the lowest\n"   \
             "possible latency -- in which case, it will be used as a lock\n"    \
             "strategy in 'FlexLock.hpp'.\n"


#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define PAD(_number, _width)         std::setfill(' ') << std::setw(_width) << std::fixed << (_number)


// spike vars
/////////////


// info section
///////////////

#include <EABase/config/eacompiler.h>
#include <EABase/config/eaplatform.h>
void printInfo() {

    std::cout << "Compile-time Info:\n"
                 "\tCompiler & Version :  " EA_COMPILER_STRING "\n"
                 "\tOS                 :  " EA_PLATFORM_NAME "\n"
                 "\tPlatform           :  " EA_PLATFORM_DESCRIPTION "\n"
                 "\tEA_CACHE_LINE_SIZE :  " << EA_CACHE_LINE_SIZE << "\n" << std::flush
    ;

}


// check section
///////////////


// spike methods
////////////////

void waitAndResume() {
    uint64_t    startWaitThread,    elapsedWaitThread;
    uint64_t startControlThread, elapsedControlThread;
    int      waitRetVal, controlRetVal;
    std::atomic<int32_t>  futexId = 0;
    int32_t  futexIdWhileWaiting;

    // this is "control thread"
    std::thread t([&] {
        startControlThread = getProcessorCycleCount();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        elapsedControlThread = getProcessorCycleCount() - startControlThread;
        futexIdWhileWaiting = futexId;
        //controlRetVal = FutexAdapter::wait(futexId);
        FutexAdapter::wake(futexId);
    });

    startWaitThread = getProcessorCycleCount();
    waitRetVal = FutexAdapter::wait(futexId);
    elapsedWaitThread = getProcessorCycleCount() - startWaitThread;

    t.join();

    std::cout << "   elapsedWaitThread: " <<    elapsedWaitThread << "\n"
                 "elapsedControlThread: " << elapsedControlThread << "\n"
                 "          waitRetVal: " <<           waitRetVal << "\n"
                 "       controlRetVal: " <<        controlRetVal << "\n"
                 " futexIdWhileWaiting: " <<  futexIdWhileWaiting << "\n"
                 "             futexId: " <<              futexId << "\n";
}

void lockAndUnlock() {
    uint64_t    startWaitThread,    elapsedWaitThread;
    uint64_t startControlThread, elapsedControlThread;
    int      waitRetVal, controlRetVal;
    Futex    futex;
    bool     try_lock;

    // this is "control thread"
    std::thread t([&] {
        startControlThread = getProcessorCycleCount();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        elapsedControlThread = getProcessorCycleCount() - startControlThread;
        try_lock = futex.try_lock();
        futex.unlock();
    });

    futex.lock();
    startWaitThread = getProcessorCycleCount();
    futex.lock();
    elapsedWaitThread = getProcessorCycleCount() - startWaitThread;

    t.join();

    std::cout << "   elapsedWaitThread: " <<    elapsedWaitThread << "\n"
                 "elapsedControlThread: " << elapsedControlThread << "\n"
                 "          waitRetVal: " <<           waitRetVal << "\n"
                 "       controlRetVal: " <<        controlRetVal << "\n"
                 "            try_lock: " <<             try_lock << "\n";
}


int main(void) {

	std::cout << DOCS;

    printInfo();

    // formatted output
    std::cout.imbue(std::locale(""));

    waitAndResume();
    lockAndUnlock();

    return 0;
}
