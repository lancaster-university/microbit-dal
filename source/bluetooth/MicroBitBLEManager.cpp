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

#include "MicroBitConfig.h"
#include "MicroBitBLEManager.h"
#include "MicroBitStorage.h"
#include "MicroBitFiber.h"


/* The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ.
 * If we're compiling under GCC, then we suppress any warnings generated from this code (but not the rest of the DAL)
 * The ARM cc compiler is more tolerant. We don't test __GNUC__ here to detect GCC as ARMCC also typically sets this
 * as a compatability option, but does not support the options used...
 */
#if !defined(__arm)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "ble.h"

extern "C"
{
#include "device_manager.h"
uint32_t btle_set_gatt_table_size(uint32_t size);
}

/*
 * Return to our predefined compiler settings.
 */
#if !defined(__arm)
#pragma GCC diagnostic pop
#endif

#define MICROBIT_PAIRING_FADE_SPEED		4

const char* MICROBIT_BLE_MANUFACTURER = NULL;
const char* MICROBIT_BLE_MODEL = "BBC micro:bit";
const char* MICROBIT_BLE_HARDWARE_VERSION = NULL;
const char* MICROBIT_BLE_FIRMWARE_VERSION = MICROBIT_DAL_VERSION;
const char* MICROBIT_BLE_SOFTWARE_VERSION = NULL;
const int8_t MICROBIT_BLE_POWER_LEVEL[] = {-30, -20, -16, -12, -8, -4, 0, 4};

/*
 * Many of the mbed interfaces we need to use only support callbacks to plain C functions, rather than C++ methods.
 * So, we maintain a pointer to the MicroBitBLEManager that's in use. Ths way, we can still access resources on the micro:bit
 * whilst keeping the code modular.
 */
static MicroBitBLEManager *manager = NULL;                      // Singleton reference to the BLE manager. many mbed BLE API callbacks still do not support member funcions yet. :-(
static uint8_t deviceID = 255;                                  // Unique ID for the peer that has connected to us.
static Gap::Handle_t pairingHandle = 0;                         // The connection handle used during a pairing process. Used to ensure that connections are dropped elegantly.

static void storeSystemAttributes(Gap::Handle_t handle)
{
    if(manager->storage != NULL && deviceID < MICROBIT_BLE_MAXIMUM_BONDS)
    {
        ManagedString key("bleSysAttrs");

        KeyValuePair* bleSysAttrs = manager->storage->get(key);

        BLESysAttribute attrib;
        BLESysAttributeStore attribStore;

        uint16_t len = sizeof(attrib.sys_attr);

        sd_ble_gatts_sys_attr_get(handle, attrib.sys_attr, &len, BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS);

        //copy our stored sysAttrs
        if(bleSysAttrs != NULL)
        {
            memcpy(&attribStore, bleSysAttrs->value, sizeof(BLESysAttributeStore));
            delete bleSysAttrs;
        }

        //check if we need to update
        if(memcmp(attribStore.sys_attrs[deviceID].sys_attr, attrib.sys_attr, len) != 0)
        {
            attribStore.sys_attrs[deviceID] = attrib;
            manager->storage->put(key, (uint8_t *)&attribStore, sizeof(attribStore));
        }
    }
}

/**
  * Callback when a BLE GATT disconnect occurs.
  */
static void bleDisconnectionCallback(const Gap::DisconnectionCallbackParams_t *reason)
{
    MicroBitEvent(MICROBIT_ID_BLE,MICROBIT_BLE_EVT_DISCONNECTED);

    storeSystemAttributes(reason->handle);

    if (manager)
	    manager->advertise();
}

/**
  * Callback when a BLE connection is established.
  */
static void bleConnectionCallback(const Gap::ConnectionCallbackParams_t*)
{
    MicroBitEvent(MICROBIT_ID_BLE,MICROBIT_BLE_EVT_CONNECTED);
}

/**
  * Callback when a BLE SYS_ATTR_MISSING.
  */
