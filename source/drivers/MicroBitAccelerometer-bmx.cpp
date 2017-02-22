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

#ifdef TARGET_NRF51_CALLIOPE

/**
 * Class definition for MicroBit Accelerometer.
 *
 * Represents an implementation of the Freescale MMA8653 3 axis accelerometer
 * Also includes basic data caching and on demand activation.
 */
#include "mbed.h"

#include "MicroBitConfig.h"
#include "MicroBitAccelerometer-bmx.h"
#include "ErrorNo.h"
#include "MicroBitEvent.h"
#include "MicroBitCompat.h"
#include "MicroBitFiber.h"

#if CONFIG_ENABLED(MICROBIT_DBG)
#  define BMX_DEBUG(...)  SERIAL_DEBUG->printf(__VA_ARGS__)
#else
#  define BMX_DEBUG(...)
#endif

/**
  * Configures the accelerometer for G range and sample rate defined
  * in this object. The nearest values are chosen to those defined
  * that are supported by the hardware. The instance variables are then
  * updated to reflect reality.
  *
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the accelerometer could not be configured.
  */
int MicroBitAccelerometer::configure()
{
    // BMX_DEBUG("RUN BMX055 start\r\n");

    id =    BMX055_ACC_ADDRESS<<1;
    wait_ms(100);

    i2c.start();
    //        BMX_DEBUG("id = %x\r\n",v);
    //        BMX_DEBUG("address = %x\r\n",address);
   
    
    //    BMX_DEBUG("RUN BMX055 id found\r\n");
    // start with all sensors in default mode with all registers reset
    writeByte(BMX055_ACC_ADDRESS,  BMX055_ACC_BGW_SOFTRESET, 0xB6);  // reset accelerometer
    wait_ms(1000); // Wait for all registers to reset

    //    BMX_DEBUG("RUN BMX055 after reset\r\n");
    
    // Configure accelerometer
    writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_PMU_RANGE, Ascale & 0x0F); // Set accelerometer full range
    writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_PMU_BW, ACCBW & 0x0F);     // Set accelerometer bandwidth
    writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_D_HBW, 0x00);              // Use filtered data

    writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_EN_0, 0x70);           // Enable ACC data ready interrupt
    writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_EN_1, 0x7F);           // Enable ACC data ready interrupt
    writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_EN_2, 0x02);           // Enable ACC data ready interrupt
    writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_OUT_CTRL, 0x02);       // open drain, active low
    writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_MAP_0, 0x00);        // Define INT1 (intACC1) as ACC data ready interrupt
    writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_MAP_1, 0x01);          // Define INT2 (intACC2) as ACC data ready interrupt

    writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_EN_0, 0x80);  // enable data ready interrupt
    writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_BGW_SPI3_WDT, 0x06);       // Set watchdog timer for 50 ms
    writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_EN_1, 0x04);  // select push-pull, active high interrupts

    writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_MAP_1, 0x01); // select INT3 (intGYRO1) as GYRO data ready interrupt, somehow requirement for acc data ready
    writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_SRC_3, 0x07);

// Configure Gyro
// start by resetting gyro, better not since it ends up in sleep mode?!
// writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_BGW_SOFTRESET, 0xB6); // reset gyro
// delay(100);
// Three power modes, 0x00 Normal,
// set bit 7 to 1 for suspend mode, set bit 5 to 1 for deep suspend mode
// sleep duration in fast-power up from suspend mode is set by bits 1 - 3
// 000 for 2 ms, 111 for 20 ms, etc.
//  writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_LPM1, 0x00);  // set GYRO normal mode
//  set GYRO sleep duration for fast power-up mode to 20 ms, for duty cycle of 50%
//  writeByte(BMX055_ACC_ADDRESS, BMX055_GYRO_LPM1, 0x0E);
// set bit 7 to 1 for fast-power-up mode,  gyro goes quickly to normal mode upon wake up
// can set external wake-up interrupts on bits 5 and 4
// auto-sleep wake duration set in bits 2-0, 001 4 ms, 111 40 ms
//  writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_LPM2, 0x00);  // set GYRO normal mode
// set gyro to fast wake up mode, will sleep for 20 ms then run normally for 20 ms
// and collect data for an effective ODR of 50 Hz, other duty cycles are possible but there
// is a minimum wake duration determined by the bandwidth duration, e.g.,  > 10 ms for 23Hz gyro bandwidth
//  writeByte(BMX055_ACC_ADDRESS, BMX055_GYRO_LPM2, 0x87);
    writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_RANGE, Gscale);  // set GYRO FS range
    writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_BW, GODRBW);     // set GYRO ODR and Bandwidth

