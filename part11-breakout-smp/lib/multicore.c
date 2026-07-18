#include "../include/multicore.h"

// The firmware's stub parks cores 1-3 at boot, each spinning on its own
// release word in the Pi 4's spin table (the same protocol Linux uses).
// Writing an address there and issuing a sev sends that core on its way.

#define STUB_RELEASE_CPU1 0xE0
#define STUB_RELEASE_CPU2 0xE8
#define STUB_RELEASE_CPU3 0xF0

extern void secondary_entry(void);

void store32(unsigned long address, unsigned long value)
{
    // The empty asm hides the address's origin from the compiler: GCC assumes
    // a constant address near zero (like our spin table's 0xe0!) must be a
    // null pointer dereference and warns. This is real memory on real hardware
    asm volatile ("" : "+r"(address));
    *(volatile unsigned long *) address = value;
}

unsigned long load32(unsigned long address)
{
    asm volatile ("" : "+r"(address));
    return *(volatile unsigned long *) address;
}

void start_core1(void (*func)(void))
{
    store32((unsigned long)&spin_cpu1, (unsigned long)func);
    store32(STUB_RELEASE_CPU1, (unsigned long)&secondary_entry);
    asm volatile ("dsb sy\n\tsev" ::: "memory"); // Ensure the writes land, then wake the cores
}

void start_core2(void (*func)(void))
{
    store32((unsigned long)&spin_cpu2, (unsigned long)func);
    store32(STUB_RELEASE_CPU2, (unsigned long)&secondary_entry);
    asm volatile ("dsb sy\n\tsev" ::: "memory");
}

void start_core3(void (*func)(void))
{
    store32((unsigned long)&spin_cpu3, (unsigned long)func);
    store32(STUB_RELEASE_CPU3, (unsigned long)&secondary_entry);
    asm volatile ("dsb sy\n\tsev" ::: "memory");
}

void clear_core1(void)
{
    store32((unsigned long)&spin_cpu1, 0);
}

void clear_core2(void)
{
    store32((unsigned long)&spin_cpu2, 0);
}

void clear_core3(void)
{
    store32((unsigned long)&spin_cpu3, 0);
}
