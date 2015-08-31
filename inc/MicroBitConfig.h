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
#define MICROBIT_SRAM_END		0x20004000

// Physical address of the top of the system stack (on mbed-classic this is the top of SRAM)
#define CORTEX_M0_STACK_BASE    MICROBIT_SRAM_END 

// Amount of memory reserved for the stack at the end of memory (bytes).
#define MICROBIT_STACK_SIZE		2000

// Physical address of the end of heap space.
#define MICROBIT_HEAP_END		(CORTEX_M0_STACK_BASE - MICROBIT_STACK_SIZE)

// Block size used by the allocator in bytes.
// n.b. Currently only 32 bits (4 bytes) is supported.
#define MICROBIT_HEAP_BLOCK_SIZE		4

// The proportion of SRAM available on the mbed heap to reserve for the micro:bit heap.
#define MICROBIT_HEAP_SIZE				0.95

// if defined, reuse the 8K of SRAM reserved for SoftDevice (Nordic's memory resident BLE stack) as heap memory.
// The amount of memory reused depends upon whether or not BLE is enabled using MICROBIT_BLE_ENABLED.
#define MICROBIT_HEAP_REUSE_SD

// The lowest address of memory that is safe to use as heap storage when BLE is DISABLED
// Used to define the base of the heap when MICROBIT_HEAP_REUSE_SD is defined.
#define MICROBIT_HEAP_BASE_BLE_DISABLED         0x20000008

// The lowest address of memory that is safe to use as heap storage when BLE is ENABLED
// This is permissable if SD is configured to release some of its internal storage that
// is normally reserved for its BLE GATT table.
#define MICROBIT_HEAP_BASE_BLE_ENABLED          0x20001C00

// The highest address of memory normally reserved for Soft Device that is safe to use as heap storage 
#define MICROBIT_HEAP_SD_LIMIT                  0x20002000

//
// Fiber scheduler configuration
//

// Scheduling quantum (milliseconds)
// Also used to drive the micro:bit runtime system ticker.
#define FIBER_TICK_PERIOD_MS    6


//
// Core micro:bit services
//

// To reduce memory cost and complexity, the micro:bit allows components to register for
// periodic callback events during interrupt context, which occur every scheduling quantum (FIBER_TICK_PERIOD_MS)
// This defines the maximum size of interrupt callback list.
#define MICROBIT_SYSTEM_COMPONENTS      10

// To reduce memory cost and complexity, the micro:bit allows components to register for
// periodic callback events when the processor is idle. 
// This defines the maximum size of the idle callback list.
#define MICROBIT_IDLE_COMPONENTS        6

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
#define MICROBIT_BLE_ENABLED

// Enable/Disable BLUEZONE mode at power up.
#define MICROBIT_BLE_BLUEZONE

// Enable/Disable BLE Service: MicroBitDFU
// This allows over the air programming during normal operation.
#define MICROBIT_BLE_DFU_SERVICE

// Enable/Disable BLE Service: MicroBitEventService
// This allows routing of events from the micro:bit message bus over BLE.
#define MICROBIT_BLE_EVENT_SERVICE

// Enable/Disable BLE Service: MicroBitDeviceInformationService
// This enables the standard BLE device information service.
#define MICROBIT_BLE_DEVICE_INFORMATION_SERVICE

//
// Panic options
//

// Enable this to invoke a panic on out of memory conditions.
#define MICROBIT_PANIC_HEAP_FULL

//
// Debug options
//

// Enable this to route debug messages through the USB serial interface.
// n.b. This also disables the user serial port 'uBit.serial'.
//#define MICROBIT_DBG

// Enable this to receive diagnostic messages from the heap allocator via the USB serial interface.
// n.b. This requires MICROBIT_DBG to be defined.
//#define MICROBIT_HEAP_DBG

#endif
