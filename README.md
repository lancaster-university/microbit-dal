# microbit-dal

## Building a project for the micro:bit using Yotta 

Instead of using the online IDE, Yotta can be used to provide an equivalent offline experience. The current compilers that are available are:

* GCC
* ARMCC

## Getting Started

### 1. Install Yotta 
The first step is to get Yotta onto your machine, to do this follow the install guide [here](http://docs.yottabuild.org/#installing)

**Note: if you are on windows, dependencies will be missed as of 8/8/15, please use the helper script located [here](https://github.com/ARMmbed/yotta/blob/master/get_yotta.py).**

### 2. Fetch the example project

If your install has gone correctly, and you have all dependencies installed, the next step is to fetch the example project using the runtime from GitHub.

```
git clone https://github.com/lancaster-university/microbit
```

**Note: To successfully build this project you will need access to the microbit-dal private repository, if you need access please email me at j.devine@lancaster.ac.uk.**

### 3. Try to build
Building rarely works first time due to dependencies currently not being installed by Yotta, so the next step is to **try** to build.

The default yotta target you will receive when you pull the aforementioned repo is bbc-microbit-classic-armcc, you can use the following command to print your current target in Yotta:

```
yt target

bbc-microbit-classic-armcc 0.0.5
mbed-armcc 0.0.8
```

If you do not have armcc installed (or don't have a license for Keil), then you will need to use GCC. To swap to the GCC target run:

```
yt target bbc-microbit-classic-gcc
```

Then you should **try** to build using the following command:

```
yt build
```

**NOTE:
To build the final hex files for the micro:bit, you will need to install the srec which can be installed via brew (`brew install srecord`), or you can install it manually from [here](http://srecord.sourceforge.net/).**

### 4. Flash your micro:bit
The final step is to check your hex works. 

The yotta build command will place files in `/build/<TARGET_NAME>/source`. The file you will need to flash will be microbit-combined.hex. Simply drag and drop the hex.

The expected result will be that the micro:bit will scroll `BELLO! :)` on its display.
