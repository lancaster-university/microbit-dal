/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef ERROR_NO_H
#define ERROR_NO_H

#include "MicroBitConfig.h"

/**
  * Error codes used in the micro:bit runtime.
  * These may be returned from functions implemented in the micro:bit runtime.
  */
enum ErrorCode{

    // No error occurred.
    MICROBIT_OK = 0,

    // Invalid parameter given.
    MICROBIT_INVALID_PARAMETER = -1001,

    // Requested operation is unsupported.
    MICROBIT_NOT_SUPPORTED = -1002,

    // Device calibration errors
    MICROBIT_CALIBRATION_IN_PROGRESS = -1003,
    MICROBIT_CALIBRATION_REQUIRED = -1004,

    // The requested operation could not be performed as the device has run out of some essential resource (e.g. allocated memory)
    MICROBIT_NO_RESOURCES = -1005,

    // The requested operation could not be performed as some essential resource is busy (e.g. the display)
    MICROBIT_BUSY = -1006,

    // The requested operation was cancelled before it completed.
    MICROBIT_CANCELLED = -1007,

    // I2C Communication error occured (typically I2C module on processor has locked up.)
    MICROBIT_I2C_ERROR = -1010,

    // The serial bus is currently in use by another fiber.
    MICROBIT_SERIAL_IN_USE = -1011,

    // The requested operation had no data to return.
    MICROBIT_NO_DATA = -1012
};

/**
  * Error codes used in the micro:bit runtime.
  */
enum PanicCode{
    // PANIC Codes. These are not return codes, but are terminal conditions.
    // These induce a panic operation, where all code stops executing, and a panic state is
    // entered where the panic code is diplayed.

    // Out out memory error. Heap storage was requested, but is not available.
    MICROBIT_OOM = 20,

    // Corruption detected in the micro:bit heap space
    MICROBIT_HEAP_ERROR = 30,

    // Dereference of a NULL pointer through the ManagedType class,
    MICROBIT_NULL_DEREFERENCE = 40,

    // A requested hardware peripheral could not be found,
    MICROBIT_HARDWARE_UNAVAILABLE_ACC = 50,
    MICROBIT_HARDWARE_UNAVAILABLE_MAG = 51,
};
#endif
