#include "MicroBitConfig.h"
/*
 * The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ
 * If we're compiling under GCC, then we suppress any warnings generated from this code (but not the rest of the DAL)
 * The ARM cc compiler is more tolerant. We don't test __GNUC__ here to detect GCC as ARMCC also typically sets this
 * as a compatability option, but does not support the options used...
 */
#if !defined(__arm)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "MicroBit.h"

#include "nrf_soc.h"

/*
 * Return to our predefined compiler settings.
 */
#if !defined(__arm)
#pragma GCC diagnostic pop
#endif


/**
  * Constructor.
  * Create a representation of a MicroBit device as a global singleton.
  * @param messageBus callback function to receive MicroBitMessageBus events.
  *
  * Exposed objects:
  * @code
  * uBit.systemTicker; //the Ticker callback that performs routines like updating the display.
  * uBit.messageBus; //The message bus where events are fired.
  * uBit.display; //The display object for the LED matrix.
  * uBit.buttonA; //The buttonA object for button a.
  * uBit.buttonB; //The buttonB object for button b.
  * uBit.buttonAB; //The buttonAB object for button a+b multi press.
  * uBit.resetButton; //The resetButton used for soft resets.
  * uBit.accelerometer; //The object that represents the inbuilt accelerometer
  * uBit.compass; //The object that represents the inbuilt compass(magnetometer)
  * uBit.io.P*; //Where P* is P0 to P16, P19 & P20 on the edge connector
  * @endcode
  */
MicroBit::MicroBit() :
	resetButton(MICROBIT_PIN_BUTTON_RESET),
    storage(),
    i2c(MICROBIT_PIN_SDA, MICROBIT_PIN_SCL),
    serial(USBTX, USBRX),
    messageBus(),
    display(),
    buttonA(MICROBIT_PIN_BUTTON_A, MICROBIT_ID_BUTTON_A),
    buttonB(MICROBIT_PIN_BUTTON_B, MICROBIT_ID_BUTTON_B),
    buttonAB(MICROBIT_ID_BUTTON_A,MICROBIT_ID_BUTTON_B, MICROBIT_ID_BUTTON_AB),
    accelerometer(i2c),
    compass(i2c, accelerometer, storage),
    compassCalibrator(compass, accelerometer, display),
    thermometer(storage),
    io(MICROBIT_ID_IO_P0,MICROBIT_ID_IO_P1,MICROBIT_ID_IO_P2,
       MICROBIT_ID_IO_P3,MICROBIT_ID_IO_P4,MICROBIT_ID_IO_P5,
       MICROBIT_ID_IO_P6,MICROBIT_ID_IO_P7,MICROBIT_ID_IO_P8,
       MICROBIT_ID_IO_P9,MICROBIT_ID_IO_P10,MICROBIT_ID_IO_P11,
       MICROBIT_ID_IO_P12,MICROBIT_ID_IO_P13,MICROBIT_ID_IO_P14,
       MICROBIT_ID_IO_P15,MICROBIT_ID_IO_P16,MICROBIT_ID_IO_P19,
       MICROBIT_ID_IO_P20),
    bleManager(storage),
    radio(),
    ble(NULL)
{
    // Clear our status
    status = 0;

    // Bring up soft reset functionality as soon as possible.
    resetButton.mode(PullUp);
    resetButton.fall(this, &MicroBit::reset);
}

/**
  * Post constructor initialisation method.
  * After *MUCH* pain, it's noted that the BLE stack can't be brought up in a
  * static context, so we bring it up here rather than in the constructor.
  * n.b. This method *must* be called in main() or later, not before.
  *
  * Example:
  * @code
  * uBit.init();
  * @endcode
  */
