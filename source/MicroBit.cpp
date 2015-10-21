#include "MicroBit.h"

char MICROBIT_BLE_DEVICE_NAME[] = "BBC micro:bit [xxxxx]";

#if CONFIG_ENABLED(MICROBIT_BLE_ENABLED) && CONFIG_ENABLED(MICROBIT_BLE_DEVICE_INFORMATION_SERVICE)
const char* MICROBIT_BLE_MANUFACTURER = "The Cast of W1A";
const char* MICROBIT_BLE_MODEL = "BBC micro:bit";
const char* MICROBIT_BLE_HARDWARE_VERSION = "1.0";
const char* MICROBIT_BLE_FIRMWARE_VERSION = MICROBIT_DAL_VERSION;
const char* MICROBIT_BLE_SOFTWARE_VERSION = NULL;
#endif

/**
  * custom function for panic for malloc & new due to scoping issue.
  */
void panic(int statusCode)
{
    uBit.panic(statusCode);   
}

/**
  * Perform a hard reset of the micro:bit.
  */
void
microbit_reset()
{
    NVIC_SystemReset();
}


/**
  * Callback when a BLE GATT disconnect occurs.
  */
void bleDisconnectionCallback(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    uBit.ble->startAdvertising(); 
}

/**
  * Constructor. 
  * Create a representation of a MicroBit device as a global singleton.
  * @param messageBus callback function to receive MicroBitMessageBus events.
  *
  * Exposed objects:
  * @code 
  * uBit.systemTicker; //the Ticker callback that performs routines like updating the display.
  * uBit.MessageBus; //The message bus where events are fired.
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
    flags(0x00),
    i2c(MICROBIT_PIN_SDA, MICROBIT_PIN_SCL),
#if CONFIG_DISABLED(MICROBIT_DBG)
    serial(USBTX, USBRX),
#endif    
    MessageBus(),
    display(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_WIDTH, MICROBIT_DISPLAY_HEIGHT),
    buttonA(MICROBIT_ID_BUTTON_A,MICROBIT_PIN_BUTTON_A),
    buttonB(MICROBIT_ID_BUTTON_B,MICROBIT_PIN_BUTTON_B),
    buttonAB(MICROBIT_ID_BUTTON_AB,MICROBIT_ID_BUTTON_A,MICROBIT_ID_BUTTON_B), 
    accelerometer(MICROBIT_ID_ACCELEROMETER, MMA8653_DEFAULT_ADDR),
    compass(MICROBIT_ID_COMPASS, MAG3110_DEFAULT_ADDR),
    thermometer(MICROBIT_ID_THERMOMETER),
    io(MICROBIT_ID_IO_P0,MICROBIT_ID_IO_P1,MICROBIT_ID_IO_P2,
       MICROBIT_ID_IO_P3,MICROBIT_ID_IO_P4,MICROBIT_ID_IO_P5,
       MICROBIT_ID_IO_P6,MICROBIT_ID_IO_P7,MICROBIT_ID_IO_P8,
       MICROBIT_ID_IO_P9,MICROBIT_ID_IO_P10,MICROBIT_ID_IO_P11,
       MICROBIT_ID_IO_P12,MICROBIT_ID_IO_P13,MICROBIT_ID_IO_P14,
       MICROBIT_ID_IO_P15,MICROBIT_ID_IO_P16,MICROBIT_ID_IO_P19,
       MICROBIT_ID_IO_P20)
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
void MicroBit::init()
{   
    //add the display to the systemComponent array
    addSystemComponent(&uBit.display);
    
    //add the compass and accelerometer to the idle array
    addIdleComponent(&uBit.accelerometer);
    addIdleComponent(&uBit.compass);
    addIdleComponent(&uBit.MessageBus);

    // Seed our random number generator
    seedRandom();

    // Generate the name for our device.
    this->deriveName();

#if CONFIG_ENABLED(MICROBIT_BLE_ENABLED)
    // Start the BLE stack.        
    ble = new BLEDevice();
    ble->init();
    ble->onDisconnection(bleDisconnectionCallback);

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

    // Start refreshing the Matrix Display
    systemTicker.attach(this, &MicroBit::systemTick, MICROBIT_DISPLAY_REFRESH_PERIOD);     
}

/**
  * Derives the friendly name for this device, autogenerated from our hardware Device ID.
  */