/*    writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_EN_0, 0x80);  // enable data ready interrupt
    writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_EN_1, 0x04);  // select push-pull, active high interrupts
    writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_MAP_1, 0x01); // select INT3 (intGYRO1) as GYRO data ready interrupt

    writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_BGW_SPI3_WDT, 0x06); // Enable watchdog timer for I2C with 50 ms window


// Configure magnetometer
    writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL1, 0x82);  // Softreset magnetometer, ends up in sleep mode
    wait_ms(100);
    writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL1, 0x01); // Wake up magnetometer
    wait_ms(100);

    writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL2, MODR << 3); // Normal mode
//writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL2, MODR << 3 | 0x02); // Forced mode

//writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_INT_EN_2, 0x84); // Enable data ready pin interrupt, active high

// Set up four standard configurations for the magnetometer
    switch (Mmode) {
        case lowPower:
            // Low-power
            writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x01);  // 3 repetitions (oversampling)
            writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x02);  // 3 repetitions (oversampling)
            break;
        case Regular:
            // Regular
            writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x04);  //  9 repetitions (oversampling)
            writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x16);  // 15 repetitions (oversampling)
            break;
        case enhancedRegular:
            // Enhanced Regular
            writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x07);  // 15 repetitions (oversampling)
            writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x22);  // 27 repetitions (oversampling)
            break;
        case highAccuracy:
            // High Accuracy
            writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x17);  // 47 repetitions (oversampling)
            writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x51);  // 83 repetitions (oversampling)
            break;
    }
    */    
    i2c.stop();

    status |= MICROBIT_COMPONENT_RUNNING;

    return MICROBIT_OK;
}


void MicroBitAccelerometer::writeByte(char id, char addr, char value)
{
    char cmd[2];
    cmd[0] = addr;
    cmd[1] = value;
    i2c.write(id<<1, cmd, 2);
}

char MicroBitAccelerometer::readByte(char id, char addr)
{
    char res;
    i2c.write( id<<1, &addr, 1 );
    i2c.read( id<<1, &res, 1 );
    return res;
}

void MicroBitAccelerometer::readBytes(uint8_t id, uint8_t addr, uint8_t num, uint8_t *rawData)
{
    i2c.write( id<<1, (char *) &addr, 1 );
    i2c.read( id<<1, (char *) rawData, num);
}

void MicroBitAccelerometer::readAccelData(int16_t * destination)
{
    uint8_t rawData[6];  // x/y/z accel register data stored here
    //    BMX_DEBUG("before read\r\n");
    readBytes(BMX055_ACC_ADDRESS, BMX055_ACC_D_X_LSB, 6, &rawData[0]);       // Read the six raw data registers into data array
    //    BMX_DEBUG("after read\r\n");
    if((rawData[0] & 0x01) && (rawData[2] & 0x01) && (rawData[4] & 0x01)) {  // Check that all 3 axes have new data
        destination[0] = (int16_t) (((int16_t)rawData[1] << 8) | rawData[0]) >> 4;  // Turn the MSB and LSB into a signed 12-bit value
        destination[1] = (int16_t) (((int16_t)rawData[3] << 8) | rawData[2]) >> 4;
        destination[2] = (int16_t) (((int16_t)rawData[5] << 8) | rawData[4]) >> 4;
    }
    //    BMX_DEBUG("after end %d %d\r\n", destination[0], destination[1]);

}

