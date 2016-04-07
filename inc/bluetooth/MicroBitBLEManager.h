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

#ifndef MICROBIT_BLE_MANAGER_H
#define MICROBIT_BLE_MANAGER_H

#include "mbed.h"
#include "MicroBitConfig.h"

/*
 * The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ
 * If we're compiling under GCC, then we suppress any warnings generated from this code (but not the rest of the DAL)
 * The ARM cc compiler is more tolerant. We don't test __GNUC__ here to detect GCC as ARMCC also typically sets this
 * as a compatability option, but does not support the options used...
 */
#if !defined (__arm)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "ble/BLE.h"

/*
 * Return to our predefined compiler settings.
 */
#if !defined (__arm)
#pragma GCC diagnostic pop
#endif

#include "ble/services/DeviceInformationService.h"
#include "MicroBitDFUService.h"
#include "MicroBitEventService.h"
#include "MicroBitLEDService.h"
#include "MicroBitAccelerometerService.h"
#include "MicroBitMagnetometerService.h"
#include "MicroBitButtonService.h"
#include "MicroBitIOPinService.h"
#include "MicroBitTemperatureService.h"
#include "ExternalEvents.h"
#include "MicroBitButton.h"
#include "MicroBitStorage.h"

#define MICROBIT_BLE_PAIR_REQUEST               0x01
#define MICROBIT_BLE_PAIR_COMPLETE              0x02
#define MICROBIT_BLE_PAIR_PASSCODE              0x04
#define MICROBIT_BLE_PAIR_SUCCESSFUL            0x08

#define MICROBIT_BLE_PAIRING_TIMEOUT	        90
#define MICROBIT_BLE_POWER_LEVELS               8
#define MICROBIT_BLE_MAXIMUM_BONDS              4
#define MICROBIT_BLE_ENABLE_BONDING 	        true

extern const int8_t MICROBIT_BLE_POWER_LEVEL[];

struct BLESysAttribute
{
    uint8_t         sys_attr[8];
};

struct BLESysAttributeStore
{
    BLESysAttribute sys_attrs[MICROBIT_BLE_MAXIMUM_BONDS];
};

/**
  * Class definition for the MicroBitBLEManager.
  *
  */
class MicroBitBLEManager : MicroBitComponent
{
    public:

	// The mbed abstraction of the BlueTooth Low Energy (BLE) hardware
    BLEDevice                       *ble;

    //an instance of MicroBitStorage used to store sysAttrs from softdevice
    MicroBitStorage* storage;

    /**
     * Constructor.
     *
     * Configure and manage the micro:bit's Bluetooth Low Energy (BLE) stack.
     *
     * @param _storage an instance of MicroBitStorage used to persist sys attribute information. (This is required for compatability with iOS).
     *
     * @note The BLE stack *cannot*  be brought up in a static context (the software simply hangs or corrupts itself).
     * Hence, the init() member function should be used to initialise the BLE stack.
     */
    MicroBitBLEManager(MicroBitStorage& _storage);

    /**
     * Constructor.
     *
     * Configure and manage the micro:bit's Bluetooth Low Energy (BLE) stack.
     *
     * @note The BLE stack *cannot*  be brought up in a static context (the software simply hangs or corrupts itself).
     * Hence, the init() member function should be used to initialise the BLE stack.
     */
    MicroBitBLEManager();

    /**
      * Post constructor initialisation method as the BLE stack cannot be brought
      * up in a static context.
      *
      * @param deviceName The name used when advertising
      * @param serialNumber The serial number exposed by the device information service
      * @param messageBus An instance of an EventModel, used during pairing.
      * @param enableBonding If true, the security manager enabled bonding.
      *
      * @code
      * bleManager.init(uBit.getName(), uBit.getSerial(), uBit.messageBus, true);
      * @endcode
      */
    void init(ManagedString deviceName, ManagedString serialNumber, EventModel& messageBus, bool enableBonding);

    /**
     * Change the output power level of the transmitter to the given value.
     *
     * @param power a value in the range 0..7, where 0 is the lowest power and 7 is the highest.
     *
     * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the value is out of range.
     *
     * @code
     * // maximum transmission power.
     * bleManager.setTransmitPower(7);
     * @endcode
     */
    int setTransmitPower(int power);

    /**
     * Enter pairing mode. This is mode is called to initiate pairing, and to enable FOTA programming
     * of the micro:bit in cases where BLE is disabled during normal operation.
     *
     * @param display An instance of MicroBitDisplay used when displaying pairing information.
     * @param authorizationButton The button to use to authorise a pairing request.
     *
     * @code
     * // initiate pairing mode
     * bleManager.pairingMode(uBit.display, uBit.buttonA);
     * @endcode
     */
    void pairingMode(MicroBitDisplay &display, MicroBitButton &authorisationButton);

    /**
     * When called, the micro:bit will begin advertising for a predefined period,
     * MICROBIT_BLE_ADVERTISING_TIMEOUT seconds to bonded devices.
     */
    void advertise();

    /**
     * Determines the number of devices currently bonded with this micro:bit.
     * @return The number of active bonds.
     */
    int getBondCount();

	/**
	 * A request to pair has been received from a BLE device.
     * If we're in pairing mode, display the passkey to the user.
     * Also, purge the bonding table if it has reached capacity.
     *
     * @note for internal use only.
	 */
	void pairingRequested(ManagedString passKey);

	/**
	 * A pairing request has been sucessfully completed.
	 * If we're in pairing mode, display a success or failure message.
     *
     * @note for internal use only.
	 */
	void pairingComplete(bool success);

	/**
     * Periodic callback in thread context.
     * We use this here purely to safely issue a disconnect operation after a pairing operation is complete.
	 */
	void idleTick();

    private:

	/**
	 * Displays the device's ID code as a histogram on the provided MicroBitDisplay instance.
     *
     * @param display The display instance used for displaying the histogram.
	 */
	void showNameHistogram(MicroBitDisplay &display);

	int				pairingStatus;
	ManagedString	passKey;
	ManagedString	deviceName;

};

#endif
