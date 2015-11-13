#include "MicroBit.h"

#define MICROBIT_BLE_ENABLE_BONDING 	true
#define MICROBIT_BLE_REQUIRE_MITM	true

#if CONFIG_ENABLED(MICROBIT_BLE_ENABLED) && CONFIG_ENABLED(MICROBIT_BLE_DEVICE_INFORMATION_SERVICE)
const char* MICROBIT_BLE_MANUFACTURER = "The Cast of W1A";
const char* MICROBIT_BLE_MODEL = "BBC micro:bit";
const char* MICROBIT_BLE_HARDWARE_VERSION = "1.0";
const char* MICROBIT_BLE_FIRMWARE_VERSION = MICROBIT_DAL_VERSION;
const char* MICROBIT_BLE_SOFTWARE_VERSION = NULL;

/*
 * Many of the mbed interfaces we need to use only support callbacks to plain C functions, rather than C++ methods.
 * So, we maintain a pointer to the MicroBitBLEManager that's in use. Ths way, we can still access resources on the micro:bit 
 * whilst keeping the code modular.
 */
static MicroBitBLEManager *manager = NULL;

/**
  * Callback when a BLE GATT disconnect occurs.
  */
static void bleDisconnectionCallback(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    (void) handle; /* -Wunused-param */
    (void) reason; /* -Wunused-param */

    if (manager)
	    manager->bleDisconnectionCallback();

}

static void passkeyDisplayCallback(Gap::Handle_t handle, const SecurityManager::Passkey_t passkey)
{
    (void) handle; /* -Wunused-param */

    printf("Input passKey: ");
    for (unsigned i = 0; i < Gap::ADDR_LEN; i++) {
        printf("%c", passkey[i]);
    }
    printf("\r\n");
}

static void securitySetupCompletedCallback(Gap::Handle_t handle, SecurityManager::SecurityCompletionStatus_t status)
{
    (void) handle; /* -Wunused-param */

    if (status == SecurityManager::SEC_STATUS_SUCCESS) {
        printf("Security success %d\r\n", status);
    } else {
        printf("Security failed %d\r\n", status);
    }
}

static void securitySetupInitiatedCallback(Gap::Handle_t handle, bool allowBonding, bool requireMITM, SecurityManager::SecurityIOCapabilities_t iocaps)
{
    (void) handle; 		/* -Wunused-param */
    (void) allowBonding; 	/* -Wunused-param */
    (void) requireMITM; 	/* -Wunused-param */
    (void) iocaps; 		/* -Wunused-param */

    printf("Security setup initiated\r\n");
}

void initializeSecurity(BLE &ble)
{
    bool enableBonding = true;
    bool requireMITM = HID_SECURITY_REQUIRE_MITM;

    ble.securityManager().onSecuritySetupInitiated(securitySetupInitiatedCallback);
    ble.securityManager().onPasskeyDisplay(passkeyDisplayCallback);
    ble.securityManager().onSecuritySetupCompleted(securitySetupCompletedCallback);

    ble.securityManager().init(enableBonding, requireMITM, HID_SECURITY_IOCAPS);
}


void MicroBitBLEManager::bleDisconnectionCallback()
{
    ble.startAdvertising(); 
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
void MicroBitBLEManager::init()
{   
    // Start the BLE stack.        
    ble = new BLEDevice();
    ble->init();

    // automatically restart advertising after a device disconnects.
    ble->onDisconnection(bleDisconnectionCallback);

    // Setup our security requirements.
    ble->securityManager().onSecuritySetupInitiated(securitySetupInitiatedCallback);
    ble->securityManager().onPasskeyDisplay(passkeyDisplayCallback);
    ble->securityManager().onSecuritySetupCompleted(securitySetupCompletedCallback);

    ble.securityManager().init(MICROBIT_BLE_ENABLE_BONDING, MICROBIT_BLE_REQUIRE_MITM, HID_SECURITY_IOCAPS);

    // Bring up any configured auxiliary services.
#if CONFIG_ENABLED(MICROBIT_BLE_DFU_SERVICE)
    ble_firmware_update_service = new MicroBitDFUService(*ble);
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_DEVICE_INFORMATION_SERVICE)
    DeviceInformationService ble_device_information_service (*ble, MICROBIT_BLE_MANUFACTURER, MICROBIT_BLE_MODEL, getSerial().toCharArray(), MICROBIT_BLE_HARDWARE_VERSION, MICROBIT_BLE_FIRMWARE_VERSION, MICROBIT_BLE_SOFTWARE_VERSION);
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
    ble->accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble->accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)MICROBIT_BLE_DEVICE_NAME, sizeof(MICROBIT_BLE_DEVICE_NAME));
    ble->setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble->setAdvertisingInterval(Gap::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(200));
    ble->startAdvertising();  
#endif    

}

