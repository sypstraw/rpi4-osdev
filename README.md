Writing a "bare metal" operating system for Raspberry Pi 4
==========================================================
> [!WARNING]
> _Saturday 18th July 2026_: Parts 12-15 still require updating to reflect the latest RPi4 firmware changes. Parts 1-11 are, however, updated and confirmed working (at least when built on an Apple MacBook Pro M1 running MacOS Tahoe 26.5.2, Clang 21.0.0, Node v24.13.1).

Introduction
------------

As a tech CEO, I don't write code any more. And I've recently realised just how much I miss it.

Currently in the throes of a nationwide "lockdown" due to Covid-19 (and having been spared my usual commute), I've found myself with more hours in the day. I have taken this time for myself and used it to fulfil a childhood ambition - to write a **bare metal** operating system that runs on commercial hardware.

What does bare metal mean?
--------------------------

When we buy a computer or a tablet/smartphone it typically comes with some basic software pre-installed. You'll likely be familiar with watching Microsoft Windows, Mac OS, iOS, Android or maybe even Linux start up as you power the device (or **boot** it) for the first time. These are all operating systems - software designed to make computer chips work out of the box for mere mortals like you and me. They help us interact with the machine by drawing to a screen, processing messages from devices like keyboards & mice, working with networking hardware to connect you to the Internet, allowing us to playback sound and much, much more.

Software developers around the world then build applications (apps) that run on top of these operating systems. These apps talk to the hardware via the operating system (**OS**), so this complex code doesn't have to be written over and over again. As a result, it's possible to be a software developer without knowing much about hardware at all! It's the OS that does the hard work that allows us to use apps like Facebook, Instagram, WhatsApp, TikTok etc.

It's fair to say that _computers can't do anything useful without an OS_. They just sit there waiting to be told what to do. So, why is it that only software giants like Microsoft, Apple and Google get to tell the majority of computers what to do as they're being switched on? Why can't we? Well, we can, and that is what bare metal programming is.

Choice of hardware
------------------

If you're excited by the prospect of telling a computer what to do then you need an interest in hardware. The computer chip that's going to do your bidding is called a **CPU** (Central Processing Unit) - it's the beating heart of every computer device. Lots of companies have designed such CPUs over the years, but two - Intel and Arm - are most widely adopted. These each have their advantages and disadvantages. If you own a smartphone, it's highly likely that it's running on a chip designed by Arm. If you own a laptop running Microsoft Windows then it's likely to be running on an Intel chip. You'll want to develop an understanding of both **architectures** eventually, but I've chosen an Arm device for this project.

The new [Raspberry Pi 4 Model B](https://www.raspberrypi.org/products/raspberry-pi-4-model-b/) is a low-cost computer that runs on a 1.5 GHz 64-bit quad-core Arm Cortex-A72 processor. It's a device that many millions of people worldwide use, and so it's exciting to write bare metal code for it. Imagine that somebody else might one day use your OS! The **RPi4** also has some useful attached hardware that will help us along the way.

Hardware prerequisites
----------------------

You'll need some hardware to get started with writing your OS:

 * An RPi4 with a dedicated power supply and HDMI lead
 * A monitor/TV connected to the RPi4 via HDMI
 * A micro-SD card to boot the RPi4 from
 * A computer to write your code on e.g. a Windows/Mac laptop (the **dev machine**)

You'll need to make sure that you can write to the micro-SD card using your dev machine. For me, that meant buying an SD card adapter, because the micro-SD card was too small for the slot in my laptop. You may need the same, or even a USB SD card reader too if your laptop doesn't have one built-in.

Other incredibly useful hardware that you simply can't do without:

 * A pair of eyebrow tweezers (I borrowed these from my wife!) - useful to insert/remove the micro-SD card into the tiny slot on the RPi4
 * A [USB to serial TTL cable](https://www.amazon.co.uk/gp/product/B01N4X3BJB/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1) - useful to see what your OS is doing long before you can write information to the screen

Software prerequisites
----------------------

If you can't get someone else's OS running, you likely won't be able to write your own. So I started by flashing the SD card with Raspbian - Raspberry Pi's recommended OS. I used the very neat [Imager tool](https://www.raspberrypi.org/downloads/) that they make available on their website to do this.

Hook up your RPi4 and make sure it boots into Raspbian. There are plenty of resources online to help you achieve/troubleshoot this. Getting Raspbian up will test that your hardware setup is working properly. Note: because I connected my RPi4 to my (not brilliant) TV, I needed to make an edit in the _config.txt_ file on the SD card (setting the `hdmi_safe` parameter to 1) to ensure that I could see the screen. Without that, it was just black. If you're still having trouble, check out the other _config.txt_ video options [here on the Raspberry Pi website](https://www.raspberrypi.com/documentation/computers/config_txt.html#video-options).

Don't proceed until you get Raspbian running!

---

The RPi4 runs on an Arm Cortex-A72 processor. Your dev machine is likely running on an Intel processor. You'll therefore need some software that helps you build code to run on a different architecture. This is called a **cross-compiler**.

Using Arm's compiler for Linux
------------------------------

Download and unpack [Arm's gcc compiler](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads). For reasons that I won't go into here, you'll need to use the "AArch64 ELF bare-metal target". Since I'm using WSL on Windows 10 to emulate Ubuntu, I downloaded the x86_64 Linux hosted cross-compiler.

I also advocate installing GNU make - you'll need it soon enough. Because I'm using WSL, for me that was simply a matter of typing `sudo apt install make` and entering my password.

Using clang on Mac OS X (Apple Silicon or Intel)
------------------------------------------------

Download and install XCode from the App Store. This will give you a raft of developer tools for free, including `make`.

I recommend using [Homebrew](https://docs.brew.sh/Installation) to install LLVM. For me, Homebrew was already installed, so this was a simple case of typing `brew install llvm`.

LLVM will give you all you need to start building for Raspberry Pi bare metal on Mac. It even works on my M1 MacBook Pro with Apple Silicon, which runs an ARM processor rather than an Intel processsor.

Building directly on a Raspberry Pi 4
-------------------------------------

You can read more about how to build on the Pi itself [here](./RPI-BUILD.md).

_Now you're ready to start writing your OS!_

[Go to part1-bootstrapping >](./part1-bootstrapping/)

Acknowledgements
----------------

Not all of the code here is my original work, but has been collated from and inspired by some fantastic contributors.

Thanks go to:

 * Zoltan Baldaszti's "Bare metal Raspberry Pi 3 tutorials" [(github)](https://github.com/bztsrc/raspi3-tutorial/)

Do reach out if you would like to be acknowledged here too!
