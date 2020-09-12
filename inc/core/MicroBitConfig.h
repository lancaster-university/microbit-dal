/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

/**
  * Compile time configuration options for the micro:bit runtime.
  */

#ifndef MICROBIT_CONFIG_H
#define MICROBIT_CONFIG_H

#include "mbed.h"
#include "yotta_cfg_mappings.h"

//
// Memory configuration
//

// The start address of usable RAM memory.
#ifndef MICROBIT_SRAM_BASE
#define MICROBIT_SRAM_BASE                      0x20000008
#endif

// Physical address of the top of SRAM.
#ifndef MICROBIT_SRAM_END
#define MICROBIT_SRAM_END                       0x20004000
#endif

// The end address of memory normally reserved for Soft Device.
#ifndef MICROBIT_SD_LIMIT
#ifdef TARGET_MCU_NRF51_16K_S130
#define MICROBIT_SD_LIMIT                       0x20002800
#else
#define MICROBIT_SD_LIMIT                       0x20002000
#endif
#endif

// The physical address in memory of the Soft Device GATT table.
#ifndef MICROBIT_SD_GATT_TABLE_START
#ifdef TARGET_MCU_NRF51_16K_S130
#define MICROBIT_SD_GATT_TABLE_START            0x20002200
#else
#define MICROBIT_SD_GATT_TABLE_START            0x20001900
#endif
#endif

// Physical address of the top of the system stack (on mbed-classic this is the top of SRAM)
#ifndef CORTEX_M0_STACK_BASE
#define CORTEX_M0_STACK_BASE                    MICROBIT_SRAM_END
#endif

// Amount of memory reserved for the stack at the end of memory (bytes).
#ifndef MICROBIT_STACK_SIZE
#define MICROBIT_STACK_SIZE                     2048
#endif

// Physical address of the end of mbed heap space.
#ifndef MICROBIT_HEAP_END
#define MICROBIT_HEAP_END                       (CORTEX_M0_STACK_BASE - MICROBIT_STACK_SIZE)
#endif

// Defines the size of a physical FLASH page in RAM.
#ifndef PAGE_SIZE
#define PAGE_SIZE                               1024
#endif

// Defines where in memory persistent data is stored.
#ifndef KEY_VALUE_STORE_PAGE
#define KEY_VALUE_STORE_PAGE	                (PAGE_SIZE * (NRF_FICR->CODESIZE - 17)) 
#endif

#ifndef BLE_BOND_DATA_PAGE 
#define BLE_BOND_DATA_PAGE                      (PAGE_SIZE * (NRF_FICR->CODESIZE - 18))
#endif

// MicroBitFileSystem uses DEFAULT_SCRATCH_PAGE to mark end of FileSystem
#ifndef DEFAULT_SCRATCH_PAGE
#define DEFAULT_SCRATCH_PAGE	                (PAGE_SIZE * (NRF_FICR->CODESIZE - 19))
#endif

// Address of the end of the current program in FLASH memory.
// This is recorded by the C/C++ linker, but the symbol name varies depending on which compiler is used.
#if defined(__arm)
extern uint32_t Image$$ER_IROM1$$RO$$Limit;
#define FLASH_PROGRAM_END (uint32_t) (&Image$$ER_IROM1$$RO$$Limit)
#else
extern uint32_t __etext;
#define FLASH_PROGRAM_END (uint32_t) (&__etext)
#endif

//
// If set to '1', this option enables the microbit heap allocator. This supports multiple heaps and interrupt safe operation.
// If set to '0', the standard GCC libc heap allocator is used, which restricts available memory in BLE scenarios, and MessageBus operations
// in ISR contexts will no longer be safe.
//
#ifndef MICROBIT_HEAP_ENABLED
#define MICROBIT_HEAP_ENABLED                   1
#endif


// Block size used by the allocator in bytes.
// n.b. Currently only 32 bits (4 bytes) is supported.
#ifndef MICROBIT_HEAP_BLOCK_SIZE
#define MICROBIT_HEAP_BLOCK_SIZE                4
#endif