void MicroBit::init()
{
    if (status & MICROBIT_INITIALIZED)
        return;

#if CONFIG_ENABLED(MICROBIT_HEAP_ALLOCATOR)
    // Bring up a nested heap allocator.
    microbit_create_nested_heap(MICROBIT_NESTED_HEAP_SIZE);
#endif

    // Bring up fiber scheduler.
    scheduler_init(&messageBus);

    // Seed our random number generator
    seedRandom();

    // Create an event handler to trap any handlers being created for I2C services.
    // We do this to enable initialisation of those services only when they're used, 
    // which saves processor time, memeory and battery life.
    messageBus.listen(MICROBIT_ID_MESSAGE_BUS_LISTENER, MICROBIT_EVT_ANY, this, &MicroBit::onListenerRegisteredEvent);

    status |= MICROBIT_INITIALIZED;

#if CONFIG_ENABLED(MICROBIT_BLE_PAIRING_MODE)
    // Test if we need to enter BLE pairing mode...
    int i=0;
    sleep(100);
    while (buttonA.isPressed() && buttonB.isPressed() && i<10)
    {
        sleep(100);
        i++;

        if (i == 10)
        {
#if CONFIG_ENABLED(MICROBIT_HEAP_ALLOCATOR) && CONFIG_ENABLED(MICROBIT_HEAP_REUSE_SD)
            microbit_create_heap(MICROBIT_SD_GATT_TABLE_START + MICROBIT_SD_GATT_TABLE_SIZE, MICROBIT_SD_LIMIT);
#endif
            // Start the BLE stack, if it isn't already running.
            if (!ble)
            {
                bleManager.init(getName(), getSerial(), messageBus, true);
                ble = bleManager.ble;
            }

            // Enter pairing mode, using the LED matrix for any necessary pairing operations
            bleManager.pairingMode(display, buttonA);
        }
    }
#endif

    // Attempt to bring up a second heap region, using unused memory normally reserved for Soft Device.
#if CONFIG_ENABLED(MICROBIT_HEAP_ALLOCATOR) && CONFIG_ENABLED(MICROBIT_HEAP_REUSE_SD)
#if CONFIG_ENABLED(MICROBIT_BLE_ENABLED)
    microbit_create_heap(MICROBIT_SD_GATT_TABLE_START + MICROBIT_SD_GATT_TABLE_SIZE, MICROBIT_SD_LIMIT);
#else
    microbit_create_heap(MICROBIT_SRAM_BASE, MICROBIT_SD_LIMIT);
#endif
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_ENABLED)
    // Start the BLE stack, if it isn't already running.
    if (!ble)
    {
        bleManager.init(getName(), getSerial(), messageBus, false);
        ble = bleManager.ble;
    }
#endif
}

/**
  * A listener to perform actions as a result of Message Bus reflection.
  *
  * In some cases we want to perform lazy instantiation of components, such as
  * the compass and the accelerometer, where we only want to add them to the idle
  * fiber when someone has the intention of using these components.
  */
void MicroBit::onListenerRegisteredEvent(MicroBitEvent evt)
{
    switch(evt.value)
    {
        case MICROBIT_ID_BUTTON_AB:
            // A user has registered to receive events from the buttonAB multibutton.
            // Disable click events from being generated by ButtonA and ButtonB, and defer the
            // control of this to the multibutton handler.
            //
            // This way, buttons look independent unless a buttonAB is requested, at which
            // point button A+B clicks can be correclty handled without breaking
            // causal ordering.
            buttonA.setEventConfiguration(MICROBIT_BUTTON_SIMPLE_EVENTS);
            buttonB.setEventConfiguration(MICROBIT_BUTTON_SIMPLE_EVENTS);
            buttonAB.setEventConfiguration(MICROBIT_BUTTON_ALL_EVENTS);
            break;

        case MICROBIT_ID_COMPASS:
            // A listener has been registered for the compass.
            // The compass uses lazy instantiation, we just need to read the data once to start it running.
            // Touch the compass through the heading() function to ensure it is calibrated. if it isn't this will launch any associated calibration algorithms.
            compass.heading();

            break;

        case MICROBIT_ID_ACCELEROMETER:
            // A listener has been registered for the accelerometer.
            // The accelerometer uses lazy instantiation, we just need to read the data once to start it running.
            accelerometer.updateSample();
            break;

        case MICROBIT_ID_THERMOMETER:
            // A listener has been registered for the thermometer.
            // The thermometer uses lazy instantiation, we just need to read the data once to start it running.
            thermometer.updateSample();
            break;
    }
}

