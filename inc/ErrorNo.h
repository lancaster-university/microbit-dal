#ifndef ERROR_NO_H
#define ERROR_NO_H

/**
  * Error codes used in the micro:bit runtime.
  */
enum Error{
    // Invalid parameter given.
    MICROBIT_INVALID_VALUE = -1, 

    // Invalid I/O operation requested.
    MICROBIT_IO_OP_NA = -2, 

    // Device calibration errors
    MICROBIT_COMPASS_IS_CALIBRATING = -3, 
    MICROBIT_COMPASS_CALIBRATE_REQUIRED = -4,

    // I2C Communication error occured (typically I2C module on processor has locked up.) 
    MICROBIT_I2C_LOCKUP = 10, 

    // Out out memory error. Heap storage was requested, but is not available.
    MICROBIT_OOM = 20, 

    // Corruption detected in the micro:bit heap space
    MICROBIT_HEAP_ERROR = 30 
};
#endif
