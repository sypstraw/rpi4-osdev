Writing a "bare metal" operating system for Raspberry Pi 4 (Part 11)
====================================================================

[< Go back to part10-multicore](../part10-multicore)

Putting it all together
-----------------------
Frankly, I'm unlikely to write much documentation for this part. This part simply builds on the work we've done so far, and delivers a new Breakout codebase with:

 * Gameplay running in the foreground on the main CPU core 0
 * Graphics updated in the background on CPU core 1
 * Looped 8-bit music playing in the background on CPU core 2
 * Bluetooth communications managed in the background on CPU core 3

I've taken the opportunity to organise the code a little better and so you'll see some changes to the _Makefile_, and a new directory structure in place. Tidy codebase = tidy mind!

Bigger stacks for busier cores
------------------------------
In part10, we gave each secondary core a tiny 512-byte stack - and that was plenty, because all they did was update progress bars and feed the audio FIFO. Now those same cores are running a graphics engine and a Bluetooth stack, and 512 bytes doesn't cut it any more.

The clearest example is in _lib/fb.c_. When the graphics core moves the paddle, `moveRectAbs()` saves the pixels it's about to disturb in a local variable:

```c
unsigned int bitmap[width][height]; // This is very unsafe if it's too big for the stack...
```

The comment was there for a reason! For our 80x20 paddle, that's 80 x 20 x 4 bytes = 6,400 bytes - over twelve times the size of the whole stack. The overspill silently trashes whatever the linker happened to place below the stack in memory. Bugs like this are especially nasty because they can go unnoticed for a long time: if the clobbered memory isn't in active use, everything appears to work - until a new toolchain, or even just a different link order, shuffles the memory map and something important moves into the firing line.

So in this part's _boot/link.ld_, each core's stack grows from 512 bytes to 16KB (`0x4000`), giving `moveRectAbs()` comfortable headroom. There's one knock-on effect in _boot/boot.S_: `secondary_entry` calculates each core's stack pointer by multiplying its core number by the stack size, so the shift changes from `#9` (x512) to `#14` (x16KB):

```c
    ldr     x2, =__stack_start   // Get ourselves a fresh stack - location depends on CPU core asking
    lsl     x3, x1, #14          // Multiply core_number by 16KB (the per-core stack size in link.ld)
    add     x3, x2, x3           // Add to the address
    mov     sp, x3
```

If you ever change the stack size again, remember these two must move together - the linker script reserves the space, but it's this little bit of assembly that decides where each core actually points its `sp`.

Important takeaway
------------------
As you read through this code, you'll maybe notice that a very different style has emerged. Multi-processing adds a dimension of complexity when coding and requires a total mindset shift.

There's a lot more signalling/semaphores, so the cores can be sure they're doing the right thing at the right time.

You're no longer "dry-running" a single, sequential thread. You're having to spot the potential for bugs/unexpected behaviour from the interplay of four concurrent threads!

_Good luck as you explore the code!_

Progress video
--------------
Click to watch a quick video of the game in action.

https://user-images.githubusercontent.com/68182575/173157526-2b22f9b2-0f55-42a0-87e1-b8278930fefe.mp4

PS: Sorry for the bad audio quality - the game itself sounds great in real life, I promise!

[Go to part12-wgt >](../part12-wgt)