void MicroBit::deriveName()
{
    const uint8_t codebook[MICROBIT_NAME_LENGTH][MICROBIT_NAME_CODE_LETTERS] = 
    {
        {'z', 'v', 'g', 'p', 't'},  
        {'u', 'o', 'i', 'e', 'a'},
        {'z', 'v', 'g', 'p', 't'},  
        {'u', 'o', 'i', 'e', 'a'},
        {'z', 'v', 'g', 'p', 't'}
    };

    char *name = MICROBIT_BLE_DEVICE_NAME+15;

    // We count right to left, so fast forward the pointer.
    name += MICROBIT_NAME_LENGTH;

    uint32_t n = NRF_FICR->DEVICEID[1];
    
    int ld = 1;
    int d = MICROBIT_NAME_CODE_LETTERS;
    int h;

    for (int i=0; i<MICROBIT_NAME_LENGTH;i++)
    {
        h = (n % d) / ld;
        n -= h;
        d *= MICROBIT_NAME_CODE_LETTERS;
        ld *= MICROBIT_NAME_CODE_LETTERS;
        *--name = codebook[i][h];
    }
}

/**
  * Return the friendly name for this device.
  *
  * @return A string representing the friendly name of this device.
  */
ManagedString MicroBit::getName()
{
    return ManagedString(MICROBIT_BLE_DEVICE_NAME+15, MICROBIT_NAME_LENGTH);
}

/**
 * Return the serial number of this device.
 *
 * @return A string representing the serial number of this device.
 */
ManagedString MicroBit::getSerial()
{
    // We take to 16 bit numbers here, as we want the full range of ID bits, but don't want negative numbers...
    int n1 = NRF_FICR->DEVICEID[1] & 0xffff;
    int n2 = (NRF_FICR->DEVICEID[1] >> 16) & 0xffff;

    // Simply concat the two numbers. 
    ManagedString s1 = ManagedString(n1);
    ManagedString s2 = ManagedString(n2);

    return s1+s2;
}

/**
  * Will reset the micro:bit when called.
  *
  * Example:
  * @code 
  * uBit.reset();
  * @endcode
  */
void MicroBit::reset()
{
  microbit_reset();
}

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
  *
  * Example:
  * @code 
  * uBit.sleep(20); //sleep for 20ms
  * @endcode
  */
int MicroBit::sleep(int milliseconds)
{
    //sanity check, we can't time travel... (yet?)
    if(milliseconds < 0)
        return MICROBIT_INVALID_PARAMETER;
        
    if (flags & MICROBIT_FLAG_SCHEDULER_RUNNING)
        fiber_sleep(milliseconds);
    else
        wait_ms(milliseconds);
}


/**
  * Generate a random number in the given range.
  * We use a simple Galois LFSR random number generator here,
  * as a Galois LFSR is sufficient for our applications, and much more lightweight
  * than the hardware random number generator built int the processor, which takes
  * a long time and uses a lot of energy.
  *
  * KIDS: You shouldn't use this is the real world to generte cryptographic keys though... 
  * have a think why not. :-)
  *
  * @param max the upper range to generate a number for. This number cannot be negative
  * @return A random, natural number between 0 and the max-1. Or MICROBIT_INVALID_VALUE (defined in ErrorNo.h) if max is <= 0.
  *
  * Example:
  * @code 
  * uBit.random(200); //a number between 0 and 199
  * @endcode
  */
int MicroBit::random(int max)
{
    //return MICROBIT_INVALID_VALUE if max is <= 0...
    if(max <= 0)
        return MICROBIT_INVALID_PARAMETER;
    
    // Cycle the LFSR (Linear Feedback Shift Register).
    // We use an optimal sequence with a period of 2^32-1, as defined by Bruce Schneider here (a true legend in the field!), 
    // For those interested, it's documented in his paper:
    // "Pseudo-Random Sequence Generator for 32-Bit CPUs: A fast, machine-independent generator for 32-bit Microprocessors"
    
    randomValue = ((((randomValue >> 31) ^ (randomValue >> 6) ^ (randomValue >> 4) ^ (randomValue >> 2) ^ (randomValue >> 1) ^ randomValue) & 0x0000001) << 31 ) | (randomValue >> 1);   
    return randomValue % max;
}


/**
  * Seed our a random number generator (RNG).
  * We use the NRF51822 in built cryptographic random number generator to seed a Galois LFSR.
  * We do this as the hardware RNG is relatively high power, and use the the BLE stack internally,
  * with a less than optimal application interface. A Galois LFSR is sufficient for our
  * applications, and much more lightweight.
  */
