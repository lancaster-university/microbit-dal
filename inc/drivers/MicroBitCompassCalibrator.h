#ifndef MICROBIT_COMPASS_CALIBRATOR_H
#define MICROBIT_COMPASS_CALIBRATOR_H

#include "MicroBitConfig.h"
#include "MicroBitCompass.h"
#include "MicroBitAccelerometer.h"
#include "MicroBitDisplay.h"


/**
  * Class definition for an interactive compass calibration algorithm.
  *
  * The algorithm uses an accelerometer to ensure that a broad range of sample data has been gathered 
  * from the compass module, then performs a least mean squares optimisation of the
  * results to determine the calibration data for the compass. The LED matrix display is
  * used to provide feedback to the user on the gestures required. 
  * This class listens for calibration requests from the compass (on the default event model), 
  * and automatically initiates a calibration sequence as necessary.
  */
class MicroBitCompassCalibrator
{
    MicroBitCompass&        compass;
    MicroBitAccelerometer&  accelerometer;
    MicroBitDisplay&        display;

    public:
    /**
     * Constructor.
     *
     * Create an object capable of calibrating the compass. The algorithm uses
     * the accelerometer to ensure that a broad range of sample data has been gathered 
     * from the compass module, then perform a least mean squares optimisation of the
     * results to determine the calibration data for the compass. The LED matrix display is
     * used to provide feedback to the user on the gestures required. 
     *
     *
     * @param compass The compass to calibrate.
     * @param accelerometer The accelerometer to gather constextual data from.
     * @param display The LED matrix to display user feedback on.
     */
    MicroBitCompassCalibrator(MicroBitCompass& _compass, MicroBitAccelerometer& _accelerometer, MicroBitDisplay& _display);

    /**
     * Performs a simple game that in parallel, calibrates the compass.
     * This function is executed automatically when the user requests a compass bearing, and compass calibration is required.
     * This function is, by design, synchronous and only returns once calibration is complete.
     */
    void calibrate(MicroBitEvent);
};

#endif
