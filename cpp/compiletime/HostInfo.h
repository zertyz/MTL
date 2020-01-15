#ifndef MTL_COMPILETIME_HostInfo_h
#define MTL_COMPILETIME_HostInfo_h

/** <pre>
 * HostInfo.hpp
 * ==================
 * created by luiz, Jan 15, 2020
 *
 * Defines macros so programs may know information that won't change at runtime for the
 * host machine, system and OS (host being where this unit is compiled):
 *
 * To be defined macros -- these macros will be defined (with 1) if what they estate is true.
 *                         the macros will have a prefix (everything before x) and a suffix (x):
 * - MTL_ARCHITECTURE_x     -- defines one macro prefixed by MTL_ARCHITECTURE_ where x is X86_64, ARM_32 or ARM_64
 * - MTL_CPU_INSTR_x        -- x is X86_64 (for intel 64 bits), ARMv6 (for rPi1), ARMv7 (for rPi2) or ARMv8 (for rPi3)
 * - MTL_CACHE_LINE_x       -- x is 64 (bytes, for intel & ARMv8) or 32 (bytes, for ARMv6 & ARMv7)
 * - MTL_OS_x               -- x is Linux, FreeBSD, Unix or Windows
 * - MTL_COMPILER_x         -- x is GCC, Clang or MSVC
 *
 * String macros -- these macros are not suitable for #ifdef conditional compilation (for they are strings),
 *                  but are very useful for diagnostic messages:
 *
 * - MTL_ARCHITECTURE     -- "X86_64", "ARM_32", "ARM_64"
 * - MTL_CPU_INSTR        -- "X86_64" (intel 64 bits), "ARMv6" (rPi1), "ARMv7" (rPi2), "ARMv8" (rPi3)
 * - MTL_CACHE_LINE       -- "64" (bytes, for intel & ARMv8) or "32" (bytes, for ARMv6 & ARMv7)
 * - MTL_CACHE_LINE_SIZE  -- the same as above, but the value is an integer
 * - MTL_OS               -- "Linux", "FreeBSD", "Unix", "Windows"
 * - MTL_COMPILER         -- "GCC", "Clang", "MSVC"
 * - MTL_COMPILER_VERSION -- compiler provided
*/

// MTL_ARCHITECTURE & MTL_ARM_INSTR
#if __ARM_ARCH == 6
    #define MTL_ARCHITECTURE_ARM_32 1
    #define MTL_ARCHITECTURE        "ARM_32"
    #define MTL_CPU_INSTR_ARMv6     1
    #define MTL_CPU_INSTR           "ARMv6"
    #define MTL_CACHE_LINE_32       1
    #define MTL_CACHE_LINE_SIZE     32
    #define MTL_CACHE_LINE          "32"
#elif __x86_64
    #define MTL_ARCHITECTURE_X86_64 1
    #define MTL_ARCHITECTURE        "X86_64"
    #define MTL_CPU_INSTR_X86_64    1
    #define MTL_CPU_INSTR           "X86_64"
    #define MTL_CACHE_LINE_64       1
    #define MTL_CACHE_LINE_SIZE     64
    #define MTL_CACHE_LINE         "64"
#else
    #define MTL_ARCHITECTURE        "Unknown Architecture"
    #define MTL_CPU_INSTR           "Unknown CPU instruction set"
    #define MTL_CACHE_LINE          "Unknown cache line size"
#endif

// MTL_OS
#ifdef __linux
    #define MTL_OS_Linux   1
    #define MTL_OS         "Linux"
#elif __FreeBSD__
    #define MTL_OS_FreeBSD 1
    #define MTL_OS         "FreeBSD"
#elif __unix
    #define MTL_OS_Unix    1
    #define MTL_OS         "Unknown Unix variant"
#elif _MSVC_____
    #define MTL_OS_Windows 1
    #define MTL_OS         "Windows"
#else
    #define MTL_OS         "Unknown OS"
#endif

// MTL_COMPILER
#ifdef __clang__
    #define MTL_COMPILER_Clang 1
    #define MTL_COMPILER       "Clang"
#elif _MSVC_____
    #define MTL_COMPILER_MSVC  1
    #define MTL_COMPILER       "MSVC"
#else
    #define MTL_COMPILER_GCC   1
    #define MTL_COMPILER       "GCC"
#endif

// MTL_COMPILER_VERSION
#define MTL_COMPILER_VERSION __VERSION__

#endif //MTL_COMPILETIME_HostInfo_h
