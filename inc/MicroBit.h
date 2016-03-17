#ifndef MICROBIT_H
#define MICROBIT_H

#include "mbed.h"

#include "MicroBitConfig.h"
#include "MicroBitHeapAllocator.h"
#include "MicroBitPanic.h"
#include "ErrorNo.h"
#include "MicroBitSystemTimer.h"
#include "Matrix4.h"
#include "MicroBitCompat.h"
#include "MicroBitComponent.h"
#include "ManagedType.h"
#include "ManagedString.h"
#include "MicroBitImage.h"
#include "MicroBitFont.h"
#include "MicroBitEvent.h"
#include "DynamicPwm.h"
#include "MicroBitI2C.h"
#include "MESEvents.h"

#include "MicroBitButton.h"
#include "MicroBitPin.h"
#include "MicroBitCompass.h"
#include "MicroBitAccelerometer.h"
#include "MicroBitThermometer.h"
#include "MicroBitLightSensor.h"
#include "MicroBitMultiButton.h"

#include "MicroBitSerial.h"
#include "MicroBitIO.h"
#include "MicroBitMatrixMaps.h"
#include "MicroBitDisplay.h"

#include "MicroBitFiber.h"
#include "MicroBitMessageBus.h"

#include "MicroBitBLEManager.h"
#include "MicroBitRadio.h"
#include "MicroBitStorage.h"

// MicroBit::flags values
#define MICROBIT_FLAG_SCHEDULER_RUNNING         0x00000001
#define MICROBIT_FLAG_ACCELEROMETER_RUNNING     0x00000002
#define MICROBIT_FLAG_DISPLAY_RUNNING           0x00000004
#define MICROBIT_FLAG_COMPASS_RUNNING           0x00000008

// MicroBit naming constants
#define MICROBIT_NAME_LENGTH                    5
#define MICROBIT_NAME_CODE_LETTERS              5

// Random number generator
#define NRF51822_RNG_ADDRESS                    0x4000D000


// mbed pin assignments of core components.
#define MICROBIT_PIN_SDA                        P0_30
#define MICROBIT_PIN_SCL                        P0_0

/**
  * Class definition for a MicroBit device.
  *
  * Represents the device as a whole, and includes member variables to that reflect the components of the system.
  */
class MicroBit
{
    private:

    void                    compassCalibrator(MicroBitEvent e);
    void                    onListenerRegisteredEvent(MicroBitEvent evt);
    uint32_t                randomValue;

    public:

	// Reset Button
	InterruptIn     		resetButton;

    // I2C Interface
    MicroBitI2C             i2c;

    // Serial Interface
    MicroBitSerial          serial;

    // Device level Message Bus abstraction
    MicroBitMessageBus      messageBus;

    // Member variables to represent each of the core components on the device.
    MicroBitDisplay         display;
    MicroBitButton          buttonA;
    MicroBitButton          buttonB;
    MicroBitMultiButton     buttonAB;
    MicroBitAccelerometer   accelerometer;
    MicroBitCompass         compass;
    MicroBitThermometer     thermometer;

    //An object of available IO pins on the device
    MicroBitIO              io;

    // Bluetooth related member variables.
	MicroBitBLEManager		bleManager;
    MicroBitRadio           radio;
    BLEDevice               *ble;

    /**
      * Constructor.
      * Create a representation of a MicroBit device as a global singleton.
      * @param messageBus callback function to receive MicroBitMessageBus events.
      *
      * Exposed objects:
      * @code
      * uBit.messageBus; //The message bus where events are fired.
      * uBit.display; //The display object for the LED matrix.
      * uBit.buttonA; //The buttonA object for button a.
      * uBit.buttonB; //The buttonB object for button b.
      * uBit.resetButton; //The resetButton used for soft resets.
      * uBit.accelerometer; //The object that represents the inbuilt accelerometer
      * uBit.compass; //The object that represents the inbuilt compass(magnetometer)
      * uBit.io.P*; //Where P* is P0 to P16, P19 & P20 on the edge connector
      * @endcode
      */
    MicroBit();

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
    void init();