/**
  * Issues a standard, 2 byte I2C command write to the accelerometer.
  *
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the register to write to.
  *
  * @param value The value to write.
  *
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the the write request failed.
  */
int MicroBitAccelerometer::writeCommand(uint8_t reg, uint8_t value)
{
    uint8_t command[2];
    command[0] = reg;
    command[1] = value;

    return i2c.write(address, (const char *)command, 2);
}

/**
  * Issues a read command, copying data into the specified buffer.
  *
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the register to access.
  *
  * @param buffer Memory area to read the data into.
  *
  * @param length The number of bytes to read.
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER or MICROBIT_I2C_ERROR if the the read request failed.
  */
int MicroBitAccelerometer::readCommand(uint8_t reg, uint8_t* buffer, int length)
{
    int result;

    if (buffer == NULL || length <= 0 )
        return MICROBIT_INVALID_PARAMETER;

    //    BMX_DEBUG("start write to address %x \r\n", address);

    result = i2c.write(address, (const char *)&reg, 1, true);
    if (result !=0)
        return MICROBIT_I2C_ERROR;
    //    BMX_DEBUG("start read from  address %x \r\n", address);

    result = i2c.read(address, (char *)buffer, length);
    if (result !=0)
        return MICROBIT_I2C_ERROR;

    return MICROBIT_OK;
}

/**
  * Constructor.
  * Create a software abstraction of an accelerometer.
  *
  * @param _i2c an instance of MicroBitI2C used to communicate with the onboard accelerometer.
  *
  * @param address the default I2C address of the accelerometer. Defaults to: MMA8653_DEFAULT_ADDR.
  *
  * @param id the unique EventModel id of this component. Defaults to: MICROBIT_ID_ACCELEROMETER
  *
  * @code
  * MicroBitI2C i2c = MicroBitI2C(I2C_SDA0, I2C_SCL0);
  *
  * MicroBitAccelerometer accelerometer = MicroBitAccelerometer(i2c);
  * @endcode
 */
MicroBitAccelerometer::MicroBitAccelerometer(MicroBitI2C& _i2c, uint16_t address, uint16_t id) : sample(), int1(MICROBIT_PIN_ACCEL_DATA_READY), i2c(_i2c)
{

	//    BMX_DEBUG("Constructor\r\n");

    // Store our identifiers.
    this->id = id;
    this->status = 0;
    this->address = address;

    // Update our internal state for 50Hz at +/- 2g (50Hz has a period af 20ms).
    this->samplePeriod = 20;
    this->sampleRange = 2;

    // Initialise gesture history
    this->sigma = 0;
    this->impulseSigma = 0;
    this->lastGesture = MICROBIT_ACCELEROMETER_EVT_NONE;
    this->currentGesture = MICROBIT_ACCELEROMETER_EVT_NONE;
    this->shake.x = 0;
    this->shake.y = 0;
    this->shake.z = 0;
    this->shake.count = 0;
    this->shake.timer = 0;
    this->shake.impulse_3 = 1;
    this->shake.impulse_6 = 1;
    this->shake.impulse_8 = 1;
}

/**
  * Attempts to read the 8 bit ID from the accelerometer, this can be used for
  * validation purposes.
  *
  * @return the 8 bit ID returned by the accelerometer, or MICROBIT_I2C_ERROR if the request fails.
  *
  * @code
  * accelerometer.whoAmI();
  * @endcode
  */
int MicroBitAccelerometer::whoAmI()
{
    uint8_t data;

    /*    result = readCommand(BMX055_ACC_WHOAMI, &data, 1);
    if (result !=0)
        return MICROBIT_I2C_ERROR;
    */
    data = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_WHOAMI);

    return (int)data;
}

/**
  * Reads the acceleration data from the accelerometer, and stores it in our buffer.
  * This only happens if the accelerometer indicates that it has new data via int1.
  *
  * On first use, this member function will attempt to add this component to the
  * list of fiber components in order to constantly update the values stored
  * by this object.
  *
  * This technique is called lazy instantiation, and it means that we do not
  * obtain the overhead from non-chalantly adding this component to fiber components.
  *
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the read request fails.
  */
