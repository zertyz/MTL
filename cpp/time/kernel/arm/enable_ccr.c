#include <linux/module.h>
#include <linux/kernel.h>

#include "../../../compiletime/HostInfo.h"
 
void enable_ccr(void *info) {

      // intel
    #ifdef __x86_64
      #error "This module is supposed to be compiled only when on Raspberry Pi (rPi1, 2, 3 are confirmed working)"

    // Raspberry Pi 2 & 3
    #elif MTL_CPU_INSTR_ARMv7 || MTL_CPU_INSTR_ARMv8

      // code taken from https://matthewarcus.wordpress.com/2018/01/27/using-the-cycle-counter-registers-on-the-raspberry-pi-3/

      // Set the User Enable register, bit 0
      asm volatile ("mcr p15, 0, %0, c9, c14, 0" :: "r" (1));
      // Enable all counters in the PNMC control-register
      asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(1));
      // Enable cycle counter specifically
      // bit 31: enable cycle counter
      // bits 0-3: enable performance counters 0-3
      asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x80000000));

    #elif MTL_CPU_INSTR_ARMv6

      // code taken from https://blog.regehr.org/archives/794

      asm volatile ("mcr p15,  0, %0, c15,  c9, 0\n" : : "r" (1)); 

    #else

      #error "Unknown machine. Possibly this module isn't needed at all."

    #endif
}

#ifdef MTL_CPU_INSTR_ARMv6
  #define RASPBERRY_PI_VERSION "rPi1"
#elif MTL_CPU_INSTR_ARMv7
  #define RASPBERRY_PI_VERSION "rPi2"
#elif MTL_CPU_INSTR_ARMv8
  #define RASPBERRY_PI_VERSION "rPi3"
#else
  #define RASPBERRY_PI_VERSION "(unknown Raspberry Pi version?!)"
#endif

int init_module(void) {
  // Each cpu has its own set of registers
  on_each_cpu(enable_ccr,NULL,0);
  printk (KERN_INFO "Userspace access to CCR enabled for " RASPBERRY_PI_VERSION "-- now MTL::TimeMeasurements may measure time very, very fast.\n");
  return 0;
}
 
void cleanup_module(void) {
}