// If defined, reuse any unused SRAM normally reserved for SoftDevice (Nordic's memory resident BLE stack) as heap memory.
// The amount of memory reused depends upon whether or not BLE is enabled using MICROBIT_BLE_ENABLED.
// Set '1' to enable.
#ifndef MICROBIT_HEAP_REUSE_SD
#define MICROBIT_HEAP_REUSE_SD                  1
#endif

// The amount of memory allocated to Soft Device to hold its BLE GATT table.
// For standard S110 builds, this should be word aligned and in the range 0x300 - 0x700.
// Any unused memory will be automatically reclaimed as HEAP memory if both MICROBIT_HEAP_REUSE_SD and MICROBIT_HEAP_ALLOCATOR are enabled.
#ifndef MICROBIT_SD_GATT_TABLE_SIZE
#define MICROBIT_SD_GATT_TABLE_SIZE             0x300
#endif

//
// Fiber scheduler configuration
//

// Scheduling quantum (milliseconds)
// Also used to drive the micro:bit runtime system ticker.
#ifndef SYSTEM_TICK_PERIOD_MS
#define SYSTEM_TICK_PERIOD_MS                   6
#endif

// Enable used_data field in Fiber structure (for thread-local data)
#ifndef MICROBIT_FIBER_USER_DATA
#define MICROBIT_FIBER_USER_DATA                0
#endif

// Indicate get_fiber_list() API is supported
#ifndef MICROBIT_GET_FIBER_LIST_SUPPORTED
#define MICROBIT_GET_FIBER_LIST_SUPPORTED       1
#endif

// Maximum size of the FiberPool
// Defines the size that the pool of unused Fiber contexts is permitted to grow to. After this point, memory
// from unused Fiber contexts will be restored to the Heap Allocator.
#ifndef MICROBIT_FIBER_MAXIMUM_FIBER_POOL_SIZE
#define MICROBIT_FIBER_MAXIMUM_FIBER_POOL_SIZE  3
#endif

//
// Message Bus:
// Default behaviour for event handlers, if not specified in the listen() call
//
// Permissable values are:
//   MESSAGE_BUS_LISTENER_REENTRANT
//   MESSAGE_BUS_LISTENER_QUEUE_IF_BUSY
//   MESSAGE_BUS_LISTENER_DROP_IF_BUSY
//   MESSAGE_BUS_LISTENER_IMMEDIATE

#ifndef EVENT_LISTENER_DEFAULT_FLAGS
#define EVENT_LISTENER_DEFAULT_FLAGS            MESSAGE_BUS_LISTENER_QUEUE_IF_BUSY
#endif

//
// Maximum event queue depth. If a queue exceeds this depth, further events will be dropped.
// Used to prevent message queues growing uncontrollably due to badly behaved user code and causing panic conditions.
//
#ifndef MESSAGE_BUS_LISTENER_MAX_QUEUE_DEPTH
#define MESSAGE_BUS_LISTENER_MAX_QUEUE_DEPTH    10
#endif

//
// Define MESSAGE_BUS concurrency behaviour. 
// Set to MESSAGE_BUS_CONCURRENT_LISTENERS to fire event handler 
// concurrently when a given event is raised, and process events sequentially as they arrive (default micro:bit semantics). 
// Set to MESSAGE_BUS_CONCURRENT_EVENTS to to fire event handlers sequentially for any given event, while still allowing 
// concurrent processing of events.
//
//
// Permissable values are:
//   0: MESSAGE_BUS_CONCURRENT_LISTENERS
//   1: MESSAGE_BUS_CONCURRENT_EVENTS
//
#ifndef MESSAGE_BUS_CONCURRENCY_MODE
#define MESSAGE_BUS_CONCURRENCY_MODE            MESSAGE_BUS_CONCURRENT_LISTENERS
#endif
//
// Core micro:bit services
//

// To reduce memory cost and complexity, the micro:bit allows components to register for
// periodic callback events during interrupt context, which occur every scheduling quantum (FIBER_TICK_PERIOD_MS)
// This defines the maximum size of interrupt callback list.
#ifndef MICROBIT_SYSTEM_COMPONENTS
#define MICROBIT_SYSTEM_COMPONENTS              10
#endif

