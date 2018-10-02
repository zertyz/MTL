#include <iostream>
#include <map>
#include <unordered_map>
#include <thread>
#include <random>
#include <algorithm>

#include <mutex>
using namespace std;

#include "../../cpp/TimeMeasurements.h"
using namespace mutua::cpputils;


#define HEAP_TRACE

#include <new>

static size_t                             heap_trace_allocated_bytes            = 0;
static size_t                             heap_trace_deallocated_bytes          = 0;
static size_t                             heap_trace_allocated_bytes_baseline   = 0;
static size_t                             heap_trace_deallocated_bytes_baseline = 0;

void* operator new(std::size_t size) {
    void* alloc_entry = std::malloc(size);
    if (!alloc_entry) {
        throw std::bad_alloc();
    }
    heap_trace_allocated_bytes += *(size_t*)(((size_t)alloc_entry)-sizeof(size_t));
    //std::cout << "(-1) equals " << size << " : " << *(size_t*)(((size_t)alloc_entry)-sizeof(size_t)) << std::endl << std::flush;
    return alloc_entry;
}

void operator delete(void* alloc_entry) noexcept {
    heap_trace_deallocated_bytes += *(size_t*)(((size_t)alloc_entry)-sizeof(size_t));
    //std::cout << "(-1) : " << *(size_t*)(((size_t)alloc_entry)-sizeof(size_t)) << std::endl << std::flush;
    std::free(alloc_entry);
}

void setHeapTraceBaseline() {
    heap_trace_allocated_bytes_baseline   = heap_trace_allocated_bytes;
    heap_trace_deallocated_bytes_baseline = heap_trace_deallocated_bytes;
}

void getHeapTraceInfo(string title) {
    std::cout << "\t" << title << ":" << std::endl;
    std::cout << "\t\tAllocations:   " << (heap_trace_allocated_bytes   - heap_trace_allocated_bytes_baseline)   << " bytes" << std::endl;
    std::cout << "\t\tDeallocations: " << (heap_trace_deallocated_bytes - heap_trace_deallocated_bytes_baseline) << " bytes" << std::endl;
    std::cout << std::endl << std::flush;
}


int main() {

    size_t r = 1;		// used to prevent optimizations to avoid loops

    cout << endl << endl;
    cout << "Memory Management Costs:" << endl;
    cout << "======================= " << endl << endl;
    for (int count=0; count<10; count++) {
        unsigned long long newStart = TimeMeasurements::getMonotonicRealTimeUS();
        for (int i = 0; i < 10'000'000; i++) {
            std::vector<int> *vectorPointer = new std::vector<int>(10);
            delete vectorPointer;
        }
        unsigned long long newFinish = TimeMeasurements::getMonotonicRealTimeUS();
        cout << "New/Delete allocation timings: " << (newFinish - newStart) << "µs" << endl << flush;

        unsigned long long uniqueStart = TimeMeasurements::getMonotonicRealTimeUS();
        for (int i = 0; i < 10'000'000; i++) {
            std::unique_ptr<std::vector<int>> vectorPointer = std::make_unique<std::vector<int>>(10);
        }
        unsigned long long uniqueFinish = TimeMeasurements::getMonotonicRealTimeUS();
        cout << "unique_ptr allocation timings: " << (uniqueFinish - uniqueStart) << "µs" << endl << flush;
    }

    cout << endl << endl;
    cout << "Mutex Costs:" << endl;
    cout << "=========== " << endl << endl;
    std::mutex opGuard;
    for (int count=0; count<10; count++) {
        unsigned long long lockUnlockStart = TimeMeasurements::getMonotonicRealTimeUS();
        for (int i = 0; i < 1'000'000'000; i++) {
            opGuard.lock();
	    r += r;
            opGuard.unlock();
        }
        unsigned long long lockUnlockFinish = TimeMeasurements::getMonotonicRealTimeUS();
        cout << "Lock/Unlock mutex timings:\t " << (lockUnlockFinish - lockUnlockStart) << "µs" << endl << flush;

        unsigned long long raiiStart = TimeMeasurements::getMonotonicRealTimeUS();
        for (int i = 0; i < 1'000'000'000; i++) {
            std::lock_guard<std::mutex> lock(opGuard);
	    r += r;
        }
        unsigned long long raiiFinish = TimeMeasurements::getMonotonicRealTimeUS();
        cout << "RAII locking timings:\t\t " << (raiiFinish - raiiStart) << "µs" << endl << flush;
    }

    cout << endl << endl;
    cout << "Vector of Strings Costs:" << endl;
    cout << "======================= " << endl << endl;
    std::vector<string> svec = {"s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "s16", "s17", "s18", "s19", "s20"};
    for (int count=0; count<10; count++) {
        unsigned long long directStart = TimeMeasurements::getMonotonicRealTimeUS();
        for (int i = 0; i < 10'000'000; i++) {
            for (int j=0; j<20; j++) {
                svec[j][0]='j';
            }
        }
        unsigned long long directFinish = TimeMeasurements::getMonotonicRealTimeUS();
        cout << "Direct Access timings:\t\t " << (directFinish - directStart) << "µs" << endl << flush;

        unsigned long long referencedStart = TimeMeasurements::getMonotonicRealTimeUS();
        for (int i = 0; i < 10'000'000; i++) {
            for (int j=0; j<20; j++) {
                string& sref = svec[j];
                sref[0]='j';
            }
        }
        unsigned long long referencedFinish = TimeMeasurements::getMonotonicRealTimeUS();
        cout << "Referenced Access timings:\t " << (referencedFinish - referencedStart) << "µs" << endl << flush;

        unsigned long long copiedStart = TimeMeasurements::getMonotonicRealTimeUS();
        for (int i = 0; i < 10'000'000; i++) {
            for (int j=0; j<20; j++) {
                string sref = svec[j];
                sref[0]='j';
            }
        }
        unsigned long long copiedFinish = TimeMeasurements::getMonotonicRealTimeUS();
        cout << "Copied Access timings:\t\t " << (copiedFinish - copiedStart) << "µs" << endl << flush;
    }

    cerr << "Final r: " << r << endl;
    getHeapTraceInfo("Program allocation totals");
    return EXIT_SUCCESS;
}
