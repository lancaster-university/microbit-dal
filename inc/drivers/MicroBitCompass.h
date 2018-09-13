/*
The MIT License (MIT)

Copyright (c) 2017 Lancaster University.

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

#ifndef MICROBIT_COMPASS
#define MICROBIT_COMPASS

#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "CoordinateSystem.h"
#include "MicroBitAccelerometer.h"


/**
 * Status flags
 */
#define MICROBIT_COMPASS_STATUS_RUNNING                  0x01
#define MICROBIT_COMPASS_STATUS_CALIBRATED               0x02
#define MICROBIT_COMPASS_STATUS_CALIBRATING              0x04
#define MICROBIT_COMPASS_STATUS_ADDED_TO_IDLE            0x08

/**
 * Accelerometer events
 */
#define MICROBIT_COMPASS_EVT_DATA_UPDATE                 1
#define MICROBIT_COMPASS_EVT_CONFIG_NEEDED               2
#define MICROBIT_COMPASS_EVT_CALIBRATE                   3
#define MICROBIT_COMPASS_EVT_CALIBRATION_NEEDED          4

struct CompassCalibration
{
    Sample3D           centre;                  // Zero offset of the compass.
    Sample3D           scale;                   // Scale factor to apply in each axis to accomodate 1st order directional fields.
    int                radius;                  // Indication of field strength - the "distance" from the centre to outmost sample.

    CompassCalibration() : centre(), scale(1024, 1024, 1024)
    {
        radius = 0;
    }
};

/**
 * Class definition for a general e-compass.
 */
class MicroBitCompass : public MicroBitComponent
{
    protected:

        uint16_t                samplePeriod;               // The time between samples, in milliseconds.
        CompassCalibration      calibration;                // The calibration data of this compass 
        Sample3D                sample;                     // The last sample read, in the coordinate system specified by the coordinateSpace variable.
        Sample3D                sampleENU;                  // The last sample read, in raw ENU format (stored in case requests are made for data in other coordinate spaces)
        CoordinateSpace         &coordinateSpace;           // The coordinate space transform (if any) to apply to the raw data from the hardware.
        MicroBitAccelerometer*  accelerometer;              // The accelerometer to use for tilt compensation.

    public:

       static MicroBitCompass   *detectedCompass;           // The autodetected instance of a MicroBitAcelerometer driver.

        /**
         * Constructor.
         * Create a software abstraction of an e-compass.
         *
         * @param id the unique EventModel id of this component. Defaults to: MICROBIT_ID_COMPASS
         * @param coordinateSpace the orientation of the sensor. Defaults to: SIMPLE_CARTESIAN
         *
         */
        MicroBitCompass(CoordinateSpace &coordinateSpace, uint16_t id = MICROBIT_ID_COMPASS);

        /**
         * Constructor.
         * Create a software abstraction of an e-compass.
         *
         * @param id the unique EventModel id of this component. Defaults to: MICROBIT_ID_COMPASS
         * @param accel the accelerometer to use for tilt compensation
         * @param coordinateSpace the orientation of the sensor. Defaults to: SIMPLE_CARTESIAN
         *
         */
        MicroBitCompass(MicroBitAccelerometer &accel, CoordinateSpace &coordinateSpace, uint16_t id = MICROBIT_ID_COMPASS);

        /**
         * Device autodetection. Scans the given I2C bus for supported compass devices.
         * if found, constructs an appropriate driver and returns it.
         *
         * @param i2c the bus to scan. 
         *
         */
        static MicroBitCompass& autoDetect(MicroBitI2C &i2c); 


        /**
         * Gets the current heading of the device, relative to magnetic north.
         *
         * If the compass is not calibrated, it will raise the COMPASS_EVT_CALIBRATE event.
         *
         * Users wishing to implement their own calibration algorithms should listen for this event,
         * using MESSAGE_BUS_LISTENER_IMMEDIATE model. This ensures that calibration is complete before
         * the user program continues.
         *
         * @return the current heading, in degrees. Or CALIBRATION_IN_PROGRESS if the compass is calibrating.
         *
         * @code
         * compass.heading();
         * @endcode
         */
        int heading();

        /**
         * Determines the overall magnetic field strength based on the latest update from the magnetometer.
         *
         * @return The magnetic force measured across all axis, in nano teslas.
         *
         * @code
         * compass.getFieldStrength();
         * @endcode
         */
        int getFieldStrength();

        /**
         * Perform a calibration of the compass.
         *
         * This method will be called automatically if a user attempts to read a compass value when
         * the compass is uncalibrated. It can also be called at any time by the user.
         *
         * The method will only return once the compass has been calibrated.
         *
         * @return MICROBIT_OK, MICROBIT_I2C_ERROR if the magnetometer could not be accessed,
         * or MICROBIT_CALIBRATION_REQUIRED if the calibration algorithm failed to complete successfully.
         *
         * @note THIS MUST BE CALLED TO GAIN RELIABLE VALUES FROM THE COMPASS
         */
        int calibrate();