static void bleSysAttrMissingCallback(const GattSysAttrMissingCallbackParams *params)
{
    int complete = 0;
    deviceID = 255;

    dm_handle_t dm_handle = {0,0,0,0};

    int ret = dm_handle_get(params->connHandle, &dm_handle);

    if (ret == 0)
        deviceID = dm_handle.device_id;

    if(manager->storage != NULL && deviceID < MICROBIT_BLE_MAXIMUM_BONDS)
    {
        ManagedString key("bleSysAttrs");

        KeyValuePair* bleSysAttrs = manager->storage->get(key);

        BLESysAttributeStore attribStore;
        BLESysAttribute attrib;

        if(bleSysAttrs != NULL)
        {
            //restore our sysAttrStore
            memcpy(&attribStore, bleSysAttrs->value, sizeof(BLESysAttributeStore));
            delete bleSysAttrs;

            attrib = attribStore.sys_attrs[deviceID];

            ret = sd_ble_gatts_sys_attr_set(params->connHandle, attrib.sys_attr, sizeof(attrib.sys_attr), BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS);

            complete = 1;

            if(ret == 0)
                ret = sd_ble_gatts_service_changed(params->connHandle, 0x000c, 0xffff);
        }
    }

    if (!complete)
        sd_ble_gatts_sys_attr_set(params->connHandle, NULL, 0, 0);

}

static void passkeyDisplayCallback(Gap::Handle_t handle, const SecurityManager::Passkey_t passkey)
{
    (void) handle; /* -Wunused-param */

	ManagedString passKey((const char *)passkey, SecurityManager::PASSKEY_LEN);

    if (manager)
	    manager->pairingRequested(passKey);
}

static void securitySetupCompletedCallback(Gap::Handle_t handle, SecurityManager::SecurityCompletionStatus_t status)
{
    (void) handle; /* -Wunused-param */

    dm_handle_t dm_handle = {0,0,0,0};
    int ret = dm_handle_get(handle, &dm_handle);

    if (ret == 0)
        deviceID = dm_handle.device_id;

    if (manager)
    {
        pairingHandle = handle;
	    manager->pairingComplete(status == SecurityManager::SEC_STATUS_SUCCESS);
    }
}

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
MicroBitBLEManager::MicroBitBLEManager(MicroBitStorage& _storage) :
    storage(&_storage)
{
    manager = this;
	this->ble = NULL;
	this->pairingStatus = 0;
}

/**
 * Constructor.
 *
 * Configure and manage the micro:bit's Bluetooth Low Energy (BLE) stack.
 *
 * @note The BLE stack *cannot*  be brought up in a static context (the software simply hangs or corrupts itself).
 * Hence, the init() member function should be used to initialise the BLE stack.
 */
MicroBitBLEManager::MicroBitBLEManager() :
    storage(NULL)
{
    manager = this;
	this->ble = NULL;
	this->pairingStatus = 0;
}

/**
 * When called, the micro:bit will begin advertising for a predefined period,
 * MICROBIT_BLE_ADVERTISING_TIMEOUT seconds to bonded devices.
 */