int MicroBitAccelerometer::updateSample()
{
    if(!(status & MICROBIT_ACCEL_ADDED_TO_IDLE))
    {
        fiber_add_idle_component(this);
        status |= MICROBIT_ACCEL_ADDED_TO_IDLE;
    }

    // Poll interrupt line from accelerometer.
    // n.b. Default is Active LO. Interrupt is cleared in data read.
    if(!int1)
    {
	    //	    BMX_DEBUG("data ready\r\n");
	int16_t ndata[3];
	readAccelData((int16_t *) ndata);  // Read the x/y/z adc values

	// Now we'll calculate the accleration value into actual g's

        // read MSB values...
        sample.x = ndata[1];
        sample.y = ndata[0];
        sample.z = ndata[2];

//	BMX_DEBUG("x=%d y=%d x=%d y=%d\r\n",data[0], data[1], data[2], data[3]);
        // Normalize the data in the 0..1024 range.
	/*	sample.x *= 8;
        sample.y *= 8;
        sample.z *= 8; 
	*/
	 BMX_DEBUG("x=%d y=%d z=%d\r\n",sample.x, sample.y, sample.z);
#if CONFIG_ENABLED(USE_ACCEL_LSB)
        // Add in LSB values.
        sample.x += (data[1] / 64);
        sample.y += (data[3] / 64);
        sample.z += (data[5] / 64);
#endif

        // Scale into millig (approx!)
	/*        sample.x *= this->sampleRange;
        sample.y *= this->sampleRange;
        sample.z *= this->sampleRange;
	*/
        // Indicate that pitch and roll data is now stale, and needs to be recalculated if needed.
        status &= ~MICROBIT_ACCEL_PITCH_ROLL_VALID;

        // Update gesture tracking
        updateGesture();

        // Indicate that a new sample is available
        MicroBitEvent e(id, MICROBIT_ACCELEROMETER_EVT_DATA_UPDATE);
    } else {
	    // BMX_DEBUG("data not ready\r\n");
    }


    return MICROBIT_OK;
};

/**
  * A service function.
  * It calculates the current scalar acceleration of the device (x^2 + y^2 + z^2).
  * It does not, however, square root the result, as this is a relatively high cost operation.
  *
  * This is left to application code should it be needed.
  *
  * @return the sum of the square of the acceleration of the device across all axes.
  */
int MicroBitAccelerometer::instantaneousAccelerationSquared()
{
    updateSample();

    // Use pythagoras theorem to determine the combined force acting on the device.
    return (int)sample.x*(int)sample.x + (int)sample.y*(int)sample.y + (int)sample.z*(int)sample.z;
}

/**
 * Service function.
 * Determines a 'best guess' posture of the device based on instantaneous data.
 *
 * This makes no use of historic data, and forms the input to the filter implemented in updateGesture().
 *
 * @return A 'best guess' of the current posture of the device, based on instanataneous data.
 */
