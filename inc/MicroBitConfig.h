/**
  * Compile time configuration options for the micro:bit runtime.
  */
  
#ifndef MICROBIT_CONFIG_H
#define MICROBIT_CONFIG_H

#include "mbed.h"

//
// Memory configuration 
//

// Physical address of the top of SRAM.
#ifndef MICROBIT_SRAM_END		
#define MICROBIT_SRAM_END		0x20004000
#endif

// Physical address of the top of the system stack (on mbed-classic this is the top of SRAM)
#ifndef CORTEX_M0_STACK_BASE    
#define CORTEX_M0_STACK_BASE    MICROBIT_SRAM_END 
#endif

// Amount of memory reserved for the stack at the end of memory (bytes).
#ifndef MICROBIT_STACK_SIZE		
#define MICROBIT_STACK_SIZE		2048
#endif

// Physical address of the end of heap space.
#ifndef MICROBIT_HEAP_END
#define MICROBIT_HEAP_END		(CORTEX_M0_STACK_BASE - MICROBIT_STACK_SIZE)
#endif

// Block size used by the allocator in bytes.
// n.b. Currently only 32 bits (4 bytes) is supported.
#ifndef MICROBIT_HEAP_BLOCK_SIZE
#define MICROBIT_HEAP_BLOCK_SIZE		4
#endif

// The proportion of SRAM available on the mbed heap to reserve for the micro:bit heap.
#ifndef MICROBIT_HEAP_SIZE
#define MICROBIT_HEAP_SIZE				0.95
#endif

// if defined, reuse the 8K of SRAM reserved for SoftDevice (Nordic's memory resident BLE stack) as heap memory.
// The amount of memory reused depends upon whether or not BLE is enabled using MICROBIT_BLE_ENABLED.
// Set '1' to enable. 
#ifndef MICROBIT_HEAP_REUSE_SD
#define MICROBIT_HEAP_REUSE_SD          1
#endif

// The lowest address of memory that is safe to use as heap storage when BLE is DISABLED
// Used to define the base of the heap when MICROBIT_HEAP_REUSE_SD is defined.
#ifndef MICROBIT_HEAP_BASE_BLE_DISABLED
#define MICROBIT_HEAP_BASE_BLE_DISABLED         0x20000008
#endif

// The lowest address of memory that is safe to use as heap storage when BLE is ENABLED
// This is permissable if SD is configured to release some of its internal storage that
// is normally reserved for its BLE GATT table.
#ifndef MICROBIT_HEAP_BASE_BLE_ENABLED
#define MICROBIT_HEAP_BASE_BLE_ENABLED          0x20001C00
#endif

// The highest address of memory normally reserved for Soft Device that is safe to use as heap storage 
#ifndef MICROBIT_HEAP_SD_LIMIT
#define MICROBIT_HEAP_SD_LIMIT                  0x20002000
#endif

//
// Fiber scheduler configuration
//

// Scheduling quantum (milliseconds)
// Also used to drive the micro:bit runtime system ticker.
#ifndef FIBER_TICK_PERIOD_MS
#define FIBER_TICK_PERIOD_MS            6
#endif

//
// Message Bus:
// Default behaviour for event handlers, if not specified in the listen() call
// 
// Permissable values are:
//   MESSAGE_BUS_LISTENER_REENTRANT
//   MESSAGE_BUS_LISTENER_QUEUE_IF_BUSY
//   MESSAGE_BUS_LISTENER_DROP_IF_BUSY
//   MESSAGE_BUS_LISTENER_NONBLOCKING

#ifndef MESSAGE_BUS_LISTENER_DEFAULT_FLAGS
#define MESSAGE_BUS_LISTENER_DEFAULT_FLAGS          MESSAGE_BUS_LISTENER_QUEUE_IF_BUSY
#endif

//
// Maximum event queue depth. If a queue exceeds this depth, further events will be dropped.
// Used to prevent message queues growing uncontrollably due to badly behaved user code and causing panic conditions.
// 
#ifndef MESSAGE_BUS_LISTENER_MAX_QUEUE_DEPTH
#define MESSAGE_BUS_LISTENER_MAX_QUEUE_DEPTH        10
#endif
//
// Core micro:bit services
//