void MicroBitBLEManager::advertise()
{
    if(ble)
        ble->gap().startAdvertising();
}

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
void MicroBitBLEManager::init(ManagedString deviceName, ManagedString serialNumber, EventModel& messageBus, bool enableBonding)
{
	ManagedString BLEName("BBC micro:bit");
	this->deviceName = deviceName;

#if !(CONFIG_ENABLED(MICROBIT_BLE_WHITELIST))
	ManagedString namePrefix(" [");
	ManagedString namePostfix("]");
	BLEName = BLEName + namePrefix + deviceName + namePostfix;
#endif

    // Start the BLE stack.
#if CONFIG_ENABLED(MICROBIT_HEAP_REUSE_SD)
    btle_set_gatt_table_size(MICROBIT_SD_GATT_TABLE_SIZE);
#endif

    ble = new BLEDevice();
    ble->init();

    // automatically restart advertising after a device disconnects.
    ble->gap().onDisconnection(bleDisconnectionCallback);
    ble->gattServer().onSysAttrMissing(bleSysAttrMissingCallback);

    // generate an event when a Bluetooth connection is established
    ble->gap().onConnection(bleConnectionCallback);

    // Configure the stack to hold onto the CPU during critical timing events.
    // mbed-classic performs __disable_irq() calls in its timers that can cause
    // MIC failures on secure BLE channels...
    ble_common_opt_radio_cpu_mutex_t opt;
    opt.enable = 1;
    sd_ble_opt_set(BLE_COMMON_OPT_RADIO_CPU_MUTEX, (const ble_opt_t *)&opt);

#if CONFIG_ENABLED(MICROBIT_BLE_PRIVATE_ADDRESSES)
	// Configure for private addresses, so kids' behaviour can't be easily tracked.
	ble->gap().setAddress(BLEProtocol::AddressType::RANDOM_PRIVATE_RESOLVABLE, {0});
#endif

    // Setup our security requirements.
    ble->securityManager().onPasskeyDisplay(passkeyDisplayCallback);
    ble->securityManager().onSecuritySetupCompleted(securitySetupCompletedCallback);
    ble->securityManager().init(enableBonding, (SecurityManager::MICROBIT_BLE_SECURITY_LEVEL == SecurityManager::SECURITY_MODE_ENCRYPTION_WITH_MITM), SecurityManager::IO_CAPS_DISPLAY_ONLY);

    if (enableBonding)
    {
        // If we're in pairing mode, review the size of the bond table.
        int bonds = getBondCount();

        // TODO: It would be much better to implement some sort of LRU/NFU policy here,
        // but this isn't currently supported in mbed, so we'd need to layer break...

        // If we're full, empty the bond table.
        if (bonds >= MICROBIT_BLE_MAXIMUM_BONDS)
            ble->securityManager().purgeAllBondingState();
    }

#if CONFIG_ENABLED(MICROBIT_BLE_WHITELIST)
    // Configure a whitelist to filter all connection requetss from unbonded devices.
    // Most BLE stacks only permit one connection at a time, so this prevents denial of service attacks.
    BLEProtocol::Address_t bondedAddresses[MICROBIT_BLE_MAXIMUM_BONDS];
    Gap::Whitelist_t whitelist;
    whitelist.addresses = bondedAddresses;
    whitelist.capacity = MICROBIT_BLE_MAXIMUM_BONDS;

    ble->securityManager().getAddressesFromBondTable(whitelist);

    ble->gap().setWhitelist(whitelist);
    ble->gap().setScanningPolicyMode(Gap::SCAN_POLICY_IGNORE_WHITELIST);
    ble->gap().setAdvertisingPolicyMode(Gap::ADV_POLICY_FILTER_CONN_REQS);
#endif

    // Configure the radio at our default power level
    setTransmitPower(MICROBIT_BLE_DEFAULT_TX_POWER);

    // Bring up core BLE services.
#if CONFIG_ENABLED(MICROBIT_BLE_DFU_SERVICE)
    new MicroBitDFUService(*ble);
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_DEVICE_INFORMATION_SERVICE)
    DeviceInformationService ble_device_information_service (*ble, MICROBIT_BLE_MANUFACTURER, MICROBIT_BLE_MODEL, serialNumber.toCharArray(), MICROBIT_BLE_HARDWARE_VERSION, MICROBIT_BLE_FIRMWARE_VERSION, MICROBIT_BLE_SOFTWARE_VERSION);
#else
    (void)serialNumber;
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_EVENT_SERVICE)
    new MicroBitEventService(*ble, messageBus);
#else
    (void)messageBus;
#endif


    // Configure for high speed mode where possible.
    Gap::ConnectionParams_t fast;
    ble->getPreferredConnectionParams(&fast);
    fast.minConnectionInterval = 8; // 10 ms
    fast.maxConnectionInterval = 16; // 20 ms
    fast.slaveLatency = 0;
    ble->setPreferredConnectionParams(&fast);

    // Setup advertising.
#if CONFIG_ENABLED(MICROBIT_BLE_WHITELIST)
    ble->accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
#else
    ble->accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
#endif

    ble->accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)BLEName.toCharArray(), BLEName.length());
    ble->setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble->setAdvertisingInterval(200);

#if (MICROBIT_BLE_ADVERTISING_TIMEOUT > 0)
    ble->gap().setAdvertisingTimeout(MICROBIT_BLE_ADVERTISING_TIMEOUT);
