#include "MicroBit.h"

/**
  * custom function for panic for malloc & new due to scoping issue.
  */
void panic(int statusCode)
{
    uBit.panic(statusCode);   
}

/**
  * Callback that performs a hard reset when a BLE GAP disconnect occurs.
  * Only used when an explicit reset is invoked locally whilst a BLE connection is in progress.
  * This allows for a clean diconnect of the BLE connection before resetting.
  */
void bleDisconnectionResetCallback(const Gap::DisconnectionCallbackParams_t *)
{
    NVIC_SystemReset();
}

/**
  * Perform a hard reset of the micro:bit.
  * If BLE connected, then try to signal a disconnect first
  */
void
microbit_reset()
{
    if(uBit.ble && uBit.ble->getGapState().connected) {
        uBit.ble->onDisconnection(bleDisconnectionResetCallback);

        uBit.ble->gap().disconnect(Gap::REMOTE_USER_TERMINATED_CONNECTION);
        // We should be reset by the disconnection callback, so we wait to
        // allow that to happen.  If it doesn't happen, then we fall through to the
        // hard rest here.  (For example there is a race condition where
        // the remote device disconnects between us testing the connection
        // state and re-setting the disconnection callback).
        uBit.sleep(1000);
    }
    NVIC_SystemReset();
}

void bleDisconnectionCallback(const Gap::DisconnectionCallbackParams_t *reason)
{
    (void) reason; /* -Wunused-param */

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
    serial(USBTX, USBRX),
    MessageBus(),
    display(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_WIDTH, MICROBIT_DISPLAY_HEIGHT),
    buttonA(MICROBIT_ID_BUTTON_A,MICROBIT_PIN_BUTTON_A, MICROBIT_BUTTON_SIMPLE_EVENTS),
    buttonB(MICROBIT_ID_BUTTON_B,MICROBIT_PIN_BUTTON_B, MICROBIT_BUTTON_SIMPLE_EVENTS),
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
       MICROBIT_ID_IO_P20),
	bleManager()
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

#if CONFIG_ENABLED(MICROBIT_BLE_ENABLED)
    // Start the BLE stack.        
    bleManager.init(this->getName(), this->getSerial());
	  
    ble = bleManager.ble;
#endif

    // Start refreshing the Matrix Display
    systemTicker.attach(this, &MicroBit::systemTick, MICROBIT_DISPLAY_REFRESH_PERIOD);     

    // Register our compass calibration algorithm.
    MessageBus.listen(MICROBIT_ID_COMPASS, MICROBIT_COMPASS_EVT_CALIBRATE, this, &MicroBit::compassCalibrator, MESSAGE_BUS_LISTENER_IMMEDIATE);
}

/**
  * Performs a simple game that in parallel, calibrates the compass.
  * This function is executed automatically when the user requests a compass bearing, and compass calibration is required.
  * This function is, by design, synchronous and only returns once calibration is complete.
  */
