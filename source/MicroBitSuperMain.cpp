#include "MicroBit.h"

MicroBit        uBit;
InterruptIn     resetButton(MICROBIT_PIN_BUTTON_RESET);

extern char* MICROBIT_BLE_DEVICE_NAME;

int main()
{    
    // Bring up soft reset button.
    resetButton.mode(PullUp);
    resetButton.fall(microbit_reset);

#if CONFIG_ENABLED(MICROBIT_DBG)

    // For diagnostics. Gives time to open the console window. :-) 
    for (int i=3; i>0; i--)
    {
        uBit.serial.printf("=== SUPERMAIN: Starting in %d ===\n", i);
        wait(1.0);
    }

    uBit.serial.printf("micro:bit runtime DAL version %s\n", MICROBIT_DAL_VERSION);

#endif    

    // Bring up our nested heap allocator.
    microbit_heap_init();

    // Bring up fiber scheduler
    scheduler_init();

    // Bring up random number generator, BLE, display and system timers.    
    uBit.init();

    // Provide time for all threaded initialisers to complete.
    uBit.sleep(100);

    //check our persistent storage for compassCalibrationData
    MicroBitStorage s = MicroBitStorage();
    MicroBitConfigurationBlock *b = s.getConfigurationBlock();

    //if we have some calibrated data, calibrate the compass!
    if(b->magic == MICROBIT_STORAGE_CONFIG_MAGIC)
    {
        if(b->compassCalibrationData != CompassSample(0,0,0))
            uBit.compass.setCalibration(b->compassCalibrationData);
    }

    delete b;

#if CONFIG_ENABLED(MICROBIT_BLE_PAIRING_MODE)
    // Test if we need to enter BLE pairing mode...
    int i=0;
    while (uBit.buttonA.isPressed() && uBit.buttonB.isPressed() && i<10)
    {
        uBit.sleep(100);
        i++;

        if (i == 10)
        {
            // Start the BLE stack, if it isn't already running.
            if (!uBit.ble)
            {
                uBit.bleManager.init(uBit.getName(), uBit.getSerial(), true);
                uBit.ble = uBit.bleManager.ble;
            }

            // Enter pairing mode, using the LED matrix for any necessary pairing operations
            uBit.bleManager.pairingMode(uBit.display);
        }
    }
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_ENABLED)
    // Start the BLE stack, if it isn't already running.
    if (!uBit.ble)
    {
        uBit.bleManager.init(uBit.getName(), uBit.getSerial(), false);
        uBit.ble = uBit.bleManager.ble;
    }
#endif

    app_main();

    // If app_main exits, there may still be other fibers running, registered event handlers etc.
    // Simply release this fiber, which will mean we enter the scheduler. Worse case, we then
    // sit in the idle task forever, in a power efficient sleep.
    release_fiber();

    // We should never get here, but just in case.
    while(1);
}