#endif

    // If we have whitelisting enabled, then prevent only enable advertising of we have any binded devices...
    // This is to further protect kids' privacy. If no-one initiates BLE, then the device is unreachable.
    // If whiltelisting is disabled, then we always advertise.
#if CONFIG_ENABLED(MICROBIT_BLE_WHITELIST)
    if (whitelist.size > 0)
#endif
        ble->startAdvertising();
}

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
int MicroBitBLEManager::setTransmitPower(int power)
{
    if (power < 0 || power >= MICROBIT_BLE_POWER_LEVELS)
        return MICROBIT_INVALID_PARAMETER;

    if (ble->gap().setTxPower(MICROBIT_BLE_POWER_LEVEL[power]) != NRF_SUCCESS)
        return MICROBIT_NOT_SUPPORTED;

    return MICROBIT_OK;
}

/**
 * Determines the number of devices currently bonded with this micro:bit.
 * @return The number of active bonds.
 */
int MicroBitBLEManager::getBondCount()
{
    BLEProtocol::Address_t bondedAddresses[MICROBIT_BLE_MAXIMUM_BONDS];
    Gap::Whitelist_t whitelist;
    whitelist.addresses = bondedAddresses;
    whitelist.capacity = MICROBIT_BLE_MAXIMUM_BONDS;
    ble->securityManager().getAddressesFromBondTable(whitelist);

    return whitelist.bonds;
}

/**
 * A request to pair has been received from a BLE device.
 * If we're in pairing mode, display the passkey to the user.
 * Also, purge the bonding table if it has reached capacity.
 *
 * @note for internal use only.
 */
void MicroBitBLEManager::pairingRequested(ManagedString passKey)
{
    // Update our mode to display the passkey.
	this->passKey = passKey;
	this->pairingStatus = MICROBIT_BLE_PAIR_REQUEST;
}

/**
 * A pairing request has been sucessfully completed.
 * If we're in pairing mode, display a success or failure message.
 *
 * @note for internal use only.
 */
void MicroBitBLEManager::pairingComplete(bool success)
{
	this->pairingStatus = MICROBIT_BLE_PAIR_COMPLETE;

	if(success)
    {
		this->pairingStatus |= MICROBIT_BLE_PAIR_SUCCESSFUL;
        fiber_add_idle_component(this);
    }
}

/**
 * Periodic callback in thread context.
 * We use this here purely to safely issue a disconnect operation after a pairing operation is complete.
 */
