# microbit-dal

## Building a project for the micro:bit using yotta 

The micro:bit DAL is built on top of [mbed](http://mbed.com) and hence uses [yotta](http://yotta.mbed.com) as an offline build system.

If you'd like to use the mbed online IDE instead, you can find instructions at [the mbed Developer site](https://developer.mbed.org/platforms/Microbit/)


When using `yotta` to build micro:bit projects there are currently two supported toolchains:

* GCC
* ARMCC

## Getting Started

### 1. Install yotta and dependencies

The first step is to get `yotta` and its dependencies onto your machine, to do this follow the install guide [here](http://docs.yottabuild.org/#installing)


For the micro:bit targets you currently still need the srecord tools, which can be installed on **Ubuntu** using
```
sudo apt-get install srecord
```

On **Mac OS X** you can use brew (`brew install srecord`), or you can install it manually from [here](http://srecord.sourceforge.net/) if you are on **Windows**. srecord is used to create the final binaries for the micro:bit so is an essential dependency.


### 2. Fetch the example project

```bash
git clone https://github.com/lancaster-university/microbit
cd microbit #The following instructions assume you're in the example directory
```

**Note: To successfully build this project before the micro:bit DAL is open sourced, you will need access to the microbit-dal private repository, if you need access please email me at j.devine@lancaster.ac.uk.**

### 3. Set your yotta target

A `yotta` target contains the information required by `yotta` in order to build a project for a specific combination of hardware. This includes the type of compiler. The microbit projects can build with both `armcc` and `gcc`, but as it gets installed with the `yotta` installer, we'll use `gcc` by default and choose a micro:bit specific target that knows about the hardware on the board.

You can use either `yotta` or `yt`, which is far easier to type!

```
yt target bbc-microbit-classic-gcc
```

The 'classic' part of this target name referes to the fact that the micro:bit uses "mbed Classic" (see https://developer.mbed.org/) which is the version of mbed before mbed OS. It is also possible to use mbed OS on the micro:bit (see [here](https://www.mbed.com/en/development/hardware/boards/)) but the DAL has not yet been ported to use mbed OS. 
 
You only need to set the target once per project. All future `yotta` commands will use this target information (for example, when resolving dependencies).

### 4. Build the project

```
yt build
```

### 5. Flash your micro:bit
The final step is to check your hex works. 

The `yotta build` command will place files in `/build/<TARGET_NAME>/source`. The file you will need to flash will be microbit-combined.hex. Simply drag and drop the hex.

In the case of our example, using `bbc-microbit-classic-gcc` we could flash the micro:bit (assuming it is plugged in and mounted at `/media/MICROBIT`) as follows:

```
cp ./build/bbc-microbit-classic-gcc/source/microbit-combined.hex /media/MICROBIT
```
The expected result will be that the micro:bit will scroll `BELLO! :)` on its display.

Note that if you'd like to copy the file from the command line, you can use the following command in any `yotta` project to do so, though it assumes you have only one micro:bit plugged in:

```
cp build/$(yt --plain target | head -n 1 | cut -f 1 -d' ')/source/$(yt --plain ls | head -n 1 | cut -f 1 -d' ')-combined.hex /media/MICROBIT/
```
