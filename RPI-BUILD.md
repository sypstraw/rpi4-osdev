Writing a "bare metal" operating system for Raspberry Pi 4
==========================================================

Building on the RPi4 itself
---------------------------

It's possible (but not super-simple) to follow this tutorial on the Raspberry Pi without need for an additional build device.

Perhaps the easiest route is to firstly re-image your Pi to use the 64-bit Raspberry Pi OS (Beta), and then use a pre-built cross-compiler:

 * Download a zipped _.img_ image file from the [64-bit image list](https://downloads.raspberrypi.org/raspios_arm64/images/), picking the newest update
 * Unzip it and use the [Raspberry Pi Imager](https://www.raspberrypi.org/software/) to write it to your SD card, selecting "Use custom" from the options and pointing it at your downloaded _.img_ file
 * Boot the Pi and follow the setup wizard to ensure you have a working Internet connection
 * Just for luck, run `sudo apt update`

You'll then need to download a cross-compiler from the Arm website.

What you're looking for is the current [AArch64 ELF bare-metal target (aarch64-none-elf)](https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-aarch64-aarch64-none-elf.tar.xz). If this link is somehow broken, you can use Google to search for "Arm GNU-A linux hosted cross compilers".

Then unpack the archive using `tar -xf <filename>`. You'll end up with a _gcc_ directory (albeit with a slightly longer name), which itself contains a _bin_ subdirectory, wherein you'll find the _gcc_ executable (again - with a longer name!). Remember this path.

Note: you can avoid re-imaging the Pi, by instead [building a cross-compiler yourself](https://wiki.osdev.org/GCC_Cross-Compiler).

Now let's build something:

 * Use `git` to clone this repo: `git clone https://github.com/sypstraw/rpi4-osdev.git`
 * Decide which part you want to build - I like testing with _part5-framebuffer_ (it's visual, so you'll know when it works!)
 * Copy the _Makefile.gcc_ to _Makefile_
 * Edit the _Makefile_ and ensure the `GCCPATH` variable points to the _bin_ subdirectory where your cross-compiler is to be found
 * Type `make` at the command line and it should build without errors

If you want to then boot with this, you'll need to copy the _kernel8.img_ file to a prepped SD card as the tutorial discusses. For the purposes of testing this process, I did the following (NOTE: it will trash your OS install unless you backup the old files so you can move them back later):

 * `sudo cp kernel8.img /boot`
 * Then edit _/boot/config.txt_ to include only these lines (for _part5-framebuffer_ anyway, otherwise read the tutorial in full for any necessary config changes for other parts...):

```c
hdmi_group=1
hdmi_mode=16
core_freq_min=500
```

Reboot and you should see the _part5-framebuffer_ demo firing up!

[Go to part1-bootstrapping >](./part1-bootstrapping/)