// To reduce memory cost and complexity, the micro:bit allows components to register for
// periodic callback events when the processor is idle.
// This defines the maximum size of the idle callback list.
#ifndef MICROBIT_IDLE_COMPONENTS
#define MICROBIT_IDLE_COMPONENTS                6
#endif

//
// BLE options
//
// The BLE stack is very memory hungry. Each service can therefore be compiled in or out
// by enabling/disabling the options below.
//
// n.b. The minimum set of services to enable over the air programming of the device will
// still be brought up in pairing mode regardless of the settings below.
//

// Enable/Disable BLE during normal operation.
// Set '1' to enable.
#ifndef MICROBIT_BLE_ENABLED
#define MICROBIT_BLE_ENABLED                    1
#endif

// Enable/Disable BLE pairing mode mode at power up.
// Set '1' to enable.
#ifndef MICROBIT_BLE_PAIRING_MODE
#define MICROBIT_BLE_PAIRING_MODE               1
#endif

// Enable/Disable the use of private resolvable addresses.
// Set '1' to enable.
// n.b. This is known to be a feature that suffers compatibility issues with many BLE central devices.
#ifndef MICROBIT_BLE_PRIVATE_ADDRESSES
#define MICROBIT_BLE_PRIVATE_ADDRESSES          0
#endif

// Convenience option to enable / disable BLE security entirely
// Open BLE links are not secure, but commonly used during the development of BLE services
// Set '1' to disable all secuity
#ifndef MICROBIT_BLE_OPEN
#define MICROBIT_BLE_OPEN                       0
#endif

// Configure for open BLE operation if so configured
#if (MICROBIT_BLE_OPEN == 1)
#define MICROBIT_BLE_SECURITY_LEVEL             SECURITY_MODE_ENCRYPTION_OPEN_LINK
#define MICROBIT_BLE_WHITELIST                  0
#define MICROBIT_BLE_ADVERTISING_TIMEOUT        0
#define MICROBIT_BLE_DEFAULT_TX_POWER           6
#endif


// Define the default, global BLE security requirements for MicroBit BLE services
// May be one of the following options (see mbed's SecurityManager class implementaiton detail)
// SECURITY_MODE_ENCRYPTION_OPEN_LINK:      No bonding, encryption, or whitelisting required.
// SECURITY_MODE_ENCRYPTION_NO_MITM:        Bonding, encyption and whitelisting but no passkey.
// SECURITY_MODE_ENCRYPTION_WITH_MITM:      Bonding, encrytion and whitelisting with passkey authentication.
//
#ifndef MICROBIT_BLE_SECURITY_LEVEL
#define MICROBIT_BLE_SECURITY_LEVEL             SECURITY_MODE_ENCRYPTION_WITH_MITM
#endif

// Enable/Disable the use of BLE whitelisting.
// If enabled, the micro:bit will only respond to connection requests from
// known, bonded devices.
#ifndef MICROBIT_BLE_WHITELIST
#define MICROBIT_BLE_WHITELIST                  1
#endif

// Define the period of time for which the BLE stack will advertise (seconds)
// Afer this period, advertising will cease. Set to '0' for no timeout (always advertise).
#ifndef MICROBIT_BLE_ADVERTISING_TIMEOUT
#define MICROBIT_BLE_ADVERTISING_TIMEOUT        0
#endif

// Define the default BLE advertising interval in ms
#ifndef MICROBIT_BLE_ADVERTISING_INTERVAL
#define MICROBIT_BLE_ADVERTISING_INTERVAL        50
#endif

// Defines default power level of the BLE radio transmitter.
// Valid values are in the range 0..7 inclusive, with 0 being the lowest power and 7 the highest power.
// Based on trials undertaken by the BBC, the radio is normally set to its lowest power level
// to best protect children's privacy.
#ifndef MICROBIT_BLE_DEFAULT_TX_POWER
#define MICROBIT_BLE_DEFAULT_TX_POWER           0
#endif