// To reduce memory cost and complexity, the micro:bit allows components to register for
// periodic callback events during interrupt context, which occur every scheduling quantum (FIBER_TICK_PERIOD_MS)
// This defines the maximum size of interrupt callback list.
#ifndef MICROBIT_SYSTEM_COMPONENTS
#define MICROBIT_SYSTEM_COMPONENTS      10
#endif

// To reduce memory cost and complexity, the micro:bit allows components to register for
// periodic callback events when the processor is idle. 
// This defines the maximum size of the idle callback list.
#ifndef MICROBIT_IDLE_COMPONENTS
#define MICROBIT_IDLE_COMPONENTS        6
#endif

//
// BLE options
//
// The BLE stack is very memory hungry. Each service can therefore be compiled in or out 
// by enabling/disabling the options below. 
//
// n.b. The minimum set of services to enable over the air programming of the device will
// still be brought up in 'BLUEZONE' mode regardless of the settings below.
//

// Enable/Disable BLE during normal operation.
// Set '1' to enable. 
#ifndef MICROBIT_BLE_ENABLED
#define MICROBIT_BLE_ENABLED        1
#endif

// Enable/Disable BLUEZONE mode at power up.
// Set '1' to enable. 
#ifndef MICROBIT_BLE_BLUEZONE
#define MICROBIT_BLE_BLUEZONE       1
#endif

// Enable/Disable BLE Service: MicroBitDFU
// This allows over the air programming during normal operation.
// Set '1' to enable. 
#ifndef MICROBIT_BLE_DFU_SERVICE
#define MICROBIT_BLE_DFU_SERVICE    1
#endif

// Enable/Disable BLE Service: MicroBitEventService
// This allows routing of events from the micro:bit message bus over BLE.
// Set '1' to enable. 
#ifndef MICROBIT_BLE_EVENT_SERVICE
#define MICROBIT_BLE_EVENT_SERVICE  1
#endif

// Enable/Disable BLE Service: MicroBitDeviceInformationService
// This enables the standard BLE device information service.
// Set '1' to enable. 
#ifndef MICROBIT_BLE_DEVICE_INFORMATION_SERVICE
#define MICROBIT_BLE_DEVICE_INFORMATION_SERVICE     1
#endif


// Enable/Disable BLE Service: MicroBitLEDService
// This enables the control and the LED matrix display via BLE.
// Set '1' to enable. 
#ifndef MICROBIT_BLE_LED_SERVICE
#define MICROBIT_BLE_LED_SERVICE                0
#endif

// Enable/Disable BLE Service: MicroBitAccelerometerService
// This enables live access to the on board 3 axis accelerometer.
// Set '1' to enable. 
#ifndef MICROBIT_BLE_ACCELEROMETER_SERVICE
#define MICROBIT_BLE_ACCELEROMETER_SERVICE     0
#endif

// Enable/Disable BLE Service: MicroBitMagnetometerService
// This enables live access to the on board 3 axis magnetometer.
// Set '1' to enable. 
#ifndef MICROBIT_BLE_MAGNETOMETER_SERVICE
#define MICROBIT_BLE_MAGNETOMETER_SERVICE       0
#endif

// Enable/Disable BLE Service: MicroBitButtonService
// This enables live access to the two micro:bit buttons.
// Set '1' to enable. 
#ifndef MICROBIT_BLE_BUTTON_SERVICE
#define MICROBIT_BLE_BUTTON_SERVICE             0
#endif

// This enables live access to the two micro:bit buttons.
// Set '1' to enable. 
#ifndef MICROBIT_BLE_IO_PIN_SERVICE
#define MICROBIT_BLE_IO_PIN_SERVICE             0
#endif

// This enables live access to the die temperature sensors on the micro:bit.
// Set '1' to enable. 
#ifndef MICROBIT_BLE_TEMPERATURE_SERVICE
#define MICROBIT_BLE_TEMPERATURE_SERVICE        0
#endif