        /**
         * Configure the compass to use the calibration data that is supplied to this call.
         *
         * Calibration data is comprised of the perceived zero offset of each axis of the compass.
         *
         * After calibration this should now take into account trimming errors in the magnetometer,
         * and any "hard iron" offsets on the device.
         *
         * @param calibration A Sample3D containing the offsets for the x, y and z axis.
         */
        void setCalibration(CompassCalibration calibration);

        /**
         * Provides the calibration data currently in use by the compass.
         *
         * More specifically, the x, y and z zero offsets of the compass.
         *
         * @return A Sample3D containing the offsets for the x, y and z axis.
         */
        CompassCalibration getCalibration();

        /**
         * Returns 0 or 1. 1 indicates that the compass is calibrated, zero means the compass requires calibration.
         */
        int isCalibrated();

        /**
         * Returns 0 or 1. 1 indicates that the compass is calibrating, zero means the compass is not currently calibrating.
         */
        int isCalibrating();

        /**
         * Clears the calibration held in persistent storage, and sets the calibrated flag to zero.
         */
        void clearCalibration();


        /**
         * Configures the device for the sample rate defined
         * in this object. The nearest values are chosen to those defined
         * that are supported by the hardware. The instance variables are then
         * updated to reflect reality.
         *
         * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the compass could not be configured.
         */
        virtual int configure();

        /**
         *
         * Defines the accelerometer to be used for tilt compensation.
         *
         * @param acceleromter Reference to the accelerometer to use.
         */
        void setAccelerometer(MicroBitAccelerometer &accelerometer);

        /**
         * Attempts to set the sample rate of the compass to the specified period value (in ms).
         *
         * @param period the requested time between samples, in milliseconds.
         * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
         *
         * @note The requested rate may not be possible on the hardware. In this case, the
         * nearest lower rate is chosen.
         *
         * @note This method should be overriden (if supported) by specific magnetometer device drivers.
         */
        virtual int setPeriod(int period);

        /**
         * Reads the currently configured sample rate of the compass.
         *
         * @return The time between samples, in milliseconds.
         */
        virtual int getPeriod();

        /**
         * Poll to see if new data is available from the hardware. If so, update it.
         * n.b. it is not necessary to explicitly call this funciton to update data
         * (it normally happens in the background when the scheduler is idle), but a check is performed
         * if the user explicitly requests up to date data.
         *
         * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the update fails.
         *
         * @note This method should be overidden by the hardware driver to implement the requested
         * changes in hardware.
         */
        virtual int requestUpdate();

        /**
         * Stores data from the compass sensor in our buffer.
         *
         * On first use, this member function will attempt to add this component to the
         * list of fiber components in order to constantly update the values stored
         * by this object.
         *
         * This lazy instantiation means that we do not
         * obtain the overhead from non-chalantly adding this component to fiber components.
         *
         * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the read request fails.
         */
        virtual int update();

        /**
         * Reads the last compass value stored, and provides it in the coordinate system requested.
         *
         * @param coordinateSpace The coordinate system to use.
         * @return The force measured in each axis, in milli-g.
         */
        Sample3D getSample(CoordinateSystem coordinateSystem);

        /**
         * Reads the last compass value stored, and in the coordinate system defined in the constructor.
         * @return The force measured in each axis, in milli-g.
         */
        Sample3D getSample();

        /**
         * reads the value of the x axis from the latest update retrieved from the compass,
         * using the default coordinate system as specified in the constructor.
         *
         * @return the force measured in the x axis, in milli-g.
         */
        int getX();

        /**
         * reads the value of the y axis from the latest update retrieved from the compass,
         * using the default coordinate system as specified in the constructor.
         *
         * @return the force measured in the y axis, in milli-g.
         */
        int getY();

        /**
         * reads the value of the z axis from the latest update retrieved from the compass,
         * using the default coordinate system as specified in the constructor.
         *
         * @return the force measured in the z axis, in milli-g.
         */
        int getZ();

        /**
         * updateSample() method maintained here as an inline method purely for backward compatibility.
         */
        inline void updateSample()
        {
            getSample();
        }


        /**
         * Destructor.
         */
        ~MicroBitCompass();

    private:

        /**
         * Internal helper used to de-duplicate code in the constructors
         *
         */
        void init(uint16_t id);

        /**
         * Calculates a tilt compensated bearing of the device, using the accelerometer.
         */
        int tiltCompensatedBearing();

        /**
         * Calculates a non-tilt compensated bearing of the device.
         */
        int basicBearing();
};

//
// Backward Compatibility
//
typedef Sample3D CompassSample;

#endif
