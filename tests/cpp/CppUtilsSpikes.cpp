#include <iostream>
#include <map>
#include <unordered_map>
#include <thread>
#include <random>
#include <algorithm>

#include <mutex>
using namespace std;

#include "../../cpp/MutuaTestMacros.h"
#include "../../cpp/TimeMeasurements.h"
#include "../../cpp/BetterExceptions.h"
#include "../../cpp/ConstExprUtils.h"
using namespace mutua::cpputils;


//constexpr array<char, 262000/*17*40960000*/> constexprHugeArray = Mutua::CppUtils::ConstExprUtils::generateRandomArray<char, 262000/*17*40960000*/>('a', 'z');
constexpr array<char, 17*40960000> constexprHugeArray = Mutua::CppUtils::ConstExprUtils::generateRandomArray<char, 17*40960000>('a', 'z');
int main() {

    size_t r = 1;		// used to prevent optimizations to avoid loops

    HEAP_MARK();

    cout << endl << endl;
    cout << "ConstExprUtils:" << endl;
    cout << "============== " << endl << endl;
    constexpr array<char, 64> constexprArray = Mutua::CppUtils::ConstExprUtils::generateRandomArray<char, 64>('a', 'z');
    string staticallyGeneratedRandomString(constexprArray.begin(), constexprArray.end());
    cout << "ConstExpr random String: '" << staticallyGeneratedRandomString << "'" << endl << flush;
    //string staticallyGeneratedHugeRandomString(constexprHugeArray.begin(), constexprArray.end());
    //cout << "constexpr huge String: '" << staticallyGeneratedHugeRandomString << "'" << endl << flush;
    cout << "constexpr huge String: '" << flush;
    for (unsigned i=0; i<constexprHugeArray.size(); i++) {
        cout << constexprHugeArray[i];
    }
    cout << "'" << endl << flush;

    cout << endl << endl;
    cout << "BetterExceptions:" << endl;
    cout << "================ " << endl << endl;

    try {
        THROW_EXCEPTION(std::overflow_error, "My Artificial Exception");
    } catch (std::exception const& e) {
        DUMP_EXCEPTION(e, "Try block aborted due to purposely thrown exception, properly caught in the catch block (since you're seeing this message)",
                       "what",   "this",
                       "when",   "Was",
                       "action", "Expressed");
    }
    DUMP_EXCEPTION(std::overflow_error("My Forced Exception"), "Just a dump of a created Exception, in order for us to see the quality of the debugging output");

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
            std::scoped_lock<std::mutex> lock(opGuard);
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

    cerr << "Final r: " << r << endl << endl << flush;

    HEAP_TRACE("Program allocation totals", cout <<);

    return EXIT_SUCCESS;
}
