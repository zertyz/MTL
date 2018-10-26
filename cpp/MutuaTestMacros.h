#ifndef MUTUA_TESTUTILS_MUTUATESTMACROS_H_
#define MUTUA_TESTUTILS_MUTUATESTMACROS_H_

/** <pre>
 * MutuaTestMacros.cpp
 * ===================
 * created by luiz, Oct 24, 2018
 *
 * Macros ease Unit Tests with Boost Test Framework.
 *
*/


////////////////////////////////////////////////////////////////
// HEAP TRACE: the 'new' and 'delete' operators are redefined //
// to track allocations and  deallocations.  Two  macros  are //
// defined: 'HEAP_MARK()' and  'HEAP_TRACE(...)'.  The  first //
// should be called to set the base  line  (or  to  zero  the //
// counters; the second, to compute and output debug info.    //
// if 'NO_HEAP_TRACE' is defined, no redefinitions  of  'new' //
// nor 'delete' takes place.                                  //
////////////////////////////////////////////////////////////////
#ifndef NO_HEAP_TRACE

#include <iostream>	// needed for std::malloc * std::free definition
#include <new>

static size_t                             heap_trace_allocated_bytes            = 0;
static size_t                             heap_trace_deallocated_bytes          = 0;

void* operator new(std::size_t size) {
    void* alloc_entry = std::malloc(size);
    if (!alloc_entry) {
        throw std::bad_alloc();
    }
    heap_trace_allocated_bytes += *(size_t*)(((size_t)alloc_entry)-sizeof(size_t));
    return alloc_entry;
}

void operator delete(void* alloc_entry) noexcept {
    heap_trace_deallocated_bytes += *(size_t*)(((size_t)alloc_entry)-sizeof(size_t));
    std::free(alloc_entry);
}

#define HEAP_MARK()                                                            \
    size_t marked_heap_trace_allocated_bytes   = heap_trace_allocated_bytes;   \
    size_t marked_heap_trace_deallocated_bytes = heap_trace_deallocated_bytes  \

#define HEAP_TRACE(_taskName, _outputFunction) {                                                      \
    size_t allocations   = heap_trace_allocated_bytes   - marked_heap_trace_allocated_bytes;          \
    size_t deallocations = heap_trace_deallocated_bytes - marked_heap_trace_deallocated_bytes;        \
    _outputFunction(string(_taskName) + " allocation costs:\n" +                                      \
                    "      allocations: " + to_string(allocations)                 + " bytes\n" +     \
                    "    deallocations: " + to_string(deallocations)               + " bytes\n" +     \
                    "         retained: " + to_string(allocations - deallocations) + " bytes\n\n");   \
}                                                                                                     \

#endif // NO_HEAP_TRACE

#endif /* MUTUA_TESTUTILS_MUTUATESTMACROS_H_ */
