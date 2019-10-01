# microbit-dal

The Device Abstraction Layer (DAL) provides the core set of drivers, mechanisms and types that make up the micro:bit runtime.

## Overview

The micro:bit runtime provides an easy to use environment for programming the BBC micro:bit in the C/C++ language, written by Lancaster University. It contains device drivers for all the hardware capabilities of the micro:bit, and also a suite of runtime mechanisms to make programming the micro:bit easier and more flexible. These range from control of the LED matrix display to peer-to-peer radio communication and secure Bluetooth Low Energy services. The micro:bit runtime is proudly built on the ARM mbed and Nordic nrf51 platforms.

In addition to supporting development in C/C++, the runtime is also designed specifically to support higher level languages provided by our partners that target the micro:bit. It is currently used as a support library for all the languages on the BBC www.microbit.org website; including the Javascript Blocks Editor and Micropython

## Links

[micro:bit runtime docs](http://lancaster-university.github.io/microbit-docs/) | [uBit](https://github.com/lancaster-university/microbit) |  [samples](https://github.com/lancaster-university/microbit-samples)

## Build Environments

| Build Environment | Documentation |
| ------------- |-------------|
| ARM mbed online | http://lancaster-university.github.io/microbit-docs/online-toolchains/#mbed |
| yotta  | http://lancaster-university.github.io/microbit-docs/offline-toolchains/#yotta |



## Hello World!

```cpp
#include "MicroBitDisplay.h"

MicroBitDisplay display;

int main()
{
    display.scroll("Hello world!");
}
```

## BBC Community Guidelines

[Microbit Community Guidelines](http://microbit.org/community/)
