#ifdef UBIT_SUPERMAIN

#include "MicroBit.h"

MicroBit        uBit;

int main()
{
#if CONFIG_ENABLED(MICROBIT_DBG)
    uBit.serial.printf("micro:bit runtime version %s\n", MICROBIT_DAL_VERSION);
#endif

    // Bring up random number generator, BLE, display and system timers.
    uBit.init();

    // Start the user application
    app_main();

    // If app_main exits, there may still be other fibers running, registered event handlers etc.
    // Simply release this fiber, which will mean we enter the scheduler. Worse case, we then
    // sit in the idle task forever, in a power efficient sleep.
    release_fiber();

    // We should never get here, but just in case.
    while(1);
}

#endif