// Enable/Disable BLE Service: MicroBitDFU
// This allows over the air programming during normal operation.
// Set '1' to enable.
#ifndef MICROBIT_BLE_DFU_SERVICE
#define MICROBIT_BLE_DFU_SERVICE                1
#endif

// Enable/Disable availability of Eddystone URL APIs
// Set '1' to enable.
#ifndef MICROBIT_BLE_EDDYSTONE_URL
#define MICROBIT_BLE_EDDYSTONE_URL               0
#endif

// Enable/Disable availability of Eddystone UID APIs
// Set '1' to enable.
#ifndef MICROBIT_BLE_EDDYSTONE_UID
#define MICROBIT_BLE_EDDYSTONE_UID               0
#endif

// Enable/Disable BLE Service: MicroBitEventService
// This allows routing of events from the micro:bit message bus over BLE.
// Set '1' to enable.
#ifndef MICROBIT_BLE_EVENT_SERVICE
#define MICROBIT_BLE_EVENT_SERVICE              1
#endif

// Enable/Disable BLE Service: MicroBitDeviceInformationService
// This enables the standard BLE device information service.
// Set '1' to enable.
#ifndef MICROBIT_BLE_DEVICE_INFORMATION_SERVICE
#define MICROBIT_BLE_DEVICE_INFORMATION_SERVICE 1
#endif

// Enable/Disable BLE Service: MicroBitPartialFlashingService
// This enables the flashing part of the partial flashing service.
// Partial flashing is currently only possible for programs built using MakeCode
// and is disabled by default.
#ifndef MICROBIT_BLE_PARTIAL_FLASHING
#define MICROBIT_BLE_PARTIAL_FLASHING           0
#endif

//
// Radio options
//

// Sets the default radio channel
#ifndef MICROBIT_RADIO_DEFAULT_FREQUENCY
#define MICROBIT_RADIO_DEFAULT_FREQUENCY 7
#endif

// Sets the minimum frequency band permissable for the device
#ifndef MICROBIT_RADIO_LOWER_FREQ_BAND
#define MICROBIT_RADIO_LOWER_FREQ_BAND 0
#endif

// Sets the maximum frequency band permissable for the device
#ifndef MICROBIT_RADIO_UPPER_FREQ_BAND
#define MICROBIT_RADIO_UPPER_FREQ_BAND 83
#endif

//
// Accelerometer options
//

// Enable this to read 10 bits of data from the acclerometer.
// Otherwise, 8 bits are used.
// Set '1' to enable.
#ifndef USE_ACCEL_LSB
#define USE_ACCEL_LSB                           0
#endif

//
// Enable a 0..360 degree range on the accelerometer getPitch()
// calculation. Set to '1' to enable.
//
// A value of '0' provides consistency with the (buggy) microbit-dal 2.0
// and earlier versions, which inadvertently provided only an ambiguous
// 0..180 degree range
//
#ifndef MICROBIT_FULL_RANGE_PITCH_CALCULATION
#define MICROBIT_FULL_RANGE_PITCH_CALCULATION   1
#endif

//
// Display options
//

// Selects the matrix configuration for the display driver.
// Known, acceptable options are:
//
#define MICROBUG_REFERENCE_DEVICE               1
#define MICROBIT_3X9                            2
#define MICROBIT_SB1                            3
#define MICROBIT_SB2                            4

#ifndef MICROBIT_DISPLAY_TYPE
#define MICROBIT_DISPLAY_TYPE                   MICROBIT_SB2
#endif

// Selects the minimum permissable brightness level for the device
// in the region of 0 (off) to 255 (full brightness)
#ifndef MICROBIT_DISPLAY_MINIMUM_BRIGHTNESS
#define MICROBIT_DISPLAY_MINIMUM_BRIGHTNESS     1
#endif

// Selects the maximum permissable brightness level for the device
// in the region of 0 (off) to 255 (full brightness)
#ifndef MICROBIT_DISPLAY_MAXIMUM_BRIGHTNESS
#define MICROBIT_DISPLAY_MAXIMUM_BRIGHTNESS     255
#endif