void MicroBit::compassCalibrator(MicroBitEvent)
{
    struct Point
    {
        uint8_t x;
        uint8_t y;
        uint8_t on;
    };

    const int PERIMETER_POINTS = 12;
    const int PIXEL1_THRESHOLD = 200;
    const int PIXEL2_THRESHOLD = 800;

	Matrix4 X(PERIMETER_POINTS, 4);
    Point perimeter[PERIMETER_POINTS] = {{1,0,0}, {2,0,0}, {3,0,0}, {4,1,0}, {4,2,0}, {4,3,0}, {3,4,0}, {2,4,0}, {1,4,0}, {0,3,0}, {0,2,0}, {0,1,0}}; 
    Point cursor = {2,2,0};

    MicroBitImage img(5,5);
    MicroBitImage smiley("0,255,0,255,0\n0,255,0,255,0\n0,0,0,0,0\n255,0,0,0,255\n0,255,255,255,0\n");
    int samples = 0;

    // Firstly, we need to take over the display. Ensure all active animations are paused.
    display.stopAnimation();
    display.scrollAsync("DRAW A CIRCLE");
    for (int i=0; i<110; i++)
    {
        if (buttonA.isPressed() || buttonB.isPressed())
        {
            break;
        }
        sleep(100);
    }

    display.stopAnimation();
    display.clear();

    while(samples < PERIMETER_POINTS)
    {
        // update our model of the flash status of the user controlled pixel.
        cursor.on = (cursor.on + 1) % 4;

        // take a snapshot of the current accelerometer data.
        int x = uBit.accelerometer.getX();
        int y = uBit.accelerometer.getY();

        // Deterine the position of the user controlled pixel on the screen.
        if (x < -PIXEL2_THRESHOLD)
            cursor.x = 0;
        else if (x < -PIXEL1_THRESHOLD)
            cursor.x = 1;
        else if (x > PIXEL2_THRESHOLD)
            cursor.x = 4;
        else if (x > PIXEL1_THRESHOLD)
            cursor.x = 3;
        else 
            cursor.x = 2;

        if (y < -PIXEL2_THRESHOLD)
            cursor.y = 0;
        else if (y < -PIXEL1_THRESHOLD)
            cursor.y = 1;
        else if (y > PIXEL2_THRESHOLD)
            cursor.y = 4;
        else if (y > PIXEL1_THRESHOLD)
            cursor.y = 3;
        else
            cursor.y = 2;

        img.clear();

        // Turn on any pixels that have been visited.
        for (int i=0; i<PERIMETER_POINTS; i++)
            if (perimeter[i].on)
                img.setPixelValue(perimeter[i].x, perimeter[i].y, 255);

        // Update the pixel at the users position.
        img.setPixelValue(cursor.x, cursor.y, 255);

        // Update the buffer to the screen.
        uBit.display.image.paste(img,0,0,0);

        // test if we need to update the state at the users position.
        for (int i=0; i<PERIMETER_POINTS; i++)
        {
            if (cursor.x == perimeter[i].x && cursor.y == perimeter[i].y && !perimeter[i].on)
            {
                // Record the sample data for later processing...
                X.set(samples, 0, compass.getX(RAW));
                X.set(samples, 1, compass.getY(RAW));
                X.set(samples, 2, compass.getZ(RAW));
                X.set(samples, 3, 1);

                // Record that this pixel has been visited.
                perimeter[i].on = 1;
                samples++;
            }
        }

        uBit.sleep(100);
    }

    // We have enough sample data to make a fairly accurate calibration.
    // We use a Least Mean Squares approximation, as detailed in Freescale application note AN2426.

    // Firstly, calculate the square of each sample.
	Matrix4 Y(X.height(), 1);
	for (int i = 0; i < X.height(); i++)
	{
		double v = X.get(i, 0)*X.get(i, 0) + X.get(i, 1)*X.get(i, 1) + X.get(i, 2)*X.get(i, 2);
		Y.set(i, 0, v);
	}

    // Now perform a Least Squares Approximation. 
	Matrix4 XT = X.transpose();
	Matrix4 Beta = XT.multiply(X).invert().multiply(XT).multiply(Y);

    // The result contains the approximate zero point of each axis, but doubled.
    // Halve each sample, and record this as the compass calibration data.
    CompassSample cal = {(int)(Beta.get(0,0) / 2), (int)(Beta.get(1,0) / 2), (int)(Beta.get(2,0) / 2)};
    compass.setCalibration(cal);

    // Show a smiley to indicate that we're done, and continue on with the user program.
    display.clear();
    display.print(smiley, 0, 0, 0, 1500);
    display.clear();
}

/**
  * Return the friendly name for this device.
  *
  * @return A string representing the friendly name of this device.
  */
