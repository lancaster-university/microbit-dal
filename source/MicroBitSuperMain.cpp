#include "MicroBit.h"

#ifdef MICROBIT_DBG
Serial pc(USBTX, USBRX);
#endif

MicroBit        uBit;
InterruptIn     resetButton(MICROBIT_PIN_BUTTON_RESET);

void
reset()
{
    NVIC_SystemReset();
}

int main()
{    
    // Bring up soft reset button.
    resetButton.mode(PullUp);
    resetButton.fall(reset);
    
#ifdef MICROBIT_DBG
    pc.baud(115200);

    // For diagnostics. Gives time to open the console window. :-) 
    for (int i=10; i>0; i--)
    {
        pc.printf("=== SUPERMAIN: Starting in %d ===\n", i);
        wait(1.0);
    }

    pc.printf("=== scheduler init... ");
    scheduler_init();
    pc.printf("complete ===\n");
        
    pc.printf("=== uBit init... ");
    uBit.init();
    pc.printf("complete ===\n");
    
    pc.printf("=== Launching app_main ===\n");
    app_main();

    pc.printf("=== app_main exited! ===\n");
    
    while(1)
        uBit.sleep(100);
    
#else

    // Bring up fiber scheduler
    scheduler_init();
    
    // bring up random number generator, BLE, display and system timers.    
    uBit.init();
    
    uBit.sleep(100);

#ifndef NO_BLE
    // Test if we need to enter BLE pairing mode...
    int i=0;
    while (uBit.buttonA.isPressed() && uBit.buttonB.isPressed() && i<10)
    {
        uBit.sleep(100);
        i++;
        
        if (i == 10 && uBit.ble_firmware_update_service != NULL)
            uBit.ble_firmware_update_service->pair();
    }
#endif
        
    app_main();
    
    while(1)
        uBit.sleep(100);
        
#endif

}
