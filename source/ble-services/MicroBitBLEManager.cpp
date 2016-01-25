#include "MicroBit.h"


/* The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ
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

/*
 * Return to our predefined compiler settings.
 */
#if !defined(__arm)
#pragma GCC diagnostic pop
#endif

#define MICROBIT_BLE_ENABLE_BONDING 	true
#define MICROBIT_BLE_REQUIRE_MITM		true

#define MICROBIT_PAIRING_FADE_SPEED		4
#define MICROBIT_BLE_POWER_LEVELS       8
#define MICROBIT_BLE_MAXIMUM_BONDS      4


const char* MICROBIT_BLE_MANUFACTURER = "The Cast of W1A";
const char* MICROBIT_BLE_MODEL = "BBC micro:bit";
const char* MICROBIT_BLE_HARDWARE_VERSION = "1.0";
const char* MICROBIT_BLE_FIRMWARE_VERSION = MICROBIT_DAL_VERSION;
const char* MICROBIT_BLE_SOFTWARE_VERSION = NULL;
const int8_t MICROBIT_BLE_POWER_LEVEL[] = {-30, -20, -16, -12, -8, -4, 0, 4};

/*
 * Many of the mbed interfaces we need to use only support callbacks to plain C functions, rather than C++ methods.
 * So, we maintain a pointer to the MicroBitBLEManager that's in use. Ths way, we can still access resources on the micro:bit
 * whilst keeping the code modular.
 */
static MicroBitBLEManager *manager = NULL;

/**
  * Callback when a BLE GATT disconnect occurs.
  */
static void bleDisconnectionCallback(const Gap::DisconnectionCallbackParams_t *reason)
{
    (void) reason; /* -Wunused-param */

    if (manager)
	    manager->advertise();
}

/**
  * Callback when a BLE GATT connect occurs.
  */
static void bleConnectionCallback(const Gap::ConnectionCallbackParams_t *reason)
{

    // Ensure that there's no stale, cached information in the client... invalidate all characteristics.
    uint16_t len = 8;

    // Configure the ServiceChanged characteristic to receive service changed indications
    // TODO: This is really a workaround as we can't maintain persistent state on the micro:bit across USB
    // reprogramming flashes.... yet.
    uint8_t data[] = {0x0B,0x00,0x02,0x00,0x02,0x00,0xB8,0x46};

    sd_ble_gatts_sys_attr_set(reason->handle, data, len, BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS); 
    sd_ble_gatts_service_changed(reason->handle, 0x000c, 0xffff); 
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

    if (manager)
	    manager->pairingComplete(status == SecurityManager::SEC_STATUS_SUCCESS);
}

/**
  * Constructor.
  *
  * Configure and manage the micro:bit's Bluetooth Low Energy (BLE) stack.
  * Note that the BLE stack *cannot*  be brought up in a static context.
  * (the software simply hangs or corrupts itself).
  * Hence, we bring it up in an explicit init() method, rather than in the constructor.
  */
MicroBitBLEManager::MicroBitBLEManager()
{
	manager = this;
	this->ble = NULL;
	this->pairingStatus = 0;
}

/**
  * Makes the micro:bit discoverable via BLE, such that bonded devices can connect
  * When called, the micro:bit will begin advertising for a predefined period, thereby allowing
  * bonded devices to connect.
  */