void MicroBit::seedRandom()
{
    randomValue = 0;
        
    // Start the Random number generator. No need to leave it running... I hope. :-)
    NRF_RNG->TASKS_START = 1;
    
    for(int i = 0; i < 4 ;i++)
    {
        // Clear the VALRDY EVENT
        NRF_RNG->EVENTS_VALRDY = 0;
        
        // Wait for a number ot be generated.
        while ( NRF_RNG->EVENTS_VALRDY == 0);
        
        randomValue = (randomValue << 8) | ((int) NRF_RNG->VALUE);
    }
    
    // Disable the generator to save power.
    NRF_RNG->TASKS_STOP = 1;
}


/**
  * Periodic callback. Used by MicroBitDisplay, FiberScheduler and buttons.
  */
void MicroBit::systemTick()
{   
    // Scheduler callback. We do this here just as a single timer is more efficient. :-)
    if (uBit.flags & MICROBIT_FLAG_SCHEDULER_RUNNING)
        scheduler_tick();  
    
    //work out if any idle components need processing, if so prioritise the idle thread
    for(int i = 0; i < MICROBIT_IDLE_COMPONENTS; i++)
        if(idleThreadComponents[i] != NULL && idleThreadComponents[i]->isIdleCallbackNeeded())
        {
            fiber_flags |= MICROBIT_FLAG_DATA_READY;
            break;
        }
        
    //update any components in the systemComponents array
    for(int i = 0; i < MICROBIT_SYSTEM_COMPONENTS; i++)
        if(systemTickComponents[i] != NULL)
            systemTickComponents[i]->systemTick();
}

/**
  * System tasks to be executed by the idle thread when the Micro:Bit isn't busy or when data needs to be read.
  */
void MicroBit::systemTasks()
{   
    //call the idleTick member function indiscriminately 
    for(int i = 0; i < MICROBIT_IDLE_COMPONENTS; i++)
        if(idleThreadComponents[i] != NULL)
            idleThreadComponents[i]->idleTick();
    
    fiber_flags &= ~MICROBIT_FLAG_DATA_READY;
}

/**
  * add a component to the array of components which invocate the systemTick member function during a systemTick 
  * @note this will be converted into a dynamic list of components
  */
void MicroBit::addSystemComponent(MicroBitComponent *component)
{
    int i = 0;
    
    while(systemTickComponents[i] != NULL && i < MICROBIT_SYSTEM_COMPONENTS)  
        i++;
    
    if(i == MICROBIT_SYSTEM_COMPONENTS)
        return;
        
    systemTickComponents[i] = component;    
}

/**
  * remove a component from the array of components
  * @note this will be converted into a dynamic list of components
  */
void MicroBit::removeSystemComponent(MicroBitComponent *component)
{
    int i = 0;
    
    while(systemTickComponents[i] != component  && i < MICROBIT_SYSTEM_COMPONENTS)  
        i++;
    
    if(i == MICROBIT_SYSTEM_COMPONENTS)
        return;

    systemTickComponents[i] = NULL;
}

/**
  * add a component to the array of components which invocate the systemTick member function during a systemTick 
  * @note this will be converted into a dynamic list of components
  */
void MicroBit::addIdleComponent(MicroBitComponent *component)
{
    int i = 0;
    
    while(idleThreadComponents[i] != NULL && i < MICROBIT_IDLE_COMPONENTS)  
        i++;
    
    if(i == MICROBIT_IDLE_COMPONENTS)
        return;
        
    idleThreadComponents[i] = component;    
}

/**
  * remove a component from the array of components
  * @note this will be converted into a dynamic list of components
  */
void MicroBit::removeIdleComponent(MicroBitComponent *component)
{
    int i = 0;
    
    while(idleThreadComponents[i] != component && i < MICROBIT_IDLE_COMPONENTS)  
        i++;
    
    if(i == MICROBIT_IDLE_COMPONENTS)
        return;

    idleThreadComponents[i] = NULL;
}

/**
  * Determine the time since this MicroBit was last reset.
  *
  * @return The time since the last reset, in milliseconds. This will result in overflow after 1.6 months.
  * TODO: handle overflow case.
  */
unsigned long MicroBit::systemTime()
{
    return ticks;
}


/**
 * Determine the version of the micro:bit runtime currently in use.
 *
 * @return A textual description of the currentlt executing micro:bit runtime.
 * TODO: handle overflow case.
 */
char *MicroBit::systemVersion()
{
    return MICROBIT_DAL_VERSION;
}

/**
  * Triggers a microbit panic where an infinite loop will occur swapping between the panicFace and statusCode if provided.
  * 
  * @param statusCode the status code of the associated error. Status codes must be in the range 0-255.
  */
void MicroBit::panic(int statusCode)
{
    //show error and enter infinite while
    uBit.display.error(statusCode);
}