    /**
     * Return the friendly name for this device.
     *
     * @return A string representing the friendly name of this device.
     */
    static ManagedString getName();

    /**
     * Return the serial number of this device.
     *
     * @return A string representing the serial number of this device.
     */
    static ManagedString getSerial();

    /**
      * Will reset the micro:bit when called.
      *
      * Example:
      * @code
      * uBit.reset();
      * @endcode
      */
    void reset();

    /**
      * Delay for the given amount of time.
      * If the scheduler is running, this will deschedule the current fiber and perform
      * a power efficent, concurrent sleep operation.
      * If the scheduler is disabled or we're running in an interrupt context, this
      * will revert to a busy wait.
      *
      * @note Values of 6 and below tend to lose resolution - do you really need to sleep for this short amount of time?
      *
      * @param milliseconds the amount of time, in ms, to wait for. This number cannot be negative.
      * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER milliseconds is less than zero.
      *
      * Example:
      * @code
      * MicroBit::sleep(20); //sleep for 20ms
      * @endcode
      */
    static int sleep(int milliseconds);

    /**
      * Seed the pseudo random number generator using the hardware generator.
      *
      * Example:
      * @code
      * uBit.seedRandom();
      * @endcode
      */
    void seedRandom();

    /**
      * Seed the pseudo random number generator using the given value.
      *
      * @param seed The 32-bit value to seed the generator with.
      *
      * Example:
      * @code
      * uBit.seedRandom(0x12345678);
      * @endcode
      */
    void seedRandom(uint32_t seed);

    /**
      * Generate a random number in the given range.
      * We use the NRF51822 in built random number generator here
      * TODO: Determine if we want to, given its relatively high power consumption!
      *
      * @param max the upper range to generate a number for. This number cannot be negative
      * @return A random, natural number between 0 and the max-1. Or MICROBIT_INVALID_PARAMETER if max is <= 0.
      *
      * Example:
      * @code
      * uBit.random(200); //a number between 0 and 199
      * @endcode
      */
    int random(int max);

    /**
      * Determine the time since this MicroBit was last reset.
      *
      * @return The time since the last reset, in milliseconds. This will result in overflow after 1.6 months.
      * TODO: handle overflow case.
      */
    unsigned long systemTime();

    /**
      * Determine the version of the micro:bit runtime currently in use.
      *
      * @return A textual description of the currentlt executing micro:bit runtime.
      * TODO: handle overflow case.
      */
    const char *systemVersion();

    /**
      * Triggers a microbit panic where an infinite loop will occur swapping between the panicFace and statusCode if provided.
      *
      * @param statusCode the status code of the associated error. Status codes must be in the range 0-255.
      */
    void panic(int statusCode = 0);

	/**
	 * add a component to the array of components which invocate the systemTick member function during a systemTick
	 * @param component The component to add.
	 * @return MICROBIT_OK on success. MICROBIT_NO_RESOURCES is returned if further components cannot be supported.
	 * @note This interface is now deprecated. See fiber_add_system_component().
	 */
	int addSystemComponent(MicroBitComponent *component);

	/**
	 * remove a component from the array of components
	 * @param component The component to remove.
	 * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMTER is returned if the given component has not been previous added.
	 * @note This interface is now deprecated. See fiber_remove_system_component().
	 */
	int removeSystemComponent(MicroBitComponent *component);

	/**
	 * add a component to the array of components which invocate the systemTick member function during a systemTick
	 * @param component The component to add.
	 * @return MICROBIT_OK on success. MICROBIT_NO_RESOURCES is returned if further components cannot be supported.
	 * @note This interface is now deprecated. See fiber_add_idle_component().
	 */
	int addIdleComponent(MicroBitComponent *component);

	/**
	 * remove a component from the array of components
	 * @param component The component to remove.
	 * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMTER is returned if the given component has not been previous added.
	 * @note This interface is now deprecated. See fiber_remove_idle_component().
	 */
	int removeIdleComponent(MicroBitComponent *component);
};

// Entry point for application programs. Called after the super-main function
// has initialized the device and runtime environment.
extern "C" void app_main();


#endif
