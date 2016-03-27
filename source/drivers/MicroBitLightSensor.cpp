/**
  * Class definition for MicroBitLightSensor.
  *
  * This is an object that interleaves light sensing with uBit.display.
  */

#include "MicroBitConfig.h"
#include "MicroBitLightSensor.h"
#include "MicroBitDisplay.h"

/**
  * After the startSensing method has been called, this method will be called
  * MICROBIT_LIGHT_SENSOR_AN_SET_TIME after.
  *
  * It will then read from the currently selected channel using the AnalogIn
  * that was configured in the startSensing method.
  */
void MicroBitLightSensor::analogReady()
{
    this->results[chan] = this->sensePin->read_u16();

    analogDisable();

    DigitalOut((PinName)(matrixMap.columnStart + chan)).write(1);

    chan++;

    chan = chan % MICROBIT_LIGHT_SENSOR_CHAN_NUM;
}

/**
  * Forcibly disables the AnalogIn, otherwise it will remain in possession
  * of the GPIO channel it is using, meaning that the display will not be
  * able to use a channel (COL).
  *
  * This is required as per PAN 3, details of which can be found here:
  *
  * https://www.nordicsemi.com/eng/nordic/download_resource/24634/5/88440387
  */
void MicroBitLightSensor::analogDisable()
{
    NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Disabled;

    NRF_ADC->CONFIG = (ADC_CONFIG_RES_8bit << ADC_CONFIG_RES_Pos) |
                      (ADC_CONFIG_INPSEL_SupplyTwoThirdsPrescaling << ADC_CONFIG_INPSEL_Pos) |
                      (ADC_CONFIG_REFSEL_VBG                       << ADC_CONFIG_REFSEL_Pos) |
                      (ADC_CONFIG_PSEL_Disabled                    << ADC_CONFIG_PSEL_Pos) |
                      (ADC_CONFIG_EXTREFSEL_None                   << ADC_CONFIG_EXTREFSEL_Pos);
}

/**
  * Constructor.
  * Create a representation of the light sensor
  */
MicroBitLightSensor::MicroBitLightSensor(const MatrixMap &map) :
    analogTrigger(),
    matrixMap(map)
{
    this->chan = 0;

    if (EventModel::defaultEventBus)
        EventModel::defaultEventBus->listen(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_LIGHT_SENSE, this, &MicroBitLightSensor::startSensing, MESSAGE_BUS_LISTENER_IMMEDIATE);

    this->sensePin = NULL;
}

/**
  * This method returns a summed average of the three sections of the display.
  *
  * A section is defined as:
  *  ___________________
  * | 1 |   | 2 |   | 3 |
  * |___|___|___|___|___|
  * |   |   |   |   |   |
  * |___|___|___|___|___|
  * | 2 |   | 3 |   | 1 |
  * |___|___|___|___|___|
  * |   |   |   |   |   |
  * |___|___|___|___|___|
  * | 3 |   | 1 |   | 2 |
  * |___|___|___|___|___|
  *
  * Where each number represents a different section on the 5 x 5 matrix display.
  *
  * @return returns a value in the range 0 - 255 where 0 is dark, and 255
  * is very bright
  *
  * @note currently returns a value in the range 0 - 255 where 0 is dark, and 255
  * is very bright perhaps we should normalise the returned values into an SI unit!
  * TODO.
  */
int MicroBitLightSensor::read()
{
    int sum = 0;

    for(int i = 0; i < MICROBIT_LIGHT_SENSOR_CHAN_NUM; i++)
        sum += results[i];

    int average = sum / MICROBIT_LIGHT_SENSOR_CHAN_NUM;

    average = min(average, MICROBIT_LIGHT_SENSOR_MAX_VALUE);

    average = max(average, MICROBIT_LIGHT_SENSOR_MIN_VALUE);

    int inverted = (MICROBIT_LIGHT_SENSOR_MAX_VALUE - average) + MICROBIT_LIGHT_SENSOR_MIN_VALUE;

    int a = 0;

    int b = 255;

    int normalised = a + ((((inverted - MICROBIT_LIGHT_SENSOR_MIN_VALUE)) * (b - a))/ (MICROBIT_LIGHT_SENSOR_MAX_VALUE - MICROBIT_LIGHT_SENSOR_MIN_VALUE));

    return normalised;
}

/**
  * The method that is invoked by sending MICROBIT_DISPLAY_EVT_LIGHT_SENSE
  * using the id MICROBIT_ID_DISPLAY.
  */
void MicroBitLightSensor::startSensing(MicroBitEvent)
{
    for(int rowCount = 0; rowCount < matrixMap.rows; rowCount++)
        DigitalOut((PinName)(matrixMap.rowStart + rowCount)).write(0);

    PinName currentPin = (PinName)(matrixMap.columnStart + chan);

    DigitalOut(currentPin).write(1);

    DigitalIn(currentPin, PullNone).~DigitalIn();

    if(this->sensePin != NULL)
        delete this->sensePin;

    this->sensePin = new AnalogIn(currentPin);

    analogTrigger.attach_us(this, &MicroBitLightSensor::analogReady, MICROBIT_LIGHT_SENSOR_AN_SET_TIME);
}

/**
  * The destructor restores the default Display Mode and tick speed, and also
  * removes the listener from the default EventModel.
  */
MicroBitLightSensor::~MicroBitLightSensor()
{
    if (EventModel::defaultEventBus)
        EventModel::defaultEventBus->ignore(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_LIGHT_SENSE, this, &MicroBitLightSensor::startSensing);
}