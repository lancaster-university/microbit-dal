#ifndef ERROR_NO_H
#define ERROR_NO_H

#include "hal/ErrorNo.h"

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
};
#endif