ManagedString MicroBit::getName()
{
    char nameBuffer[MICROBIT_NAME_LENGTH];
    const uint8_t codebook[MICROBIT_NAME_LENGTH][MICROBIT_NAME_CODE_LETTERS] = 
    {
        {'z', 'v', 'g', 'p', 't'},  
        {'u', 'o', 'i', 'e', 'a'},
        {'z', 'v', 'g', 'p', 't'},  
        {'u', 'o', 'i', 'e', 'a'},
        {'z', 'v', 'g', 'p', 't'}
    };

    // We count right to left, so create a pointer to the end of the buffer.
	char *name = nameBuffer;
    name += MICROBIT_NAME_LENGTH;

	// Derive our name from the nrf51822's unique ID.
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

    return ManagedString(nameBuffer, MICROBIT_NAME_LENGTH);
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

    return s1 + s2;
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
  * @note Values of below below the scheduling period (typical 6ms) tend to lose resolution.
  * 
  * @param milliseconds the amount of time, in ms, to wait for. This number cannot be negative.
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER milliseconds is less than zero. 
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

    return MICROBIT_OK;
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
    uint32_t m, result;

    //return MICROBIT_INVALID_VALUE if max is <= 0...
    if(max <= 0)
        return MICROBIT_INVALID_PARAMETER;

    // Our maximum return value is actually one less than passed
    max--;

    do {
        m = (uint32_t)max;
        result = 0;
		do {
            // Cycle the LFSR (Linear Feedback Shift Register).
            // We use an optimal sequence with a period of 2^32-1, as defined by Bruce Schneier here (a true legend in the field!), 
            // For those interested, it's documented in his paper:
            // "Pseudo-Random Sequence Generator for 32-Bit CPUs: A fast, machine-independent generator for 32-bit Microprocessors"
            // https://www.schneier.com/paper-pseudorandom-sequence.html
			uint32_t rnd = randomValue;

            rnd = ((((rnd >> 31)
                          ^ (rnd >> 6)
                          ^ (rnd >> 4)
                          ^ (rnd >> 2)
                          ^ (rnd >> 1)
                          ^ rnd)
                          & 0x0000001)
                          << 31 )
                          | (rnd >> 1);

			randomValue = rnd;

            result = ((result << 1) | (rnd & 0x00000001));
        } while(m >>= 1); 
    } while (result > (uint32_t)max);


    return result;
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
  * @param component The component to add.
  * @return MICROBIT_OK on success. MICROBIT_NO_RESOURCES is returned if further components cannot be supported.
  * @note this will be converted into a dynamic list of components
  */
int MicroBit::addSystemComponent(MicroBitComponent *component)
{
    int i = 0;
    
    while(systemTickComponents[i] != NULL && i < MICROBIT_SYSTEM_COMPONENTS)  
        i++;
    
    if(i == MICROBIT_SYSTEM_COMPONENTS)
        return MICROBIT_NO_RESOURCES;
        
    systemTickComponents[i] = component;    
    return MICROBIT_OK;
}

/**
  * remove a component from the array of components
  * @param component The component to remove.
  * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMTER is returned if the given component has not been previous added.
  * @note this will be converted into a dynamic list of components
  */
int MicroBit::removeSystemComponent(MicroBitComponent *component)
{
    int i = 0;
    
    while(systemTickComponents[i] != component  && i < MICROBIT_SYSTEM_COMPONENTS)  
        i++;
    
    if(i == MICROBIT_SYSTEM_COMPONENTS)
        return MICROBIT_INVALID_PARAMETER;

    systemTickComponents[i] = NULL;

    return MICROBIT_OK;
}

/**
  * add a component to the array of components which invocate the systemTick member function during a systemTick 
  * @param component The component to add.
  * @return MICROBIT_OK on success. MICROBIT_NO_RESOURCES is returned if further components cannot be supported.
  * @note this will be converted into a dynamic list of components
  */
int MicroBit::addIdleComponent(MicroBitComponent *component)
{
    int i = 0;
    
    while(idleThreadComponents[i] != NULL && i < MICROBIT_IDLE_COMPONENTS)  
        i++;
    
    if(i == MICROBIT_IDLE_COMPONENTS)
        return MICROBIT_NO_RESOURCES;
        
    idleThreadComponents[i] = component;    

    return MICROBIT_OK;
}

/**
  * remove a component from the array of components
  * @param component The component to remove.
  * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMTER is returned if the given component has not been previous added.
  * @note this will be converted into a dynamic list of components
  */
int MicroBit::removeIdleComponent(MicroBitComponent *component)
{
    int i = 0;
    
    while(idleThreadComponents[i] != component && i < MICROBIT_IDLE_COMPONENTS)  
        i++;
    
    if(i == MICROBIT_IDLE_COMPONENTS)
        return MICROBIT_INVALID_PARAMETER;

    idleThreadComponents[i] = NULL;

    return MICROBIT_OK;
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
const char *MicroBit::systemVersion()
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

