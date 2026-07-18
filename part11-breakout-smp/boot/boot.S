.section ".text.boot"  // Make sure the linker puts this at the start of the kernel image

.global _start  // Execution starts here

_start:
    // Only the main core arrives here - the firmware's stub has already
    // parked cores 1-3, leaving them spinning on the Pi 4's spin table

    // Set stack to start below our code
    ldr     x1, =_start
    mov     sp, x1

    // Clean the BSS section
    ldr     x1, =__bss_start     // Start address
    ldr     w2, =__bss_size      // Size of the section
3:  cbz     w2, 4f               // Quit loop if zero
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b               // Loop if non-zero

    // Jump to our main() routine in C (make sure it doesn't return)
4:  bl      main
    // In case it does return, halt the master core too
5:  wfe
    b       5b

.global secondary_entry  // Cores 1-3 arrive here when released from the stub's spin table

secondary_entry:
    // The stub has already dropped us to EL2 and programmed the timer - in
    // fact we couldn't set cntfrq_el0 if we wanted to: it's only writable
    // from EL3, and trying from EL2 raises an exception!

    mrs     x1, mpidr_el1        // Check our processor ID
    and     x1, x1, #3

    ldr     x2, =__stack_start   // Get ourselves a fresh stack - location depends on CPU core asking
    lsl     x3, x1, #14          // Multiply core_number by 16KB (the per-core stack size in link.ld)
    add     x3, x2, x3           // Add to the address
    mov     sp, x3

    adr     x5, spin_cpu0        // Base watch address
1:  ldr     x4, [x5, x1, lsl #3] // Add (8 * core_number) to the base address and load what's there
    cbnz    x4, 2f               // If it's non-zero, go run it...
    wfe                          // ...otherwise sleep until the next event
    b       1b
2:  blr     x4                   // Run the code at the address in x4 (sets x30, so it can return to us)
    b       1b                   // Once that code returns, we start looping and watching all over again

.ltorg

// Our own per-core release words, watched by secondary_entry. Not to be
// confused with the firmware stub's spin table down at 0xd8-0xf0!
.balign 8
.globl spin_cpu0
spin_cpu0:
        .quad 0

.globl spin_cpu1
spin_cpu1:
        .quad 0

.globl spin_cpu2
spin_cpu2:
        .quad 0

.globl spin_cpu3
spin_cpu3:
        .quad 0