void MicroBitBLEManager::advertise()
{
    if(ble)
        ble->gap().startAdvertising();
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
void MicroBitBLEManager::init(ManagedString deviceName, ManagedString serialNumber, bool enableBonding)
{
	ManagedString BLEName("BBC micro:bit");

	this->deviceName = deviceName;

    // Start the BLE stack.
    ble = new BLEDevice();
    ble->init();

    // automatically restart advertising after a device disconnects.
    ble->onDisconnection(bleDisconnectionCallback);
    ble->onConnection(bleConnectionCallback);

    // configure the stack to hold on to CPU during critical timing events.
    // mbed-classic performs __disabe_irq calls in its timers, which can cause MIC failures
    // on secure BLE channels.
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
    ble->securityManager().init(enableBonding, MICROBIT_BLE_REQUIRE_MITM, SecurityManager::IO_CAPS_DISPLAY_ONLY);

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

    // Bring up any configured auxiliary services.
#if CONFIG_ENABLED(MICROBIT_BLE_DFU_SERVICE)
    new MicroBitDFUService(*ble);
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_DEVICE_INFORMATION_SERVICE)
    DeviceInformationService ble_device_information_service (*ble, MICROBIT_BLE_MANUFACTURER, MICROBIT_BLE_MODEL, serialNumber.toCharArray(), MICROBIT_BLE_HARDWARE_VERSION, MICROBIT_BLE_FIRMWARE_VERSION, MICROBIT_BLE_SOFTWARE_VERSION);
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_EVENT_SERVICE)
    new MicroBitEventService(*ble);
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_LED_SERVICE)
    new MicroBitLEDService(*ble);
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_ACCELEROMETER_SERVICE)
    new MicroBitAccelerometerService(*ble);
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_MAGNETOMETER_SERVICE)
    new MicroBitMagnetometerService(*ble);
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_BUTTON_SERVICE)
    new MicroBitButtonService(*ble);
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_IO_PIN_SERVICE)
    new MicroBitIOPinService(*ble);
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_TEMPERATURE_SERVICE)
    new MicroBitTemperatureService(*ble);
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
 * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the value is out of range.
 *
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
 * Determines the number of devices currently bonded with this micro:bit
 * @return The number of active bonds.
 */
int MicroBitBLEManager::getBondCount()
{
    BLEProtocol::Address_t bondedAddresses[MICROBIT_BLE_MAXIMUM_BONDS];
    Gap::Whitelist_t whitelist;
    whitelist.addresses = bondedAddresses;
    whitelist.capacity = MICROBIT_BLE_MAXIMUM_BONDS;
    ble->securityManager().getAddressesFromBondTable(whitelist);

    return whitelist.size;
}

/**
 * A request to pair has been received from a BLE device.
 * If we're in pairing mode, display the passkey to the user.
 * Also, purge the binding table if it has reached capacity.
 */
void MicroBitBLEManager::pairingRequested(ManagedString passKey)
{
    // Firstly, determine if there is free space in the bonding table.
    // If not, clear it out to make room.
    
    // TODO: It would be much better to implement some sort of LRU/NFU policy here,
    // but this isn't currently supported in mbed, so we'd need to layer break...

    // If we're full, empty the bond table.
    if (getBondCount() >= MICROBIT_BLE_MAXIMUM_BONDS)
        ble->securityManager().purgeAllBondingState();
    
    // Update our mode to display the passkey. 
	this->passKey = passKey;
	this->pairingStatus = MICROBIT_BLE_PAIR_REQUEST;
}

/**
 * A pairing request has been sucesfully completed.
 * If we're in pairing mode, display feedback to the user.
 */
void MicroBitBLEManager::pairingComplete(bool success)
{
	this->pairingStatus = MICROBIT_BLE_PAIR_COMPLETE;

	if(success)
		this->pairingStatus |= MICROBIT_BLE_PAIR_SUCCESSFUL;
}

/**
 * Enter pairing mode. This is mode is called to initiate pairing, and to enable FOTA programming
 * of the micro:bit in cases where BLE is disabled during normal operation.
 */
void MicroBitBLEManager::pairingMode(MicroBitDisplay &display)
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

			if (uBit.buttonA.isPressed())
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
				uBit.sleep(800);
				display.clear();
				uBit.sleep(200);

				if (pairingStatus & MICROBIT_BLE_PAIR_COMPLETE)
					break;
			}

			uBit.sleep(1000);
		}

		if (pairingStatus & MICROBIT_BLE_PAIR_COMPLETE)
		{
			if (pairingStatus & MICROBIT_BLE_PAIR_SUCCESSFUL)
			{
				MicroBitImage tick("0,0,0,0,0\n0,0,0,0,255\n0,0,0,255,0\n255,0,255,0,0\n0,255,0,0,0\n");
				display.print(tick,0,0,0);
                uBit.sleep(5000);

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

		uBit.sleep(30);
		timeInPairingMode++;

		if (timeInPairingMode >= MICROBIT_BLE_PAIRING_TIMEOUT * 30)
			microbit_reset();
	}
}

/**
  * Displays the device's ID code as a histogram on the LED matrix display.
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