// Defines the maximum length strong that can be written to the
// display over BLE.
#ifndef MICROBIT_BLE_MAXIMUM_SCROLLTEXT
#define MICROBIT_BLE_MAXIMUM_SCROLLTEXT     20
#endif
//
// Accelerometer options
//

// Enable this to read 10 bits of data from the acclerometer.
// Otherwise, 8 bits are used.
// Set '1' to enable. 
#ifndef USE_ACCEL_LSB
#define USE_ACCEL_LSB               0
#endif

//
// Display options
//

// Selects the matrix configuration for the display driver.
// Known, acceptable options are:
//
#define MICROBUG_REFERENCE_DEVICE       1
#define MICROBIT_3X9                    2
#define MICROBIT_SB1                    3
#define MICROBIT_SB2                    4

#ifndef MICROBIT_DISPLAY_TYPE
#define MICROBIT_DISPLAY_TYPE MICROBIT_SB2
#endif

// Selects the minimum permissable brightness level for the device
// in the region of 0 (off) to 255 (full brightness)
#ifndef MICROBIT_DISPLAY_MINIMUM_BRIGHTNESS 
#define MICROBIT_DISPLAY_MINIMUM_BRIGHTNESS     2     
#endif

// Selects the maximum permissable brightness level for the device
// in the region of 0 (off) to 255 (full brightness)
#ifndef MICROBIT_DISPLAY_MAXIMUM_BRIGHTNESS 
#define MICROBIT_DISPLAY_MAXIMUM_BRIGHTNESS     255
#endif

// Selects the default brightness for the display
// in the region of zero (off) to 255 (full brightness)
#ifndef MICROBIT_DISPLAY_DEFAULT_BRIGHTNESS 
#define MICROBIT_DISPLAY_DEFAULT_BRIGHTNESS     (MICROBIT_DISPLAY_MINIMUM_BRIGHTNESS + ((MICROBIT_DISPLAY_MAXIMUM_BRIGHTNESS - MICROBIT_DISPLAY_MINIMUM_BRIGHTNESS) / 2))
#endif

// Selects the default scroll speed for the display.
// The time taken to move a single pixel (ms).
#ifndef MICROBIT_DEFAULT_SCROLL_SPEED 
#define MICROBIT_DEFAULT_SCROLL_SPEED       120
#endif

// Selects the number of pixels a scroll will move in each quantum.
#ifndef MICROBIT_DEFAULT_SCROLL_STRIDE 
#define MICROBIT_DEFAULT_SCROLL_STRIDE      -1 
#endif

// Selects the time each character will be shown on the display during print operations.
// The time each character is shown on the screen  (ms).
#ifndef MICROBIT_DEFAULT_PRINT_SPEED 
#define MICROBIT_DEFAULT_PRINT_SPEED        400
#endif


//
// Panic options
//

// Enable this to invoke a panic on out of memory conditions.
// Set '1' to enable. 
#ifndef MICROBIT_PANIC_HEAP_FULL
#define MICROBIT_PANIC_HEAP_FULL    1
#endif

//
// Debug options
//

// Enable this to route debug messages through the USB serial interface.
// n.b. This also disables the user serial port 'uBit.serial'.
// Set '1' to enable. 
#ifndef MICROBIT_DBG
#define MICROBIT_DBG            0
#endif

// Enable this to receive diagnostic messages from the heap allocator via the USB serial interface.
// n.b. This requires MICROBIT_DBG to be defined.
// Set '1' to enable. 
#ifndef MICROBIT_HEAP_DBG
#define MICROBIT_HEAP_DBG       0
#endif

// Versioning options.
// We use semantic versioning (http://semver.org/) to identify differnet versions of the micro:bit runtime.
// Where possible we use yotta (an ARM mbed build tool) to help us track versions.
// if this isn't available, it can be defined manually as a configuration option.
//
#ifndef MICROBIT_DAL_VERSION
#define MICROBIT_DAL_VERSION   "unknown" 
#endif


//
// Helper macro used by the micro:bit runtime to determine if a boolean configuration option is set.
//
#define CONFIG_ENABLED(X) (X == 1)
#define CONFIG_DISABLED(X) (X != 1)

#endif
