#ifndef ERROR_NO_H
#define ERROR_NO_H

/**
  * Error codes used in the micro:bit runtime.
  * These may be returned from functions implemented in the micro:bit runtime.
  */
enum ErrorCode{

    // No error occurred.
    MICROBIT_OK = 0,

    // Invalid parameter given.
    MICROBIT_INVALID_PARAMETER = -1001, 

    // Requested operation is unspupported.
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
    MICROBIT_I2C_ERROR = -1010 
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
};
#endif