uint16_t MicroBitAccelerometer::instantaneousPosture()
{
    bool shakeDetected = false;

    // Test for shake events.
    // We detect a shake by measuring zero crossings in each axis. In other words, if we see a strong acceleration to the left followed by
    // a strong acceleration to the right, then we can infer a shake. Similarly, we can do this for each axis (left/right, up/down, in/out).
    //
    // If we see enough zero crossings in succession (MICROBIT_ACCELEROMETER_SHAKE_COUNT_THRESHOLD), then we decide that the device
    // has been shaken.
    if ((getX() < -MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && shake.x) || (getX() > MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && !shake.x))
    {
        shakeDetected = true;
        shake.x = !shake.x;
    }

    if ((getY() < -MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && shake.y) || (getY() > MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && !shake.y))
    {
        shakeDetected = true;
        shake.y = !shake.y;
    }

    if ((getZ() < -MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && shake.z) || (getZ() > MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && !shake.z))
    {
        shakeDetected = true;
        shake.z = !shake.z;
    }

    // If we detected a zero crossing in this sample period, count this.
    if (shakeDetected && shake.count < MICROBIT_ACCELEROMETER_SHAKE_COUNT_THRESHOLD)
    {
        shake.count++;
  
        if (shake.count == 1)
            shake.timer = 0;

        if (shake.count == MICROBIT_ACCELEROMETER_SHAKE_COUNT_THRESHOLD)
        {
            shake.shaken = 1;
            shake.timer = 0;
            return MICROBIT_ACCELEROMETER_EVT_SHAKE;
        }
    }

    // measure how long we have been detecting a SHAKE event.
    if (shake.count > 0)
    {
        shake.timer++;

        // If we've issued a SHAKE event already, and sufficient time has assed, allow another SHAKE event to be issued.
        if (shake.shaken && shake.timer >= MICROBIT_ACCELEROMETER_SHAKE_RTX)
        {
            shake.shaken = 0;
            shake.timer = 0;
            shake.count = 0;
        }

        // Decay our count of zero crossings over time. We don't want them to accumulate if the user performs slow moving motions.
        else if (!shake.shaken && shake.timer >= MICROBIT_ACCELEROMETER_SHAKE_DAMPING)
        {
            shake.timer = 0;
            if (shake.count > 0)
                shake.count--;
        }
    }

    if (instantaneousAccelerationSquared() < MICROBIT_ACCELEROMETER_FREEFALL_THRESHOLD)
        return MICROBIT_ACCELEROMETER_EVT_FREEFALL;

    // Determine our posture.
    if (getX() < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_LEFT;

    if (getX() > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_RIGHT;

    if (getY() < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_DOWN;

    if (getY() > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_UP;

    if (getZ() < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_FACE_DOWN;

    if (getZ() > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_FACE_UP;

    return MICROBIT_ACCELEROMETER_EVT_NONE;
}

/**
  * Updates the basic gesture recognizer. This performs instantaneous pose recognition, and also some low pass filtering to promote
  * stability.
  */
void MicroBitAccelerometer::updateGesture()
{
    // Check for High/Low G force events - typically impulses, impacts etc.
    // Again, during such spikes, these event take priority of the posture of the device.
    // For these events, we don't perform any low pass filtering.
    int force = instantaneousAccelerationSquared();
    //BMX_DEBUG("force=%d\r\n",force);


    if (force > MICROBIT_ACCELEROMETER_2G_THRESHOLD)
    {
// TODO correctly support 2G - fake 3G using the 2G threshold
#if defined(MICROBIT_ACCELEROMETER_2G_THRESHOLD) && defined(MICROBIT_ACCELEROMETER_EVT_2G)
        if (force > MICROBIT_ACCELEROMETER_2G_THRESHOLD && !shake.impulse_2)
        {
            MicroBitEvent e(MICROBIT_ID_GESTURE, MICROBIT_ACCELEROMETER_EVT_2G);
            shake.impulse_2 = 1;
        }
        if (force > MICROBIT_ACCELEROMETER_3G_THRESHOLD && !shake.impulse_3)
        {
            MicroBitEvent e(MICROBIT_ID_GESTURE, MICROBIT_ACCELEROMETER_EVT_3G);
            shake.impulse_3 = 1;
        }
#else
        if (force > MICROBIT_ACCELEROMETER_2G_THRESHOLD && !shake.impulse_3)
        {
            MicroBitEvent e(MICROBIT_ID_GESTURE, MICROBIT_ACCELEROMETER_EVT_3G);
            shake.impulse_3 = 1;
        }
#endif
        if (force > MICROBIT_ACCELEROMETER_6G_THRESHOLD && !shake.impulse_6)
        {
            MicroBitEvent e(MICROBIT_ID_GESTURE, MICROBIT_ACCELEROMETER_EVT_6G);
            shake.impulse_6 = 1;
        }
        if (force > MICROBIT_ACCELEROMETER_8G_THRESHOLD && !shake.impulse_8)
        {
            MicroBitEvent e(MICROBIT_ID_GESTURE, MICROBIT_ACCELEROMETER_EVT_8G);
            shake.impulse_8 = 1;
        }

        impulseSigma = 0;
    }

    // Reset the impulse event onve the acceleration has subsided.
    if (impulseSigma < MICROBIT_ACCELEROMETER_GESTURE_DAMPING)
        impulseSigma++;
    else
        shake.impulse_2 = shake.impulse_3 = shake.impulse_6 = shake.impulse_8 = 0;


    // Determine what it looks like we're doing based on the latest sample...
    uint16_t g = instantaneousPosture();

    if (g == MICROBIT_ACCELEROMETER_EVT_SHAKE)
    {
        MicroBitEvent e(MICROBIT_ID_GESTURE, MICROBIT_ACCELEROMETER_EVT_SHAKE);
        return;
    }

    // Perform some low pass filtering to reduce jitter from any detected effects
    if (g == currentGesture)
    {
        if (sigma < MICROBIT_ACCELEROMETER_GESTURE_DAMPING)
            sigma++;
    }
    else
    {
        currentGesture = g;
        sigma = 0;
    }

    // If we've reached threshold, update our record and raise the relevant event...
    if (currentGesture != lastGesture && sigma >= MICROBIT_ACCELEROMETER_GESTURE_DAMPING)
    {
        lastGesture = currentGesture;
        MicroBitEvent e(MICROBIT_ID_GESTURE, lastGesture);
    }
}

/**
  * Attempts to set the sample rate of the accelerometer to the specified value (in ms).
  *
  * @param period the requested time between samples, in milliseconds.
  *
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
  *
  * @code
  * // sample rate is now 20 ms.
  * accelerometer.setPeriod(20);
  * @endcode
  *
  * @note The requested rate may not be possible on the hardware. In this case, the
  * nearest lower rate is chosen.
  */
int MicroBitAccelerometer::setPeriod(int period)
{
    this->samplePeriod = period;
    return this->configure();
}

/**
  * Reads the currently configured sample rate of the accelerometer.
  *
  * @return The time between samples, in milliseconds.
  */
int MicroBitAccelerometer::getPeriod()
{
    return (int)samplePeriod;
}

/**
  * Attempts to set the sample range of the accelerometer to the specified value (in g).
  *
  * @param range The requested sample range of samples, in g.
  *
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
  *
  * @code
  * // the sample range of the accelerometer is now 8G.
  * accelerometer.setRange(8);
  * @endcode
  *
  * @note The requested range may not be possible on the hardware. In this case, the
  * nearest lower range is chosen.
  */
int MicroBitAccelerometer::setRange(int range)
{
    this->sampleRange = range;
    return this->configure();
}

/**
  * Reads the currently configured sample range of the accelerometer.
  *
  * @return The sample range, in g.
  */
int MicroBitAccelerometer::getRange()
{
    return (int)sampleRange;
}

/**
  * Reads the value of the X axis from the latest update retrieved from the accelerometer.
  *
  * @param system The coordinate system to use. By default, a simple cartesian system is provided.
  *
  * @return The force measured in the X axis, in milli-g.
  *
  * @code
  * accelerometer.getX();
  * @endcode
  */
int MicroBitAccelerometer::getX(MicroBitCoordinateSystem system)
{
	//	BMX_DEBUG("before\r\n");
	updateSample();

    switch (system)
    {
        case SIMPLE_CARTESIAN:
            return -sample.x;

        case NORTH_EAST_DOWN:
            return sample.y;

        case RAW:
        default:
            return sample.x;
    }
}

/**
  * Reads the value of the Y axis from the latest update retrieved from the accelerometer.
  *
  * @return The force measured in the Y axis, in milli-g.
  *
  * @code
  * accelerometer.getY();
  * @endcode
  */
int MicroBitAccelerometer::getY(MicroBitCoordinateSystem system)
{
    updateSample();

    switch (system)
    {
        case SIMPLE_CARTESIAN:
            return -sample.y;

        case NORTH_EAST_DOWN:
            return -sample.x;

        case RAW:
        default:
            return sample.y;
    }
}

/**
  * Reads the value of the Z axis from the latest update retrieved from the accelerometer.
  *
  * @return The force measured in the Z axis, in milli-g.
  *
  * @code
  * accelerometer.getZ();
  * @endcode
  */
int MicroBitAccelerometer::getZ(MicroBitCoordinateSystem system)
{
    updateSample();

    switch (system)
    {
        case NORTH_EAST_DOWN:
            return -sample.z;

        case SIMPLE_CARTESIAN:
        case RAW:
        default:
            return sample.z;
    }
}

/**
  * Provides a rotation compensated pitch of the device, based on the latest update retrieved from the accelerometer.
  *
  * @return The pitch of the device, in degrees.
  *
  * @code
  * accelerometer.getPitch();
  * @endcode
  */
int MicroBitAccelerometer::getPitch()
{
    return (int) ((360*getPitchRadians()) / (2*PI));
}

/**
  * Provides a rotation compensated pitch of the device, based on the latest update retrieved from the accelerometer.
  *
  * @return The pitch of the device, in radians.
  *
  * @code
  * accelerometer.getPitchRadians();
  * @endcode
  */
float MicroBitAccelerometer::getPitchRadians()
{
    if (!(status & MICROBIT_ACCEL_PITCH_ROLL_VALID))
        recalculatePitchRoll();

    return pitch;
}

/**
  * Provides a rotation compensated roll of the device, based on the latest update retrieved from the accelerometer.
  *
  * @return The roll of the device, in degrees.
  *
  * @code
  * accelerometer.getRoll();
  * @endcode
  */
int MicroBitAccelerometer::getRoll()
{
    return (int) ((360*getRollRadians()) / (2*PI));
}

/**
  * Provides a rotation compensated roll of the device, based on the latest update retrieved from the accelerometer.
  *
  * @return The roll of the device, in radians.
  *
  * @code
  * accelerometer.getRollRadians();
  * @endcode
  */
float MicroBitAccelerometer::getRollRadians()
{
    if (!(status & MICROBIT_ACCEL_PITCH_ROLL_VALID))
        recalculatePitchRoll();

    return roll;
}

/**
  * Recalculate roll and pitch values for the current sample.
  *
  * @note We only do this at most once per sample, as the necessary trigonemteric functions are rather
  *       heavyweight for a CPU without a floating point unit.
  */
void MicroBitAccelerometer::recalculatePitchRoll()
{
    double x = (double) getX(NORTH_EAST_DOWN);
    double y = (double) getY(NORTH_EAST_DOWN);
    double z = (double) getZ(NORTH_EAST_DOWN);

    roll = atan2(y, z);
    pitch = atan(-x / (y*sin(roll) + z*cos(roll)));

    status |= MICROBIT_ACCEL_PITCH_ROLL_VALID;
}

/**
  * Retrieves the last recorded gesture.
  *
  * @return The last gesture that was detected.
  *
  * Example:
  * @code
  * MicroBitDisplay display;
  *
  * if (accelerometer.getGesture() == SHAKE)
  *     display.scroll("SHAKE!");
  * @endcode
  */
uint16_t MicroBitAccelerometer::getGesture()
{
    return lastGesture;
}

/**
  * A periodic callback invoked by the fiber scheduler idle thread.
  *
  * Internally calls updateSample().
  */
void MicroBitAccelerometer::idleTick()
{
    updateSample();
}

/**
  * Destructor for MicroBitAccelerometer, where we deregister from the array of fiber components.
  */
MicroBitAccelerometer::~MicroBitAccelerometer()
{
    fiber_remove_idle_component(this);
}

const MMA8653SampleRangeConfig MMA8653SampleRange[MMA8653_SAMPLE_RANGES] = {
    {2, 0},
    {4, 1},
    {8, 2}
};

const MMA8653SampleRateConfig MMA8653SampleRate[MMA8653_SAMPLE_RATES] = {
    {1250,      0x00},
    {2500,      0x08},
    {5000,      0x10},
    {10000,     0x18},
    {20000,     0x20},
    {80000,     0x28},
    {160000,    0x30},
    {640000,    0x38}
};
#endif // TARGET_NRF51_CALLIOPE