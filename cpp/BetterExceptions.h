#ifndef MUTUA_CPPUTILS_BETTEREXCEPTIONS_H
#define MUTUA_CPPUTILS_BETTEREXCEPTIONS_H

#include <sstream>

/** <pre>
 * BetterExceptions.cpp
 * ====================
 * created by luiz, Oct 19, 2018
 *
 * Macros to help to better diagnose where throws were made.
 *
*/

#define BUILD_EXCEPTION_DEBUG_MSG(exception, message, ...)                                                           \
    [](auto const& e, string msg, string pretty_func, std::array<string, 100> paramDuples) {                         \
        /* paramDuples := {param1Name, param1Value, param2Name, param2Value, ...} */                                 \
        int paramDuplesLength = paramDuples.size();                                                                  \
        std::stringstream cstr;                                                                                      \
        cstr << "\n## Exception '" << e.what() << "', caught in:" << std::endl                                       \
             <<   "##     " << __FILE__ << ":" << __LINE__ << " -- " << pretty_func << std::endl                     \
             <<   "##     Debug Info: {";                                                                            \
        for (int i=0; i<paramDuplesLength; i+=2) {                                                                   \
            if (paramDuples[i] != "") {                                                                              \
                cstr << (i > 0 ? ", " : "") << paramDuples[i] << "='" << paramDuples[i+1] << "'";                    \
            }                                                                                                        \
        }                                                                                                            \
        cstr << "}" << std::endl                                                                                     \
             <<   "## --> " << msg << std::endl                                                                      \
             <<   "## Stack: " << std::endl                                                                          \
             <<   "##     (no stack trace available -- see https://www.italiancpp.org/2017/01/27/spy-your-memory-usage-with-operator-new/)" << std::endl                                                   \
             <<   "## Caused by: Exception '....' ... https://en.cppreference.com/w/cpp/error/rethrow_if_nested" << std::endl; \
        return cstr.str();                                                                                           \
    }(exception, message, __PRETTY_FUNCTION__, {__VA_ARGS__});

#define DUMP_EXCEPTION(exception, message, ...) {                                                                                          \
    string exceptionDebugMsg = BUILD_EXCEPTION_DEBUG_MSG(exception, message, __VA_ARGS__); \
    std::cerr << exceptionDebugMsg << std::endl << std::flush;                             \
}

// double expansion trick macros to give us __LINE__ as a string
#define S(x) #x
#define S_(x) S(x)
#define THROW_EXCEPTION(_ExceptionType, _msg) throw _ExceptionType(_msg + "', thrown at '" __FILE__ ":" S_(__LINE__) " -- "s + std::string(__PRETTY_FUNCTION__))

#endif //MUTUA_CPPUTILS_BETTEREXCEPTIONS_H