void MicroBitBLEManager::idleTick()
{
    if (ble)
        ble->disconnect(pairingHandle, Gap::REMOTE_DEV_TERMINATION_DUE_TO_POWER_OFF);

    fiber_remove_idle_component(this);
}

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
void MicroBitBLEManager::pairingMode(MicroBitDisplay& display, MicroBitButton& authorisationButton)
{
	ManagedString namePrefix("BBC micro:bit [");
	ManagedString namePostfix("]");
	ManagedString BLEName = namePrefix + deviceName + namePostfix;

	ManagedString msg("PAIRING MODE!");

	int timeInPairingMode = 0;
	int brightness = 255;
	int fadeDirection = 0;

    ble->gap().stopAdvertising();

    // Clear the whitelist (if we have one), so that we're discoverable by all BLE devices.
#if CONFIG_ENABLED(MICROBIT_BLE_WHITELIST)
    BLEProtocol::Address_t addresses[MICROBIT_BLE_MAXIMUM_BONDS];
    Gap::Whitelist_t whitelist;
    whitelist.addresses = addresses;
    whitelist.capacity = MICROBIT_BLE_MAXIMUM_BONDS;
    whitelist.size = 0;
    ble->gap().setWhitelist(whitelist);
    ble->gap().setAdvertisingPolicyMode(Gap::ADV_POLICY_IGNORE_WHITELIST);
#endif

	// Update the advertised name of this micro:bit to include the device name
    ble->clearAdvertisingPayload();

    ble->accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble->accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)BLEName.toCharArray(), BLEName.length());
    ble->setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble->setAdvertisingInterval(200);

    ble->gap().setAdvertisingTimeout(0);
    ble->gap().startAdvertising();

	// Stop any running animations on the display
	display.stopAnimation();
	display.scroll(msg);

	// Display our name, visualised as a histogram in the display to aid identification.
	showNameHistogram(display);

	while(1)
	{
		if (pairingStatus & MICROBIT_BLE_PAIR_REQUEST)
		{
			timeInPairingMode = 0;
			MicroBitImage arrow("0,0,255,0,0\n0,255,0,0,0\n255,255,255,255,255\n0,255,0,0,0\n0,0,255,0,0\n");
			display.print(arrow,0,0,0);

			if (fadeDirection == 0)
				brightness -= MICROBIT_PAIRING_FADE_SPEED;
			else
				brightness += MICROBIT_PAIRING_FADE_SPEED;

			if (brightness <= 40)
				display.clear();

			if (brightness <= 0)
				fadeDirection = 1;

			if (brightness >= 255)
				fadeDirection = 0;

			if (authorisationButton.isPressed())
			{
				pairingStatus &= ~MICROBIT_BLE_PAIR_REQUEST;
				pairingStatus |= MICROBIT_BLE_PAIR_PASSCODE;
			}
		}

		if (pairingStatus & MICROBIT_BLE_PAIR_PASSCODE)
		{
			timeInPairingMode = 0;
			display.setBrightness(255);
			for (int i=0; i<passKey.length(); i++)
			{
				display.image.print(passKey.charAt(i),0,0);
				fiber_sleep(800);
				display.clear();
				fiber_sleep(200);

				if (pairingStatus & MICROBIT_BLE_PAIR_COMPLETE)
					break;
			}

			fiber_sleep(1000);
		}

		if (pairingStatus & MICROBIT_BLE_PAIR_COMPLETE)
		{
			if (pairingStatus & MICROBIT_BLE_PAIR_SUCCESSFUL)
			{
				MicroBitImage tick("0,0,0,0,0\n0,0,0,0,255\n0,0,0,255,0\n255,0,255,0,0\n0,255,0,0,0\n");
				display.print(tick,0,0,0);
                fiber_sleep(15000);
		        timeInPairingMode = MICROBIT_BLE_PAIRING_TIMEOUT * 30;

                /*
                 * Disabled, as the API to return the number of active bonds is not reliable at present...
                 *
                display.clear();
                ManagedString c(getBondCount());
                ManagedString c2("/");
                ManagedString c3(MICROBIT_BLE_MAXIMUM_BONDS);
                ManagedString c4("USED");

                display.scroll(c+c2+c3+c4);
                *
                *
                */
			}
			else
			{
				MicroBitImage cross("255,0,0,0,255\n0,255,0,255,0\n0,0,255,0,0\n0,255,0,255,0\n255,0,0,0,255\n");
				display.print(cross,0,0,0);
			}
		}

		fiber_sleep(100);
		timeInPairingMode++;

		if (timeInPairingMode >= MICROBIT_BLE_PAIRING_TIMEOUT * 30)
			microbit_reset();
	}
}

/**
 * Displays the device's ID code as a histogram on the provided MicroBitDisplay instance.
 *
 * @param display The display instance used for displaying the histogram.
 */
void MicroBitBLEManager::showNameHistogram(MicroBitDisplay &display)
{
    uint32_t n = NRF_FICR->DEVICEID[1];
    int ld = 1;
    int d = MICROBIT_DFU_HISTOGRAM_HEIGHT;
    int h;

    display.clear();
    for (int i=0; i<MICROBIT_DFU_HISTOGRAM_WIDTH;i++)
    {
        h = (n % d) / ld;

        n -= h;
        d *= MICROBIT_DFU_HISTOGRAM_HEIGHT;
        ld *= MICROBIT_DFU_HISTOGRAM_HEIGHT;

        for (int j=0; j<h+1; j++)
            display.image.setPixelValue(MICROBIT_DFU_HISTOGRAM_WIDTH-i-1, MICROBIT_DFU_HISTOGRAM_HEIGHT-j-1, 255);
    }
}