// Selects the default brightness for the display
// in the region of zero (off) to 255 (full brightness)
#ifndef MICROBIT_DISPLAY_DEFAULT_BRIGHTNESS
#define MICROBIT_DISPLAY_DEFAULT_BRIGHTNESS     MICROBIT_DISPLAY_MAXIMUM_BRIGHTNESS
#endif

// Selects the default scroll speed for the display.
// The time taken to move a single pixel (ms).
#ifndef MICROBIT_DEFAULT_SCROLL_SPEED
#define MICROBIT_DEFAULT_SCROLL_SPEED           120
#endif

// Selects the number of pixels a scroll will move in each quantum.
#ifndef MICROBIT_DEFAULT_SCROLL_STRIDE
#define MICROBIT_DEFAULT_SCROLL_STRIDE          -1
#endif

// Selects the time each character will be shown on the display during print operations.
// The time each character is shown on the screen  (ms).
#ifndef MICROBIT_DEFAULT_PRINT_SPEED
#define MICROBIT_DEFAULT_PRINT_SPEED            400
#endif

//Configures the default serial mode used by serial read and send calls.
#ifndef MICROBIT_DEFAULT_SERIAL_MODE
#define MICROBIT_DEFAULT_SERIAL_MODE            SYNC_SLEEP
#endif

//
// File System configuration defaults
//

//
// Defines the logical block size for the file system.
// Must be a factor of the physical PAGE_SIZE (ideally a power of two less).
//
#ifndef MBFS_BLOCK_SIZE
#define MBFS_BLOCK_SIZE		256
#endif

//
// FileSystem writeback cache size, in bytes. Defines how many bytes will be stored
// in RAM before being written back to FLASH. Set to zero to disable this feature.
// Should be <= MBFS_BLOCK_SIZE.
//
#ifndef MBFS_CACHE_SIZE
#define MBFS_CACHE_SIZE	    0   
#endif

//
// I/O Options
//


//
// Define the default mode in which the digital input pins are configured.
// valid options are PullDown, PullUp and PullNone.
//
#ifndef MICROBIT_DEFAULT_PULLMODE
#define MICROBIT_DEFAULT_PULLMODE                PullDown
#endif

//
// Panic options
//

// Enable this to invoke a panic on out of memory conditions.
// Set '1' to enable.
#ifndef MICROBIT_PANIC_HEAP_FULL
#define MICROBIT_PANIC_HEAP_FULL                1
#endif

//
// Debug options
//

// Enable this to route debug messages through the USB serial interface.
// n.b. This also disables the user serial port 'uBit.serial'.
// Set '1' to enable.
#ifndef MICROBIT_DBG
#define MICROBIT_DBG                            0
#endif

// Enable this to receive diagnostic messages from the heap allocator via the USB serial interface.
// n.b. This requires MICROBIT_DBG to be defined.
// Set '1' to enable.
#ifndef MICROBIT_HEAP_DBG
#define MICROBIT_HEAP_DBG                       0
#endif

// Versioning options.
// We use semantic versioning (http://semver.org/) to identify differnet versions of the micro:bit runtime.
// Where possible we use yotta (an ARM mbed build tool) to help us track versions.
// if this isn't available, it can be defined manually as a configuration option.
//
#ifndef MICROBIT_DAL_VERSION
#define MICROBIT_DAL_VERSION                    "unknown"
#endif

// micro:bit Modes
// The micro:bit may be in different states: running a user's application or into BLE pairing mode
// These modes can be representeded using these #defines
#ifndef MICROBIT_MODE_PAIRING
#define MICROBIT_MODE_PAIRING                   0
#endif
#ifndef MICROBIT_MODE_APPLICATION
#define MICROBIT_MODE_APPLICATION               1
#endif

//
// Helper macro used by the micro:bit runtime to determine if a boolean configuration option is set.
//
#define CONFIG_ENABLED(X) (X == 1)
#define CONFIG_DISABLED(X) (X != 1)

#if CONFIG_ENABLED(MICROBIT_HEAP_ALLOCATOR)
#include "MicroBitHeapAllocator.h"
#endif

#if CONFIG_ENABLED(MICROBIT_DBG)
extern RawSerial* SERIAL_DEBUG;
#endif

#endif
