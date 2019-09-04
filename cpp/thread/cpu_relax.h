/**
 * cpu_relax.h
 *
 *  Created on: 3 de set de 2019
 *      Author: luiz
 *
 *  Provides the `cpu_relax()` macro, calling specific machine instructions
 *  designed to be used in spin loops (PAUSE for x86 and YIELD for ARM), for
 *  their ability to preserve some power (and heat) while freeing the core'
 *  busses to execute code from another CPU.
 */

#ifndef GITHUB_CPP_THREAD_cpu_relax_h_
#define GITHUB_CPP_THREAD_cpu_relax_h_


// the 'cpu_relax()' is a macro calling a specific machine instruction
//////////////////////////////////////////////////////////////////////
#if __x86_64
    #define cpu_relax() asm volatile ("pause" ::: "memory");
#elif  __arm__
    #define cpu_relax() asm volatile ("yield" ::: "memory");
#else
    #error Unknown CPU. Please, update cpu_relax.h
#endif
//////////////////////////////////////////////////////////////////////



#endif /* GITHUB_CPP_THREAD_cpu_relax_h_ */
